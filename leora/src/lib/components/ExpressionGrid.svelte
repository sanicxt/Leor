<script lang="ts">
    import { sendCommand, bleState } from "$lib/ble.svelte";

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
            class="bento-button bg-paper text-ink px-3 py-3 capitalize hover:bg-ink hover:text-paper disabled:opacity-50 disabled:cursor-not-allowed disabled:hover:bg-paper disabled:hover:text-ink"
            onclick={() => {
                console.log("Button clicked:", expr);
                sendCommand(expr);
            }}
            disabled={!bleState.connected}
        >
            <span>{expr}</span>
        </button>
    {/each}
</div>
