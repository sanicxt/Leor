<script lang="ts">
    import { sendCommand } from "$lib/ble.svelte";

    const directions = [
        ["nw", "n", "ne"],
        ["w", "center", "e"],
        ["sw", "s", "se"],
    ];
</script>

<div class="grid grid-cols-3 gap-1 w-fit">
    {#each directions as row}
        {#each row as dir}
            <button
                class="w-14 h-14 rounded-2xl font-bold text-xs transition-all duration-300 flex items-center justify-center
               {dir === 'center'
                    ? 'bg-amber-500 hover:bg-amber-400 text-white shadow-lg shadow-amber-500/20 scale-105 active:scale-95'
                    : 'bg-zinc-800/50 hover:bg-zinc-700 text-zinc-400 hover:text-white border border-white/5 hover:border-amber-500/30'}"
                onclick={() => sendCommand(dir)}
            >
                {dir === "center" ? "●" : dir.toUpperCase()}
            </button>
        {/each}
    {/each}
</div>
