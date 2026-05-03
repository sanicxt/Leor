<script lang="ts">
    import {
        sendCommand,
        bleState,
        getSettingsNd,
        setSettingsNd,
    } from "$lib/ble.svelte";

    let notifDuration = $derived(getSettingsNd());

    async function updateDuration(value: number) {
        setSettingsNd(value);
        await sendCommand(`s:nd=${value}`);
    }
</script>

<div class="bento-card bg-bento-yellow p-6 space-y-5">
    <div class="mb-4 border-b-2 border-bento-border pb-2">
        <h2 class="text-xl font-black uppercase">Notifications</h2>
        <p class="text-sm font-bold opacity-80">Message, call & calendar overlay duration</p>
    </div>

    <div class="space-y-2 p-3 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl">
        <div class="flex items-center justify-between">
            <div class="flex items-center gap-2">
                <svg class="w-3.5 h-3.5 text-ink" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                    <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M15 17h5l-1.405-1.405A2.032 2.032 0 0118 14.158V11a6.002 6.002 0 00-4-5.659V5a2 2 0 10-4 0v.341C7.67 6.165 6 8.388 6 11v3.159c0 .538-.214 1.055-.595 1.436L4 17h5m6 0v1a3 3 0 11-6 0v-1m6 0H9" />
                </svg>
                <span class="text-ink/80 text-xs font-bold uppercase tracking-wider">Display Duration</span>
            </div>
            <span class="text-ink font-mono font-bold text-xs px-2 py-0.5 bg-paper border-2 border-bento-border rounded-lg shadow-[2px_2px_0px_0px_var(--color-bento-border)]">
                {(notifDuration / 1000).toFixed(0)}s
            </span>
        </div>
        <input
            type="range"
            min="1000"
            max="30000"
            step="500"
            value={notifDuration}
            oninput={(e) => setSettingsNd(parseInt(e.currentTarget.value))}
            onchange={(e) => updateDuration(parseInt(e.currentTarget.value))}
            disabled={!bleState.connected}
            class="slider"
        />
        <div class="flex justify-between text-[10px] text-ink/60 font-bold px-1">
            <span>1s</span>
            <span>30s</span>
        </div>
        <p class="text-ink/60 font-bold text-[10px]">How long notifications stay on-screen before auto-dismissing</p>
    </div>
</div>
