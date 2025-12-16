<script lang="ts">
    import { Motion } from "svelte-motion";
    import { cva, type VariantProps } from "class-variance-authority";
    import { cn } from "$lib/utils";

    const dockVariants = cva(
        "mx-auto w-max mt-8 h-[58px] p-2 flex gap-2 rounded-2xl border border-white/10 bg-neutral-900/60 backdrop-blur-md shadow-lg",
    );

    type DockVariants = VariantProps<typeof dockVariants>;

    interface DockProps extends DockVariants {
        className?: string;
        magnification?: number;
        distance?: number;
        direction?: "top" | "middle" | "bottom";
    }

    let className: DockProps["className"] = undefined;
    export { className as class };
    export let magnification: DockProps["magnification"] = 60;
    export let distance: DockProps["distance"] = 140;
    export let direction: DockProps["direction"] = "middle";

    let dockElement: HTMLDivElement;
    let mouseX = Infinity;
    function handleMouseMove(e: MouseEvent) {
        mouseX = e.pageX;
    }

    function handleMouseLeave() {
        mouseX = Infinity;
    }

    $: dockClass = cn(dockVariants({ className }), {
        "items-start": direction === "top",
        "items-center": direction === "middle",
        "items-end": direction === "bottom",
    });
</script>

<Motion let:motion>
    <!-- svelte-ignore a11y-no-static-element-interactions -->
    <div
        use:motion
        bind:this={dockElement}
        on:mousemove={(e) => handleMouseMove(e)}
        on:mouseleave={handleMouseLeave}
        class={dockClass}
        role="toolbar"
        tabindex="0"
    >
        <slot {mouseX} {magnification} {distance}>
            <!-- Your Content -->
            Default
        </slot>
    </div>
</Motion>
