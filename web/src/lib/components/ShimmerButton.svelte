<script lang="ts">
    import { cn } from "$lib/utils";
    import type { Snippet } from "svelte";

    interface Props {
        shimmerColor?: string;
        shimmerSize?: string;
        shimmerDuration?: string;
        borderRadius?: string;
        background?: string;
        class?: string;
        onclick?: (e: MouseEvent) => void;
        disabled?: boolean;
        children?: Snippet;
    }

    let {
        shimmerColor = "#ffffff",
        shimmerSize = "0.05em",
        shimmerDuration = "3s",
        borderRadius = "100px",
        background = "rgba(0,0,0,1)",
        class: className = "",
        onclick,
        disabled = false,
        children,
    }: Props = $props();
</script>

<button
    style:--spread="90deg"
    style:--shimmer-color={shimmerColor}
    style:--radius={borderRadius}
    style:--speed={shimmerDuration}
    style:--cut={shimmerSize}
    style:--bg={background}
    class={cn(
        "group relative z-0 flex cursor-pointer items-center justify-center overflow-hidden whitespace-nowrap border border-white/10 px-6 py-3 text-white [border-radius:var(--radius)]",
        "transform-gpu transition-transform duration-300 ease-in-out active:translate-y-[1px] disabled:opacity-50 disabled:cursor-not-allowed",
        className,
    )}
    {onclick}
    {disabled}
>
    <!-- Shimmer Container (behind backdrop) -->
    <div
        class="absolute inset-0 overflow-visible [container-type:size] -z-30 blur-[2px]"
    >
        <div
            class="absolute inset-0 h-[100cqh] animate-shimmer-slide [aspect-ratio:1] [border-radius:0] [mask:none]"
        >
            <div
                class="animate-shimmer-spin absolute inset-[-100%] w-auto rotate-0 [background:conic-gradient(from_calc(270deg-(var(--spread)*0.5)),transparent_0,var(--shimmer-color)_var(--spread),transparent_var(--spread))]"
            />
        </div>
    </div>

    <div class="relative z-10 flex items-center justify-center">
        {#if children}
            {@render children()}
        {:else}
            Shimmer Button
        {/if}
    </div>

    <!-- Inner Shadow/Highlight (on top) -->
    <div
        class="absolute inset-0 h-full w-full rounded-[--radius] shadow-[inset_0_-8px_10px_#ffffff1f] transform-gpu transition-all duration-300 ease-in-out group-hover:shadow-[inset_0_-6px_10px_#ffffff3f] group-active:shadow-[inset_0_-10px_10px_#ffffff3f] z-20 pointer-events-none"
    ></div>

    <!-- Backdrop (masks the center, reveals shimmer in "cut" area) -->
    <div
        class="absolute -z-20 [background:var(--bg)] [border-radius:var(--radius)] [inset:var(--cut)]"
    ></div>
</button>

<style>
    @keyframes shimmer-spin {
        0% {
            transform: translateZ(0) rotate(0);
        }
        15%,
        35% {
            transform: translateZ(0) rotate(90deg);
        }
        65%,
        85% {
            transform: translateZ(0) rotate(270deg);
        }
        100% {
            transform: translateZ(0) rotate(360deg);
        }
    }

    @keyframes shimmer-slide {
        to {
            transform: translate(calc(100cqw - 100%), 0);
        }
    }

    .animate-shimmer-spin {
        animation: shimmer-spin calc(var(--speed) * 2) infinite linear;
    }

    .animate-shimmer-slide {
        animation: shimmer-slide var(--speed) ease-in-out infinite alternate;
    }
</style>
