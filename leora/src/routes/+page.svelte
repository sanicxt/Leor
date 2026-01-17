<script lang="ts">
    import { fly, fade } from "svelte/transition";
    import MasterBackground from "$lib/components/MasterBackground.svelte";
    import AnimatedTabs from "$lib/components/AnimatedTabs.svelte";
    import ShimmerButton from "$lib/components/ShimmerButton.svelte";
    import { cn } from "$lib/utils";

    // Icons
    import {
        Home,
        Settings,
        TvMinimalPlay,
        PencilLine,
        Wifi,
        WifiOff,
    } from "lucide-svelte";

    // Components
    import ExpressionGrid from "$lib/components/ExpressionGrid.svelte";
    import MouthControls from "$lib/components/MouthControls.svelte";
    import ActionButtons from "$lib/components/ActionButtons.svelte";
    import LookPad from "$lib/components/LookPad.svelte";
    import ShufflePanel from "$lib/components/ShufflePanel.svelte";
    import AppearanceSettings from "$lib/components/AppearanceSettings.svelte";
    import GestureManager from "$lib/components/GestureManager.svelte";
    import GestureSettings from "$lib/components/GestureSettings.svelte";

    import {
        getConnected,
        getLastStatus,
        getLastGesture,
        connect,
        disconnect,
        isWebBluetoothSupported,
        bleState,
    } from "$lib/ble.svelte";

    let activeTab = "home"; // 'home' | 'settings' | 'gestures'

    async function handleConnect() {
        if (getConnected()) {
            await disconnect();
        } else {
            await connect();
        }
    }
</script>

<svelte:head>
    <title>Leor OS</title>
</svelte:head>

<div
    class="min-h-screen bg-black text-white font-sans selection:bg-pink-500/30 overflow-hidden relative"
