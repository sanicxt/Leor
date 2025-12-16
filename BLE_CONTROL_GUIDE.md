# leor BLE Control Guide

leor is a small, expressive desk-toy robot. It uses Bluetooth Low Energy (BLE) so you can control it without keeping WiFi running.

## Quick start (phone)

1. Install **nRF Connect** (iOS/Android)
2. Scan and connect to the device named **leor** (or whatever you set in `BLE_DEVICE_NAME`)
3. Open the **Command** characteristic and write a command like `happy`
4. Enable notifications on **Status** (you’ll get a text reply)
5. Enable notifications on **Gesture** (you’ll get events like `pat`/`rub`)

## BLE details (UUIDs)

Service UUID: `4fafc201-1fb5-459e-8fcc-c5c9c331914b`

Characteristics:

- Command (Write): `beb5483e-36e1-4688-b7f5-ea07361b26a8`
- Status (Read + Notify): `1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e`
  - Sends replies like `OK: happy`, `Unknown: xyz`, `connected`, `ready`
- Gesture (Read + Notify): `d1e5f0a1-2b3c-4d5e-6f7a-8b9c0d1e2f3a`
  - Sends events like `pat`, `rub`, `matched:happy`

## Sending commands

Commands are plain text. Examples:

- `happy`
- `blink`
- `gesture:match=1`

You can also send multiple commands in one write by separating them with newlines or semicolons:

- `neutral\nblink\nhappy`
- `neutral; blink; happy`

## Web Bluetooth (simple demo)

Create a simple HTML file:

```html
<!DOCTYPE html>
<html>
<head>
  <title>leor BLE Control</title>
</head>
<body>
  <h1>leor Control</h1>
  <button onclick="connect()">Connect</button>
  <button onclick="sendCommand('happy')">Happy</button>
  <button onclick="sendCommand('sad')">Sad</button>
  <button onclick="sendCommand('laugh')">Laugh</button>
  <button onclick="sendCommand('blink')">Blink</button>
  <pre id="log"></pre>
  
  <script>
    let commandChar = null;
    let statusChar = null;
    let gestureChar = null;

    const SERVICE = '4fafc201-1fb5-459e-8fcc-c5c9c331914b';
    const CMD = 'beb5483e-36e1-4688-b7f5-ea07361b26a8';
    const STATUS = '1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e';
    const GESTURE = 'd1e5f0a1-2b3c-4d5e-6f7a-8b9c0d1e2f3a';

    function log(line) {
      const el = document.getElementById('log');
      el.textContent = (line + '\n') + el.textContent;
    }

    function decode(value) {
      return new TextDecoder().decode(value);
    }
    
    async function connect() {
      try {
        const device = await navigator.bluetooth.requestDevice({
          filters: [{name: 'leor'}],
          optionalServices: [SERVICE]
        });
        
        const server = await device.gatt.connect();
        const service = await server.getPrimaryService(SERVICE);
        commandChar = await service.getCharacteristic(CMD);
        statusChar = await service.getCharacteristic(STATUS);
        gestureChar = await service.getCharacteristic(GESTURE);

        await statusChar.startNotifications();
        statusChar.addEventListener('characteristicvaluechanged', (e) => {
          log('[status] ' + decode(e.target.value));
        });

        await gestureChar.startNotifications();
        gestureChar.addEventListener('characteristicvaluechanged', (e) => {
          log('[gesture] ' + decode(e.target.value));
        });

        log('Connected!');
      } catch(error) {
        console.error('Error:', error);
        log('Error: ' + error);
      }
    }
    
    async function sendCommand(cmd) {
      if (!commandChar) {
        log('Connect first');
        return;
      }
      
      const encoder = new TextEncoder();
      await commandChar.writeValue(encoder.encode(cmd));
      log('[tx] ' + cmd);
    }
  </script>
</body>
</html>
```

### Option 3: Python with Bleak

