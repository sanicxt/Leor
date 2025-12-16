<script lang="ts">
    import {
        sendCommand,
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

    async function toggleShuffle() {
        const newVal = !getShuffleEnabled();
        setShuffleEnabled(newVal);
        await sendCommand(`sh:${newVal ? "on" : "off"}`);
    }

    async function updateExprTiming(e: Event, type: "min" | "max") {
        const val = parseInt((e.target as HTMLInputElement).value);
        let min = type === "min" ? val : getShuffleExprMin();
        let max = type === "max" ? val : getShuffleExprMax();

        if (min > max) {
            if (type === "min") min = max;
            else max = min;
        }

        setShuffleExprMin(min);
        setShuffleExprMax(max);
        await sendCommand(`sh:e=${min}-${max}`);
    }

    async function updateNeutralTiming(e: Event, type: "min" | "max") {
        const val = parseInt((e.target as HTMLInputElement).value);
        let min = type === "min" ? val : getShuffleNeutralMin();
        let max = type === "max" ? val : getShuffleNeutralMax();

        if (min > max) {
            if (type === "min") min = max;
            else max = min;
        }

        setShuffleNeutralMin(min);
        setShuffleNeutralMax(max);
        await sendCommand(`sh:n=${min}-${max}`);
    }
    import { onMount } from "svelte";

    // Debug effect
    $effect(() => {
        console.log("UI Shuffle State:", getShuffleEnabled());
    });
</script>

<div
    class="bg-zinc-900/40 border border-white/5 rounded-3xl p-6 backdrop-blur-md h-full space-y-6"
>
    <div class="flex items-center justify-between">
        <h3 class="text-zinc-400 text-xs font-bold tracking-widest uppercase">
            Shuffle Mode
        </h3>
        <button
            class="w-12 h-6 rounded-full transition-all duration-300 relative focus:outline-none focus:ring-2 focus:ring-indigo-500/50
             {getShuffleEnabled()
                ? 'bg-indigo-500 shadow-lg shadow-indigo-500/20'
                : 'bg-zinc-700'}"
            onclick={toggleShuffle}
            aria-label="Toggle shuffle mode"
        >
            <span
                class="absolute left-0 top-1 w-4 h-4 bg-white rounded-full transition-transform duration-300 shadow-sm
               {getShuffleEnabled() ? 'translate-x-7' : 'translate-x-1'}"
            ></span>
        </button>
    </div>

    <div class="space-y-4">
        <div class="space-y-2">
            <div class="flex justify-between text-sm">
                <label class="text-zinc-400">Expression Duration</label>
                <span class="text-indigo-200 font-medium"
                    >{getShuffleExprMin()}-{getShuffleExprMax()}s</span
                >
            </div>
            <div class="flex gap-2 items-center relative py-2">
                <input
                    type="range"
                    min="1"
                    max="10"
                    value={getShuffleExprMin()}
                    oninput={(e) => updateExprTiming(e, "min")}
                    class="flex-1 h-1.5 bg-zinc-700 rounded-lg appearance-none cursor-pointer accent-indigo-500 hover:accent-indigo-400"
                    aria-label="Minimum expression duration"
                />
                <input
                    type="range"
                    min="1"
                    max="10"
                    value={getShuffleExprMax()}
                    oninput={(e) => updateExprTiming(e, "max")}
                    class="flex-1 h-1.5 bg-zinc-700 rounded-lg appearance-none cursor-pointer accent-indigo-500 hover:accent-indigo-400"
                    aria-label="Maximum expression duration"
                />
            </div>
        </div>

        <div class="space-y-2">
            <div class="flex justify-between text-sm">
                <label class="text-zinc-400">Neutral Duration</label>
                <span class="text-indigo-200 font-medium"
                    >{getShuffleNeutralMin()}-{getShuffleNeutralMax()}s</span
                >
            </div>
            <div class="flex gap-2 items-center relative py-2">
                <input
                    type="range"
                    min="1"
                    max="10"
                    value={getShuffleNeutralMin()}
                    oninput={(e) => updateNeutralTiming(e, "min")}
                    class="flex-1 h-1.5 bg-zinc-700 rounded-lg appearance-none cursor-pointer accent-indigo-500 hover:accent-indigo-400"
                    aria-label="Minimum neutral duration"
                />
                <input
                    type="range"
                    min="1"
                    max="10"
                    value={getShuffleNeutralMax()}
                    oninput={(e) => updateNeutralTiming(e, "max")}
                    class="flex-1 h-1.5 bg-zinc-700 rounded-lg appearance-none cursor-pointer accent-indigo-500 hover:accent-indigo-400"
                    aria-label="Maximum neutral duration"
                />
            </div>
        </div>
    </div>
</div>
