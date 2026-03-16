<script lang="ts">
    import { sendCommand, bleState } from "$lib/ble.svelte";

    const actions = [
        { cmd: "blink", label: "Blink" },
        { cmd: "wink", label: "Wink L" },
        { cmd: "winkr", label: "Wink R" },
    ];
</script>

<div class="flex flex-wrap gap-2">
    {#each actions as action}
        <button
            class="bento-button bg-paper text-ink px-4 py-2 hover:bg-ink hover:text-paper disabled:opacity-50 disabled:cursor-not-allowed disabled:hover:bg-paper disabled:hover:text-ink"
            onclick={() => sendCommand(action.cmd)}
            disabled={!bleState.connected}
        >
            {action.label}
        </button>
    {/each}
</div>
