<script lang="ts">
    import ShufflePanel from "$lib/components/ShufflePanel.svelte";
    import AppearanceSettings from "$lib/components/AppearanceSettings.svelte";
    import GestureManager from "$lib/components/GestureManager.svelte";
    import { base } from "$app/paths";
    import FlickeringGrid from "$lib/components/FlickeringGrid.svelte";
</script>

<div
    class="min-h-screen bg-black text-white font-sans selection:bg-indigo-500/30 relative"
>
    <!-- Background: Flickering Grid -->
    <div class="fixed inset-0 z-0 w-full h-full bg-black">
        <FlickeringGrid
            class="z-0 absolute inset-0 size-full [mask-image:radial-gradient(circle_at_center,white_40%,transparent_100%)]"
            squareSize={4}
            gridGap={6}
            color="#6B7280"
            maxOpacity={0.5}
            flickerChance={0.1}
        />
    </div>

    <!-- Header -->
    <header
        class="border-b border-white/5 px-6 py-4 flex items-center gap-4 sticky top-0 bg-black/20 backdrop-blur-xl z-50"
    >
        <a
            href="{base}/"
            class="p-2 -ml-2 rounded-full hover:bg-white/5 text-zinc-400 hover:text-white transition-colors"
        >
            <svg
                xmlns="http://www.w3.org/2000/svg"
                width="24"
                height="24"
                viewBox="0 0 24 24"
                fill="none"
                stroke="currentColor"
                stroke-width="2"
                stroke-linecap="round"
                stroke-linejoin="round"><path d="m15 18-6-6 6-6" /></svg
            >
        </a>
        <h1
            class="text-3xl font-bold bg-gradient-to-br from-white to-zinc-500 bg-clip-text text-transparent"
        >
            Settings
        </h1>
    </header>

    <main class="p-4 sm:p-6 max-w-2xl mx-auto space-y-6 pb-32">
        <section>
            <ShufflePanel />
        </section>

        <section>
            <AppearanceSettings />
        </section>

        <section>
            <GestureManager />
        </section>

        <!-- Placeholder for future settings -->
        <section>
            <div
                class="bg-zinc-900/40 border border-white/5 rounded-3xl p-6 backdrop-blur-md text-zinc-400 text-sm space-y-2"
            >
                <div class="mb-4">
                    <h3
                        class="text-zinc-400 text-xs font-bold tracking-widest uppercase"
                    >
                        About
                    </h3>
                </div>
                <p>Leor Robot Control v1.0.0</p>
                <p>Built with SvelteKit & Web Bluetooth</p>
            </div>
        </section>
    </main>
</div>
