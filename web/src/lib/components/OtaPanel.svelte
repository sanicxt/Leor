<script lang="ts">
    import { bleState, sendOTA } from "$lib/ble.svelte";

    let fileInput: HTMLInputElement;
    let selectedFile: File | null = $state(null);
    let progress  = $state(0);       // 0-100
    let statusMsg = $state('');
    let running   = $state(false);
    let done      = $state(false);
    let failed    = $state(false);

    function onFileChange(e: Event) {
        const f = (e.target as HTMLInputElement).files?.[0] ?? null;
        selectedFile = f;
        progress  = 0;
        statusMsg = f ? `Ready: ${f.name} (${(f.size / 1024).toFixed(1)} KB)` : '';
        done   = false;
        failed = false;
    }

    async function startOTA() {
        if (!selectedFile) return;
        running  = true;
        done     = false;
        failed   = false;
        progress = 0;
        statusMsg = 'Starting OTA...';

        const buf = await selectedFile.arrayBuffer();
        const ok = await sendOTA(buf, (pct, msg) => {
            progress  = pct;
            statusMsg = msg;
        });

        running = false;
        done    = ok;
        failed  = !ok;
    }
</script>

<div class="bento-card bg-bento-peach p-6 space-y-5">
    <!-- Header -->
    <div class="mb-4 border-b-2 border-bento-border pb-2">
        <h2 class="text-xl font-black uppercase">Firmware Update</h2>
        <p class="text-sm font-bold opacity-80">Flash new firmware over BLE</p>
    </div>

    <!-- File picker -->
    <div
        class="p-4 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl space-y-3"
    >
        <label
            class="flex flex-col items-center justify-center gap-2 border-2 border-dashed
                   {selectedFile ? 'border-bento-blue bg-bento-blue/20' : 'border-bento-border/30 hover:border-bento-border/60 bg-paper hover:bg-ink/5'}
                   rounded-xl p-5 cursor-pointer transition-colors"
        >
            <svg class="w-8 h-8 {selectedFile ? 'text-bento-blue' : 'text-ink/60'}" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="1.5"
                    d="M9 12h6m-6 4h6m2 5H7a2 2 0 01-2-2V5a2 2 0 012-2h5.586a1 1 0 01.707.293l5.414 5.414a1 1 0 01.293.707V19a2 2 0 01-2 2z" />
            </svg>
            <span class="text-xs font-bold {selectedFile ? 'text-ink' : 'text-ink/60'}">
                {selectedFile ? selectedFile.name : 'Select .bin firmware file'}
            </span>
            <input
                bind:this={fileInput}
                type="file"
                accept=".bin"
                class="hidden"
                onchange={onFileChange}
            />
        </label>

        <!-- Progress bar -->
        {#if running || done || failed}
            <div class="space-y-1.5">
                <div class="w-full h-3 border-2 border-bento-border rounded-full overflow-hidden bg-paper shadow-[inset_1px_1px_0px_0px_var(--color-bento-border)]">
                    <div
                        class="h-full border-r-2 border-bento-border transition-all duration-300
                               {done ? 'bg-bento-green'
                                     : failed ? 'bg-bento-pink'
                                     : 'bg-bento-blue'}"
                        style="width: {progress}%"
                    ></div>
                </div>
                <div class="flex justify-between text-xs font-bold uppercase tracking-wider">
                    <span class="text-ink">
                        {statusMsg}
                    </span>
                    <span class="text-ink">{progress}%</span>
                </div>
            </div>
        {:else if statusMsg}
            <p class="text-xs font-bold text-ink uppercase tracking-wider">{statusMsg}</p>
        {/if}

        <button
            onclick={startOTA}
            disabled={!selectedFile || !bleState.connected || running}
            class="bento-button w-full py-3 flex items-center justify-center gap-2 {selectedFile && bleState.connected && !running ? 'bg-bento-green text-ink' : 'bg-paper text-ink'} disabled:opacity-50"
        >
            {#if running}
                <span class="flex items-center justify-center gap-2">
                    <svg class="w-4 h-4 animate-spin text-ink" fill="none" viewBox="0 0 24 24">
                        <circle class="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" stroke-width="4"/>
                        <path class="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8v8z"/>
                    </svg>
                    Flashing...
                </span>
            {:else if done}
                ✓ Done — Device Rebooting
            {:else}
                Flash Firmware
            {/if}
        </button>

        {#if !bleState.connected}
            <p class="text-xs font-bold text-ink/60 text-center uppercase tracking-wider">Connect to device first</p>
        {/if}
    </div>

    <!-- Info -->
    <div class="mt-3 text-xs font-bold text-ink/70 flex items-start gap-2 bg-paper p-3 rounded-xl border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)]">
        <svg class="w-5 h-5 shrink-0" fill="none" stroke="currentColor" viewBox="0 0 24 24">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2"
                d="M13 16h-1v-4h-1m1-4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z" />
        </svg>
        <span class="leading-relaxed">
            Select <code class="bg-bento-yellow px-1 border border-bento-border rounded">.bin</code> from the build output
            (<code class="bg-bento-yellow px-1 border border-bento-border rounded">sketch/Leor.ino.bin</code>).
            The device restarts automatically.
            Rollback is automatic if the new firmware fails to boot.
        </span>
    </div>
</div>
