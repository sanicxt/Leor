<script lang="ts">
    import { sendCommand, bleState, setGestureMatching } from "$lib/ble.svelte";

    // Available expressions that can be mapped to gestures
    const expressions = [
        "",
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
        "blink",
        "wink",
        "laugh",
        "cry",
        "uwu",
        "xd",
    ];

    // Edge Impulse model gestures (5 classes @ 23Hz)
    let gestures = $state([
        {
            name: "neutral",
            action: "",
            icon: "⏸️",
            description: "Device at rest",
            color: "zinc",
        },
        {
            name: "patpat",
            action: "happy",
            icon: "👋",
            description: "Gentle pats",
            color: "amber",
        },
        {
            name: "pickup",
            action: "curious",
            icon: "🤲",
            description: "Lift up device",
            color: "sky",
        },
        {
            name: "shake",
            action: "confused",
            icon: "🔄",
            description: "Shake motion",
            color: "violet",
        },
        {
            name: "swipe",
            action: "surprised",
            icon: "👉",
            description: "Swipe gesture",
            color: "rose",
        },
    ]);

    // State
    let lastDetectedGesture = $state("");
    let detectionCount = $state(0);
    let clearTimer: ReturnType<typeof setTimeout> | null = null;
    let editingGesture = $state<string | null>(null);

    // Watch for gesture matches in BLE status
    $effect(() => {
        const status = bleState.lastStatus;
        if (!status || !status.startsWith("gm:")) return;

        const gesture = status.substring(3);
        if (gesture === lastDetectedGesture) return;

        lastDetectedGesture = gesture;
        detectionCount++;

        if (clearTimer) clearTimeout(clearTimer);
        clearTimer = setTimeout(() => {
            lastDetectedGesture = "";
        }, 3000);
    });

    // Sync gesture actions from BLE when mappings are received
    // Read directly from bleState proxy for $effect to track dependencies
    $effect(() => {
        const mappings = bleState.gestureMappings;
        if (mappings.length === 0) return;

        // Check if any updates are needed
        const needsUpdate = mappings.some((mapping) => {
            const gesture = gestures.find((g) => g.name === mapping.name);
            return gesture && mapping.action !== gesture.action;
        });

        if (needsUpdate) {
            // Create new array reference for reactivity
            gestures = gestures.map((g) => {
                const mapping = mappings.find((m) => m.name === g.name);
                return mapping && mapping.action !== g.action
                    ? { ...g, action: mapping.action }
                    : g;
            });
        }
    });

    async function toggleMatch() {
        const newVal = !bleState.gestureMatching;
        setGestureMatching(newVal);
        await sendCommand(`gm=${newVal ? "1" : "0"}`);
    }

    async function updateGestureMapping(
        gestureName: string,
        newAction: string,
    ) {
        const index = gestures.findIndex((g) => g.name === gestureName);
        if (index !== -1) {
            // Update local gestures array with new reference for reactivity
            gestures = gestures.map((g, i) =>
                i === index ? { ...g, action: newAction } : g,
            );

            // Update bleState to keep sync effect in sync
            const bleMappingIndex = bleState.gestureMappings.findIndex(
                (m) => m.name === gestureName,
            );
            if (bleMappingIndex !== -1) {
                bleState.gestureMappings[bleMappingIndex].action = newAction;
            }

            await sendCommand(`ga=${index}:${newAction}`);
        }
        editingGesture = null;
    }

    function getActionEmoji(action: string): string {
        const emojis: Record<string, string> = {
            happy: "😊",
            sad: "😢",
            angry: "😠",
            love: "😍",
            surprised: "😮",
            confused: "😵",
            sleepy: "😴",
            curious: "🤔",
            nervous: "😰",
            knocked: "🤕",
            blink: "😉",
            wink: "😜",
            laugh: "😂",
            cry: "😭",
        };
        return emojis[action] || "😐";
    }
</script>

<div
    class="bg-gradient-to-br from-emerald-950/40 to-green-950/30 border border-emerald-500/20 rounded-2xl p-5 backdrop-blur-lg space-y-5"
