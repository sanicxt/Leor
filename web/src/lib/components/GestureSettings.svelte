<script lang="ts">
    import {
        sendCommand,
        bleState,
        getGestureReactionTime,
        setGestureReactionTime,
        getGestureConfidence,
        setGestureConfidence,
        getGestureCooldown,
        setGestureCooldown,
    } from "$lib/ble.svelte";

    async function updateReactionTime(val: number) {
        setGestureReactionTime(val);
        await sendCommand(`grt=${val}`);
    }

    async function updateConfidence(val: number) {
        setGestureConfidence(val);
        await sendCommand(`gcf=${val}`);
    }

    async function updateCooldown(val: number) {
        setGestureCooldown(val);
        await sendCommand(`gcd=${val}`);
    }

    // Quick presets
    async function applySensitive() {
        await updateConfidence(60);
        await updateCooldown(500);
    }
    async function applyBalanced() {
        await updateConfidence(75);
        await updateCooldown(1500);
    }
    async function applyStrict() {
        await updateConfidence(90);
        await updateCooldown(2500);
    }
</script>

<div class="bento-card bg-bento-peach p-6 space-y-5">
    <!-- Header -->
    <div class="mb-4 border-b-2 border-bento-border pb-2">
        <h2 class="text-xl font-black uppercase">Gesture Tuning</h2>
        <p class="text-sm font-bold opacity-80">
            Fine-tune detection parameters
        </p>
    </div>

    <!-- Tuning Presets -->
    <div class="grid grid-cols-3 gap-2">
        <button
            onclick={applySensitive}
            disabled={!bleState.connected}
            class="bento-button flex flex-col items-center gap-1 py-2 bg-paper text-ink transition-all disabled:opacity-50 disabled:cursor-not-allowed"
        >
            <span class="text-lg">🎯</span>
            <span class="text-[10px] font-bold">Sensitive</span>
        </button>
        <button
            onclick={applyBalanced}
            disabled={!bleState.connected}
            class="bento-button flex flex-col items-center gap-1 py-2 bg-paper text-ink transition-all disabled:opacity-50 disabled:cursor-not-allowed"
        >
            <span class="text-lg">⚖️</span>
            <span class="text-[10px] font-bold">Balanced</span>
        </button>
        <button
            onclick={applyStrict}
            disabled={!bleState.connected}
            class="bento-button flex flex-col items-center gap-1 py-2 bg-paper text-ink transition-all disabled:opacity-50 disabled:cursor-not-allowed"
        >
            <span class="text-lg">🔒</span>
            <span class="text-[10px] font-bold">Strict</span>
        </button>
    </div>

    <!-- Fine Controls -->
    <div class="space-y-4">
        <!-- Expression Duration -->
        <div class="p-3 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl space-y-2">
            <div class="flex items-center justify-between">
                <div class="flex items-center gap-2">
                    <svg class="w-3.5 h-3.5 text-ink" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                        <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 8v4l3 3m6-3a9 9 0 11-18 0 9 9 0 0118 0z" />
                    </svg>
                    <span class="text-ink/80 text-xs font-bold uppercase tracking-wider">Expression Duration</span>
                </div>
                <span class="text-ink font-mono font-bold text-xs px-2 py-0.5 bg-paper border-2 border-bento-border rounded-lg shadow-[2px_2px_0px_0px_var(--color-bento-border)]">
                    {(getGestureReactionTime() / 1000).toFixed(1)}s
                </span>
            </div>
                <input
                    type="range"
                    min="500"
                    max="5000"
                    step="100"
                    value={getGestureReactionTime()}
                    onchange={(e) => updateReactionTime(parseInt(e.currentTarget.value))}
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
            <p class="text-ink/60 font-bold text-[10px]">How long a gesture must be held before triggering</p>
        </div>

        <!-- Confidence Threshold -->
        <div class="p-3 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl space-y-2">
            <div class="flex items-center justify-between">
                <div class="flex items-center gap-2">
                    <svg class="w-3.5 h-3.5 text-ink" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                        <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M9 12l2 2 4-4m5.618-4.016A11.955 11.955 0 0112 2.944a11.955 11.955 0 01-8.618 3.04A12.02 12.02 0 003 9c0 5.591 3.824 10.29 9 11.622 5.176-1.332 9-6.03 9-11.622 0-1.042-.133-2.052-.382-3.016z" />
                    </svg>
                    <span class="text-ink/80 text-xs font-bold uppercase tracking-wider">Confidence Threshold</span>
                </div>
                <span class="text-ink font-mono font-bold text-xs px-2 py-0.5 bg-paper border-2 border-bento-border rounded-lg shadow-[2px_2px_0px_0px_var(--color-bento-border)]">
                    {getGestureConfidence()}%
                </span>
            </div>
            <input
                type="range"
                min="50"
                max="95"
                step="5"
                value={getGestureConfidence()}
                onchange={(e) => updateConfidence(parseInt(e.currentTarget.value))}
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
            <div class="flex justify-between text-[9px] text-ink/60 font-bold px-1">
                <span>More sensitive</span>
                <span>Fewer false +</span>
            </div>
        </div>

        <!-- Cooldown -->
        <div class="p-3 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl space-y-2">
            <div class="flex items-center justify-between">
                <div class="flex items-center gap-2">
                    <svg class="w-3.5 h-3.5 text-ink" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                        <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M10 9v6m4-6v6m7-3a9 9 0 11-18 0 9 9 0 0118 0z" />
                    </svg>
                    <span class="text-ink/80 text-xs font-bold uppercase tracking-wider">Detection Cooldown</span>
                </div>
                <span class="text-ink font-mono font-bold text-xs px-2 py-0.5 bg-paper border-2 border-bento-border rounded-lg shadow-[2px_2px_0px_0px_var(--color-bento-border)]">
                    {(getGestureCooldown() / 1000).toFixed(1)}s
                </span>
            </div>
            <input
                type="range"
                min="500"
                max="5000"
                step="250"
                value={getGestureCooldown()}
                onchange={(e) => updateCooldown(parseInt(e.currentTarget.value))}
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
            <div class="flex justify-between text-[9px] text-ink/60 font-bold px-1">
                <span>Rapid response</span>
                <span>Debounced</span>
            </div>
        </div>
    </div>
</div>