>
    <!-- Background: Master Grid & Lights -->
    <MasterBackground />

    <!-- Content Area -->
    <main class="relative z-10 h-screen overflow-y-auto pb-32 scrollbar-hide">
        <div class="p-6 max-w-6xl mx-auto min-h-full">
            <!-- Header Status -->
            <header
                class="flex justify-between items-center mb-8 bg-black/20 backdrop-blur-md p-4 rounded-2xl border border-white/5"
            >
                <div class="flex items-center gap-3">
                    <h1
                        class="text-3xl font-bold bg-gradient-to-br from-white to-zinc-500 bg-clip-text text-transparent"
                    >
                        Leor OS
                    </h1>
                    {#if getConnected()}
                        <span
                            class="px-2 py-0.5 rounded-full bg-emerald-500/20 text-emerald-400 text-xs font-mono border border-emerald-500/20"
                            >ONLINE</span
                        >
                    {:else}
                        <span
                            class="px-2 py-0.5 rounded-full bg-rose-500/20 text-rose-400 text-xs font-mono border border-rose-500/20"
                            >OFFLINE</span
                        >
                    {/if}
                </div>

                <!-- Live Status Ticker & Connection -->
                <div class="flex items-center gap-6">
                    {#if getConnected()}
                        <div
                            class="flex items-center gap-4 text-xs font-mono opacity-70 hidden sm:flex"
                        >
                            {#if getLastStatus()}
                                <span
                                    >STATUS: {getLastStatus().split(
                                        "\n",
                                    )[0]}</span
                                >
                            {/if}
                            {#if getLastGesture()}
                                <span class="text-pink-400"
                                    >GESTURE: {getLastGesture()}</span
                                >
                            {/if}
                        </div>
                    {/if}

                    <ShimmerButton
                        class="shadow-2xl {getConnected()
                            ? 'shadow-emerald-500/20'
                            : 'shadow-rose-500/20'}"
                        shimmerColor={getConnected() ? "#34d399" : "#fb7185"}
                        background="rgba(0, 0, 0, 1)"
                        onclick={handleConnect}
                    >
                        <div class="flex items-center gap-2">
                            {#if getConnected()}
                                <TvMinimalPlay
                                    class="w-5 h-5 text-emerald-400 animate-pulse"
                                />
                                <span
                                    class="whitespace-pre-wrap text-center text-sm font-medium leading-none tracking-tight text-emerald-100 lg:text-lg"
                                >
                                    Connected
                                </span>
                            {:else}
                                <TvMinimalPlay class="w-5 h-5 text-rose-400" />
                                <span
                                    class="whitespace-pre-wrap text-center text-sm font-medium leading-none tracking-tight text-rose-100 lg:text-lg"
                                >
                                    Connect
                                </span>
                            {/if}
                        </div>
                    </ShimmerButton>
                </div>
            </header>

            <!-- Views -->
            {#if activeTab === "home"}
                <div
                    in:fly={{ y: 20, duration: 300 }}
                    class="grid grid-cols-1 lg:grid-cols-12 gap-6"
                >
                    <!-- Center Stage: Expressions -->
                    <div class="lg:col-span-12">
                        <div
                            class="bg-zinc-900/40 border border-white/5 rounded-3xl p-6 backdrop-blur-md"
                        >
                            <h2
                                class="text-zinc-400 text-xs font-bold tracking-widest uppercase mb-4"
                            >
                                Expressions
                            </h2>
                            <ExpressionGrid />
                        </div>
                    </div>

                    <!-- Controls -->
                    <div class="lg:col-span-8 space-y-6">
                        <div
                            class="bg-zinc-900/40 border border-white/5 rounded-3xl p-6 backdrop-blur-md"
                        >
                            <h2
                                class="text-zinc-400 text-xs font-bold tracking-widest uppercase mb-4"
                            >
                                Keyboard
                            </h2>
                            <ActionButtons />
                        </div>
                        <div
                            class="bg-zinc-900/40 border border-white/5 rounded-3xl p-6 backdrop-blur-md"
                        >
                            <h2
                                class="text-zinc-400 text-xs font-bold tracking-widest uppercase mb-4"
                            >
                                Mouth
                            </h2>
                            <MouthControls />
                        </div>
                    </div>

                    <!-- Look Pad -->
                    <div class="lg:col-span-4">
                        <div
                            class="bg-zinc-900/40 border border-white/5 rounded-3xl p-6 backdrop-blur-md h-full flex flex-col justify-center items-center"
                        >
                            <h2
                                class="text-zinc-400 text-xs font-bold tracking-widest uppercase mb-8"
                            >
                                Gaze Control
                            </h2>
                            <LookPad />
                        </div>
                    </div>
                </div>
            {:else if activeTab === "settings"}
                <div
                    in:fly={{ y: 20, duration: 300 }}
                    class="max-w-6xl mx-auto space-y-6"
                >
                    <!-- Mobile: Stack vertically. Desktop: 2-column grid -->
                    <div class="grid grid-cols-1 lg:grid-cols-2 gap-6">
                        <!-- Left: Shuffle Mode -->
                        <ShufflePanel />

                        <!-- Right: Gesture Tuning -->
                        <GestureSettings />
                    </div>

                    <!-- Full width: Appearance -->
                    <AppearanceSettings />
                </div>
            {:else if activeTab === "gestures"}
                <div
                    in:fly={{ y: 20, duration: 300 }}
                    class="max-w-4xl mx-auto"
                >
                    <GestureManager />
                </div>
            {/if}

            <!-- Bottom Spacer for Tabs -->
            <div class="h-32"></div>
        </div>
    </main>

    <!-- Navigation Tabs -->
    <div
        class="fixed bottom-8 left-1/2 -translate-x-1/2 z-50 pointer-events-none"
    >
        <div class="pointer-events-auto">
            <AnimatedTabs
                bind:activeTab
                tabs={[
                    { id: "home", title: "Home" },
                    { id: "settings", title: "Settings" },
                    { id: "gestures", title: "Gestures" },
                ]}
            />
        </div>
    </div>
</div>
