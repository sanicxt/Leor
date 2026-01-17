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

<div
    class="bg-gradient-to-br from-cyan-950/40 to-teal-950/30 border border-cyan-500/20 rounded-2xl p-5 backdrop-blur-lg space-y-5"
>
    <!-- Header -->
    <div class="flex items-center gap-3">
        <div
            class="w-10 h-10 rounded-xl bg-gradient-to-br from-cyan-500 to-teal-600 flex items-center justify-center shadow-lg shadow-cyan-500/20"
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
                    d="M12 6V4m0 2a2 2 0 100 4m0-4a2 2 0 110 4m-6 8a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4m6 6v10m6-2a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4"
                />
            </svg>
        </div>
        <div>
            <h3 class="text-white text-sm font-semibold">Gesture Tuning</h3>
            <p class="text-cyan-300/60 text-xs">
                Fine-tune detection parameters
            </p>
        </div>
    </div>

    <!-- Quick Presets -->
    <div class="grid grid-cols-3 gap-2">
        <button
            onclick={applySensitive}
            disabled={!bleState.connected}
            class="flex flex-col items-center gap-1 py-2.5 rounded-xl bg-cyan-500/10 hover:bg-cyan-500/20 border border-cyan-500/20 transition-all disabled:opacity-40"
        >
            <span class="text-lg">🎯</span>
            <span class="text-cyan-300 text-[10px] font-medium">Sensitive</span>
        </button>
        <button
            onclick={applyBalanced}
            disabled={!bleState.connected}
            class="flex flex-col items-center gap-1 py-2.5 rounded-xl bg-teal-500/10 hover:bg-teal-500/20 border border-teal-500/20 transition-all disabled:opacity-40"
        >
            <span class="text-lg">⚖️</span>
            <span class="text-teal-300 text-[10px] font-medium">Balanced</span>
        </button>
        <button
            onclick={applyStrict}
            disabled={!bleState.connected}
            class="flex flex-col items-center gap-1 py-2.5 rounded-xl bg-emerald-500/10 hover:bg-emerald-500/20 border border-emerald-500/20 transition-all disabled:opacity-40"
        >
            <span class="text-lg">🔒</span>
            <span class="text-emerald-300 text-[10px] font-medium">Strict</span>
        </button>
    </div>

    <!-- Fine Controls -->
    <div class="space-y-4">
        <!-- Expression Duration -->
        <div class="p-3 rounded-xl bg-white/5 space-y-2">
            <div class="flex items-center justify-between">
                <div class="flex items-center gap-2">
                    <svg
                        class="w-3.5 h-3.5 text-cyan-400"
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
                    <span class="text-zinc-400 text-xs"
                        >Expression Duration</span
                    >
                </div>
                <span class="text-cyan-400 font-mono text-xs font-bold"
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
                class="w-full h-1.5 bg-zinc-700 rounded-full appearance-none cursor-pointer accent-cyan-500 disabled:opacity-50"
            />
        </div>

        <!-- Confidence Threshold -->
        <div class="p-3 rounded-xl bg-white/5 space-y-2">
            <div class="flex items-center justify-between">
                <div class="flex items-center gap-2">
                    <svg
                        class="w-3.5 h-3.5 text-teal-400"
                        fill="none"
                        stroke="currentColor"
                        viewBox="0 0 24 24"
                    >
                        <path
                            stroke-linecap="round"
                            stroke-linejoin="round"
                            stroke-width="2"
                            d="M9 12l2 2 4-4m5.618-4.016A11.955 11.955 0 0112 2.944a11.955 11.955 0 01-8.618 3.04A12.02 12.02 0 003 9c0 5.591 3.824 10.29 9 11.622 5.176-1.332 9-6.03 9-11.622 0-1.042-.133-2.052-.382-3.016z"
                        />
                    </svg>
                    <span class="text-zinc-400 text-xs"
                        >Confidence Threshold</span
                    >
                </div>
                <span class="text-teal-400 font-mono text-xs font-bold"
                    >{getGestureConfidence()}%</span
                >
            </div>
            <input
                type="range"
                min="50"
                max="95"
                step="5"
                value={getGestureConfidence()}
                onchange={(e) =>
                    updateConfidence(parseInt(e.currentTarget.value))}
                disabled={!bleState.connected}
                class="w-full h-1.5 bg-zinc-700 rounded-full appearance-none cursor-pointer accent-teal-500 disabled:opacity-50"
            />
            <div class="flex justify-between text-[9px] text-zinc-600 px-1">
                <span>More sensitive</span>
                <span>Fewer false +</span>
            </div>
        </div>

        <!-- Cooldown -->
        <div class="p-3 rounded-xl bg-white/5 space-y-2">
            <div class="flex items-center justify-between">
                <div class="flex items-center gap-2">
                    <svg
                        class="w-3.5 h-3.5 text-emerald-400"
                        fill="none"
                        stroke="currentColor"
                        viewBox="0 0 24 24"
                    >
                        <path
                            stroke-linecap="round"
                            stroke-linejoin="round"
                            stroke-width="2"
                            d="M10 9v6m4-6v6m7-3a9 9 0 11-18 0 9 9 0 0118 0z"
                        />
                    </svg>
                    <span class="text-zinc-400 text-xs">Detection Cooldown</span
                    >
                </div>
                <span class="text-emerald-400 font-mono text-xs font-bold"
                    >{(getGestureCooldown() / 1000).toFixed(1)}s</span
                >
            </div>
            <input
                type="range"
                min="500"
                max="5000"
                step="250"
                value={getGestureCooldown()}
                onchange={(e) =>
                    updateCooldown(parseInt(e.currentTarget.value))}
                disabled={!bleState.connected}
                class="w-full h-1.5 bg-zinc-700 rounded-full appearance-none cursor-pointer accent-emerald-500 disabled:opacity-50"
            />
            <div class="flex justify-between text-[9px] text-zinc-600 px-1">
                <span>Rapid response</span>
                <span>Debounced</span>
            </div>
        </div>
    </div>
</div>
