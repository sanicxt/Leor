<script lang="ts">
    import {
        sendCommand,
        bleState,
        getBreathingEnabled,
        getBreathingIntensity,
        getBreathingSpeed,
        setBreathingEnabled,
        setBreathingIntensity,
        setBreathingSpeed,
    } from "$lib/ble.svelte";

    let breathingEnabled = $derived(getBreathingEnabled());
    let breathingIntensity = $derived(getBreathingIntensity());
    let breathingSpeed = $derived(getBreathingSpeed());

    async function toggleBreathing() {
        const newVal = !breathingEnabled;
        setBreathingEnabled(newVal);
        await sendCommand(`br=${newVal ? 1 : 0}`);
    }

    async function updateIntensity(value: number) {
        setBreathingIntensity(value);
        await sendCommand(`bri=${value.toFixed(2)}`);
    }

    async function updateSpeed(value: number) {
        setBreathingSpeed(value);
        await sendCommand(`brs=${value.toFixed(2)}`);
    }

    // Preset buttons
    async function setSubtle() {
        await updateIntensity(0.03);
        await updateSpeed(0.25);
    }

    async function setNormal() {
        await updateIntensity(0.08);
        await updateSpeed(0.3);
    }

    async function setStrong() {
        await updateIntensity(0.15);
        await updateSpeed(0.4);
    }
</script>

<div
    class="bg-gradient-to-br from-cyan-950/40 to-blue-950/30 border border-cyan-500/20 rounded-2xl p-5 backdrop-blur-lg space-y-5"
