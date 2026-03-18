<script lang="ts">
    import { sendCommand, bleState } from "$lib/ble.svelte.ts";

    const displayTypes = [
        { name: "SH1106", value: "sh1106", description: "Default display" },
        { name: "SSD1306", value: "ssd1306", description: "Alternative" },
    ];

    let selectedDisplay = $state("sh1106");
    let i2cAddress = $state("0x3C");
    let applyingSettings = $state(false);
    let restartRequired = $state(false);

    // Sync from device when connected
    $effect(() => {
        if (bleState.connected && bleState.display.type) {
            selectedDisplay = bleState.display.type;
            i2cAddress = bleState.display.addr;
        }
    });

    async function applyDisplayType() {
        if (!bleState.connected) return;

        applyingSettings = true;
        restartRequired = false;

        try {
            // Send display type command
            await sendCommand(`display:type=${selectedDisplay}`);
            await new Promise((r) => setTimeout(r, 100));

            // Send I2C address
            await sendCommand(`display:addr=${i2cAddress}`);

            // Changes always require restart
            restartRequired = true;
        } finally {
            applyingSettings = false;
        }
    }

    async function restartDevice() {
        if (!bleState.connected) return;

        if (confirm("Restart device to apply display changes?")) {
            await sendCommand("restart");
            restartRequired = false;

            // Connection will be lost, inform user
            setTimeout(() => {
                alert(
                    "Device is restarting. Please reconnect in a few seconds.",
                );
            }, 500);
        }
    }
</script>

<div class="bento-card bg-bento-peach p-6 space-y-5">
    <!-- Header -->
    <div class="mb-4 border-b-2 border-bento-border pb-2">
        <h2 class="text-xl font-black uppercase">Display Settings</h2>
        <p class="text-sm font-bold opacity-80">Configure OLED display</p>
    </div>

    <!-- Display Type Selection -->
    <div>
        <label class="block text-ink text-xs font-bold mb-2 uppercase tracking-wider">Display Type</label>
        <div class="grid grid-cols-2 gap-3">
            {#each displayTypes as display}
                <button
                    onclick={() => (selectedDisplay = display.value)}
                    disabled={!bleState.connected}
                    class="bento-button group relative p-3 transition-all duration-200 disabled:opacity-50 {selectedDisplay === display.value ? 'bg-bento-yellow' : 'bg-paper'}"
                >
                    <div class="flex flex-col items-center gap-2">
                        <div class="w-8 h-8 rounded-lg border-2 border-bento-border flex items-center justify-center transition-colors {selectedDisplay === display.value ? 'bg-paper text-ink' : 'bg-black/10 dark:bg-white/10 text-ink/60 group-hover:text-ink'}">
                            <svg class="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                                <rect x="3" y="4" width="18" height="14" rx="2" stroke-width="2" />
                                <path d="M8 2v4M16 2v4" stroke-width="2" stroke-linecap="round" />
                            </svg>
                        </div>
                        <span class="text-sm font-bold {selectedDisplay === display.value ? 'text-ink' : 'text-ink/70 group-hover:text-ink'}">{display.name}</span>
                        <span class="text-xs font-bold {selectedDisplay === display.value ? 'text-ink/60' : 'text-ink/40'}">{display.description}</span>
                    </div>
                    {#if selectedDisplay === display.value}
                        <div class="absolute top-2 right-2 w-5 h-5 bg-ink rounded-full flex items-center justify-center">
                            <svg class="w-3 h-3 text-paper" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="3" d="M5 13l4 4L19 7" />
                            </svg>
                        </div>
                    {/if}
                </button>
            {/each}
        </div>
    </div>

    <!-- I2C Address Input -->
    <div>
        <label class="block text-ink text-xs font-bold mb-2 uppercase tracking-wider">I2C Address</label>
        <div class="flex gap-2">
            <input
                type="text"
                bind:value={i2cAddress}
                disabled={!bleState.connected}
                placeholder="0x3C"
                class="flex-1 px-3 py-2.5 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl text-ink font-bold placeholder-ink/40 outline-none transition-all disabled:opacity-50 font-mono text-sm"
            />
            <div class="flex items-center px-3 py-2 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl">
                <span class="text-ink font-bold text-xs">Hex</span>
            </div>
        </div>
        <p class="text-ink/60 font-bold text-xs mt-2">Try 0x3C or 0x3D</p>
    </div>

    <!-- Action Button -->
    <button
        onclick={applyDisplayType}
        disabled={!bleState.connected || applyingSettings}
        class="bento-button w-full flex items-center justify-center gap-2 py-2.5 bg-bento-yellow text-ink transition-all disabled:opacity-50"
    >
        <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M5 13l4 4L19 7" />
        </svg>
        {applyingSettings ? "Saving..." : "Apply Settings"}
    </button>

    <!-- Restart Required Banner -->
    {#if restartRequired}
        <div class="flex gap-3 p-3 bg-bento-peach border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl items-center">
            <svg class="w-5 h-5 text-ink flex-shrink-0" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-3L13.732 4c-.77-1.333-2.694-1.333-3.464 0L3.34 16c-.77 1.333.192 3 1.732 3z" />
            </svg>
            <div class="flex-1">
                <p class="text-ink font-bold text-xs uppercase tracking-wider">Restart Required</p>
                <p class="text-ink/80 text-xs mt-0.5 font-bold">Changes saved. Restart to apply.</p>
            </div>
            <button onclick={restartDevice} class="bento-button px-3 py-1.5 bg-bento-pink text-ink text-xs">
                Restart
            </button>
        </div>
    {/if}

    <!-- Info Note -->
    <div class="flex gap-2 p-3 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl">
        <svg class="w-4 h-4 text-ink flex-shrink-0 mt-0.5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M13 16h-1v-4h-1m1-4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z" />
        </svg>
        <p class="text-ink/80 text-xs font-bold leading-relaxed">
            Both display types work with any color. Try 0x3C or 0x3D if not working.
        </p>
    </div>
</div>
