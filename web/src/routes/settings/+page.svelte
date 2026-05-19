<script lang="ts">
    import { slide } from "svelte/transition";
    import ShufflePanel from "$lib/components/ShufflePanel.svelte";
    import BreathingControl from "$lib/components/BreathingControl.svelte";
    import AppearanceSettings from "$lib/components/AppearanceSettings.svelte";
    import DisplaySettings from "$lib/components/DisplaySettings.svelte";
    import GestureManager from "$lib/components/GestureManager.svelte";
    import GestureSettings from "$lib/components/GestureSettings.svelte";
    import GestureCalibration from "$lib/components/GestureCalibration.svelte";
    import PowerSettings from "$lib/components/PowerSettings.svelte";
    import NotificationSettings from "$lib/components/NotificationSettings.svelte";
    import OtaPanel from "$lib/components/OtaPanel.svelte";
    import { base } from "$app/paths";
    import MasterBackground from "$lib/components/MasterBackground.svelte";

    let active = $state('appearance');

    function toggle(id: string) {
        active = active === id ? '' : id;
    }
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
        <a
            href="{base}/"
            class="p-2 -ml-2 rounded-full hover:bg-white/5 text-zinc-400 hover:text-white transition-colors"
        >
            <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="m15 18-6-6 6-6" /></svg>
        </a>
        <h1 class="text-3xl font-bold bg-gradient-to-br from-white to-zinc-500 bg-clip-text text-transparent">Settings</h1>
    </header>

    <main class="p-4 sm:p-6 max-w-2xl mx-auto pb-32 relative z-10">
        <!-- Appearance -->
        <div class="mb-2">
            <button
                onclick={() => toggle('appearance')}
                class="w-full flex items-center justify-between px-4 py-3 rounded-xl border border-white/10 bg-white/[0.06] hover:bg-white/[0.10] transition-colors group"
            >
                <div class="flex items-center gap-3">
                    <span class="text-xs font-bold text-zinc-300 uppercase tracking-widest transition-colors">Appearance</span>
                    {#if active !== 'appearance'}
                        <span class="text-[10px] font-bold text-zinc-500 uppercase">Shuffle · Eyes · Breathing</span>
                    {/if}
                </div>
                <svg class="w-4 h-4 text-zinc-500 transition-transform duration-200 {active === 'appearance' ? 'rotate-180' : ''}" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                    <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M19 9l-7 7-7-7" />
                </svg>
            </button>
            {#if active === 'appearance'}
                <div class="space-y-4 mt-4" transition:slide={{ duration: 200 }}>
                    <ShufflePanel />
                    <BreathingControl />
                    <AppearanceSettings />
                </div>
            {/if}
        </div>

        <!-- Display -->
        <div class="mb-2">
            <button
                onclick={() => toggle('display')}
                class="w-full flex items-center justify-between px-4 py-3 rounded-xl border border-white/10 bg-white/[0.06] hover:bg-white/[0.10] transition-colors group"
            >
                <div class="flex items-center gap-3">
                    <span class="text-xs font-bold text-zinc-300 uppercase tracking-widest transition-colors">Display</span>
                    {#if active !== 'display'}
                        <span class="text-[10px] font-bold text-zinc-500 uppercase">OLED · Notifications</span>
                    {/if}
                </div>
                <svg class="w-4 h-4 text-zinc-500 transition-transform duration-200 {active === 'display' ? 'rotate-180' : ''}" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                    <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M19 9l-7 7-7-7" />
                </svg>
            </button>
            {#if active === 'display'}
                <div class="space-y-4 mt-4" transition:slide={{ duration: 200 }}>
                    <DisplaySettings />
                    <NotificationSettings />
                </div>
            {/if}
        </div>

        <!-- Gestures -->
        <div class="mb-2">
            <button
                onclick={() => toggle('gestures')}
                class="w-full flex items-center justify-between px-4 py-3 rounded-xl border border-white/10 bg-white/[0.06] hover:bg-white/[0.10] transition-colors group"
            >
                <div class="flex items-center gap-3">
                    <span class="text-xs font-bold text-zinc-300 uppercase tracking-widest transition-colors">Gestures</span>
                    {#if active !== 'gestures'}
                        <span class="text-[10px] font-bold text-zinc-500 uppercase">Detection · Mapping · Calibration</span>
                    {/if}
                </div>
                <svg class="w-4 h-4 text-zinc-500 transition-transform duration-200 {active === 'gestures' ? 'rotate-180' : ''}" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                    <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M19 9l-7 7-7-7" />
                </svg>
            </button>
            {#if active === 'gestures'}
                <div class="space-y-4 mt-4" transition:slide={{ duration: 200 }}>
                    <GestureManager />
                    <GestureSettings />
                    <GestureCalibration />
                </div>
            {/if}
        </div>

        <!-- System -->
        <div class="mb-2">
            <button
                onclick={() => toggle('system')}
                class="w-full flex items-center justify-between px-4 py-3 rounded-xl border border-white/10 bg-white/[0.06] hover:bg-white/[0.10] transition-colors group"
            >
                <div class="flex items-center gap-3">
                    <span class="text-xs font-bold text-zinc-300 uppercase tracking-widest transition-colors">System</span>
                    {#if active !== 'system'}
                        <span class="text-[10px] font-bold text-zinc-500 uppercase">Power · Firmware</span>
                    {/if}
                </div>
                <svg class="w-4 h-4 text-zinc-500 transition-transform duration-200 {active === 'system' ? 'rotate-180' : ''}" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                    <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M19 9l-7 7-7-7" />
                </svg>
            </button>
            {#if active === 'system'}
                <div class="space-y-4 mt-4" transition:slide={{ duration: 200 }}>
                    <PowerSettings />
                    <OtaPanel />
                </div>
            {/if}
        </div>

        <!-- About -->
        <div class="bg-gradient-to-br from-zinc-900/60 to-zinc-950/40 border border-white/10 rounded-3xl p-6 backdrop-blur-xl text-zinc-400 text-sm space-y-2 relative overflow-hidden">
            <div class="absolute top-0 right-0 w-32 h-32 bg-white/5 rounded-full blur-3xl -mr-16 -mt-16"></div>
            <div class="mb-4">
                <h3 class="text-zinc-500 text-[10px] font-bold tracking-[0.2em] uppercase">About</h3>
            </div>
            <p>Leor Robot Control v1.0.0</p>
            <p>Built with SvelteKit & Web Bluetooth</p>
        </div>
    </main>
</div>