>
    <!-- Header with Main Toggle -->
    <div class="flex items-center justify-between">
        <div class="flex items-center gap-3">
            <div
                class="w-10 h-10 rounded-xl bg-gradient-to-br from-cyan-500 to-blue-600 flex items-center justify-center shadow-lg shadow-cyan-500/20"
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
                        d="M4.318 6.318a4.5 4.5 0 000 6.364L12 20.364l7.682-7.682a4.5 4.5 0 00-6.364-6.364L12 7.636l-1.318-1.318a4.5 4.5 0 00-6.364 0z"
                    />
                </svg>
            </div>
            <div>
                <h3 class="text-white text-sm font-semibold">
                    Breathing Effect
                </h3>
                <p class="text-cyan-300/60 text-xs">Living eye animation</p>
            </div>
        </div>
        <button
            class="w-14 h-8 rounded-full transition-all duration-300 relative focus:outline-none focus:ring-2 focus:ring-cyan-500/50 disabled:opacity-50
             {breathingEnabled
                ? 'bg-gradient-to-r from-cyan-500 to-blue-500 shadow-lg shadow-cyan-500/30'
                : 'bg-zinc-700/80'}"
            onclick={toggleBreathing}
            disabled={!bleState.connected}
            aria-label="Toggle breathing"
        >
            <span
                class="absolute left-1 top-1 w-6 h-6 bg-white rounded-full transition-transform duration-300 shadow-md flex items-center justify-center
               {breathingEnabled ? 'translate-x-6' : 'translate-x-0'}"
            >
                {#if breathingEnabled}
                    <svg
                        class="w-3.5 h-3.5 text-cyan-500"
                        fill="none"
                        stroke="currentColor"
                        viewBox="0 0 24 24"
                    >
                        <path
                            stroke-linecap="round"
                            stroke-linejoin="round"
                            stroke-width="3"
                            d="M5 13l4 4L19 7"
                        />
                    </svg>
                {/if}
            </span>
        </button>
    </div>

    <!-- Quick Presets -->
    <div class="grid grid-cols-3 gap-2">
        <button
            onclick={setSubtle}
            disabled={!bleState.connected}
            class="px-3 py-2 bg-cyan-500/10 hover:bg-cyan-500/20 border border-cyan-500/30 text-cyan-200 rounded-lg font-medium transition-all disabled:opacity-50 disabled:cursor-not-allowed text-xs"
        >
            Subtle
        </button>
        <button
            onclick={setNormal}
            disabled={!bleState.connected}
            class="px-3 py-2 bg-cyan-500/10 hover:bg-cyan-500/20 border border-cyan-500/30 text-cyan-200 rounded-lg font-medium transition-all disabled:opacity-50 disabled:cursor-not-allowed text-xs"
        >
            Normal
        </button>
        <button
            onclick={setStrong}
            disabled={!bleState.connected}
            class="px-3 py-2 bg-cyan-500/10 hover:bg-cyan-500/20 border border-cyan-500/30 text-cyan-200 rounded-lg font-medium transition-all disabled:opacity-50 disabled:cursor-not-allowed text-xs"
        >
            Strong
        </button>
    </div>

    <!-- Intensity Slider -->
    <div class="p-3 rounded-xl bg-white/5 space-y-2">
        <div class="flex justify-between items-center">
            <label
                for="breath-intensity"
                class="text-cyan-300 text-xs font-medium"
            >
                Intensity
            </label>
            <span
                class="text-cyan-400 text-xs font-mono px-2 py-0.5 rounded"
            >
                {(breathingIntensity * 100).toFixed(0)}%
            </span>
        </div>
        <input
            id="breath-intensity"
            type="range"
            min="0.01"
            max="0.20"
            step="0.01"
            value={breathingIntensity}
            oninput={(e) =>
                updateIntensity(
                    parseFloat((e.target as HTMLInputElement).value),
                )}
            disabled={!bleState.connected}
            class="w-full h-2 bg-zinc-700/50 rounded-lg appearance-none cursor-pointer accent-cyan-500 disabled:opacity-50 disabled:cursor-not-allowed
                   [&::-webkit-slider-thumb]:appearance-none [&::-webkit-slider-thumb]:w-4 [&::-webkit-slider-thumb]:h-4
                   [&::-webkit-slider-thumb]:rounded-full [&::-webkit-slider-thumb]:bg-gradient-to-br
                   [&::-webkit-slider-thumb]:from-cyan-400 [&::-webkit-slider-thumb]:to-blue-500
                   [&::-webkit-slider-thumb]:shadow-lg [&::-webkit-slider-thumb]:shadow-cyan-500/50
                   [&::-webkit-slider-thumb]:cursor-pointer"
        />
        <div class="flex justify-between text-[10px] text-cyan-300/40">
            <span>Subtle</span>
            <span>Strong</span>
        </div>
    </div>

    <!-- Speed Slider -->
    <div class="p-3 rounded-xl bg-white/5 space-y-2">
        <div class="flex justify-between items-center">
            <label for="breath-speed" class="text-cyan-300 text-xs font-medium">
                Speed
            </label>
            <span class="text-cyan-400 text-xs font-mono">
                {(1 / breathingSpeed).toFixed(1)}s
            </span>
        </div>
        <input
            id="breath-speed"
            type="range"
            min="0.1"
            max="1.0"
            step="0.05"
            value={breathingSpeed}
            oninput={(e) =>
                updateSpeed(parseFloat((e.target as HTMLInputElement).value))}
            disabled={!bleState.connected}
            class="w-full h-2 bg-zinc-700/50 rounded-lg appearance-none cursor-pointer accent-cyan-500 disabled:opacity-50 disabled:cursor-not-allowed
                   [&::-webkit-slider-thumb]:appearance-none [&::-webkit-slider-thumb]:w-4 [&::-webkit-slider-thumb]:h-4
                   [&::-webkit-slider-thumb]:rounded-full [&::-webkit-slider-thumb]:bg-gradient-to-br
                   [&::-webkit-slider-thumb]:from-cyan-400 [&::-webkit-slider-thumb]:to-blue-500
                   [&::-webkit-slider-thumb]:shadow-lg [&::-webkit-slider-thumb]:shadow-cyan-500/50
                   [&::-webkit-slider-thumb]:cursor-pointer"
        />
        <div class="flex justify-between text-[10px] text-cyan-300/40">
            <span>Slow (10s)</span>
            <span>Fast (1s)</span>
        </div>
    </div>
</div>
