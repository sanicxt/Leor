<script lang="ts">
    import { cn } from "$lib/utils";
    import {
        Motion,
        useMotionValue,
        useSpring,
        useTransform,
    } from "svelte-motion";

    export let magnification = 60;
    export let distance = 140;
    export let mouseX: number = Infinity;
    export let className: string | undefined = undefined;

    // Bridge the primitive number prop to a MotionValue
    let mouseXMotion = useMotionValue(mouseX);
    $: mouseXMotion.set(mouseX);

    let ref: HTMLDivElement;

    let distanceCalc = useTransform(mouseXMotion, (val: number) => {
        const bounds = ref?.getBoundingClientRect() ?? { x: 0, width: 0 };
        return val - bounds.x - bounds.width / 2;
    });

    let widthSync = useTransform(
        distanceCalc,
        [-distance, 0, distance],
        [40, magnification, 40],
    );

    let width = useSpring(widthSync, {
        mass: 0.1,
        stiffness: 150,
        damping: 12,
    });
</script>

<Motion style={{ width }} let:motion>
    <div
        bind:this={ref}
        use:motion
        class={cn("aspect-square cursor-pointer", className)}
    >
        <div
            class="flex items-center justify-center w-full h-full rounded-full bg-neutral-900/50 border border-white/5 hover:bg-neutral-800/80 hover:border-white/20 transition-all shadow-lg"
        >
            <slot />
        </div>
    </div>
</Motion>
