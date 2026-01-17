<script lang="ts">
    import { sendCommand, bleState } from "$lib/ble.svelte";

    // Gesture tuning parameters (with defaults)
    let reactionTime = $state(1500); // ms to show expression
    let confidence = $state(70); // % threshold (0-100)
    let cooldown = $state(2000); // ms between detections

    // Sync with device on connect
    $effect(() => {
        if (bleState.connected) {
            // Request current values (would need firmware support)
            sendCommand("gs:");
        }
    });

    // Parse settings from BLE status
    $effect(() => {
        const status = bleState.lastStatus;
        if (!status?.startsWith("gs:")) return;

        // Parse format: gs:rt=1500,cf=70,cd=2000
        const match = status.match(/rt=(\d+)/);
        if (match) reactionTime = parseInt(match[1]);
        const cfMatch = status.match(/cf=(\d+)/);
        if (cfMatch) confidence = parseInt(cfMatch[1]);
        const cdMatch = status.match(/cd=(\d+)/);
        if (cdMatch) cooldown = parseInt(cdMatch[1]);
    });

    async function updateReactionTime() {
        await sendCommand(`grt=${reactionTime}`);
    }

    async function updateConfidence() {
        await sendCommand(`gcf=${confidence}`);
    }

    async function updateCooldown() {
        await sendCommand(`gcd=${cooldown}`);
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
                >{(reactionTime / 1000).toFixed(1)}s</span
            >
        </div>
        <input
            type="range"
            min="500"
            max="5000"
            step="100"
            bind:value={reactionTime}
            onchange={updateReactionTime}
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
            <span class="text-indigo-400 font-mono text-sm">{confidence}%</span>
        </div>
        <input
            type="range"
            min="50"
            max="95"
            step="5"
            bind:value={confidence}
            onchange={updateConfidence}
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
                >{(cooldown / 1000).toFixed(1)}s</span
            >
        </div>
        <input
            type="range"
            min="500"
            max="5000"
            step="250"
            bind:value={cooldown}
            onchange={updateCooldown}
            disabled={!bleState.connected}
            class="w-full h-2 bg-zinc-700 rounded-lg appearance-none cursor-pointer accent-indigo-500 disabled:opacity-50"
        />
        <p class="text-zinc-600 text-[10px]">
            Minimum time between gesture detections
        </p>
    </div>
</div>
