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
</script>

<div
    class="bg-zinc-900/40 border border-white/5 rounded-3xl p-6 backdrop-blur-md h-full space-y-5"
>
    <!-- Header with toggle -->
    <div class="flex items-center justify-between">
        <div>
            <h3
                class="text-zinc-400 text-xs font-bold tracking-widest uppercase"
            >
                Shuffle Mode
            </h3>
            <p class="text-zinc-600 text-[10px] mt-1">
                Randomly cycle through expressions
            </p>
        </div>
        <button
            class="w-12 h-6 rounded-full transition-all duration-300 relative focus:outline-none focus:ring-2 focus:ring-indigo-500/50 disabled:opacity-50
             {getShuffleEnabled()
                ? 'bg-indigo-500 shadow-lg shadow-indigo-500/20'
                : 'bg-zinc-700'}"
            onclick={toggleShuffle}
            disabled={!bleState.connected}
            aria-label="Toggle shuffle mode"
        >
            <span
                class="absolute left-0 top-1 w-4 h-4 bg-white rounded-full transition-transform duration-300 shadow-sm
               {getShuffleEnabled() ? 'translate-x-7' : 'translate-x-1'}"
            ></span>
        </button>
    </div>

    <!-- Expression Duration -->
    <div class="space-y-2">
        <div class="flex items-center justify-between">
            <label class="text-zinc-300 text-sm">Expression Duration</label>
            <span class="text-indigo-400 font-mono text-sm"
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
        <p class="text-zinc-600 text-[10px]">
            How long each expression is shown
        </p>
    </div>

    <!-- Neutral Duration -->
    <div class="space-y-2">
        <div class="flex items-center justify-between">
            <label class="text-zinc-300 text-sm">Neutral Duration</label>
            <span class="text-indigo-400 font-mono text-sm"
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
        <p class="text-zinc-600 text-[10px]">Rest time between expressions</p>
    </div>
</div>
