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
</script>

<div
    class="bg-zinc-900/40 border border-white/5 rounded-3xl p-6 backdrop-blur-md space-y-5"
>
    <div>
        <h3 class="text-zinc-400 text-xs font-bold tracking-widest uppercase">
            Gesture Tuning
        </h3>
        <p class="text-zinc-600 text-[10px] mt-1">
            Fine-tune gesture detection parameters
        </p>
    </div>

    <!-- Reaction Time -->
    <div class="space-y-2">
        <div class="flex items-center justify-between">
            <label class="text-zinc-300 text-sm">Expression Duration</label>
            <span class="text-indigo-400 font-mono text-sm"
                >{(getGestureReactionTime() / 1000).toFixed(1)}s</span
            >
        </div>
        <input
            type="range"
            min="500"
            max="5000"
            step="100"
            value={getGestureReactionTime()}
            onchange={(e) =>
                updateReactionTime(parseInt(e.currentTarget.value))}
            disabled={!bleState.connected}
            class="w-full h-2 bg-zinc-700 rounded-lg appearance-none cursor-pointer accent-indigo-500 disabled:opacity-50"
        />
        <p class="text-zinc-600 text-[10px]">
            How long to show expression after gesture
        </p>
    </div>

    <!-- Confidence Threshold -->
    <div class="space-y-2">
        <div class="flex items-center justify-between">
            <label class="text-zinc-300 text-sm">Confidence Threshold</label>
            <span class="text-indigo-400 font-mono text-sm"
                >{getGestureConfidence()}%</span
            >
        </div>
        <input
            type="range"
            min="50"
            max="95"
            step="5"
            value={getGestureConfidence()}
            onchange={(e) => updateConfidence(parseInt(e.currentTarget.value))}
            disabled={!bleState.connected}
            class="w-full h-2 bg-zinc-700 rounded-lg appearance-none cursor-pointer accent-indigo-500 disabled:opacity-50"
        />
        <p class="text-zinc-600 text-[10px]">
            Higher = fewer false positives, lower = more sensitive
        </p>
    </div>

    <!-- Cooldown -->
    <div class="space-y-2">
        <div class="flex items-center justify-between">
            <label class="text-zinc-300 text-sm">Detection Cooldown</label>
            <span class="text-indigo-400 font-mono text-sm"
                >{(getGestureCooldown() / 1000).toFixed(1)}s</span
            >
        </div>
        <input
            type="range"
            min="500"
            max="5000"
            step="250"
            value={getGestureCooldown()}
            onchange={(e) => updateCooldown(parseInt(e.currentTarget.value))}
            disabled={!bleState.connected}
            class="w-full h-2 bg-zinc-700 rounded-lg appearance-none cursor-pointer accent-indigo-500 disabled:opacity-50"
        />
        <p class="text-zinc-600 text-[10px]">
            Minimum time between gesture detections
        </p>
    </div>
</div>
