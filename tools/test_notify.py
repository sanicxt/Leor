#!/usr/bin/env python3
"""
BLE Notification Test Script for Leor ESP32 Companion Robot.

Usage:
    # Interactive mode
    python3 tools/test_notify.py

    # Automated cycle through test notifications every 3s
    python3 tools/test_notify.py --cycle
    python3 tools/test_notify.py --cycle --interval 5

    # One-shot commands
    python3 tools/test_notify.py msg WhatsApp "John: Hello there!"
    python3 tools/test_notify.py call Phone Mom
    python3 tools/test_notify.py call-end
    python3 tools/test_notify.py cal Calendar "10:00-10:30" "Room A"
    python3 tools/test_notify.py raw "happy"
"""

import argparse
import asyncio
import sys

from bleak import BleakClient, BleakScanner

SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
COMMAND_CHAR = "beb5483e-36e1-4688-b7f5-ea07361b26a8"
STATUS_CHAR = "1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e"

CYCLE_STEPS = [
    ("msg WhatsApp", "John: Hello there! How are you?"),
    ("msg Discord", "New message in general channel"),
    ("msg Telegram", "Code review requested"),
    ("call Phone", "Mom"),
    ("call-end", ""),
    ("msg Instagram", "Someone liked your photo"),
    ("cal Calendar", "Team Standup|10:00-10:30|Room A"),
    ("msg Twitter", "New follower: @user123"),
    ("cal Calendar", "Lunch|12:00-13:00|Cafe"),
]


def status_handler(sender: int, data: bytearray) -> None:
    text = data.decode("utf-8", errors="replace").strip()
    if text:
        print(f"  \033[90m[RX] {text}\033[0m")


async def find_device(timeout: int = 10) -> str | None:
    print(f"Scanning for Leor devices (timeout={timeout}s)...")

    scanner = BleakScanner()
    await scanner.start()
    await asyncio.sleep(timeout)
    await scanner.stop()

    found = []
    for addr, dev in scanner.discovered_devices_and_advertisement_data.items():
        d, adv = dev
        if d.name and "Leor" in d.name:
            found.append(d)
        elif hasattr(adv, "service_uuids") and SERVICE_UUID in adv.service_uuids:
            found.append(d)

    if not found:
        for d in scanner.discovered_devices:
            if d.name and d.name.strip():
                found.append(d)

    if not found:
        print("No Leor devices found.")
        return None

    if len(found) == 1:
        addr = found[0].address
        name = found[0].name or "Unknown"
        print(f"  Auto-selected: {name} [{addr}]")
        return addr

    print("\nMultiple devices found. Select one:")
    for i, d in enumerate(found):
        name = d.name or "Unknown"
        print(f"  [{i}] {name} [{d.address}]")
    try:
        idx = int(input("  Choice: "))
        return found[idx].address
    except (ValueError, IndexError):
        print("Invalid selection.")
        return None


async def send_command(client: BleakClient, command: str) -> None:
    cmd_bytes = command.encode("utf-8")
    print(f"  \033[94m[TX] {command}\033[0m")
    await client.write_gatt_char(COMMAND_CHAR, cmd_bytes, response=True)
    await asyncio.sleep(0.3)


async def run_interactive(address: str) -> None:
    async with BleakClient(address) as client:
        print(f"Connected: {address}")
        await client.start_notify(STATUS_CHAR, status_handler)
        print("\nEnter commands (or 'quit'):")
        print("  msg <app>|<body>")
        print("  call <app>|<caller>")
        print("  call-end")
        print("  cal <app>|<time>|<location>")
        print("  raw <command>")
        print("  ? = help")
        print()
        while True:
            try:
                line = (await asyncio.get_event_loop().run_in_executor(None, input, "> ")).strip()
                if not line or line == "quit":
                    break
                if line == "?":
                    print("  msg <app>|<body>")
                    print("  call <app>|<caller>")
                    print("  call-end")
                    print("  cal <app>|<time>|<location>")
                    print("  raw <command>")
                    continue
                parts = line.split(None, 1)
                shorthand = parts[0].lower()
                rest = parts[1] if len(parts) > 1 else ""
                if shorthand == "msg":
                    p = rest.find("|")
                    if p == -1:
                        sp = rest.find(" ")
                        if sp == -1:
                            print("  Usage: msg <app>|<body>  or  msg <app> <body>")
                            continue
                        cmd = f"notify:msg:{rest[:sp]}|{rest[sp+1:]}"
                    else:
                        cmd = f"notify:msg:{rest}"
                elif shorthand == "call":
                    p = rest.find("|")
                    if p == -1:
                        sp = rest.find(" ")
                        if sp == -1:
                            print("  Usage: call <app>|<caller>  or  call <app> <caller>")
                            continue
                        cmd = f"notify:call:{rest[:sp]}|{rest[sp+1:]}"
                    else:
                        cmd = f"notify:call:{rest}"
                elif shorthand == "call-end":
                    cmd = "notify:call:end"
                elif shorthand == "cal":
                    p1 = rest.find("|")
                    if p1 == -1:
                        sp = rest.find(" ")
                        if sp == -1:
                            print("  Usage: cal <app>|<time>|<location>")
                            continue
                        app = rest[:sp]
                        rest2 = rest[sp+1:]
                        sp2 = rest2.find(" ")
                        time_str = rest2[:sp2] if sp2 != -1 else rest2
                        loc = rest2[sp2+1:] if sp2 != -1 else ""
                        cmd = f"notify:cal:{app}|{time_str}|{loc}"
                    else:
                        p2 = rest.find("|", p1 + 1)
                        if p2 == -1:
                            print("  Usage: cal <app>|<time>|<location>")
                            continue
                        cmd = f"notify:cal:{rest}"
                elif shorthand == "raw":
                    cmd = rest
                else:
                    print(f"  Unknown: {shorthand} (try ?)")
                    continue
                await send_command(client, cmd)
            except (EOFError, KeyboardInterrupt):
                break


