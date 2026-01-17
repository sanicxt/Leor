<script lang="ts">
    import {
        sendCommand,
        bleState,
        getGestureMatching,
        setGestureMatching,
    } from "$lib/ble.svelte";

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
    ];

    // Edge Impulse model gestures (5 classes @ 23Hz)
    // Order matches model output alphabetically
    let gestures = $state([
        {
            name: "neutral",
            action: "",
            icon: "⏸️",
            description: "Device at rest",
        },
        {
            name: "patpat",
            action: "happy",
            icon: "👋",
            description: "Gentle pats",
        },
        {
            name: "pickup",
            action: "curious",
            icon: "🤲",
            description: "Lift up device",
        },
        {
            name: "shake",
            action: "confused",
            icon: "🔄",
            description: "Shake motion",
        },
        {
            name: "swipe",
            action: "surprised",
            icon: "👉",
            description: "Swipe gesture",
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

    async function toggleMatch() {
        const newVal = !getGestureMatching();
        setGestureMatching(newVal);
        await sendCommand(`gm=${newVal ? "1" : "0"}`);
    }

    async function updateGestureMapping(
        gestureName: string,
        newAction: string,
    ) {
        const gesture = gestures.find((g) => g.name === gestureName);
        if (gesture) {
            gesture.action = newAction;
            // Send update to device: ga=index:action
            const index = gestures.findIndex((g) => g.name === gestureName);
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
    class="bg-zinc-900/40 border border-white/5 rounded-3xl p-6 backdrop-blur-md space-y-5"
>
    <!-- Header -->
    <div class="flex items-center justify-between flex-wrap gap-3">
        <div>
            <h3
                class="text-zinc-400 text-xs font-bold tracking-widest uppercase"
            >
                Gesture Recognition
            </h3>
            <p class="text-zinc-600 text-[10px] mt-1">
                Edge Impulse • 5 gestures @ 23Hz
            </p>
        </div>

        <div class="flex items-center gap-3">
            {#if getGestureMatching()}
                <span
                    class="px-2 py-1 bg-emerald-500/20 text-emerald-400 rounded-lg text-[10px] font-bold animate-pulse"
                >
                    ACTIVE
                </span>
            {/if}
            <button
                class="w-14 h-7 rounded-full transition-all duration-300 relative {getGestureMatching()
                    ? 'bg-emerald-500 shadow-lg shadow-emerald-500/30'
                    : 'bg-zinc-700'} disabled:opacity-50"
                onclick={toggleMatch}
                disabled={!bleState.connected}
                aria-label="Toggle gesture matching"
            >
                <span
                    class="absolute left-0.5 top-0.5 w-6 h-6 bg-white rounded-full transition-transform duration-300 shadow {getGestureMatching()
                        ? 'translate-x-7'
                        : 'translate-x-0'}"
                ></span>
            </button>
        </div>
    </div>

    <!-- Last Detection Display -->
    {#if lastDetectedGesture}
        <div
            class="bg-gradient-to-r from-indigo-500/20 to-purple-500/20 border border-indigo-500/30 rounded-2xl p-4"
        >
            <div class="text-center">
                <div class="text-4xl mb-2">
                    {gestures.find((g) => g.name === lastDetectedGesture)
                        ?.icon || "❓"} → {getActionEmoji(
                        gestures.find((g) => g.name === lastDetectedGesture)
                            ?.action || "",
                    )}
                </div>
                <div class="text-indigo-300 text-lg font-bold capitalize">
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
            class="text-[10px] text-zinc-500 uppercase tracking-widest font-bold"
        >
            Gesture → Expression Mappings
        </div>

        {#each gestures as gesture, i}
            <div
                class="flex items-center justify-between bg-zinc-800/50 rounded-xl px-4 py-3 border transition-all {lastDetectedGesture ===
                gesture.name
                    ? 'border-indigo-500/50 bg-indigo-500/10'
                    : 'border-white/5'}"
            >
                <div class="flex items-center gap-3">
                    <span class="text-xl">{gesture.icon}</span>
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
                        <span class="text-zinc-600 text-xs">baseline</span>
                    {:else if editingGesture === gesture.name}
                        <select
                            class="bg-zinc-700 border border-indigo-500/50 rounded-lg px-2 py-1 text-sm text-white"
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
                            class="flex items-center gap-2 px-3 py-1.5 bg-zinc-700/50 hover:bg-zinc-700 rounded-lg transition-all border border-white/5"
                            onclick={() => (editingGesture = gesture.name)}
                            disabled={!bleState.connected}
                        >
                            <span class="text-lg"
                                >{getActionEmoji(gesture.action)}</span
                            >
                            <span class="text-indigo-400 text-sm"
                                >{gesture.action}</span
                            >
                            <svg
                                xmlns="http://www.w3.org/2000/svg"
                                width="12"
                                height="12"
                                viewBox="0 0 24 24"
                                fill="none"
                                stroke="currentColor"
                                stroke-width="2"
                                stroke-linecap="round"
                                stroke-linejoin="round"
                                class="text-zinc-500"
                                ><path d="M12 20h9"></path><path
                                    d="M16.5 3.5a2.12 2.12 0 0 1 3 3L7 19l-4 1 1-4Z"
                                ></path></svg
                            >
                        </button>
                    {/if}
                </div>
            </div>
        {/each}
    </div>

    <!-- Info -->
    <div class="text-center text-zinc-600 text-[10px]">
        Tap expression to change mapping • Changes sync to device
    </div>
</div>
