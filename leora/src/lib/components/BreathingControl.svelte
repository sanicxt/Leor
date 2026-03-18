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

<div class="bento-card bg-bento-blue p-6 space-y-5">
    <!-- Header with Main Toggle -->
    <div class="mb-4 border-b-2 border-bento-border pb-2 flex items-center justify-between">
        <div>
            <h2 class="text-xl font-black uppercase">Breathing Effect</h2>
            <p class="text-sm font-bold opacity-80">Living eye animation</p>
        </div>
        <button
            class="w-14 h-8 rounded-full border-2 border-bento-border transition-all duration-300 relative focus:outline-none disabled:opacity-50
             {breathingEnabled ? 'bg-bento-yellow shadow-[2px_2px_0px_0px_var(--color-bento-border)]' : 'bg-paper shadow-[2px_2px_0px_0px_var(--color-bento-border)]'}"
            onclick={toggleBreathing}
            disabled={!bleState.connected}
            aria-label="Toggle breathing"
        >
            <span
                class="absolute left-1 top-0.5 w-6 h-6 bg-paper border-[1.5px] border-bento-border rounded-full transition-transform duration-300 flex items-center justify-center
               {breathingEnabled ? 'translate-x-6' : 'translate-x-0'}"
            >
                {#if breathingEnabled}
                    <svg class="w-3 h-3 text-ink" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                        <path stroke-linecap="round" stroke-linejoin="round" stroke-width="3" d="M5 13l4 4L19 7" />
                    </svg>
                {:else}
                    <svg class="w-3 h-3 text-ink" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                        <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M6 18L18 6M6 6l12 12" />
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
            class="bento-button px-3 py-2 bg-paper text-ink text-xs disabled:opacity-50"
        >
            Subtle
        </button>
        <button
            onclick={setNormal}
            disabled={!bleState.connected}
            class="bento-button px-3 py-2 bg-paper text-ink text-xs disabled:opacity-50"
        >
            Normal
        </button>
        <button
            onclick={setStrong}
            disabled={!bleState.connected}
            class="bento-button px-3 py-2 bg-paper text-ink text-xs disabled:opacity-50"
        >
            Strong
        </button>
    </div>

    <!-- Intensity Slider -->
    <div class="p-3 rounded-xl bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] space-y-2">
        <div class="flex justify-between items-center">
            <label for="breath-intensity" class="text-ink text-xs font-bold uppercase tracking-wider">
                Intensity
            </label>
            <span class="text-ink text-xs font-mono font-bold px-2 py-0.5 bg-paper border-2 border-bento-border rounded-lg shadow-[2px_2px_0px_0px_var(--color-bento-border)]">
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
            oninput={(e) => updateIntensity(parseFloat((e.target as HTMLInputElement).value))}
            disabled={!bleState.connected}
            class="w-full h-2 bg-paper border-2 border-bento-border rounded-full appearance-none cursor-pointer disabled:opacity-50
                   [&::-webkit-slider-thumb]:appearance-none
                   [&::-webkit-slider-thumb]:w-4
                   [&::-webkit-slider-thumb]:h-4
                   [&::-webkit-slider-thumb]:rounded-sm
                   [&::-webkit-slider-thumb]:bg-bento-yellow
                   [&::-webkit-slider-thumb]:shadow-[2px_2px_0px_0px_var(--color-bento-border)]
                   [&::-webkit-slider-thumb]:cursor-pointer
                   [&::-webkit-slider-thumb]:border-2
                   [&::-webkit-slider-thumb]:border-bento-border
                   [&::-webkit-slider-thumb]:transition-transform
                   [&::-webkit-slider-thumb]:active:scale-125
                   [&::-webkit-slider-thumb]:active:shadow-none
                   [&::-webkit-slider-thumb]:active:translate-y-[2px]
                   [&::-webkit-slider-thumb]:active:translate-x-[2px]"
        />
        <div class="flex justify-between text-[10px] text-ink/60 font-bold">
            <span>Subtle</span>
            <span>Strong</span>
        </div>
    </div>

    <!-- Speed Slider -->
    <div class="p-3 rounded-xl bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] space-y-2">
        <div class="flex justify-between items-center">
            <label for="breath-speed" class="text-ink text-xs font-bold uppercase tracking-wider">
                Speed
            </label>
            <span class="text-ink text-xs font-mono font-bold px-2 py-0.5 bg-paper border-2 border-bento-border rounded-lg shadow-[2px_2px_0px_0px_var(--color-bento-border)]">
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
            oninput={(e) => updateSpeed(parseFloat((e.target as HTMLInputElement).value))}
            disabled={!bleState.connected}
            class="w-full h-2 bg-paper border-2 border-bento-border rounded-full appearance-none cursor-pointer disabled:opacity-50
                   [&::-webkit-slider-thumb]:appearance-none
                   [&::-webkit-slider-thumb]:w-4
                   [&::-webkit-slider-thumb]:h-4
                   [&::-webkit-slider-thumb]:rounded-sm
                   [&::-webkit-slider-thumb]:bg-bento-yellow
                   [&::-webkit-slider-thumb]:shadow-[2px_2px_0px_0px_var(--color-bento-border)]
                   [&::-webkit-slider-thumb]:cursor-pointer
                   [&::-webkit-slider-thumb]:border-2
                   [&::-webkit-slider-thumb]:border-bento-border
                   [&::-webkit-slider-thumb]:transition-transform
                   [&::-webkit-slider-thumb]:active:scale-125
                   [&::-webkit-slider-thumb]:active:shadow-none
                   [&::-webkit-slider-thumb]:active:translate-y-[2px]
                   [&::-webkit-slider-thumb]:active:translate-x-[2px]"
        />
        <div class="flex justify-between text-[10px] text-ink/60 font-bold">
            <span>Slow (10s)</span>
            <span>Fast (1s)</span>
        </div>
    </div>
</div>
