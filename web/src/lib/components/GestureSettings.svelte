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
        getGestureShakeThreshold,
        setGestureShakeThreshold,
        getGesturePatThreshold,
        setGesturePatThreshold,
        getGestureSwipeThreshold,
        setGestureSwipeThreshold,
        getGestureTouchThreshold,
        setGestureTouchThreshold,
        getGesturePickupTiltDeg,
        setGesturePickupTiltDeg,
        getGestureInverted,
        setGestureInverted,
    } from "$lib/ble.svelte";

    async function updateInversion(val: boolean) {
        setGestureInverted(val);
        await sendCommand(`ginv=${val ? 1 : 0}`);
    }

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

    async function updateShakeThreshold(val: number) {
        setGestureShakeThreshold(val);
        await sendCommand(`gst=${val}`);
    }

    async function updatePatThreshold(val: number) {
        setGesturePatThreshold(val);
        await sendCommand(`gpt=${val}`);
    }

    async function updateSwipeThreshold(val: number) {
        setGestureSwipeThreshold(val);
        await sendCommand(`gvt=${val}`);
    }

    async function updateTouchThreshold(val: number) {
        setGestureTouchThreshold(val);
        await sendCommand(`gtt=${val}`);
    }

    async function updatePickupTiltDeg(val: number) {
        setGesturePickupTiltDeg(val);
        await sendCommand(`gtd=${val}`);
    }

    // Quick presets
    async function applySensitive() {
        await updateConfidence(60);
        await updateCooldown(500);
        await updateShakeThreshold(150);
        await updatePatThreshold(0.25);
        await updateSwipeThreshold(0.35);
        await updateTouchThreshold(0.03);
        await updatePickupTiltDeg(20);
    }
    async function applyBalanced() {
        await updateConfidence(75);
        await updateCooldown(1500);
        await updateShakeThreshold(200);
        await updatePatThreshold(0.32);
        await updateSwipeThreshold(0.45);
        await updateTouchThreshold(0.05);
        await updatePickupTiltDeg(30);
    }
    async function applyStrict() {
        await updateConfidence(90);
        await updateCooldown(2500);
        await updateShakeThreshold(300);
        await updatePatThreshold(0.5);
        await updateSwipeThreshold(0.7);
        await updateTouchThreshold(0.15);
        await updatePickupTiltDeg(45);
    }
</script>

