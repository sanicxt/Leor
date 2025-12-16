<script lang="ts">
    import { Motion, AnimateSharedLayout } from "svelte-motion";

    export let tabs: { id: string; title: string }[] = [];
    export let activeTab: string = "";
</script>

<div
    class="relative flex flex-nowrap items-center justify-center bg-zinc-900/40 p-1.5 rounded-full border border-white/5 backdrop-blur-md shadow-2xl"
>
    <AnimateSharedLayout>
        {#each tabs as item}
            {@const isActive = activeTab === item.id}
            <button
                class="group relative z-[1] rounded-full px-4 py-2 transition-colors duration-300 {isActive
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
                            class="absolute inset-0 rounded-full bg-white shadow-lg shadow-white/10"
                        ></div>
                    </Motion>
                {/if}
                <span
                    class="relative text-sm block font-medium duration-300 z-10 {isActive
                        ? 'text-black'
                        : 'text-zinc-400 group-hover:text-zinc-200'}"
                >
                    {item.title}
                </span>
            </button>
        {/each}
    </AnimateSharedLayout>
</div>
