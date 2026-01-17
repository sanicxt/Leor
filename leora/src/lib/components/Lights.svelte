<script lang="ts">
    import { cn } from "$lib/utils";

    type Direction = "top" | "bottom";

    interface Props {
        direction?: Direction;
        class?: string;
        colors?: [string, string, string];
    }

    let {
        direction = "bottom",
        class: className = "",
        colors = ["#3b82f6", "#22d3ee", "#facc15"],
    }: Props = $props();

    let gradientDirection = $derived(direction === "top" ? "0deg" : "180deg");
    let positionClass = $derived(
        direction === "top" ? "top-0 h-[400px]" : "bottom-0 h-[400px]",
    );

    // Create a style that spreads colors horizontally across the bottom
    let bgStyle = $derived(`
        background: 
            radial-gradient(circle at 20% 100%, ${colors[0]} 0%, transparent 60%),
            radial-gradient(circle at 50% 100%, ${colors[1]} 0%, transparent 70%),
            radial-gradient(circle at 80% 100%, ${colors[2]} 0%, transparent 60%);
        filter: blur(80px);
        opacity: 0.35;
    `);
</script>

<div
    class={cn(
        "w-full absolute left-0 overflow-visible pointer-events-none",
        positionClass,
        className,
    )}
>
    <div style={bgStyle} class="w-full h-full relative"></div>
</div>
