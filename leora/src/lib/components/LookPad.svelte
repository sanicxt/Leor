<script lang="ts">
    import { sendCommand, bleState } from "$lib/ble.svelte";

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
                class="bento-button w-14 h-14 text-xl flex items-center justify-center {dir === 'center' ? 'bg-ink text-paper scale-105 hover:bg-paper hover:text-ink' : 'bg-paper text-ink hover:bg-ink hover:text-paper'} disabled:opacity-50 disabled:cursor-not-allowed disabled:hover:bg-paper disabled:hover:text-ink"
                onclick={() => sendCommand(dir)}
                disabled={!bleState.connected}
            >
                {directionIcons[dir]}
            </button>
        {/each}
    {/each}
</div>
