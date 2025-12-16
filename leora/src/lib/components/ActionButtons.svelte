<script lang="ts">
    import { sendCommand } from "$lib/ble.svelte";

    const actions = [
        { cmd: "blink", label: "Blink" },
        { cmd: "wink", label: "Wink L" },
        { cmd: "winkr", label: "Wink R" },
        { cmd: "laugh", label: "Laugh" },
        { cmd: "cry", label: "Cry" },
    ];
</script>

<div class="flex flex-wrap gap-2">
    {#each actions as action}
        <button
            class="px-4 py-2 bg-emerald-500/10 hover:bg-emerald-500/20 text-emerald-200 hover:text-white rounded-xl
             border border-emerald-500/20 hover:border-emerald-500/50 transition-all duration-300 text-sm font-medium
             shadow-sm hover:shadow-emerald-500/20 hover:-translate-y-0.5"
            onclick={() => sendCommand(action.cmd)}
        >
            {action.label}
        </button>
    {/each}
</div>
