<script lang="ts">
    import { sendCommand } from "$lib/ble.svelte";

    // Arrow icons for each direction
    const directionIcons: Record<string, string> = {
        nw: "↖",
        n: "↑",
        ne: "↗",
        w: "←",
        center: "●",
        e: "→",
        sw: "↙",
        s: "↓",
        se: "↘",
    };

    const directions = [
        ["nw", "n", "ne"],
        ["w", "center", "e"],
        ["sw", "s", "se"],
    ];
</script>

<div class="grid grid-cols-3 gap-2 w-fit">
    {#each directions as row}
        {#each row as dir}
            <button
                class="w-14 h-14 rounded-xl font-bold text-lg transition-all duration-200 flex items-center justify-center
               {dir === 'center'
                    ? 'bg-gradient-to-br from-cyan-500 to-teal-600 hover:from-cyan-400 hover:to-teal-500 text-white shadow-lg shadow-cyan-500/30 scale-105 active:scale-95'
                    : 'bg-white/5 hover:bg-cyan-500/20 text-zinc-500 hover:text-cyan-400 border border-white/10 hover:border-cyan-500/40 active:scale-95'}"
                onclick={() => sendCommand(dir)}
            >
                {directionIcons[dir]}
            </button>
        {/each}
    {/each}
</div>
