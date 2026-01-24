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

<div
    class="bg-gradient-to-br from-zinc-900/60 to-zinc-950/40 border border-white/10 rounded-3xl p-6 backdrop-blur-xl relative overflow-hidden"
>
    <!-- Decorative gradient -->
    <div
        class="absolute top-0 right-0 w-48 h-48 bg-gradient-to-bl from-cyan-500/10 to-transparent rounded-full blur-3xl"
    ></div>

    <!-- Section Header -->
    <div class="relative mb-6">
        <div class="flex items-center gap-2 mb-2">
            <svg
                class="w-5 h-5 text-cyan-400"
                fill="none"
                stroke="currentColor"
                viewBox="0 0 24 24"
            >
                <rect
                    x="2"
                    y="3"
                    width="20"
                    height="14"
                    rx="2"
                    stroke-width="2"
                />
                <path d="M8 21h8" stroke-width="2" stroke-linecap="round" />
                <path d="M12 17v4" stroke-width="2" stroke-linecap="round" />
            </svg>
            <h3 class="text-white text-lg font-semibold">Display Settings</h3>
        </div>
        <p class="text-zinc-400 text-sm">
            Configure OLED display type and I2C address
        </p>
    </div>

    <div class="relative space-y-5">
        <!-- Display Type Selection -->
        <div>
            <label class="block text-zinc-300 text-sm font-medium mb-3"
                >Display Type</label
            >
            <div class="grid grid-cols-2 gap-3">
                {#each displayTypes as display}
                    <button
                        onclick={() => (selectedDisplay = display.value)}
                        disabled={!bleState.connected}
                        class="group relative p-4 rounded-xl border transition-all duration-200 disabled:opacity-40 disabled:cursor-not-allowed {selectedDisplay ===
                        display.value
                            ? 'bg-cyan-500/20 border-cyan-500/50 shadow-lg shadow-cyan-500/20'
                            : 'bg-white/5 border-white/10 hover:border-cyan-500/30 hover:bg-white/10'}"
                    >
                        <div class="flex flex-col items-center gap-2">
                            <div
                                class="w-10 h-10 rounded-lg flex items-center justify-center transition-colors {selectedDisplay ===
                                display.value
                                    ? 'bg-cyan-500/30 text-cyan-300'
                                    : 'bg-white/10 text-zinc-400 group-hover:text-cyan-400'}"
                            >
                                <svg
                                    class="w-6 h-6"
                                    fill="none"
                                    stroke="currentColor"
                                    viewBox="0 0 24 24"
                                >
                                    <rect
                                        x="3"
                                        y="4"
                                        width="18"
                                        height="14"
                                        rx="2"
                                        stroke-width="2"
                                    />
                                    <path
                                        d="M8 2v4M16 2v4"
                                        stroke-width="2"
                                        stroke-linecap="round"
                                    />
                                </svg>
                            </div>
                            <span
                                class="font-semibold {selectedDisplay ===
                                display.value
                                    ? 'text-cyan-300'
                                    : 'text-zinc-300'}">{display.name}</span
                            >
                            <span
                                class="text-xs {selectedDisplay ===
                                display.value
                                    ? 'text-cyan-400/70'
                                    : 'text-zinc-500'}"
                                >{display.description}</span
                            >
                        </div>
                        {#if selectedDisplay === display.value}
                            <div
                                class="absolute top-2 right-2 w-5 h-5 bg-cyan-500 rounded-full flex items-center justify-center"
                            >
                                <svg
                                    class="w-3 h-3 text-white"
                                    fill="none"
                                    stroke="currentColor"
                                    viewBox="0 0 24 24"
                                >
                                    <path
                                        stroke-linecap="round"
                                        stroke-linejoin="round"
                                        stroke-width="3"
                                        d="M5 13l4 4L19 7"
                                    />
                                </svg>
                            </div>
                        {/if}
                    </button>
                {/each}
            </div>
        </div>

        <!-- I2C Address Input -->
        <div>
            <label class="block text-zinc-300 text-sm font-medium mb-2"
                >I2C Address</label
            >
            <div class="flex gap-2">
                <input
                    type="text"
                    bind:value={i2cAddress}
                    disabled={!bleState.connected}
                    placeholder="0x3C"
                    class="flex-1 px-4 py-3 bg-zinc-900/50 border border-white/10 rounded-xl text-white placeholder-zinc-500 focus:border-cyan-500/50 focus:ring-2 focus:ring-cyan-500/20 outline-none transition-all disabled:opacity-40 disabled:cursor-not-allowed font-mono text-sm"
                />
                <div
                    class="flex items-center px-4 py-2 bg-zinc-800/50 rounded-xl border border-white/10"
                >
                    <span class="text-zinc-400 text-xs">Hex format</span>
                </div>
            </div>
            <p class="text-zinc-500 text-xs mt-2">
                Common addresses: 0x3C (blue), 0x3D (white)
            </p>
        </div>

        <!-- Action Button -->
        <button
            onclick={applyDisplayType}
            disabled={!bleState.connected || applyingSettings}
            class="w-full flex items-center justify-center gap-2 py-2.5 rounded-xl bg-cyan-500/20 hover:bg-cyan-500/30 border border-cyan-500/30 text-cyan-300 text-sm font-medium transition-all disabled:opacity-40"
        >
            <svg
                class="w-4 h-4"
                fill="none"
                stroke="currentColor"
                viewBox="0 0 24 24"
            >
                <path
                    stroke-linecap="round"
                    stroke-linejoin="round"
                    stroke-width="2"
                    d="M5 13l4 4L19 7"
                />
            </svg>
            {applyingSettings ? "Saving..." : "Save Changes"}
        </button>

        <!-- Restart Required Banner -->
        {#if restartRequired}
            <div
                class="flex gap-3 p-4 bg-orange-500/10 border border-orange-500/30 rounded-xl items-center"
            >
                <svg
                    class="w-5 h-5 text-orange-400 flex-shrink-0"
                    fill="none"
                    stroke="currentColor"
                    viewBox="0 0 24 24"
                >
                    <path
                        stroke-linecap="round"
                        stroke-linejoin="round"
                        stroke-width="2"
                        d="M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-3L13.732 4c-.77-1.333-2.694-1.333-3.464 0L3.34 16c-.77 1.333.192 3 1.732 3z"
                    />
                </svg>
                <div class="flex-1">
                    <p class="text-orange-300 font-medium text-sm">
                        Restart Required
                    </p>
                    <p class="text-orange-200/80 text-xs mt-1">
                        Changes saved. Restart device to apply.
                    </p>
                </div>
                <button
                    onclick={restartDevice}
                    class="px-4 py-2 bg-orange-600 hover:bg-orange-500 text-white rounded-lg font-medium transition-all text-sm"
                >
                    Restart Now
                </button>
            </div>
        {/if}

        <!-- Info Box -->
        <div
            class="flex gap-3 p-4 bg-blue-500/10 border border-blue-500/20 rounded-xl"
        >
            <svg
                class="w-5 h-5 text-blue-400 flex-shrink-0 mt-0.5"
                fill="none"
                stroke="currentColor"
                viewBox="0 0 24 24"
            >
                <path
                    stroke-linecap="round"
                    stroke-linejoin="round"
                    stroke-width="2"
                    d="M13 16h-1v-4h-1m1-4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z"
                />
            </svg>
            <div class="text-sm">
                <p class="text-blue-300 font-medium mb-1">Note:</p>
                <p class="text-blue-200/80 text-xs leading-relaxed">
                    Both blue and white displays work with either driver. The
                    I2C address may vary by manufacturer. If the display doesn't
                    work, try the other address (0x3C or 0x3D).
                </p>
            </div>
        </div>
    </div>
</div>
