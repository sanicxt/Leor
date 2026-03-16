<script lang="ts">
    import { Motion, AnimateSharedLayout } from "svelte-motion";

    export let tabs: { id: string; title: string }[] = [];
    export let activeTab: string = "";
</script>

<div
    class="relative flex flex-nowrap items-center justify-center bg-paper p-2 rounded-full border-4 border-ink shadow-[4px_4px_0px_0px_var(--color-ink)]"
>
    <AnimateSharedLayout>
        {#each tabs as item}
            {@const isActive = activeTab === item.id}
            <button
                class="group relative z-[1] rounded-full px-5 py-2.5 transition-colors duration-300 font-bold uppercase tracking-wider text-xs {isActive
                    ? 'z-0'
                    : ''}"
                on:click={() => (activeTab = item.id)}
            >
                {#if isActive}
                    <Motion
                        layoutId="clicked-btn"
                        transition={{
                            duration: 0.25,
                            type: "spring",
                            stiffness: 300,
                            damping: 25,
                        }}
                        let:motion
                    >
                        <div
                            use:motion
                            class="absolute inset-0 rounded-full bg-bento-peach border-2 border-ink shadow-[2px_2px_0px_0px_var(--color-ink)]"
                        ></div>
                    </Motion>
                {/if}
                <span
                    class="relative block duration-300 z-10 {isActive
                        ? 'text-ink'
                        : 'text-ink/60 group-hover:text-ink'}"
                >
                    {item.title}
                </span>
            </button>
        {/each}
    </AnimateSharedLayout>
</div>
