<script lang="ts">
    import { sendCommand, bleState } from "$lib/ble.svelte";

    async function toggleLowPowerMode() {
        const newVal = !bleState.bleLowPowerMode;
        bleState.bleLowPowerMode = newVal;
        await sendCommand(`ble:lp=${newVal ? "1" : "0"}`);
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