>
    <!-- Header -->
    <div class="flex items-center justify-between flex-wrap gap-3">
        <div class="flex items-center gap-3">
            <div
                class="w-10 h-10 rounded-xl bg-gradient-to-br from-emerald-500 to-green-600 flex items-center justify-center shadow-lg shadow-emerald-500/20"
            >
                <svg
                    class="w-5 h-5 text-white"
                    fill="none"
                    stroke="currentColor"
                    viewBox="0 0 24 24"
                >
                    <path
                        stroke-linecap="round"
                        stroke-linejoin="round"
                        stroke-width="2"
                        d="M7 11.5V14m0-2.5v-6a1.5 1.5 0 113 0m-3 6a1.5 1.5 0 00-3 0v2a7.5 7.5 0 0015 0v-5a1.5 1.5 0 00-3 0m-6-3V11m0-5.5v-1a1.5 1.5 0 013 0v1m0 0V11m0-5.5a1.5 1.5 0 013 0v3m0 0V11"
                    />
                </svg>
            </div>
            <div>
                <h3 class="text-white text-sm font-semibold">
                    Gesture Recognition
                </h3>
                <p class="text-emerald-300/60 text-xs">
                    Edge Impulse • 5 gestures @ 23Hz
                </p>
            </div>
        </div>

        <div class="flex items-center gap-3">
            {#if bleState.gestureMatching}
                <span
                    class="flex items-center gap-1.5 px-2.5 py-1 bg-emerald-500/20 border border-emerald-500/30 rounded-full"
                >
                    <span
                        class="w-1.5 h-1.5 bg-emerald-400 rounded-full animate-pulse"
                    ></span>
                    <span
                        class="text-emerald-400 text-[10px] font-bold uppercase tracking-wide"
                        >Live</span
                    >
                </span>
            {/if}
            <button
                class="w-14 h-8 rounded-full transition-all duration-300 relative {bleState.gestureMatching
                    ? 'bg-gradient-to-r from-emerald-500 to-green-500 shadow-lg shadow-emerald-500/30'
                    : 'bg-zinc-700/80'} disabled:opacity-50"
                onclick={toggleMatch}
                disabled={!bleState.connected}
                aria-label="Toggle gesture matching"
            >
                <span
                    class="absolute left-1 top-1 w-6 h-6 bg-white rounded-full transition-transform duration-300 shadow-md flex items-center justify-center {bleState.gestureMatching
                        ? 'translate-x-6'
                        : 'translate-x-0'}"
                >
                    {#if bleState.gestureMatching}
                        <svg
                            class="w-3.5 h-3.5 text-emerald-500"
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
                    {:else}
                        <svg
                            class="w-3.5 h-3.5 text-zinc-400"
                            fill="none"
                            stroke="currentColor"
                            viewBox="0 0 24 24"
                        >
                            <path
                                stroke-linecap="round"
                                stroke-linejoin="round"
                                stroke-width="2"
                                d="M6 18L18 6M6 6l12 12"
                            />
                        </svg>
                    {/if}
                </span>
            </button>
        </div>
    </div>

    <!-- Last Detection Display -->
    {#if lastDetectedGesture}
        <div
            class="relative overflow-hidden bg-gradient-to-r from-emerald-500/20 via-green-500/20 to-teal-500/20 border border-emerald-500/30 rounded-xl p-4"
        >
            <div
                class="absolute inset-0 bg-gradient-to-r from-emerald-500/5 to-transparent animate-pulse"
            ></div>
            <div class="relative text-center">
                <div
                    class="text-4xl mb-2 flex items-center justify-center gap-3"
                >
                    <span class="transform hover:scale-110 transition-transform"
                        >{gestures.find((g) => g.name === lastDetectedGesture)
                            ?.icon || "❓"}</span
                    >
                    <svg
                        class="w-6 h-6 text-emerald-400"
                        fill="none"
                        stroke="currentColor"
                        viewBox="0 0 24 24"
                    >
                        <path
                            stroke-linecap="round"
                            stroke-linejoin="round"
                            stroke-width="2"
                            d="M14 5l7 7m0 0l-7 7m7-7H3"
                        />
                    </svg>
                    <span class="transform hover:scale-110 transition-transform"
                        >{getActionEmoji(
                            gestures.find((g) => g.name === lastDetectedGesture)
                                ?.action || "",
                        )}</span
                    >
                </div>
                <div class="text-emerald-300 text-lg font-bold capitalize">
                    {lastDetectedGesture}
                </div>
                <div class="text-zinc-500 text-xs">
                    Detection #{detectionCount}
                </div>
            </div>
        </div>
    {/if}

    <!-- Gesture Mappings -->
    <div class="space-y-2">
        <div
            class="flex items-center gap-2 text-[10px] text-zinc-500 uppercase tracking-widest font-bold px-1"
        >
            <svg
                class="w-3 h-3"
                fill="none"
                stroke="currentColor"
                viewBox="0 0 24 24"
            >
                <path
                    stroke-linecap="round"
                    stroke-linejoin="round"
                    stroke-width="2"
                    d="M8 7h12m0 0l-4-4m4 4l-4 4m0 6H4m0 0l4 4m-4-4l4-4"
                />
            </svg>
            Gesture Mappings
        </div>

        {#each gestures as gesture, i}
            <div
                class="flex items-center justify-between rounded-xl px-4 py-3 transition-all duration-300 {lastDetectedGesture ===
                gesture.name
                    ? 'bg-emerald-500/20 border-2 border-emerald-500/50 shadow-lg shadow-emerald-500/10'
                    : 'bg-white/5 border border-white/5 hover:bg-white/10'}"
            >
                <div class="flex items-center gap-3">
                    <div
                        class="w-10 h-10 rounded-lg bg-gradient-to-br from-zinc-700 to-zinc-800 flex items-center justify-center text-xl shadow-inner"
                    >
                        {gesture.icon}
                    </div>
                    <div>
                        <div class="text-white font-medium capitalize text-sm">
                            {gesture.name}
                        </div>
                        <div class="text-zinc-600 text-[10px]">
                            {gesture.description}
                        </div>
                    </div>
                </div>

                <div class="flex items-center gap-2">
                    {#if gesture.name === "neutral"}
                        <span class="text-zinc-600 text-xs italic"
                            >baseline</span
                        >
                    {:else if editingGesture === gesture.name}
                        <select
                            class="bg-zinc-800 border-2 border-emerald-500/50 rounded-lg px-3 py-1.5 text-sm text-white focus:outline-none focus:ring-2 focus:ring-emerald-500/30"
                            value={gesture.action}
                            onchange={(e) =>
                                updateGestureMapping(
                                    gesture.name,
                                    (e.target as HTMLSelectElement).value,
                                )}
                            onblur={() => (editingGesture = null)}
                        >
                            {#each expressions.filter((e) => e !== "") as expr}
                                <option value={expr}>{expr}</option>
                            {/each}
                        </select>
                    {:else}
                        <button
                            class="group flex items-center gap-2 px-3 py-2 bg-zinc-800/80 hover:bg-zinc-700 rounded-lg transition-all border border-white/5 hover:border-emerald-500/30"
                            onclick={() => (editingGesture = gesture.name)}
                            disabled={!bleState.connected}
                        >
                            <span
                                class="text-xl group-hover:scale-110 transition-transform"
                                >{getActionEmoji(gesture.action)}</span
                            >
                            <span class="text-emerald-400 text-sm font-medium"
                                >{gesture.action}</span
                            >
                            <svg
                                class="w-3 h-3 text-zinc-500 group-hover:text-emerald-400 transition-colors"
                                fill="none"
                                stroke="currentColor"
                                viewBox="0 0 24 24"
                            >
                                <path
                                    stroke-linecap="round"
                                    stroke-linejoin="round"
                                    stroke-width="2"
                                    d="M15.232 5.232l3.536 3.536m-2.036-5.036a2.5 2.5 0 113.536 3.536L6.5 21.036H3v-3.572L16.732 3.732z"
                                />
                            </svg>
                        </button>
                    {/if}
                </div>
            </div>
        {/each}
    </div>

    <!-- Footer Info -->
    <div
        class="flex items-center justify-center gap-2 text-zinc-600 text-[10px]"
    >
        <svg
            class="w-3 h-3"
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
        Tap expression to change • Syncs automatically
    </div>
</div>
