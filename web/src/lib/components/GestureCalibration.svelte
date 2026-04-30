<script lang="ts">
    import { 
        bleState,
        startCalibration,
        abortCalibration,
        pollCalibrationStatus,
        resetCalibrationState,
    } from "$lib/ble.svelte";

    interface GestureDef {
        name: string;
        icon: string;
        description: string;
        instruction: string;
    }

    const gestures: GestureDef[] = [
        { name: 'pat',     icon: '👋', description: 'Gentle tap on top',  instruction: 'Rest a finger on the touch sensor, then tap the top surface repeatedly' },
        { name: 'shake',   icon: '🔄', description: 'Vigorous shake',     instruction: 'Hold device still, then shake it vigorously side-to-side' },
        { name: 'swipe',   icon: '👉', description: 'Horizontal swipe',   instruction: 'Rest a finger on the touch sensor, then swipe horizontally across a surface' },
        { name: 'pickup',  icon: '⬆️',  description: 'Lift / tilt up',    instruction: 'Hold device still, then pick it up and tilt it upwards' },
    ];

    let pollInterval: ReturnType<typeof setInterval> | null = null;

    function isCalibrating() {
        return bleState.calPhase === 'wait' || bleState.calPhase === 'capturing';
    }

    function startPolling() {
        stopPolling();
        pollInterval = setInterval(async () => {
            if (bleState.calPhase === 'idle' || bleState.calPhase === 'complete' || bleState.calPhase === 'timeout') {
                stopPolling();
                return;
            }
            await pollCalibrationStatus();
        }, 500);
    }

    function stopPolling() {
        if (pollInterval) {
            clearInterval(pollInterval);
            pollInterval = null;
        }
    }

    // Cleanup on unmount
    $effect(() => {
        return () => stopPolling();
    });

    async function start(g: GestureDef) {
        await startCalibration(g.name);
        startPolling();
    }

    async function stop() {
        stopPolling();
        await abortCalibration();
    }

    function getPhaseLabel(): string {
        switch (bleState.calPhase) {
            case 'wait': return 'Get ready...';
            case 'capturing': return 'Capturing — perform the gesture!';
            case 'complete': return 'Calibration complete!';
            case 'timeout': return 'Calibration timed out';
            default: return '';
        }
    }

    function getPhasePercent(): number {
        switch (bleState.calPhase) {
            case 'wait': return 10;
            case 'capturing': return 10 + Math.min(90, (bleState.calCaptureMs / 3000) * 90);
            case 'complete': return 100;
            default: return 0;
        }
    }
</script>

