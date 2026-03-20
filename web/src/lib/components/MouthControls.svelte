<script lang="ts">
    import { sendCommand, bleState } from "$lib/ble.svelte";

    const mouthTypes = ["smile", "frown", "open", "ooo", "flat", "uwum", "xdm"];
    const mouthAnims = ["talk", "chew", "wobble"];
</script>

<div class="space-y-4">
    <div class="flex flex-wrap gap-3">
        {#each mouthTypes as mouth}
            <button
                class="bento-button bg-paper text-ink capitalize px-4 py-2 hover:bg-ink hover:text-bento-border disabled:opacity-50 disabled:cursor-not-allowed disabled:hover:bg-paper disabled:hover:text-ink"
                onclick={() => sendCommand(mouth)}
                disabled={!bleState.connected}
            >
                {mouth}
            </button>
        {/each}
    </div>
    <div class="flex flex-wrap gap-3">
        {#each mouthAnims as anim}
            <button
                class="bento-button bg-paper text-ink capitalize px-4 py-2 hover:bg-ink hover:text-bento-border disabled:opacity-50 disabled:cursor-not-allowed disabled:hover:bg-paper disabled:hover:text-ink"
                onclick={() => sendCommand(anim)}
                disabled={!bleState.connected}
            >
                {anim}
            </button>
        {/each}
    </div>
</div>
