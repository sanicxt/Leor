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

<div
    class="bg-gradient-to-br from-violet-950/40 to-purple-950/30 border border-violet-500/20 rounded-2xl p-5 backdrop-blur-lg"
>
    <!-- Header -->
    <div class="flex items-center gap-3 mb-4">
        <div
            class="w-10 h-10 rounded-xl bg-gradient-to-br from-violet-500 to-purple-600 flex items-center justify-center shadow-lg shadow-violet-500/20"
        >
            <svg class="w-5 h-5 text-white" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2"
                    d="M4 16v1a3 3 0 003 3h10a3 3 0 003-3v-1m-4-8l-4-4m0 0L8 8m4-4v12" />
            </svg>
        </div>
        <div>
            <h2 class="text-lg font-semibold text-white">OTA Firmware Update</h2>
            <p class="text-xs text-zinc-400">Flash new firmware over Bluetooth</p>
        </div>
    </div>

    <!-- File picker -->
    <div
        class="p-4 bg-white/5 rounded-xl border border-white/5 space-y-3"
    >
        <label
            class="flex flex-col items-center justify-center gap-2 border-2 border-dashed
                   {selectedFile ? 'border-violet-500/50 bg-violet-500/5' : 'border-zinc-700 hover:border-zinc-500'}
                   rounded-xl p-5 cursor-pointer transition-colors"
        >
            <svg class="w-8 h-8 {selectedFile ? 'text-violet-400' : 'text-zinc-600'}" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="1.5"
                    d="M9 12h6m-6 4h6m2 5H7a2 2 0 01-2-2V5a2 2 0 012-2h5.586a1 1 0 01.707.293l5.414 5.414a1 1 0 01.293.707V19a2 2 0 01-2 2z" />
            </svg>
            <span class="text-xs text-zinc-400">
                {selectedFile ? selectedFile.name : 'Click to select .bin firmware file'}
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
                <div class="w-full h-2 bg-zinc-800 rounded-full overflow-hidden">
                    <div
                        class="h-full rounded-full transition-all duration-300
                               {done ? 'bg-gradient-to-r from-emerald-500 to-green-400'
                                     : failed ? 'bg-red-500'
                                     : 'bg-gradient-to-r from-violet-500 to-purple-400'}"
                        style="width: {progress}%"
                    ></div>
                </div>
                <div class="flex justify-between text-[10px]">
                    <span class="{done ? 'text-emerald-400' : failed ? 'text-red-400' : 'text-violet-300'}">
                        {statusMsg}
                    </span>
                    <span class="text-zinc-500 font-mono">{progress}%</span>
                </div>
            </div>
        {:else if statusMsg}
            <p class="text-xs text-zinc-400">{statusMsg}</p>
        {/if}

        <!-- Flash button -->
        <button
            onclick={startOTA}
            disabled={!selectedFile || !bleState.connected || running}
            class="w-full py-2.5 rounded-xl font-semibold text-sm transition-all
                   {selectedFile && bleState.connected && !running
                       ? 'bg-gradient-to-r from-violet-500 to-purple-500 text-white hover:opacity-90 shadow-lg shadow-violet-500/20'
                       : 'bg-zinc-800 text-zinc-600 cursor-not-allowed'}
                   disabled:opacity-50"
        >
            {#if running}
                <span class="flex items-center justify-center gap-2">
                    <svg class="w-4 h-4 animate-spin" fill="none" viewBox="0 0 24 24">
                        <circle class="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" stroke-width="4"/>
                        <path class="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8v8z"/>
                    </svg>
                    Flashing...
                </span>
            {:else if done}
                ✓ Done — device rebooting
            {:else}
                Flash Firmware
            {/if}
        </button>

        {#if !bleState.connected}
            <p class="text-[10px] text-amber-500/80 text-center">Connect to device first</p>
        {/if}
    </div>

    <!-- Info -->
    <div class="mt-3 text-xs text-zinc-500 flex items-start gap-2">
        <svg class="w-4 h-4 shrink-0 mt-0.5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2"
                d="M13 16h-1v-4h-1m1-4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z" />
        </svg>
        <span>
            Select the <code class="text-zinc-400">.bin</code> from your Arduino build output
            (<code class="text-zinc-400">sketch/Leor.ino.bin</code>).
            The device will restart automatically after a successful flash.
            Rollback is automatic if the new firmware fails to boot.
        </span>
    </div>
</div>