<div class="bento-card bg-bento-blue p-6 space-y-6">
    <!-- Header -->
    <div class="mb-4 border-b-2 border-bento-border pb-2">
        <h2 class="text-xl font-black uppercase">Gesture Calibration</h2>
        <p class="text-sm font-bold opacity-80">Auto-tune thresholds from real data</p>
    </div>

    <!-- Active Calibration Progress -->
    {#if isCalibrating() || bleState.calPhase === 'complete' || bleState.calPhase === 'timeout'}
        <div class="p-4 bg-paper border-2 border-bento-border rounded-xl shadow-[2px_2px_0px_0px_var(--color-bento-border)] space-y-3">
            <div class="flex items-center justify-between">
                <div class="flex items-center gap-2">
                    <span class="text-2xl">
                        {gestures.find(g => g.name === bleState.calGesture)?.icon || '📡'}
                    </span>
                    <span class="text-ink font-black uppercase">
                        {bleState.calGesture}
                    </span>
                </div>
                {#if isCalibrating()}
                    <button
                        onclick={stop}
                        class="bento-button bg-bento-pink text-ink px-3 py-1 text-xs uppercase"
                    >
                        Cancel
                    </button>
                {:else}
                    <button
                        onclick={resetCalibrationState}
                        class="bento-button bg-paper text-ink px-3 py-1 text-xs uppercase"
                    >
                        Dismiss
                    </button>
                {/if}
            </div>

            <!-- Progress bar -->
            <div class="space-y-1">
                <div class="h-3 bg-paper border-2 border-bento-border rounded-full overflow-hidden">
                    <div
                        class="h-full transition-all duration-300 rounded-full {bleState.calPhase === 'complete' ? 'bg-bento-green' : bleState.calPhase === 'timeout' ? 'bg-bento-pink' : 'bg-bento-yellow'}"
                        style="width: {getPhasePercent()}%"
                    ></div>
                </div>
                <div class="flex justify-between text-[10px] text-ink/60 font-bold">
                    <span>{getPhaseLabel()}</span>
                    {#if bleState.calPhase === 'capturing'}
                        <span class="font-mono">{(bleState.calCaptureMs / 1000).toFixed(1)}s</span>
                    {/if}
                </div>
            </div>

            <!-- Results -->
            {#if bleState.calPhase === 'complete'}
                <div class="grid grid-cols-3 gap-2 pt-2 border-t-2 border-bento-border">
                    <div class="text-center p-2 bg-paper border-2 border-bento-border rounded-lg">
                        <div class="text-[10px] text-ink/60 font-bold uppercase">Peak</div>
                        <div class="text-ink font-mono font-bold text-sm">{bleState.calPeak.toFixed(2)}</div>
                    </div>
                    <div class="text-center p-2 bg-paper border-2 border-bento-border rounded-lg">
                        <div class="text-[10px] text-ink/60 font-bold uppercase">Old</div>
                        <div class="text-ink font-mono font-bold text-sm opacity-50">{bleState.calOldThreshold.toFixed(2)}</div>
                    </div>
                    <div class="text-center p-2 bg-bento-green border-2 border-bento-border rounded-lg shadow-[2px_2px_0px_0px_var(--color-bento-border)]">
                        <div class="text-[10px] text-ink/60 font-bold uppercase">New</div>
                        <div class="text-ink font-mono font-bold text-sm">{bleState.calNewThreshold.toFixed(2)}</div>
                    </div>
                </div>
                <div class="text-center text-[10px] text-ink/60 font-bold">
                    Threshold set to {(bleState.calNewThreshold / bleState.calPeak * 100).toFixed(0)}% of peak · {bleState.calSamples} samples
                </div>
            {/if}

            {#if bleState.calPhase === 'timeout'}
                <div class="text-center p-3 bg-bento-pink border-2 border-bento-border rounded-lg">
                    <p class="text-ink font-bold text-sm">No sensor data received. Check device orientation and retry.</p>
                </div>
            {/if}
        </div>
    {/if}

    <!-- Gesture Buttons -->
    <div class="grid grid-cols-2 gap-3">
        {#each gestures as g}
            {@const active = bleState.calGesture === g.name && isCalibrating()}
            <button
                onclick={() => start(g)}
                disabled={!bleState.connected || isCalibrating()}
                class="bento-button flex flex-col items-center gap-2 p-4 rounded-xl transition-all duration-300 disabled:opacity-50 disabled:cursor-not-allowed
                    {active ? 'bg-bento-yellow shadow-[4px_4px_0px_0px_var(--color-bento-border)] scale-[1.02]' : 'bg-paper shadow-[2px_2px_0px_0px_var(--color-bento-border)] hover:bg-paper/80'}"
            >
                <span class="text-2xl {active ? 'animate-bounce' : ''}">{g.icon}</span>
                <div class="text-center">
                    <div class="text-ink font-black uppercase text-xs">{g.name}</div>
                    <div class="text-ink/60 font-bold text-[10px]">{g.description}</div>
                </div>
            </button>
        {/each}
    </div>

    <!-- Instructions -->
    {#if bleState.calPhase === 'wait' || bleState.calPhase === 'capturing'}
        <div class="p-3 bg-paper border-2 border-bento-border rounded-xl">
            <p class="text-ink/80 font-bold text-xs leading-relaxed">
                {gestures.find(g => g.name === bleState.calGesture)?.instruction || ''}
            </p>
        </div>
    {/if}

    <!-- Footer Info -->
    <div class="flex items-center justify-center gap-2 text-ink/60 text-[10px] uppercase font-bold tracking-wider">
        <svg class="w-3 h-3" fill="none" stroke="currentColor" viewBox="0 0 24 24">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M13 16h-1v-4h-1m1-4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z" />
        </svg>
        Perform gesture during capture · Threshold = 70% of peak
    </div>
</div>
