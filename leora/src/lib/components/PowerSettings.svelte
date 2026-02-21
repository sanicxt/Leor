<script lang="ts">
    import {
        sendCommand,
        bleState,
        getSettingsTd,
        setSettingsTd,
        getSettingsWp,
        setSettingsWp,
        getSettingsPp,
        setSettingsPp,
    } from "$lib/ble.svelte";

    let touchHoldDelay = $derived(getSettingsTd());
    let wakePin = $derived(getSettingsWp());
    let pwrPin = $derived(getSettingsPp());

    // ESP32-C3 RTC-capable GPIO pins (0-5)
    const rtcPins = [0, 1, 2, 3, 4, 5];

    async function toggleLowPowerMode() {
        const newVal = !bleState.bleLowPowerMode;
        bleState.bleLowPowerMode = newVal;
        await sendCommand(`ble:lp=${newVal ? "1" : "0"}`);
    }

    async function updateTouchHoldDelay(value: number) {
        setSettingsTd(value);
        await sendCommand(`s:td=${value}`);
    }

    async function updateWakePin(value: number) {
        setSettingsWp(value);
        await sendCommand(`s:wp=${value}`);
    }

    async function updatePwrPin(value: number) {
        setSettingsPp(value);
        await sendCommand(`s:pp=${value}`);
    }

    let rebooting = $state(false);

    async function rebootDevice() {
        rebooting = true;
        await sendCommand("restart");
        // Device disconnects on reboot; show spinner briefly
        setTimeout(() => {
            rebooting = false;
        }, 3000);
    }
</script>

<div
    class="bg-gradient-to-br from-sky-950/40 to-blue-950/30 border border-sky-500/20 rounded-2xl p-5 backdrop-blur-lg"