<div class="bento-card bg-bento-peach p-6 space-y-6">
    <!-- Header -->
    <div class="mb-4 border-b-2 border-bento-border pb-2">
        <h2 class="text-xl font-black uppercase">Gesture Tuning</h2>
        <p class="text-sm font-bold opacity-80">Fine-tune detection parameters</p>
    </div>

    <!-- Tuning Presets -->
    <div class="grid grid-cols-3 gap-3">
        <button onclick={applySensitive} disabled={!bleState.connected} class="bento-button py-2 bg-paper text-ink text-[10px] font-black uppercase tracking-tight flex items-center justify-center gap-1.5">
            <span class="text-base">🎯</span> SENSITIVE
        </button>
        <button onclick={applyBalanced} disabled={!bleState.connected} class="bento-button py-2 bg-paper text-ink text-[10px] font-black uppercase tracking-tight flex items-center justify-center gap-1.5">
            <span class="text-base">⚖️</span> BALANCED
        </button>
        <button onclick={applyStrict} disabled={!bleState.connected} class="bento-button py-2 bg-paper text-ink text-[10px] font-black uppercase tracking-tight flex items-center justify-center gap-1.5">
            <span class="text-base">🔒</span> STRICT
        </button>
    </div>
    <div class="flex items-center justify-between p-3 rounded-2xl bg-paper/30 border-2 border-bento-border/10">
        <span class="text-[10px] font-black uppercase tracking-widest text-ink/70">Invert Y Axis (Pitch)</span>
        <button            class="w-10 h-6 rounded-full border-2 border-bento-border transition-all relative {getGestureInverted() ? 'bg-bento-blue' : 'bg-paper'}"
            aria-label="Invert gyro axes"
            onclick={() => updateInversion(!getGestureInverted())}
            disabled={!bleState.connected}
        >
            <div class="absolute top-0.5 left-0.5 w-4 h-4 rounded-full border border-bento-border bg-paper transition-transform duration-200 {getGestureInverted() ? 'translate-x-4' : 'translate-x-0'}"></div>
        </button>
    </div>

    <div class="space-y-4">
        <!-- Expression Duration -->
        <div class="space-y-2 p-3 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl">
            <div class="flex items-center justify-between">
                <div class="flex items-center gap-2">
                    <svg class="w-3.5 h-3.5 text-ink" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 8v4l3 3m6-3a9 9 0 11-18 0 9 9 0 0118 0z" /></svg>
                    <span class="text-ink/80 text-xs font-bold uppercase tracking-wider">Expr Duration</span>
                </div>
                <span class="text-ink font-mono font-bold text-xs px-2 py-0.5 bg-paper border-2 border-bento-border rounded-lg shadow-[2px_2px_0px_0px_var(--color-bento-border)]">{(getGestureReactionTime() / 1000).toFixed(1)}s</span>
            </div>
            <input type="range" min="500" max="5000" step="100" value={getGestureReactionTime()} onchange={(e) => updateReactionTime(parseInt(e.currentTarget.value))} disabled={!bleState.connected} class="w-full h-2 bg-paper border-2 border-bento-border rounded-full appearance-none cursor-pointer [&::-webkit-slider-thumb]:appearance-none [&::-webkit-slider-thumb]:w-4 [&::-webkit-slider-thumb]:h-4 [&::-webkit-slider-thumb]:rounded-sm [&::-webkit-slider-thumb]:bg-bento-yellow [&::-webkit-slider-thumb]:shadow-[2px_2px_0px_0px_var(--color-bento-border)] [&::-webkit-slider-thumb]:border-2 [&::-webkit-slider-thumb]:border-bento-border" />
            <p class="text-ink/60 font-bold text-[10px]">How long a gesture must be held before triggering</p>
        </div>

        <!-- Detection Cooldown -->
        <div class="space-y-2 p-3 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl">
            <div class="flex items-center justify-between">
                <div class="flex items-center gap-2">
                    <svg class="w-3.5 h-3.5 text-ink" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M10 9v6m4-6v6m7-3a9 9 0 11-18 0 9 9 0 0118 0z" /></svg>
                    <span class="text-ink/80 text-xs font-bold uppercase tracking-wider">Cooldown</span>
                </div>
                <span class="text-ink font-mono font-bold text-xs px-2 py-0.5 bg-paper border-2 border-bento-border rounded-lg shadow-[2px_2px_0px_0px_var(--color-bento-border)]">{(getGestureCooldown() / 1000).toFixed(1)}s</span>
            </div>
            <input type="range" min="500" max="5000" step="250" value={getGestureCooldown()} onchange={(e) => updateCooldown(parseInt(e.currentTarget.value))} disabled={!bleState.connected} class="w-full h-2 bg-paper border-2 border-bento-border rounded-full appearance-none cursor-pointer [&::-webkit-slider-thumb]:appearance-none [&::-webkit-slider-thumb]:w-4 [&::-webkit-slider-thumb]:h-4 [&::-webkit-slider-thumb]:rounded-sm [&::-webkit-slider-thumb]:bg-bento-yellow [&::-webkit-slider-thumb]:shadow-[2px_2px_0px_0px_var(--color-bento-border)] [&::-webkit-slider-thumb]:border-2 [&::-webkit-slider-thumb]:border-bento-border" />
            <p class="text-ink/60 font-bold text-[10px]">Minimum time between consecutive gesture triggers</p>
        </div>

        <div class="grid grid-cols-2 gap-4">
            <!-- Shake Energy -->
            <div class="space-y-2 p-3 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl">
                <div class="flex items-center justify-between">
                    <div class="flex items-center gap-2">
                        <svg class="w-3.5 h-3.5 text-ink" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15" /></svg>
                        <span class="text-ink/80 text-[10px] font-bold uppercase tracking-wider">Shake</span>
                    </div>
                    <span class="text-ink font-mono font-bold text-[10px] px-1.5 py-0.5 bg-paper border-2 border-bento-border rounded-lg shadow-[2px_2px_0px_0px_var(--color-bento-border)]">{getGestureShakeThreshold()}</span>
                </div>
                <input type="range" min="100" max="500" step="10" value={getGestureShakeThreshold()} onchange={(e) => updateShakeThreshold(parseFloat(e.currentTarget.value))} disabled={!bleState.connected} class="w-full h-1.5 bg-paper border-2 border-bento-border rounded-full appearance-none cursor-pointer" />
            </div>

            <!-- Pat Impact -->
            <div class="space-y-2 p-3 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl">
                <div class="flex items-center justify-between">
                    <div class="flex items-center gap-2">
                        <svg class="w-3.5 h-3.5 text-ink" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M7 11l5-5m0 0l5 5m-5-5v12" /></svg>
                        <span class="text-ink/80 text-[10px] font-bold uppercase tracking-wider">Pat</span>
                    </div>
                    <span class="text-ink font-mono font-bold text-[10px] px-1.5 py-0.5 bg-paper border-2 border-bento-border rounded-lg shadow-[2px_2px_0px_0px_var(--color-bento-border)]">{getGesturePatThreshold().toFixed(2)}g</span>
                </div>
                <input type="range" min="0.1" max="0.8" step="0.01" value={getGesturePatThreshold()} onchange={(e) => updatePatThreshold(parseFloat(e.currentTarget.value))} disabled={!bleState.connected} class="w-full h-1.5 bg-paper border-2 border-bento-border rounded-full appearance-none cursor-pointer" />
            </div>

            <!-- Nudge Impact -->
            <div class="space-y-2 p-3 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl">
                <div class="flex items-center justify-between">
                    <div class="flex items-center gap-2">
                        <svg class="w-3.5 h-3.5 text-ink" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M13 5l7 7-7 7M5 5l7 7-7 7" /></svg>
                        <span class="text-ink/80 text-[10px] font-bold uppercase tracking-wider">Nudge</span>
                    </div>
                    <span class="text-ink font-mono font-bold text-[10px] px-1.5 py-0.5 bg-paper border-2 border-bento-border rounded-lg shadow-[2px_2px_0px_0px_var(--color-bento-border)]">{getGestureSwipeThreshold().toFixed(2)}g</span>
                </div>
                <input type="range" min="0.2" max="1.0" step="0.05" value={getGestureSwipeThreshold()} onchange={(e) => updateSwipeThreshold(parseFloat(e.currentTarget.value))} disabled={!bleState.connected} class="w-full h-1.5 bg-paper border-2 border-bento-border rounded-full appearance-none cursor-pointer" />
            </div>

            <!-- Touch Min -->
            <div class="space-y-2 p-3 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl">
                <div class="flex items-center justify-between">
                    <div class="flex items-center gap-2">
                        <svg class="w-3.5 h-3.5 text-ink" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M15 15l-2 5L9 9l11 4-5 2zm0 0l5 5" /></svg>
                        <span class="text-ink/80 text-[10px] font-bold uppercase tracking-wider">Touch</span>
                    </div>
                    <span class="text-ink font-mono font-bold text-[10px] px-1.5 py-0.5 bg-paper border-2 border-bento-border rounded-lg shadow-[2px_2px_0px_0px_var(--color-bento-border)]">{(getGestureTouchThreshold() * 100).toFixed(0)}%</span>
                </div>
                <input type="range" min="0.01" max="0.4" step="0.01" value={getGestureTouchThreshold()} onchange={(e) => updateTouchThreshold(parseFloat(e.currentTarget.value))} disabled={!bleState.connected} class="w-full h-1.5 bg-paper border-2 border-bento-border rounded-full appearance-none cursor-pointer" />
            </div>

            <!-- Pickup Tilt Angle -->
            <div class="space-y-2 p-3 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl">
                <div class="flex items-center justify-between">
                    <div class="flex items-center gap-2">
                        <svg class="w-3.5 h-3.5 text-ink" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M5 3l14 9-7 7 3-14L3 7l7 7" /></svg>
                        <span class="text-ink/80 text-[10px] font-bold uppercase tracking-wider">Pickup Tilt</span>
                    </div>
                    <span class="text-ink font-mono font-bold text-[10px] px-1.5 py-0.5 bg-paper border-2 border-bento-border rounded-lg shadow-[2px_2px_0px_0px_var(--color-bento-border)]">{getGesturePickupTiltDeg().toFixed(0)}°</span>
                </div>
                <input type="range" min="10" max="80" step="5" value={getGesturePickupTiltDeg()} onchange={(e) => updatePickupTiltDeg(parseFloat(e.currentTarget.value))} disabled={!bleState.connected} class="w-full h-1.5 bg-paper border-2 border-bento-border rounded-full appearance-none cursor-pointer" />
            </div>
        </div>
    </div>
</div>
