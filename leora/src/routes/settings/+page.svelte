<script lang="ts">
    import ShufflePanel from "$lib/components/ShufflePanel.svelte";
    import BreathingControl from "$lib/components/BreathingControl.svelte";
    import AppearanceSettings from "$lib/components/AppearanceSettings.svelte";
    import DisplaySettings from "$lib/components/DisplaySettings.svelte";
    import GestureManager from "$lib/components/GestureManager.svelte";
    import GestureSettings from "$lib/components/GestureSettings.svelte";
    import PowerSettings from "$lib/components/PowerSettings.svelte";
    import { base } from "$app/paths";
    import MasterBackground from "$lib/components/MasterBackground.svelte";
</script>

<div
    class="min-h-screen bg-black text-white font-sans selection:bg-indigo-500/30 relative"
>
    <!-- Background: Master Grid & Lights -->
    <MasterBackground />

    <!-- Header -->
    <header
        class="border-b border-white/5 px-6 py-4 flex items-center gap-4 sticky top-0 bg-black/20 backdrop-blur-xl z-50"
    >
        <!-- svelte-ignore a11y_consider_explicit_label -->
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

    <main class="p-4 sm:p-6 max-w-2xl mx-auto space-y-6 pb-32 relative z-10">
        <section>
            <ShufflePanel />
        </section>

        <section>
            <BreathingControl />
        </section>

        <section>
            <AppearanceSettings />
        </section>

        <section>
            <DisplaySettings />
        </section>

        <section>
            <GestureManager />
        </section>

        <section>
            <GestureSettings />
        </section>

        <section>
            <PowerSettings />
        </section>

        <!-- About Section -->
        <section>
            <div
                class="bg-gradient-to-br from-zinc-900/60 to-zinc-950/40 border border-white/10 rounded-3xl p-6 backdrop-blur-xl text-zinc-400 text-sm space-y-2 relative overflow-hidden"
            >
                <div
                    class="absolute top-0 right-0 w-32 h-32 bg-white/5 rounded-full blur-3xl -mr-16 -mt-16"
                ></div>
                <div class="mb-4">
                    <h3
                        class="text-zinc-500 text-[10px] font-bold tracking-[0.2em] uppercase"
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
