<script lang="ts">
    import {
        sendCommand,
        bleState,
        getShuffleEnabled,
        setShuffleEnabled,
        getShuffleExprMin,
        getShuffleExprMax,
        getShuffleNeutralMin,
        getShuffleNeutralMax,
        setShuffleExprMin,
        setShuffleExprMax,
        setShuffleNeutralMin,
        setShuffleNeutralMax,
    } from "$lib/ble.svelte";
    import DualRangeSlider from "$lib/components/DualRangeSlider.svelte";

    async function toggleShuffle() {
        const newVal = !getShuffleEnabled();
        setShuffleEnabled(newVal);
        await sendCommand(`sh:${newVal ? "on" : "off"}`);
    }

    async function handleExprChange(min: number, max: number) {
        setShuffleExprMin(min);
        setShuffleExprMax(max);
        await sendCommand(`sh:e=${min}-${max}`);
    }

    async function handleNeutralChange(min: number, max: number) {
        setShuffleNeutralMin(min);
        setShuffleNeutralMax(max);
        await sendCommand(`sh:n=${min}-${max}`);
    }

    // Quick shuffle presets - just update sliders and send values
    async function applyQuickChange() {
        // Fast: 1-2s expressions, 0.5-1.5s neutral
        await handleExprChange(1, 2);
        await handleNeutralChange(1, 2);
    }
    async function applySlowChange() {
        // Slow: 4-8s expressions, 3-6s neutral
        await handleExprChange(4, 8);
        await handleNeutralChange(3, 6);
    }
</script>

<div class="bento-card bg-bento-pink p-6 space-y-6">
    <!-- Header -->
    <div class="mb-4 border-b-2 border-bento-border pb-2 flex items-center justify-between">
        <div>
            <h2 class="text-xl font-black uppercase">Shuffle Mode</h2>
            <p class="text-sm font-bold opacity-80">Auto-cycle expressions</p>
        </div>
        <button
            class="w-14 h-8 rounded-full border-2 border-bento-border transition-all duration-300 relative focus:outline-none disabled:opacity-50
             {getShuffleEnabled() ? 'bg-bento-green shadow-[2px_2px_0px_0px_var(--color-bento-border)]' : 'bg-paper shadow-[2px_2px_0px_0px_var(--color-bento-border)]'}"
            onclick={toggleShuffle}
            disabled={!bleState.connected}
        >
            <span class="absolute left-1 top-0.5 w-6 h-6 bg-paper border-[1.5px] border-bento-border rounded-full transition-transform duration-300 flex items-center justify-center {getShuffleEnabled() ? 'translate-x-6' : 'translate-x-0'}">
                {#if getShuffleEnabled()}
                    <svg class="w-3 h-3 text-ink" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="3" d="M5 13l4 4L19 7" /></svg>
                {:else}
                    <svg class="w-3 h-3 text-ink" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M6 18L18 6M6 6l12 12" /></svg>
                {/if}
            </span>
        </button>
    </div>

    {#if getShuffleEnabled()}
        <div class="flex items-center justify-center gap-2 py-2 px-4 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl -mt-2">
            <span class="w-2 h-2 bg-bento-green border border-bento-border rounded-full animate-pulse"></span>
            <span class="text-ink text-xs font-bold uppercase tracking-wider text-center">Shuffling active</span>
        </div>
    {/if}

    <div class="space-y-4">
        <!-- Expression Duration -->
        <div class="space-y-2 p-3 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl">
            <div class="flex items-center justify-between">
                <div class="flex items-center gap-2">
                    <svg class="w-3.5 h-3.5 text-ink" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 8v4l3 3m6-3a9 9 0 11-18 0 9 9 0 0118 0z" /></svg>
                    <span class="text-ink/80 text-xs font-bold uppercase tracking-wider">Expr Time</span>
                </div>
                <span class="text-ink font-mono font-bold text-xs px-2 py-0.5 bg-paper border-2 border-bento-border rounded-lg shadow-[2px_2px_0px_0px_var(--color-bento-border)]">{getShuffleExprMin()}-{getShuffleExprMax()}s</span>
            </div>
            <DualRangeSlider min={1} max={10} valueMin={getShuffleExprMin()} valueMax={getShuffleExprMax()} onchange={handleExprChange} disabled={!bleState.connected} />
            <p class="text-ink/60 font-bold text-[10px]">Duration each expression is displayed</p>
        </div>

        <!-- Rest Period -->
        <div class="space-y-2 p-3 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl">
            <div class="flex items-center justify-between">
                <div class="flex items-center gap-2">
                    <svg class="w-3.5 h-3.5 text-ink" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M20.354 15.354A9 9 0 018.646 3.646 9.003 9.003 0 0012 21a9.003 9.003 0 008.354-5.646z" /></svg>
                    <span class="text-ink/80 text-xs font-bold uppercase tracking-wider">Rest Period</span>
                </div>
                <span class="text-ink font-mono font-bold text-xs px-2 py-0.5 bg-paper border-2 border-bento-border rounded-lg shadow-[2px_2px_0px_0px_var(--color-bento-border)]">{getShuffleNeutralMin()}-{getShuffleNeutralMax()}s</span>
            </div>
            <DualRangeSlider min={1} max={10} valueMin={getShuffleNeutralMin()} valueMax={getShuffleNeutralMax()} onchange={handleNeutralChange} disabled={!bleState.connected} />
            <p class="text-ink/60 font-bold text-[10px]">Neutral pause between expressions</p>
        </div>
    </div>

    <!-- Actions -->
    <div class="grid grid-cols-2 gap-3 mt-2">
        <button onclick={applyQuickChange} disabled={!bleState.connected} class="bento-button py-2.5 bg-paper text-ink text-xs font-bold transition-all flex items-center justify-center gap-2">
            <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M13 10V3L4 14h7v7l9-11h-7z" /></svg>
            Quick Changes
        </button>
        <button onclick={applySlowChange} disabled={!bleState.connected} class="bento-button py-2.5 bg-paper text-ink text-xs font-bold transition-all flex items-center justify-center gap-2">
            <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 8v4l3 3m6-3a9 9 0 11-18 0 9 9 0 0118 0z" /></svg>
            Slow & Calm
        </button>
    </div>
</div>
