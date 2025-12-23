<script lang="ts">
    import { sendCommand, getLastStatus } from "$lib/ble.svelte";
    import * as ml from "$lib/ml";

    // State
    let matchEnabled = $state(false);
    let isRecording = $state(false);
    let isTraining = $state(false);
    let isDeploying = $state(false);
    let deployProgress = $state(0);
    let currentGesture = $state("");
    let sampleCount = $state(0);
    let trainingProgress = $state({ epoch: 0, loss: 0, accuracy: 0 });
    let stats = $state(ml.getStats());

    // New gesture input - default to neutral if missing samples
    let newGestureName = $state("neutral");
    let newGestureAction = $state("neutral");
    let liveGyro = $state({ x: 0, y: 0, z: 0 });

    // Non-reactive buffer for sample collection
    let sampleBuffer: number[] = [];

    // Parse incoming gyro data from ESP32
    $effect(() => {
        const status = getLastStatus();
        if (!status) return;

        if (status.startsWith("gd:") && isRecording) {
            const parts = status.substring(3).split(",");
            if (parts.length === 3) {
                const x = parseFloat(parts[0]);
                const y = parseFloat(parts[1]);
                const z = parseFloat(parts[2]);
                liveGyro = { x, y, z };
                sampleBuffer.push(x, y, z);
                sampleCount = Math.floor(sampleBuffer.length / 3);
            }
        }
    });

    // Helper to check if neutral is ready
    const neutralCount = $derived(stats.samplesPerGesture["neutral"] || 0);
    const isNeutralReady = $derived(neutralCount >= 5);

    async function startRecording() {
        if (!currentGesture) {
            currentGesture = newGestureName.trim();
        }
        if (!currentGesture) {
            alert("Enter a gesture name first");
            return;
        }

        sampleBuffer = [];
        sampleCount = 0;
        liveGyro = { x: 0, y: 0, z: 0 };
        isRecording = true;
        await sendCommand("gs");

        setTimeout(() => {
            if (isRecording) {
                finishRecording();
            }
        }, 2500); // 2.5 sec to capture 50+ samples (BLE latency buffer)
    }

    async function finishRecording() {
        isRecording = false;
        await sendCommand("gx");

        const numSamples = Math.floor(sampleBuffer.length / 3);
        if (numSamples >= 3) {
            const xs: number[] = [],
                ys: number[] = [],
                zs: number[] = [];
            for (let i = 0; i < sampleBuffer.length; i += 3) {
                xs.push(sampleBuffer[i]);
                ys.push(sampleBuffer[i + 1]);
                zs.push(sampleBuffer[i + 2]);
            }

            function resample(arr: number[], targetLen: number): number[] {
                const result: number[] = [];
                for (let i = 0; i < targetLen; i++) {
                    const pos = (i / (targetLen - 1)) * (arr.length - 1);
                    const low = Math.floor(pos);
                    const high = Math.min(low + 1, arr.length - 1);
                    const frac = pos - low;
                    result.push(arr[low] * (1 - frac) + arr[high] * frac);
                }
                return result;
            }

            const flat = [
                ...resample(xs, 50),
                ...resample(ys, 50),
                ...resample(zs, 50),
            ];
            ml.addSample(flat, currentGesture);
            stats = ml.getStats();

            // If just finished neutral and it's enough, clear name for next one
            if (
                currentGesture === "neutral" &&
                ml.getStats().samplesPerGesture["neutral"] >= 5
            ) {
                newGestureName = "";
                newGestureAction = "happy";
            }
        }
    }

    async function cancelRecording() {
        isRecording = false;
        await sendCommand("gx");
        sampleBuffer = [];
        sampleCount = 0;
    }

    async function trainModel() {
        if (!isNeutralReady) {
            alert(
                "Please record at least 5 samples of 'neutral' first. This is mandatory for baseline matching.",
            );
            return;
        }
        if (stats.totalSamples < 2) {
            alert("Need at least 2 gestures to train");
            return;
        }

        isTraining = true;
        await ml.trainModel((epoch: number, loss: number, acc: number) => {
            trainingProgress = { epoch, loss, accuracy: acc };
        });
        isTraining = false;
    }

    async function deployModel() {
        if (!isNeutralReady) {
            alert("Neutral baseline missing. Record 5+ 'neutral' samples.");
            return;
        }
        try {
            const base64Weights = await ml.exportWeights();
            isDeploying = true;
            const chunkSize = 17;
            for (let i = 0; i < base64Weights.length; i += chunkSize) {
                await sendCommand(
                    `gw+${base64Weights.substring(i, i + chunkSize)}`,
                );
                await new Promise((r) => setTimeout(r, 25));
                deployProgress = Math.round(
                    ((i + chunkSize) / base64Weights.length) * 100,
                );
            }
            await sendCommand("gw!");

            const labels = ml.getGestureLabels();
            for (let i = 0; i < labels.length; i++) {
                // Neutral action is always 'neutral' or nothing
                const action =
                    labels[i] === "neutral"
                        ? "neutral"
                        : newGestureAction || "happy";
                await sendCommand(`gl=${i}:${labels[i]}:${action}`);
            }
        } catch (e) {
            console.error("[Gesture] Deploy failed:", e);
        } finally {
            isDeploying = false;
        }
    }

    async function toggleMatch() {
        matchEnabled = !matchEnabled;
        await sendCommand(`gm=${matchEnabled ? "1" : "0"}`);
    }

    function clearData() {
        if (
            confirm(
                "Clear all training data? You will need to re-train the mandatory neutral gesture.",
            )
        ) {
            ml.clearData();
            stats = ml.getStats();
            newGestureName = "neutral";
            newGestureAction = "neutral";
        }
    }

    const actions = [
        "happy",
        "sad",
        "angry",
        "love",
        "surprised",
        "confused",
        "sleepy",
        "curious",
        "nervous",
        "knocked",
        "neutral",
        "blink",
        "wink",
        "laugh",
        "cry",
    ];
