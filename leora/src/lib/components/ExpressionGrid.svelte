<script lang="ts">
    import { sendCommand } from "$lib/ble.svelte";

    const expressions = [
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
        "uwu",
        "xd",
        "laugh",
        "cry",
        "neutral",
        "idle",
    ];
</script>

<div class="grid grid-cols-2 xs:grid-cols-3 sm:grid-cols-4 gap-3">
    {#each expressions as expr}
        <button
            class="px-3 py-3.5 bg-zinc-900/50 hover:bg-zinc-800 text-zinc-300 hover:text-white rounded-xl
             border border-white/5 hover:border-indigo-500/50 transition-all duration-300
             text-sm font-medium capitalize relative overflow-hidden group shadow-sm hover:shadow-indigo-500/20 hover:-translate-y-0.5"
            onclick={() => {
                console.log("Button clicked:", expr);
                sendCommand(expr);
            }}
        >
            <div
                class="absolute inset-0 bg-gradient-to-br from-indigo-500/10 to-purple-500/10 opacity-0 group-hover:opacity-100 transition-opacity"
            ></div>
            <span class="relative z-10">{expr}</span>
        </button>
    {/each}
</div>
