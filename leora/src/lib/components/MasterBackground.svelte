<script lang="ts">
    import InteractiveGrid from "./InteractiveGrid.svelte";
    import Lights from "./Lights.svelte";
    import { cn } from "$lib/utils";
    import { bleState } from "$lib/ble.svelte";
    import { fade } from "svelte/transition";

    interface Props {
        class?: string;
    }

    let { class: className = "" }: Props = $props();

    // Palettes matching "Blueish Green" and "Purple/Red"
    // Connected: Deep Blue -> Teal -> Emerald
    // Disconnected: Purple -> Rose -> Red
    let colors = $derived(
        (bleState.connected
            ? ["#2563eb", "#0d9488", "#10b981"] // Blueish Green palette
            : ["#7c3aed", "#e11d48", "#dc2626"]) as [string, string, string], // Purple/Red palette
    );
</script>

<div
    class={cn(
        "fixed inset-0 z-0 w-full h-full bg-black overflow-hidden",
        className,
    )}
>
    <!-- SVG Grid Layer -->
    <div class="absolute inset-0 z-0 bg-grid-white/[0.1]"></div>

    <!-- Lights Layer (Ambience) with Fade Animation -->
    <div class="absolute bottom-0 left-0 w-full h-full z-10">
        {#key bleState.connected}
            <div
                in:fade={{ duration: 800 }}
                out:fade={{ duration: 800 }}
                class="absolute inset-0"
            >
                <Lights direction="bottom" {colors} />
            </div>
        {/key}
    </div>

    <!-- Interactive Grid Layer (Boxes) -->
    <div class="absolute inset-0 z-20">
        <InteractiveGrid rows={25} cols={25} class="opacity-40" />
    </div>

    <!-- Radial Gradient Mask to focus content -->
    <div
        class="absolute inset-0 z-30 pointer-events-none [background:radial-gradient(circle_at_center,transparent_0%,black_90%)]"
    ></div>
</div>