</script>

<div
    class="bg-zinc-900/40 border border-white/5 rounded-3xl p-6 backdrop-blur-md space-y-6"
>
    <div class="flex items-center justify-between flex-wrap gap-3">
        <h3 class="text-zinc-400 text-xs font-bold tracking-widest uppercase">
            Gesture Training
            {#if isRecording}
                <span
                    class="ml-2 px-2 py-0.5 bg-rose-500/20 text-rose-400 rounded-full text-xs animate-pulse"
                >
                    Recording... {sampleCount}/50
                </span>
            {/if}
        </h3>

        <div class="flex items-center gap-2">
            <span class="text-xs text-zinc-400">Matching</span>
            <button
                class="w-12 h-6 rounded-full transition-all duration-300 relative {matchEnabled
                    ? 'bg-indigo-500 shadow-lg shadow-indigo-500/20'
                    : 'bg-zinc-700'}"
                onclick={toggleMatch}
            >
                <span
                    class="absolute left-0 top-1 w-4 h-4 bg-white rounded-full transition-transform duration-300 {matchEnabled
                        ? 'translate-x-7'
                        : 'translate-x-1'}"
                ></span>
            </button>
        </div>
    </div>

    <div class="space-y-4">
        {#if !isNeutralReady}
            <div
                class="bg-indigo-500/10 border border-indigo-500/20 rounded-xl p-4"
            >
                <div
                    class="text-indigo-300 text-sm font-medium mb-1 flex items-center gap-2"
                >
                    <span
                        class="w-2 h-2 rounded-full bg-indigo-400 animate-ping"
                    ></span>
                    Mandatory: Neutral Training
                </div>
                <p class="text-zinc-400 text-xs leading-relaxed">
                    Record at least 5 samples of you <b
                        >holding the device still</b
                    >. This baseline is required for accurate gesture detection.
                </p>
                <div
                    class="mt-3 h-1.5 bg-zinc-800 rounded-full overflow-hidden"
                >
                    <div
                        class="h-full bg-indigo-500 transition-all duration-500"
                        style="width: {(neutralCount / 5) * 100}%"
                    ></div>
                </div>
            </div>
        {/if}

        <div class="flex flex-col sm:flex-row gap-2">
            <input
                type="text"
                bind:value={newGestureName}
                placeholder="Gesture name"
                class="flex-1 bg-zinc-800/50 border border-white/10 rounded-xl px-4 py-2 text-sm text-white focus:outline-none focus:border-indigo-500/50"
                onchange={() => (currentGesture = newGestureName.trim())}
                disabled={!isNeutralReady && newGestureName === "neutral"}
            />
            <select
                bind:value={newGestureAction}
                class="bg-zinc-800/50 border border-white/10 rounded-xl px-4 py-2 text-sm text-white"
                disabled={!isNeutralReady && newGestureName === "neutral"}
            >
                {#each actions as action}
                    <option value={action}>{action}</option>
                {/each}
            </select>
        </div>

        <button
            class="w-full px-4 py-3 {isRecording
                ? 'bg-rose-500/20 text-rose-300 border-rose-500/30'
                : 'bg-rose-500/10 hover:bg-rose-500/20 text-rose-200 border-rose-500/20'} rounded-xl border transition-all font-medium"
            onclick={isRecording ? cancelRecording : startRecording}
        >
            {isRecording ? "⏹ Cancel Recording" : "⏺ Record Sample"}
        </button>

        {#if isRecording}
            <div
                class="bg-zinc-800/50 rounded-xl p-4 border border-rose-500/30 text-center"
            >
                <div
                    class="text-[10px] text-rose-400 uppercase tracking-widest mb-2 font-bold"
                >
                    📡 Sampling...
                </div>
                <div class="flex justify-center gap-8">
                    <div>
                        <div class="text-[10px] text-zinc-500">X</div>
                        <div class="text-sm font-mono">
                            {liveGyro.x.toFixed(2)}
                        </div>
                    </div>
                    <div>
                        <div class="text-[10px] text-zinc-500">Y</div>
                        <div class="text-sm font-mono">
                            {liveGyro.y.toFixed(2)}
                        </div>
                    </div>
                    <div>
                        <div class="text-[10px] text-zinc-500">Z</div>
                        <div class="text-sm font-mono">
                            {liveGyro.z.toFixed(2)}
                        </div>
                    </div>
                </div>
            </div>
        {/if}

        <div class="bg-zinc-800/30 rounded-xl p-4 border border-white/5">
            <div
                class="text-[10px] text-zinc-500 uppercase tracking-widest mb-3 font-bold"
            >
                Active Gestures
            </div>
            <div class="space-y-2">
                <!-- Always show neutral if not enough samples -->
                {#if !stats.gestures.includes("neutral")}
                    <div
                        class="flex items-center justify-between bg-zinc-700/20 rounded-lg px-3 py-2 border border-dashed border-zinc-600"
                    >
                        <div class="flex items-center gap-2">
                            <span class="text-sm text-zinc-400">neutral</span>
                            <span
                                class="px-1.5 py-0.5 rounded-md bg-zinc-800 text-zinc-500 text-[9px] font-bold"
                                >REQUIRED</span
                            >
                        </div>
                        <span class="text-[10px] text-zinc-600">0 samples</span>
                    </div>
                {/if}

                {#each stats.gestures as gesture}
                    <div
                        class="flex items-center justify-between bg-zinc-700/30 rounded-lg px-3 py-2 border border-white/5"
                    >
                        <div class="flex items-center gap-2">
                            <span class="text-sm text-white">{gesture}</span>
                            {#if gesture === "neutral"}
                                <span
                                    class="px-1.5 py-0.5 rounded-md {isNeutralReady
                                        ? 'bg-indigo-500/20 text-indigo-400'
                                        : 'bg-rose-500/20 text-rose-400'} text-[9px] font-bold"
                                >
                                    {isNeutralReady ? "OK" : "REQUIRED"}
                                </span>
                            {/if}
                        </div>
                        <div class="flex items-center gap-3">
                            <span class="text-[10px] text-zinc-500"
                                >{stats.samplesPerGesture[gesture] || 0} samples</span
                            >
                            <button
                                class="text-rose-400/50 hover:text-rose-400 px-1"
                                onclick={() => {
                                    ml.deleteGesture(gesture);
                                    stats = ml.getStats();
                                }}
                            >
                                <svg
                                    xmlns="http://www.w3.org/2000/svg"
                                    width="14"
                                    height="14"
                                    viewBox="0 0 24 24"
                                    fill="none"
                                    stroke="currentColor"
                                    stroke-width="2"
                                    stroke-linecap="round"
                                    stroke-linejoin="round"
                                    ><path d="M3 6h18" /><path
                                        d="M19 6v14c0 1-1 2-2 2H7c-1 0-2-1-2-2V6"
                                    /><path
                                        d="M8 6V4c0-1 1-2 2-2h4c1 0 2 1 2 2v2"
                                    /></svg
                                >
                            </button>
                        </div>
                    </div>
                {/each}
            </div>
        </div>

        <div class="flex gap-2">
            <button
                class="flex-1 px-4 py-2 bg-zinc-800 hover:bg-zinc-700 text-zinc-300 rounded-xl border border-white/5 transition-all text-xs font-medium disabled:opacity-30"
                onclick={trainModel}
                disabled={!isNeutralReady || isTraining}
            >
                {isTraining ? "Training..." : "🧠 Train"}
            </button>
            <button
                class="flex-1 px-4 py-2 bg-indigo-500/10 hover:bg-indigo-500/20 text-indigo-300 rounded-xl border border-indigo-500/20 transition-all text-xs font-medium disabled:opacity-30"
                onclick={deployModel}
                disabled={!isNeutralReady || isDeploying}
            >
                {isDeploying ? `Deploying ${deployProgress}%` : "📤 Deploy"}
            </button>
        </div>

        <button
            class="w-full py-2 hover:text-rose-400 text-zinc-600 transition-all text-[10px] uppercase font-bold tracking-widest"
            onclick={clearData}
        >
            Reset All Training Data
        </button>
    </div>
</div>