```python
import asyncio
from bleak import BleakClient, BleakScanner

SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
COMMAND_CHAR_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8"
GESTURE_CHAR_UUID = "d1e5f0a1-2b3c-4d5e-6f7a-8b9c0d1e2f3a"

async def find_leor():
    devices = await BleakScanner.discover()
    for d in devices:
        if d.name == "leor":
            return d.address
    return None

async def send_command(address, command):
    async with BleakClient(address) as client:
        await client.write_gatt_char(COMMAND_CHAR_UUID, command.encode())
        print(f"Sent: {command}")

async def main():
    address = await find_leor()
    if not address:
        print("leor not found!")
        return
    
    print(f"Found leor at {address}")
    await send_command(address, "happy")

asyncio.run(main())
```

## Available Commands

### Expressions

`happy`, `sad`, `angry`, `love`, `surprised`, `confused`, `sleepy`, `curious`, `nervous`, `knocked`, `neutral`, `idle`

### Mouth

`smile`, `frown`, `open`, `ooo`, `flat`, `talk`, `chew`, `wobble`

### Actions

`blink`, `wink` (left), `winkr` (right), `laugh`, `cry`

### Look Direction

`nw`, `n`, `ne`, `w`, `center`, `e`, `sw`, `s`, `se`

### Settings

Settings are sent using a single command that starts with `set:`.

Example (safe defaults):

`set:ew=36,eh=36,es=10,er=8,mw=20,lt=1000,vt=2000,bi=3`

What each setting does:

- `ew` (Eye Width): changes how wide the eyes look (bigger = cuter / more “open” eyes)
- `eh` (Eye Height): changes how tall the eyes look
- `es` (Eye Spacing): distance between left/right eye
- `er` (Border Radius): corner roundness (higher = rounder, softer eyes)
- `mw` (Mouth Width): mouth size/width (mouth height is fixed internally)
- `lt` (Laugh Time, ms): how long the laugh animation lasts
- `vt` (Love Time, ms): how long the love/heart-eyes animation lasts
- `bi` (Blink Interval, seconds): base autoblink interval (variation is fixed internally)

Notes:

- These apply immediately.
- They are not persisted to flash by default (reboot resets to whatever the sketch sets).

### Gestures

`gesture:train=wave,happy` - Train a new gesture
`gesture:match=1` - Enable gesture matching
`gesture:match=0` - Disable gesture matching
`gesture:list` - List all gestures
`gesture:delete=wave` - Delete a gesture

### Shuffle (Random Expression Cycling)

Shuffle mode cycles between neutral and random expressions automatically.

**Default:** ON with 2-5s expression, 2-5s neutral

#### Commands

| Command | Description |
|---------|-------------|
| `shuffle:` | Get current status |
| `shuffle:on` | Enable shuffle |
| `shuffle:off` | Disable shuffle |
| `shuffle:expr=N` | Set expression duration to N seconds (fixed) |
| `shuffle:expr=MIN-MAX` | Set expression duration range (random) |
| `shuffle:neutral=N` | Set neutral duration to N seconds (fixed) |
| `shuffle:neutral=MIN-MAX` | Set neutral duration range (random) |

**Short aliases:** `e=` for `expr=`, `n=` for `neutral=`

#### Examples

```bash
shuffle:on                     # Enable shuffle
shuffle:off                    # Disable shuffle
shuffle:expr=3                 # Expression shows for exactly 3s
shuffle:expr=2-5               # Expression shows for random 2-5s
shuffle:neutral=5              # Neutral for exactly 5s
shuffle:n=3-8                  # Neutral for random 3-8s
shuffle:on,expr=2-3,neutral=5  # Combined: enable + set timings
```

#### Response Format

```text
Shuffle: ON, expr=2-5s, neutral=2-5s
```

#### React Native Implementation

