<script lang="ts">
    import { sendCommand } from "$lib/ble.svelte";

    const mouthTypes = ["smile", "frown", "open", "ooo", "flat"];
    const mouthAnims = ["talk", "chew", "wobble"];
</script>

<div class="space-y-4">
    <div class="flex flex-wrap gap-3">
        {#each mouthTypes as mouth}
            <button
                class="px-4 py-2 bg-zinc-900/50 hover:bg-zinc-800 text-zinc-300 hover:text-white rounded-xl
               border border-white/5 hover:border-purple-500/50 transition-all duration-300 text-sm capitalize
               shadow-sm hover:shadow-purple-500/20 hover:-translate-y-0.5"
                onclick={() => sendCommand(mouth)}
            >
                {mouth}
            </button>
        {/each}
    </div>
    <div class="flex flex-wrap gap-3">
        {#each mouthAnims as anim}
            <button
                class="px-4 py-2 bg-purple-500/10 hover:bg-purple-500/20 text-purple-200 hover:text-white rounded-xl
               border border-purple-500/20 hover:border-purple-500/50 transition-all duration-300 text-sm capitalize font-medium
               shadow-sm hover:shadow-purple-500/20 hover:-translate-y-0.5"
                onclick={() => sendCommand(anim)}
            >
                {anim}
            </button>
        {/each}
    </div>
</div>
