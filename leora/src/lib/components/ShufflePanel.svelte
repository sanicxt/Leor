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

<div
    class="bg-gradient-to-br from-violet-950/40 to-fuchsia-950/30 border border-violet-500/20 rounded-2xl p-5 backdrop-blur-lg space-y-5"
>
    <!-- Header with Main Toggle -->
    <div class="flex items-center justify-between">
        <div class="flex items-center gap-3">
            <div
                class="w-10 h-10 rounded-xl bg-gradient-to-br from-violet-500 to-fuchsia-600 flex items-center justify-center shadow-lg shadow-violet-500/20"
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
                        d="M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15"
                    />
                </svg>
            </div>
            <div>
                <h3 class="text-white text-sm font-semibold">Shuffle Mode</h3>
                <p class="text-violet-300/60 text-xs">Auto-cycle expressions</p>
            </div>
        </div>
        <button
            class="w-14 h-8 rounded-full transition-all duration-300 relative focus:outline-none focus:ring-2 focus:ring-violet-500/50 disabled:opacity-50
             {getShuffleEnabled()
                ? 'bg-gradient-to-r from-violet-500 to-fuchsia-500 shadow-lg shadow-violet-500/30'
                : 'bg-zinc-700/80'}"
            onclick={toggleShuffle}
            disabled={!bleState.connected}
            aria-label="Toggle shuffle mode"
        >
            <span
                class="absolute left-1 top-1 w-6 h-6 bg-white rounded-full transition-transform duration-300 shadow-md flex items-center justify-center
               {getShuffleEnabled() ? 'translate-x-6' : 'translate-x-0'}"
            >
                {#if getShuffleEnabled()}
                    <svg
                        class="w-3.5 h-3.5 text-violet-500"
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

    <!-- Status Badge -->
    {#if getShuffleEnabled()}
        <div
            class="flex items-center justify-center gap-2 py-2 px-4 bg-violet-500/10 border border-violet-500/20 rounded-xl"
        >
            <span class="w-2 h-2 bg-violet-400 rounded-full animate-pulse"
            ></span>
            <span class="text-violet-300 text-xs font-medium"
                >Shuffling expressions...</span
            >
        </div>
    {/if}

    <!-- Timing Controls -->
    <div class="space-y-4">
        <!-- Expression Duration -->
        <div class="p-4 rounded-xl bg-white/5 space-y-3">
            <div class="flex items-center justify-between">
                <div class="flex items-center gap-2">
                    <svg
                        class="w-4 h-4 text-violet-400"
                        fill="none"
                        stroke="currentColor"
                        viewBox="0 0 24 24"
                    >
                        <path
                            stroke-linecap="round"
                            stroke-linejoin="round"
                            stroke-width="2"
                            d="M14.828 14.828a4 4 0 01-5.656 0M9 10h.01M15 10h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z"
                        />
                    </svg>
                    <span class="text-zinc-300 text-sm">Expression Time</span>
                </div>
                <span
                    class="text-violet-400 font-mono text-sm font-bold px-2 py-0.5 bg-violet-500/20 rounded-lg"
                    >{getShuffleExprMin()}-{getShuffleExprMax()}s</span
                >
            </div>
            <DualRangeSlider
                min={1}
                max={10}
                valueMin={getShuffleExprMin()}
                valueMax={getShuffleExprMax()}
                onchange={handleExprChange}
                disabled={!bleState.connected}
            />
            <p class="text-zinc-500 text-[10px]">
                Duration each expression is displayed
            </p>
        </div>

        <!-- Neutral Duration -->
        <div class="p-4 rounded-xl bg-white/5 space-y-3">
            <div class="flex items-center justify-between">
                <div class="flex items-center gap-2">
                    <svg
                        class="w-4 h-4 text-fuchsia-400"
                        fill="none"
                        stroke="currentColor"
                        viewBox="0 0 24 24"
                    >
                        <path
                            stroke-linecap="round"
                            stroke-linejoin="round"
                            stroke-width="2"
                            d="M12 8v4l3 3m6-3a9 9 0 11-18 0 9 9 0 0118 0z"
                        />
                    </svg>
                    <span class="text-zinc-300 text-sm">Rest Period</span>
                </div>
                <span
                    class="text-fuchsia-400 font-mono text-sm font-bold px-2 py-0.5 bg-fuchsia-500/20 rounded-lg"
                    >{getShuffleNeutralMin()}-{getShuffleNeutralMax()}s</span
                >
            </div>
            <DualRangeSlider
                min={1}
                max={10}
                valueMin={getShuffleNeutralMin()}
                valueMax={getShuffleNeutralMax()}
                onchange={handleNeutralChange}
                disabled={!bleState.connected}
            />
            <p class="text-zinc-500 text-[10px]">
                Neutral pause between expressions
            </p>
        </div>
    </div>

    <!-- Quick Presets -->
    <div class="grid grid-cols-2 gap-3">
        <button
            onclick={applyQuickChange}
            disabled={!bleState.connected}
            class="flex items-center justify-center gap-2 py-2.5 rounded-xl bg-violet-500/20 hover:bg-violet-500/30 border border-violet-500/30 text-violet-300 text-sm font-medium transition-all disabled:opacity-40"
        >
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
                    d="M13 10V3L4 14h7v7l9-11h-7z"
                />
            </svg>
            Quick Changes
        </button>
        <button
            onclick={applySlowChange}
            disabled={!bleState.connected}
            class="flex items-center justify-center gap-2 py-2.5 rounded-xl bg-fuchsia-500/20 hover:bg-fuchsia-500/30 border border-fuchsia-500/30 text-fuchsia-300 text-sm font-medium transition-all disabled:opacity-40"
        >
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
                    d="M12 8v4l3 3m6-3a9 9 0 11-18 0 9 9 0 0118 0z"
                />
            </svg>
            Slow & Calm
        </button>
    </div>
</div>