```typescript
// Types
interface ShuffleSettings {
  enabled: boolean;
  exprMin: number;  // seconds
  exprMax: number;
  neutralMin: number;
  neutralMax: number;
}

// Parse shuffle status response
function parseShuffleStatus(response: string): ShuffleSettings {
  // Response format: "Shuffle: ON, expr=2-5s, neutral=2-5s"
  const enabled = response.includes('ON');
  
  const exprMatch = response.match(/expr=(\d+)-(\d+)s/);
  const neutralMatch = response.match(/neutral=(\d+)-(\d+)s/);
  
  return {
    enabled,
    exprMin: exprMatch ? parseInt(exprMatch[1]) : 2,
    exprMax: exprMatch ? parseInt(exprMatch[2]) : 5,
    neutralMin: neutralMatch ? parseInt(neutralMatch[1]) : 2,
    neutralMax: neutralMatch ? parseInt(neutralMatch[2]) : 5,
  };
}

// Build shuffle command
function buildShuffleCommand(settings: Partial<ShuffleSettings>): string {
  const parts: string[] = [];
  
  if (settings.enabled !== undefined) {
    parts.push(settings.enabled ? 'on' : 'off');
  }
  
  if (settings.exprMin !== undefined && settings.exprMax !== undefined) {
    if (settings.exprMin === settings.exprMax) {
      parts.push(`expr=${settings.exprMin}`);
    } else {
      parts.push(`expr=${settings.exprMin}-${settings.exprMax}`);
    }
  }
  
  if (settings.neutralMin !== undefined && settings.neutralMax !== undefined) {
    if (settings.neutralMin === settings.neutralMax) {
      parts.push(`neutral=${settings.neutralMin}`);
    } else {
      parts.push(`neutral=${settings.neutralMin}-${settings.neutralMax}`);
    }
  }
  
  return 'shuffle:' + parts.join(',');
}

// Usage examples
await sendCommand('shuffle:');  // Get status
await sendCommand('shuffle:on');
await sendCommand('shuffle:off');
await sendCommand(buildShuffleCommand({ 
  enabled: true, 
  exprMin: 1, 
  exprMax: 3,
  neutralMin: 4,
  neutralMax: 8
}));
// Sends: "shuffle:on,expr=1-3,neutral=4-8"
```

#### UI Component Example

```tsx
import React, { useState } from 'react';
import { View, Switch, Text } from 'react-native';
import Slider from '@react-native-community/slider';

function ShuffleControls({ sendCommand }) {
  const [enabled, setEnabled] = useState(true);
  const [exprTime, setExprTime] = useState([2, 5]);
  const [neutralTime, setNeutralTime] = useState([2, 5]);

  const toggleShuffle = async (value: boolean) => {
    setEnabled(value);
    await sendCommand(`shuffle:${value ? 'on' : 'off'}`);
  };

  const updateTiming = async () => {
    await sendCommand(
      `shuffle:expr=${exprTime[0]}-${exprTime[1]},neutral=${neutralTime[0]}-${neutralTime[1]}`
    );
  };

  return (
    <View>
      <View style={{ flexDirection: 'row', alignItems: 'center' }}>
        <Text>Shuffle Mode</Text>
        <Switch value={enabled} onValueChange={toggleShuffle} />
      </View>
      
      <Text>Expression: {exprTime[0]}-{exprTime[1]}s</Text>
      {/* Add range sliders for timing */}
      
      <Text>Neutral: {neutralTime[0]}-{neutralTime[1]}s</Text>
      {/* Add range sliders for timing */}
    </View>
  );
}
```

## Power note

BLE is typically much lower power than WiFi for an always-on control link, especially when idle. Exact current depends on your board, connection interval, and how often you send commands.

## Troubleshooting

**Can’t find leor?**

- Make sure Bluetooth is enabled on your phone/laptop
- Make sure the device name matches your `BLE_DEVICE_NAME`
- If you were connected before: disconnect in your BLE app, then power-cycle leor

**Connection drops?**

- BLE has shorter range than WiFi (~10m typical)
- Move closer to leor
- Reduce interference from other BLE devices

**Commands not working?**

- Enable notifications on **Status** to see the reply text
- Make sure you’re writing to the **Command** characteristic
- Check Serial Monitor for `[BLE] RX:` logs
