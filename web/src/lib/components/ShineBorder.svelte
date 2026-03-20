<script lang="ts">
    import { cn } from "$lib/utils";
    import type { Snippet } from "svelte";

    interface Props {
        children?: Snippet;
        borderRadius?: number;
        borderWidth?: number;
        duration?: number;
        color?: string | string[];
        className?: string;
    }

    let {
        children,
        borderRadius = 9999, // circle
        borderWidth = 1.5,
        duration = 10,
        color = ["#6366f1", "#a855f7", "#6366f1"], // indigo-500, purple-500
        className = "",
    }: Props = $props();

    const colorStr = typeof color === "string" ? color : color.join(",");
</script>

<div
    style="--border-radius: {borderRadius}px"
    class={cn("relative w-fit h-fit rounded-[--border-radius]", className)}
>
    <div
        style="
            --border-width: {borderWidth}px;
            --duration: {duration}s;
            --mask-linear-gradient: linear-gradient(#fff 0 0) content-box, linear-gradient(#fff 0 0);
            --background-radial-gradient: radial-gradient(transparent,transparent, {colorStr},transparent,transparent);
        "
        class="pointer-events-none absolute inset-0 size-full animate-shine-border [background:var(--background-radial-gradient)] [background-size:300%_300%] [mask-composite:exclude_!important] [mask:var(--mask-linear-gradient)] rounded-[--border-radius] p-[--border-width]"
    ></div>
    {@render children?.()}
</div>

<style>
    @keyframes shine-border {
        0% {
            background-position: 0% 0%;
        }
        50% {
            background-position: 100% 100%;
        }
        100% {
            background-position: 0% 0%;
        }
    }

    .animate-shine-border {
        animation: shine-border var(--duration) infinite linear;
    }
</style>
