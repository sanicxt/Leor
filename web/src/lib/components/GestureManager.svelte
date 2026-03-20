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

<div class="bento-card bg-bento-green p-6 space-y-5">
    <!-- Header -->
    <div class="mb-4 border-b-2 border-bento-border pb-2 flex items-center justify-between flex-wrap gap-3">
        <div>
            <h2 class="text-xl font-black uppercase">Gesture Recognition</h2>
            <p class="text-sm font-bold opacity-80">Edge Impulse • 5 gestures @ 23Hz</p>
        </div>

        <div class="flex items-center gap-3">
            {#if bleState.gestureMatching}
                <span class="flex items-center gap-1.5 px-2.5 py-1 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-full">
                    <span class="w-2 h-2 bg-bento-green border border-bento-border rounded-full animate-pulse"></span>
                    <span class="text-ink text-[10px] font-bold uppercase tracking-wide">Live</span>
                </span>
            {/if}
            <button
                class="w-14 h-8 rounded-full border-2 border-bento-border transition-all duration-300 relative focus:outline-none disabled:opacity-50
                 {bleState.gestureMatching
                    ? 'bg-bento-yellow shadow-[2px_2px_0px_0px_var(--color-bento-border)]'
                    : 'bg-paper shadow-[2px_2px_0px_0px_var(--color-bento-border)]'}"
                onclick={toggleMatch}
                disabled={!bleState.connected}
                aria-label="Toggle gesture matching"
            >
                <span
                    class="absolute left-1 top-0.5 w-6 h-6 bg-paper border-[1.5px] border-bento-border rounded-full transition-transform duration-300 flex items-center justify-center
                   {bleState.gestureMatching ? 'translate-x-6' : 'translate-x-0'}"
                >
                    {#if bleState.gestureMatching}
                        <svg class="w-3 h-3 text-ink" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="3" d="M5 13l4 4L19 7" />
                        </svg>
                    {:else}
                        <svg class="w-3 h-3 text-ink" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M6 18L18 6M6 6l12 12" />
                        </svg>
                    {/if}
                </span>
            </button>
        </div>
    </div>

    <!-- Last Detection Display -->
    {#if lastDetectedGesture}
        <div class="p-4 text-center bg-paper border-2 border-bento-border shadow-[4px_4px_0px_0px_var(--color-bento-border)] rounded-2xl">
            <div class="relative text-center">
                <div class="text-4xl mb-2 flex items-center justify-center gap-3">
                    <span class="transform hover:scale-110 transition-transform">
                        {gestures.find((g) => g.name === lastDetectedGesture)?.icon || "❓"}
                    </span>
                    <svg class="w-6 h-6 text-ink" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                        <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M14 5l7 7m0 0l-7 7m7-7H3" />
                    </svg>
                    <span class="transform hover:scale-110 transition-transform">
                        {getActionEmoji(gestures.find((g) => g.name === lastDetectedGesture)?.action || "")}
                    </span>
                </div>
                <div class="text-ink text-lg font-black uppercase">
                    {lastDetectedGesture}
                </div>
                <div class="text-ink/60 font-bold text-xs mt-1">
                    Detection #{detectionCount}
                </div>
            </div>
        </div>
    {/if}

    <!-- Gesture Mappings -->
    <div class="space-y-3">
        <div class="flex items-center gap-2 text-[10px] text-ink/60 uppercase tracking-widest font-bold px-1">
            <svg class="w-3 h-3" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M8 7h12m0 0l-4-4m4 4l-4 4m0 6H4m0 0l4 4m-4-4l4-4" />
            </svg>
            Gesture Mappings
        </div>

        {#each gestures as gesture, i}
            <div
                class="flex items-center justify-between p-3 transition-all duration-300 border-2 border-bento-border rounded-xl {lastDetectedGesture === gesture.name
                    ? 'bg-bento-yellow shadow-[4px_4px_0px_0px_var(--color-bento-border)] scale-[1.02]'
                    : 'bg-paper shadow-[2px_2px_0px_0px_var(--color-bento-border)] hover:bg-paper/80'}"
            >
                <div class="flex items-center gap-3">
                    <div class="w-10 h-10 rounded-xl bg-paper border-2 border-bento-border flex items-center justify-center text-xl shadow-[2px_2px_0px_0px_var(--color-bento-border)]">
                        {gesture.icon}
                    </div>
                    <div>
                        <div class="text-ink font-bold uppercase text-sm">
                            {gesture.name}
                        </div>
                        <div class="text-ink/60 font-bold text-[10px]">
                            {gesture.description}
                        </div>
                    </div>
                </div>

                <div class="flex items-center gap-2">
                    {#if gesture.name === "neutral"}
                        <span class="text-ink/60 font-bold text-xs uppercase pr-2">BASELINE</span>
                    {:else if editingGesture === gesture.name}
                        <select
                            class="bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl px-2 py-1 text-sm text-ink font-bold focus:outline-none"
                            value={gesture.action}
                            onchange={(e) => updateGestureMapping(gesture.name, (e.target as HTMLSelectElement).value)}
                            onblur={() => (editingGesture = null)}
                        >
                            {#each expressions.filter((e) => e !== "") as expr}
                                <option value={expr}>{expr}</option>
                            {/each}
                        </select>
                    {:else}
                        <button
                            class="bento-button group flex items-center gap-2 px-3 py-1.5 bg-paper text-ink transition-all disabled:opacity-50 disabled:cursor-not-allowed"
                            onclick={() => (editingGesture = gesture.name)}
                            disabled={!bleState.connected}
                        >
                            <span class="text-xl group-hover:scale-110 transition-transform">{getActionEmoji(gesture.action)}</span>
                            <span class="font-bold text-xs uppercase">{gesture.action}</span>
                            <svg class="w-3 h-3 text-ink/60 group-hover:text-ink transition-colors" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M15.232 5.232l3.536 3.536m-2.036-5.036a2.5 2.5 0 113.536 3.536L6.5 21.036H3v-3.572L16.732 3.732z" />
                            </svg>
                        </button>
                    {/if}
                </div>
            </div>
        {/each}
    </div>

    <!-- Footer Info -->
    <div class="flex items-center justify-center gap-2 text-ink/60 text-[10px] uppercase font-bold tracking-wider mt-4">
        <svg class="w-3 h-3" fill="none" stroke="currentColor" viewBox="0 0 24 24">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M13 16h-1v-4h-1m1-4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z" />
        </svg>
        Tap expression to change • Syncs automatically
    </div>
</div>