>
    <!-- Header -->
    <div class="flex items-center justify-between mb-4">
        <div class="flex items-center gap-3">
            <div
                class="w-10 h-10 rounded-xl bg-gradient-to-br from-sky-500 to-blue-600 flex items-center justify-center shadow-lg shadow-sky-500/20"
            >
                <svg
                    class="w-5 h-5 text-white"
                    fill="none"
                    stroke="currentColor"
                    viewBox="0 0 24 24"
                >
                    <path
                        stroke-linecap="round"
                        stroke-linejoin="round"
                        stroke-width="2"
                        d="M13 10V3L4 14h7v7l9-11h-7z"
                    />
                </svg>
            </div>
            <div>
                <h2 class="text-lg font-semibold text-white">Power Settings</h2>
                <p class="text-xs text-zinc-400">BLE power management</p>
            </div>
        </div>
    </div>

    <!-- Touch Hold Delay -->
    <div class="mt-4 p-4 bg-white/5 rounded-xl border border-white/5 space-y-3">
        <div class="flex items-center justify-between">
            <div class="flex items-center gap-3">
                <div
                    class="w-10 h-10 rounded-lg bg-sky-500/20 flex items-center justify-center"
                >
                    <svg
                        class="w-5 h-5 text-sky-400"
                        fill="none"
                        stroke="currentColor"
                        viewBox="0 0 24 24"
                    >
                        <path
                            stroke-linecap="round"
                            stroke-linejoin="round"
                            stroke-width="2"
                            d="M9 11V6a3 3 0 016 0v5m-6 0h6m-6 0v5a3 3 0 006 0v-5"
                        />
                    </svg>
                </div>
                <div>
                    <div class="text-white font-medium text-sm">
                        Touch Hold to Sleep
                    </div>
                    <div class="text-zinc-500 text-xs">
                        Long touch duration before deep sleep
                    </div>
                </div>
            </div>
            <span class="text-sky-300 text-xs font-mono"
                >{(touchHoldDelay / 1000).toFixed(1)}s</span
            >
        </div>

        <input
            type="range"
            min="1000"
            max="15000"
            step="500"
            value={touchHoldDelay}
            onchange={(e) =>
                updateTouchHoldDelay(parseInt(e.currentTarget.value))}
            disabled={!bleState.connected}
            class="w-full h-1.5 bg-zinc-700 rounded-full appearance-none cursor-pointer accent-sky-500 disabled:opacity-50"
        />

        <div class="flex justify-between text-[10px] text-zinc-500 px-1">
            <span>1s</span>
            <span>15s</span>
        </div>
    </div>

    <!-- Pin Configuration -->
    <div class="mt-3 p-4 bg-white/5 rounded-xl border border-white/5 space-y-4">
        <div class="flex items-center gap-3 mb-1">
            <div
                class="w-10 h-10 rounded-lg bg-sky-500/20 flex items-center justify-center"
            >
                <svg
                    class="w-5 h-5 text-sky-400"
                    fill="none"
                    stroke="currentColor"
                    viewBox="0 0 24 24"
                >
                    <path
                        stroke-linecap="round"
                        stroke-linejoin="round"
                        stroke-width="2"
                        d="M9 3H5a2 2 0 00-2 2v4m6-6h10a2 2 0 012 2v4M9 3v18m0 0h10a2 2 0 002-2V9M9 21H5a2 2 0 01-2-2V9m0 0h18"
                    />
                </svg>
            </div>
            <div>
                <div class="text-white font-medium text-sm">
                    GPIO Pin Assignment
                </div>
                <div class="text-zinc-500 text-xs">
                    RTC-capable only (0–5) · Restart required
                </div>
            </div>
        </div>

        <!-- Wake Pin -->
        <div class="flex items-center justify-between">
            <div>
                <div class="text-zinc-300 text-sm">Touch / Wake pin</div>
                <div class="text-zinc-500 text-xs">
                    Active-HIGH touch module input
                </div>
            </div>
            <div class="flex gap-1">
                {#each rtcPins as pin}
                    <button
                        class="w-8 h-8 rounded-lg text-xs font-mono font-bold transition-all
                            {wakePin === pin
                            ? 'bg-sky-500 text-white shadow-lg shadow-sky-500/30'
                            : 'bg-zinc-800 text-zinc-400 hover:bg-zinc-700'}
                            disabled:opacity-50"
                        onclick={() => updateWakePin(pin)}
                        disabled={!bleState.connected || pin === pwrPin}
                        >{pin}</button
                    >
                {/each}
            </div>
        </div>

        <!-- Power Control Pin -->
        <div class="flex items-center justify-between">
            <div>
                <div class="text-zinc-300 text-sm">Power control pin</div>
                <div class="text-zinc-500 text-xs">
                    PNP base · LOW=on, HIGH=off
                </div>
            </div>
            <div class="flex gap-1">
                {#each rtcPins as pin}
                    <button
                        class="w-8 h-8 rounded-lg text-xs font-mono font-bold transition-all
                            {pwrPin === pin
                            ? 'bg-sky-500 text-white shadow-lg shadow-sky-500/30'
                            : 'bg-zinc-800 text-zinc-400 hover:bg-zinc-700'}
                            disabled:opacity-50"
                        onclick={() => updatePwrPin(pin)}
                        disabled={!bleState.connected || pin === wakePin}
                        >{pin}</button
                    >
                {/each}
            </div>
        </div>

        <p class="text-[10px] text-amber-500/80 flex items-center gap-1">
            <svg
                class="w-3 h-3 shrink-0"
                fill="currentColor"
                viewBox="0 0 20 20"
                ><path
                    fill-rule="evenodd"
                    d="M8.257 3.099c.765-1.36 2.722-1.36 3.486 0l5.58 9.92c.75 1.334-.213 2.98-1.742 2.98H4.42c-1.53 0-2.493-1.646-1.743-2.98l5.58-9.92zM11 13a1 1 0 11-2 0 1 1 0 012 0zm-1-8a1 1 0 00-1 1v3a1 1 0 002 0V6a1 1 0 00-1-1z"
                    clip-rule="evenodd"
                /></svg
            >
            Changes take effect after device restart. Wake and power pins cannot
            be the same.
        </p>

        <!-- Reboot Button -->
        <button
            onclick={rebootDevice}
            disabled={!bleState.connected || rebooting}
            class="w-full py-2.5 rounded-xl font-semibold text-sm transition-all
                   {bleState.connected && !rebooting
                ? 'bg-gradient-to-r from-amber-500/80 to-orange-500/80 text-white hover:opacity-90 shadow-lg shadow-amber-500/15'
                : 'bg-zinc-800 text-zinc-600 cursor-not-allowed'}
                   disabled:opacity-50"
        >
            {#if rebooting}
                <span class="flex items-center justify-center gap-2">
                    <svg
                        class="w-4 h-4 animate-spin"
                        fill="none"
                        viewBox="0 0 24 24"
                    >
                        <circle
                            class="opacity-25"
                            cx="12"
                            cy="12"
                            r="10"
                            stroke="currentColor"
                            stroke-width="4"
                        />
                        <path
                            class="opacity-75"
                            fill="currentColor"
                            d="M4 12a8 8 0 018-8v8z"
                        />
                    </svg>
                    Rebooting...
                </span>
            {:else}
                Reboot Device
            {/if}
        </button>
    </div>

    <!-- Low Power Toggle -->
    <div
        class="flex items-center justify-between p-4 bg-white/5 rounded-xl border border-white/5"
    >
        <div class="flex items-center gap-3">
            <div
                class="w-10 h-10 rounded-lg bg-sky-500/20 flex items-center justify-center"
            >
                <svg
                    class="w-5 h-5 text-sky-400"
                    fill="none"
                    stroke="currentColor"
                    viewBox="0 0 24 24"
                >
                    <path
                        stroke-linecap="round"
                        stroke-linejoin="round"
                        stroke-width="2"
                        d="M9 3v2m6-2v2M9 19v2m6-2v2M5 9H3m2 6H3m18-6h-2m2 6h-2M7 19h10a2 2 0 002-2V7a2 2 0 00-2-2H7a2 2 0 00-2 2v10a2 2 0 002 2zM9 9h6v6H9V9z"
                    />
                </svg>
            </div>
            <div>
                <div class="text-white font-medium text-sm">Low Power Mode</div>
                <div class="text-zinc-500 text-xs">
                    Reduce TX power & slower advertising
                </div>
            </div>
        </div>

        <button
            class="w-14 h-8 rounded-full transition-all duration-300 relative {bleState.bleLowPowerMode
                ? 'bg-gradient-to-r from-sky-500 to-blue-500 shadow-lg shadow-sky-500/30'
                : 'bg-zinc-700/80'} disabled:opacity-50"
            onclick={toggleLowPowerMode}
            disabled={!bleState.connected}
            aria-label="Toggle low power mode"
        >
            <span
                class="absolute left-1 top-1 w-6 h-6 bg-white rounded-full transition-transform duration-300 shadow-md flex items-center justify-center {bleState.bleLowPowerMode
                    ? 'translate-x-6'
                    : 'translate-x-0'}"
            >
                {#if bleState.bleLowPowerMode}
                    <svg
                        class="w-3.5 h-3.5 text-sky-500"
                        fill="none"
                        stroke="currentColor"
                        viewBox="0 0 24 24"
                    >
                        <path
                            stroke-linecap="round"
                            stroke-linejoin="round"
                            stroke-width="2"
                            d="M5 13l4 4L19 7"
                        />
                    </svg>
                {:else}
                    <svg
                        class="w-3.5 h-3.5 text-zinc-400"
                        fill="none"
                        stroke="currentColor"
                        viewBox="0 0 24 24"
                    >
                        <path
                            stroke-linecap="round"
                            stroke-linejoin="round"
                            stroke-width="2"
                            d="M6 18L18 6M6 6l12 12"
                        />
                    </svg>
                {/if}
            </span>
        </button>
    </div>

    <!-- Power Info -->
    <div class="mt-3 text-xs text-zinc-500 flex items-center gap-2">
        <svg
            class="w-4 h-4"
            fill="none"
            stroke="currentColor"
            viewBox="0 0 24 24"
        >
            <path
                stroke-linecap="round"
                stroke-linejoin="round"
                stroke-width="2"
                d="M13 16h-1v-4h-1m1-4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z"
            />
        </svg>
        <span>
            {#if bleState.bleLowPowerMode}
                TX Power: -3dBm • Range: ~10m • Slower advertising
            {:else}
                TX Power: +9dBm • Range: ~30m • Fast advertising
            {/if}
        </span>
    </div>
</div>
