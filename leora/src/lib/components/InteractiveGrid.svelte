<script lang="ts">
    import { cn } from "$lib/utils";
    import { Motion } from "svelte-motion";

    interface Props {
        class?: string;
        rows?: number;
        cols?: number;
    }

    let { class: className = "", rows: r = 20, cols: c = 20 }: Props = $props();

    let rows = new Array(r).fill(1).map((_, i) => i);
    let cols = new Array(c).fill(1).map((_, i) => i);
</script>

<div
    class={cn(
        "relative z-0 flex w-full h-full justify-center overflow-hidden",
        className,
    )}
>
    <div class="flex">
        {#each rows as i}
            <div class="flex flex-col">
                {#each cols as j}
                    <Motion
                        whileHover={{
                            backgroundColor: "rgba(255, 255, 255, 0.05)",
                            transition: { duration: 0 },
                        }}
                        animate={{
                            transition: { duration: 2 },
                        }}
                        let:motion
                    >
                        <div
                            use:motion
                            class="md:w-12 md:h-12 w-9 h-9 border-r border-t last:border-b last:border-l border-neutral-900/50 relative"
                        ></div>
                    </Motion>
                {/each}
            </div>
        {/each}
    </div>
</div>
