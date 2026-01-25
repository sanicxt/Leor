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
    class="bg-gradient-to-br from-cyan-950/40 to-teal-950/30 border border-cyan-500/20 rounded-2xl p-5 backdrop-blur-lg space-y-5"
>
    <!-- Header -->
    <div class="flex items-center justify-between">
        <div class="flex items-center gap-3">
            <div
                class="w-10 h-10 rounded-xl bg-gradient-to-br from-cyan-500 to-teal-600 flex items-center justify-center shadow-lg shadow-cyan-500/20"
            >
                <svg
                    class="w-5 h-5 text-white"
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
            </div>
            <div>
                <h3 class="text-white text-sm font-semibold">
                    Display Settings
                </h3>
                <p class="text-cyan-300/60 text-xs">Configure OLED display</p>
            </div>
        </div>
    </div>

    <!-- Display Type Selection -->
    <div>
        <label
            class="block text-cyan-300/80 text-xs font-medium mb-2 uppercase tracking-wider"
            >Display Type</label
        >
        <div class="grid grid-cols-2 gap-3">
            {#each displayTypes as display}
                <button
                    onclick={() => (selectedDisplay = display.value)}
                    disabled={!bleState.connected}
                    class="group relative p-3 rounded-xl border transition-all duration-200 disabled:opacity-40 disabled:cursor-not-allowed {selectedDisplay ===
                    display.value
                        ? 'bg-cyan-500/20 border-cyan-400/40'
                        : 'bg-cyan-500/5 border-cyan-500/10 hover:border-cyan-400/30 hover:bg-cyan-500/10'}"
                >
                    <div class="flex flex-col items-center gap-2">
                        <div
                            class="w-8 h-8 rounded-lg flex items-center justify-center transition-colors {selectedDisplay ===
                            display.value
                                ? 'bg-cyan-400/30 text-cyan-200'
                                : 'bg-cyan-500/10 text-cyan-400/60 group-hover:text-cyan-300'}"
                        >
                            <svg
                                class="w-5 h-5"
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
                            class="text-sm font-medium {selectedDisplay ===
                            display.value
                                ? 'text-cyan-200'
                                : 'text-cyan-300/70 group-hover:text-cyan-200'}"
                            >{display.name}</span
                        >
                        <span
                            class="text-xs {selectedDisplay ===
                            display.value
                                ? 'text-cyan-300/60'
                                : 'text-cyan-400/40'}"
                            >{display.description}</span
                        >
                    </div>
                    {#if selectedDisplay === display.value}
                        <div
                            class="absolute top-2 right-2 w-5 h-5 bg-cyan-400 rounded-full flex items-center justify-center shadow-lg shadow-cyan-400/30"
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
        <label
            class="block text-cyan-300/80 text-xs font-medium mb-2 uppercase tracking-wider"
            >I2C Address</label
        >
        <div class="flex gap-2">
            <input
                type="text"
                bind:value={i2cAddress}
                disabled={!bleState.connected}
                placeholder="0x3C"
                class="flex-1 px-3 py-2.5 bg-cyan-500/5 border border-cyan-500/20 rounded-xl text-cyan-100 placeholder-cyan-400/40 focus:border-cyan-400/50 focus:ring-2 focus:ring-cyan-400/20 outline-none transition-all disabled:opacity-40 disabled:cursor-not-allowed font-mono text-sm"
            />
            <div
                class="flex items-center px-3 py-2 bg-cyan-500/10 rounded-xl border border-cyan-500/20"
            >
                <span class="text-cyan-300/60 text-xs">Hex</span>
            </div>
        </div>
        <p class="text-cyan-300/40 text-xs mt-2">
            Try 0x3C or 0x3D
        </p>
    </div>

    <!-- Action Button -->
    <button
        onclick={applyDisplayType}
        disabled={!bleState.connected || applyingSettings}
        class="w-full flex items-center justify-center gap-2 py-2.5 rounded-xl bg-cyan-500/20 hover:bg-cyan-500/30 border border-cyan-400/30 text-cyan-200 text-sm font-medium transition-all disabled:opacity-40 disabled:cursor-not-allowed"
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
        {applyingSettings ? "Saving..." : "Apply Settings"}
    </button>

    <!-- Restart Required Banner -->
    {#if restartRequired}
        <div
            class="flex gap-3 p-3 bg-orange-500/10 border border-orange-500/20 rounded-xl items-center"
        >
            <svg
                class="w-5 h-5 text-orange-300 flex-shrink-0"
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
                <p class="text-orange-200 font-medium text-xs">
                    Restart Required
                </p>
                <p class="text-orange-200/60 text-xs mt-0.5">
                    Changes saved. Restart to apply.
                </p>
            </div>
            <button
                onclick={restartDevice}
                class="px-3 py-1.5 bg-orange-500/30 hover:bg-orange-500/40 border border-orange-400/30 text-orange-200 rounded-lg font-medium transition-all text-xs"
            >
                Restart
            </button>
        </div>
    {/if}

    <!-- Info Note -->
    <div
        class="flex gap-2 p-3 bg-cyan-500/5 border border-cyan-500/10 rounded-xl"
    >
        <svg
            class="w-4 h-4 text-cyan-400/60 flex-shrink-0 mt-0.5"
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
        <p class="text-cyan-300/60 text-xs leading-relaxed">
            Both display types work with any color. Try 0x3C or 0x3D if not
            working.
        </p>
    </div>
</div>
