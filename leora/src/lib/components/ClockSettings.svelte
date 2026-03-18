<script lang="ts">
    import { onMount } from "svelte";
    import {
        bleState,
        sendCommand,
        getClockEnabled,
        getClockSeconds,
        getClockTimezoneOffset,
        getClock24Hour,
    } from "$lib/ble.svelte";

    let browserNow = $state(new Date());
    let syncing = $state(false);
    let formatBusy = $state(false);

    function pad2(value: number) {
        return String(value).padStart(2, "0");
    }

    function formatDeviceClock(totalSeconds: number, use24: boolean) {
        const sec = ((totalSeconds % 86400) + 86400) % 86400;
        let hh = Math.floor(sec / 3600);
        const mm = Math.floor((sec % 3600) / 60);
        let ampm = "";

        if (!use24) {
            ampm = hh >= 12 ? "PM" : "AM";
            hh = hh % 12;
            if (hh === 0) hh = 12;
        }

        return {
            time: `${use24 ? pad2(hh) : hh}:${pad2(mm)}`,
            ampm,
        };
    }

    function formatBrowserTime(date = new Date()) {
        return `${pad2(date.getHours())}:${pad2(date.getMinutes())}:${pad2(date.getSeconds())}`;
    }

    async function syncNow() {
        if (!bleState.connected) return;
        syncing = true;
        try {
            await sendCommand(`clock:sync=${Date.now()},${new Date().getTimezoneOffset()}`);
        } finally {
            syncing = false;
        }
    }

    async function toggleClock() {
        if (!bleState.connected) return;
        syncing = true;
        try {
            const enabled = !getClockEnabled();
            bleState.clockEnabled = enabled;
            await sendCommand(enabled ? "clock:on" : "clock:off");
        } finally {
            syncing = false;
        }
    }

    async function setFormat(use24: boolean) {
        if (!bleState.connected) return;
        formatBusy = true;
        try {
            bleState.clock24Hour = use24;
            await sendCommand(`clock:fmt=${use24 ? '24' : '12'}`);
        } finally {
            formatBusy = false;
        }
    }

    onMount(() => {
        const timer = setInterval(() => {
            browserNow = new Date();
        }, 1000);

        return () => clearInterval(timer);
    });
</script>

<div class="bento-card bg-bento-peach p-6 space-y-5">
    <div class="mb-4 border-b-2 border-bento-border pb-2 flex items-center justify-between gap-3">
        <div>
            <h2 class="text-xl font-black uppercase tracking-tight">Clock</h2>
            <p class="text-sm font-bold opacity-80">Browser sync for the OLED clock</p>
        </div>
        <div class="px-3 py-1 rounded-full border-2 border-bento-border bg-paper shadow-[2px_2px_0px_0px_var(--color-bento-border)] text-[10px] font-black uppercase tracking-wider text-ink">
            {bleState.connected ? "BLE connected" : "BLE disconnected"}
        </div>
    </div>

    <button
        onclick={toggleClock}
        disabled={!bleState.connected || syncing}
        class={`bento-button w-full rounded-xl border-2 border-bento-border px-4 py-3 text-left transition-all disabled:opacity-50 ${getClockEnabled() ? 'bg-bento-blue text-ink shadow-[2px_2px_0px_0px_var(--color-bento-border)]' : 'bg-paper text-ink'}`}
    >
        <div class="flex items-center justify-between gap-3">
            <div>
                <div class="text-xs font-black uppercase tracking-wider">Clock Power</div>
                <div class="text-[10px] font-bold opacity-70">Turn the OLED clock on or off</div>
            </div>
            <div class="flex items-center gap-2">
                <div class={`w-12 h-7 rounded-full border-2 border-bento-border p-1 transition-all ${getClockEnabled() ? 'bg-bento-green' : 'bg-paper'}`}>
                    <div class={`h-4 w-4 rounded-full bg-ink transition-transform ${getClockEnabled() ? 'translate-x-5' : 'translate-x-0'}`}></div>
                </div>
                <span class="text-[10px] font-black uppercase tracking-wider">{getClockEnabled() ? 'On' : 'Off'}</span>
            </div>
        </div>
    </button>

    <div class="space-y-3">
        <div class="p-4 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl space-y-3">
            <div class="flex items-center justify-between gap-2">
                <div>
                    <div class="text-ink font-black uppercase text-xs tracking-wider">Browser Time</div>
                    <div class="text-ink/60 text-[10px] font-bold">Used to sync the OLED</div>
                </div>
                <span class="font-mono text-sm font-black px-2.5 py-1 rounded-xl border-2 border-bento-border bg-paper text-ink shadow-[2px_2px_0px_0px_var(--color-bento-border)]">
                    {formatBrowserTime(browserNow)}
                </span>
            </div>

            <div class="flex items-center justify-between gap-2 text-[10px] font-bold">
                <span class="uppercase text-ink/60">Device</span>
                <span class="font-mono text-sm text-ink">{formatDeviceClock(getClockSeconds(), getClock24Hour()).time}{getClock24Hour() ? '' : ` ${formatDeviceClock(getClockSeconds(), false).ampm}`}</span>
            </div>
        </div>

        <div class="p-4 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl space-y-3">
            <div class="flex items-center justify-between gap-2">
                <div>
                    <div class="text-ink font-black uppercase text-xs tracking-wider">Display Format</div>
                    <div class="text-ink/60 text-[10px] font-bold">Match the OLED theme</div>
                </div>
                <div class="px-2.5 py-1 rounded-full border-2 border-bento-border bg-bento-yellow text-[10px] font-black uppercase tracking-wider text-ink shadow-[2px_2px_0px_0px_var(--color-bento-border)]">
                    {getClock24Hour() ? "24h" : "12h"}
                </div>
            </div>

            <div class="grid grid-cols-2 gap-2">
                <button
                    onclick={() => setFormat(false)}
                    disabled={!bleState.connected || formatBusy}
                    class="bento-button rounded-xl border-2 border-bento-border px-3 py-3 text-left transition-all disabled:opacity-50 {getClock24Hour() ? 'bg-paper text-ink' : 'bg-bento-pink text-ink shadow-[2px_2px_0px_0px_var(--color-bento-border)]'}"
                >
                    <div class="text-xs font-black uppercase tracking-wider">12 Hour</div>
                    <div class="text-[10px] font-bold opacity-70">AM / PM</div>
                </button>
                <button
                    onclick={() => setFormat(true)}
                    disabled={!bleState.connected || formatBusy}
                    class="bento-button rounded-xl border-2 border-bento-border px-3 py-3 text-left transition-all disabled:opacity-50 {getClock24Hour() ? 'bg-bento-blue text-ink shadow-[2px_2px_0px_0px_var(--color-bento-border)]' : 'bg-paper text-ink'}"
                >
                    <div class="text-xs font-black uppercase tracking-wider">24 Hour</div>
                    <div class="text-[10px] font-bold opacity-70">00:00</div>
                </button>
            </div>
        </div>

        <button
            onclick={syncNow}
            disabled={!bleState.connected || syncing}
            class="bento-button w-full px-3 py-3 rounded-xl bg-bento-pink text-ink text-sm disabled:opacity-50"
        >
            {syncing ? "Syncing…" : "Sync now"}
        </button>
    </div>
</div>
