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
    let sampleCount = $state(0); // Just show count, not full array
    let trainingProgress = $state({ epoch: 0, loss: 0, accuracy: 0 });
    let stats = $state(ml.getStats());

    // New gesture input
    let newGestureName = $state("");
    let newGestureAction = $state("happy");
    let liveGyro = $state({ x: 0, y: 0, z: 0 });

    // Non-reactive buffer for sample collection (avoids UI freeze)
    let sampleBuffer: number[] = [];

    // Parse incoming gyro data from ESP32
    $effect(() => {
        const status = getLastStatus();
        if (!status) return;

        // Parse gyro data: gd:x,y,z
        if (status.startsWith("gd:") && isRecording) {
            const parts = status.substring(3).split(",");
            if (parts.length === 3) {
                const x = parseFloat(parts[0]);
                const y = parseFloat(parts[1]);
                const z = parseFloat(parts[2]);
                liveGyro = { x, y, z };
                sampleBuffer.push(x, y, z); // Push to non-reactive buffer
                sampleCount = Math.floor(sampleBuffer.length / 3); // Update count only
            }
        }

        // Handle streaming responses
        if (status === "gs:1") {
            console.log("[Gesture] Streaming started");
        } else if (status === "gs:0") {
            console.log("[Gesture] Streaming stopped");
        }
    });

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
        await sendCommand("gs"); // Start streaming

        // Auto-stop after 1 second

        setTimeout(() => {
            if (isRecording) {
                finishRecording();
            }
        }, 1000);
    }

    async function finishRecording() {
        isRecording = false;
        await sendCommand("gx"); // Stop streaming

        const numSamples = Math.floor(sampleBuffer.length / 3);
        console.log(`[Gesture] Recorded ${numSamples} samples in 1 second`);

        if (numSamples >= 3) {
            // Need at least a few samples
            // Separate into X, Y, Z arrays
            const xs: number[] = [],
                ys: number[] = [],
                zs: number[] = [];
            for (let i = 0; i < sampleBuffer.length; i += 3) {
                xs.push(sampleBuffer[i]);
                ys.push(sampleBuffer[i + 1]);
                zs.push(sampleBuffer[i + 2]);
            }

            // Resample to exactly 25 points using linear interpolation
            function resample(arr: number[], targetLen: number): number[] {
                if (arr.length === targetLen) return arr;
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

            const rsX = resample(xs, 25);
            const rsY = resample(ys, 25);
            const rsZ = resample(zs, 25);

            // Flatten: [x0..x24, y0..y24, z0..z24]
            const flat = [...rsX, ...rsY, ...rsZ];

            ml.addSample(flat, currentGesture);
            stats = ml.getStats();
            console.log(
                `[Gesture] Added resampled sample for "${currentGesture}"`,
            );
        } else {
            console.warn("[Gesture] Not enough samples collected");
        }
    }

    async function cancelRecording() {
        isRecording = false;
        await sendCommand("gx");
        sampleBuffer = [];
        sampleCount = 0;
    }

    async function trainModel() {
        if (stats.totalSamples < 2) {
            alert("Need at least 2 samples to train");
            return;
        }

        isTraining = true;
        trainingProgress = { epoch: 0, loss: 0, accuracy: 0 };

        try {
            await ml.trainModel((epoch: number, loss: number, acc: number) => {
                trainingProgress = { epoch, loss, accuracy: acc };
            });
        } catch (e) {
            console.error("[Gesture] Training failed:", e);
        }

        isTraining = false;
    }

    async function deployModel() {
        try {
            // Export weights
            const base64Weights = await ml.exportWeights();
            console.log(`[Gesture] Weights: ${base64Weights.length} chars`);

            // Verify weights weren't NaN/Inf (simple check by decoding a few or trusting process)
            // But better to let user know if training exploded
            if (base64Weights.length < 100) {
                throw new Error("Generated weights are empty/invalid");
            }

            // Validate weights locally
            // We can't easily decode base64 back to float here efficiently without code duplication
            // but we can rely on ESP32 validation or assume if training accuracy was OK, weights are OK.
            // However, let's trust the ESP32 debug for now.

            isDeploying = true;
            deployProgress = 0;

            // Send weights in chunks via BLE
            // MTU is often 20 bytes. gw+ (3) + 17 chars = 20 bytes.
            const chunkSize = 17;
            const totalChunks = Math.ceil(base64Weights.length / chunkSize);
            for (let i = 0; i < base64Weights.length; i += chunkSize) {
                const chunk = base64Weights.substring(i, i + chunkSize);
                await sendCommand(`gw+${chunk}`);
                // Small delay to prevent congestion
                await new Promise((r) => setTimeout(r, 25));

                deployProgress = Math.round(
                    ((i + chunkSize) / base64Weights.length) * 100,
                );
            }

            // Finalize transfer
            await sendCommand("gw!");

            // Send gesture labels
            const labels = ml.getGestureLabels();
            for (let i = 0; i < labels.length; i++) {
                const action = newGestureAction || "happy";
                await sendCommand(`gl=${i}:${labels[i]}:${action}`);
            }

            console.log("[Gesture] Model deployed to ESP32");
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
        ml.clearData();
        stats = ml.getStats();
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
            Gesture Training (Browser ML)
            {#if isRecording}
                <span
                    class="ml-2 px-2 py-0.5 bg-rose-500/20 text-rose-400 rounded-full text-xs animate-pulse"
                >
                    Recording... {sampleCount}/25
                </span>
            {/if}
            {#if isTraining}
                <span
                    class="ml-2 px-2 py-0.5 bg-amber-500/20 text-amber-400 rounded-full text-xs animate-pulse"
                >
                    Training... Epoch {trainingProgress.epoch}
                </span>
            {/if}
        </h3>

        <!-- Matching Toggle -->
        <div class="flex items-center gap-2">
            <span class="text-xs text-zinc-400">Matching</span>
            <button
                class="w-12 h-6 rounded-full transition-all duration-300 relative focus:outline-none focus:ring-2 focus:ring-indigo-500/50
                {matchEnabled
                    ? 'bg-indigo-500 shadow-lg shadow-indigo-500/20'
                    : 'bg-zinc-700'}"
                onclick={toggleMatch}
            >
                <span
                    class="absolute left-0 top-1 w-4 h-4 bg-white rounded-full transition-transform duration-300 shadow-sm
                    {matchEnabled ? 'translate-x-7' : 'translate-x-1'}"
                ></span>
            </button>
        </div>
    </div>

    <!-- Record Section -->
    <div class="space-y-4">
        <div class="flex flex-col sm:flex-row gap-2">
            <input
                type="text"
                bind:value={newGestureName}
                placeholder="Gesture name (e.g. 'shake')"
                class="flex-1 bg-zinc-800/50 border border-white/10 rounded-xl px-4 py-2 text-sm text-white placeholder-zinc-500 focus:outline-none focus:border-indigo-500/50"
                onchange={() => (currentGesture = newGestureName.trim())}
            />
            <select
                bind:value={newGestureAction}
                class="bg-zinc-800/50 border border-white/10 rounded-xl px-4 py-2 text-sm text-white"
            >
                {#each actions as action}
                    <option value={action}>{action}</option>
                {/each}
            </select>
        </div>

        <!-- Record Button -->
        <div class="flex gap-2">
            {#if isRecording}
                <button
                    class="flex-1 px-4 py-3 bg-rose-500/20 text-rose-300 rounded-xl border border-rose-500/30 transition-all"
                    onclick={cancelRecording}
                >
                    ⏹ Cancel Recording
                </button>
            {:else}
                <button
                    class="flex-1 px-4 py-3 bg-rose-500/10 hover:bg-rose-500/20 text-rose-200 rounded-xl border border-rose-500/20 hover:border-rose-500/40 transition-all font-medium"
                    onclick={startRecording}
                >
                    ⏺ Record Sample
                </button>
            {/if}
        </div>

        <!-- Live Gyro Display (during recording) -->
        {#if isRecording}
            <div
                class="bg-zinc-800/50 rounded-xl p-4 border border-rose-500/30 animate-pulse"
            >
                <div class="text-xs text-rose-400 mb-2">📡 Live Gyro Data</div>
                <div class="grid grid-cols-3 gap-2 text-center">
                    <div>
                        <div class="text-xs text-zinc-500">X</div>
                        <div class="text-lg font-mono text-white">
                            {liveGyro.x.toFixed(2)}
                        </div>
                    </div>
                    <div>
                        <div class="text-xs text-zinc-500">Y</div>
                        <div class="text-lg font-mono text-white">
                            {liveGyro.y.toFixed(2)}
                        </div>
                    </div>
                    <div>
                        <div class="text-xs text-zinc-500">Z</div>
                        <div class="text-lg font-mono text-white">
                            {liveGyro.z.toFixed(2)}
                        </div>
                    </div>
                </div>
                <div class="mt-2 text-xs text-center text-zinc-400">
                    {sampleCount}/25 samples
                </div>
            </div>
        {/if}

        <!-- Stats & Gesture List -->
        <div class="bg-zinc-800/30 rounded-xl p-4 border border-white/5">
            <div class="text-xs text-zinc-500 mb-2">
                Training Data (max 5 gestures)
            </div>
            <div class="text-sm text-white">
                {stats.totalSamples} samples across {stats.gestures.length}/5
                gestures
            </div>
            {#if stats.gestures.length > 0}
                <div class="mt-3 space-y-2">
                    {#each stats.gestures as gesture}
                        <div
                            class="flex items-center justify-between bg-zinc-700/30 rounded-lg px-3 py-2"
                        >
                            <div>
                                <span class="text-sm text-white">{gesture}</span
                                >
                                <span class="text-xs text-zinc-500 ml-2">
                                    {stats.samplesPerGesture[gesture] || 0} samples
                                </span>
                            </div>
                            <button
                                class="text-rose-400 hover:text-rose-300 text-xs px-2 py-1 rounded hover:bg-rose-500/10"
                                onclick={() => {
                                    ml.deleteGesture(gesture);
                                    stats = ml.getStats();
                                }}
                            >
                                Delete
                            </button>
                        </div>
                    {/each}
                </div>
            {/if}
        </div>

        <!-- Train & Deploy -->
        <div class="flex gap-2">
            <button
                class="flex-1 px-4 py-2 bg-amber-500/10 hover:bg-amber-500/20 text-amber-200 rounded-xl border border-amber-500/20 transition-all disabled:opacity-50"
                onclick={trainModel}
                disabled={isTraining || stats.totalSamples < 2}
            >
                🧠 Train Model
            </button>
            <button
                class="flex-1 px-4 py-2 bg-emerald-500/10 hover:bg-emerald-500/20 text-emerald-200 rounded-xl border border-emerald-500/20 transition-all"
                onclick={deployModel}
            >
                {#if isDeploying}
                    Deploying... {deployProgress}%
                {:else}
                    📤 Deploy to ESP32
                {/if}
            </button>
        </div>

        <!-- Training Progress -->
        {#if isTraining}
            <div
                class="bg-zinc-800/30 rounded-xl p-4 border border-amber-500/20"
            >
                <div class="flex justify-between text-xs text-zinc-400 mb-1">
                    <span>Epoch {trainingProgress.epoch}/100</span>
                    <span>Loss: {trainingProgress.loss.toFixed(4)}</span>
                </div>
                <div class="h-2 bg-zinc-700 rounded-full overflow-hidden">
                    <div
                        class="h-full bg-amber-500 transition-all"
                        style="width: {trainingProgress.epoch}%"
                    ></div>
                </div>
            </div>
        {/if}

        <!-- Clear Button -->
        <button
            class="w-full px-4 py-2 bg-zinc-800 hover:bg-zinc-700 text-zinc-400 rounded-xl border border-white/5 transition-all text-sm"
            onclick={clearData}
        >
            Clear Training Data
        </button>
    </div>
</div>