async def run_cycle(address: str, interval: int) -> None:
    async with BleakClient(address) as client:
        print(f"Connected: {address}")
        await client.start_notify(STATUS_CHAR, status_handler)
        print(f"Cycling through {len(CYCLE_STEPS)} test cases every {interval}s\n")
        try:
            step = 0
            while True:
                raw_type, raw_body = CYCLE_STEPS[step % len(CYCLE_STEPS)]
                parts = raw_type.split(None, 1)
                shorthand = parts[0]
                app = parts[1] if len(parts) > 1 else ""

                if shorthand == "call-end":
                    cmd = "notify:call:end"
                    label = "call-end"
                elif shorthand == "cal":
                    cmd = f"notify:cal:{app}|{raw_body}"
                    label = f"cal {app}"
                else:
                    cmd = f"notify:{shorthand}:{app}|{raw_body}"
                    label = f"{shorthand} {app}"

                print(f"\n\033[93m[{step + 1}/{len(CYCLE_STEPS)}] {label}\033[0m")
                await send_command(client, cmd)
                step += 1
                await asyncio.sleep(interval)
        except KeyboardInterrupt:
            print("\nCycle stopped.")


async def main():
    parser = argparse.ArgumentParser(description="Test Leor notification overlay")
    parser.add_argument("type", nargs="?", choices=["msg", "call", "call-end", "cal", "raw"],
                        default=None, help="Notification type")
    parser.add_argument("args", nargs="*", help="Arguments for the notification")
    parser.add_argument("--device", help="BLE device address (skips scan)")
    parser.add_argument("--scan-timeout", type=int, default=10, help="Scan timeout")
    parser.add_argument("--cycle", action="store_true", help="Auto-cycle through test notifications")
    parser.add_argument("--interval", type=int, default=3, help="Seconds between cycle steps (default: 3)")

    args = parser.parse_args()

    if args.device:
        address = args.device
    else:
        address = await find_device(args.scan_timeout)
        if not address:
            sys.exit(1)

    if args.cycle:
        await run_cycle(address, args.interval)
    elif args.type is None:
        await run_interactive(address)
    else:
        # One-shot
        if args.type == "msg":
            if len(args.args) < 2:
                print("Usage: msg <app>|<body>"); sys.exit(1)
            cmd = f"notify:msg:{args.args[0]}|{args.args[1]}"
        elif args.type == "call":
            if len(args.args) < 2:
                print("Usage: call <app>|<caller>"); sys.exit(1)
            cmd = f"notify:call:{args.args[0]}|{args.args[1]}"
        elif args.type == "call-end":
            cmd = "notify:call:end"
        elif args.type == "cal":
            if len(args.args) < 3:
                print("Usage: cal <app>|<time>|<location>"); sys.exit(1)
            cmd = f"notify:cal:{args.args[0]}|{args.args[1]}|{args.args[2]}"
        elif args.type == "raw":
            if not args.args:
                print("Usage: raw <command>"); sys.exit(1)
            cmd = " ".join(args.args)
        else:
            return

        async with BleakClient(address) as client:
            print(f"Connected: {address}")
            await client.start_notify(STATUS_CHAR, status_handler)
            await send_command(client, cmd)
            await asyncio.sleep(0.5)


if __name__ == "__main__":
    asyncio.run(main())
