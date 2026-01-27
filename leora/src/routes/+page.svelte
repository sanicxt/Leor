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
    import DisplaySettings from "$lib/components/DisplaySettings.svelte";
    import GestureManager from "$lib/components/GestureManager.svelte";
    import GestureSettings from "$lib/components/GestureSettings.svelte";
    import PowerSettings from "$lib/components/PowerSettings.svelte";
    import BreathingControl from "$lib/components/BreathingControl.svelte";

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
            <!-- Premium Header -->
            <header class="mb-8">
                <div
                    class="relative bg-gradient-to-br from-zinc-900/80 to-zinc-950/80 backdrop-blur-xl p-5 rounded-2xl border border-white/10 shadow-2xl overflow-hidden"
                >
                    <!-- Decorative gradient -->
                    <div
                        class="absolute top-0 right-0 w-64 h-64 bg-gradient-to-bl from-orange-500/10 to-transparent rounded-full blur-3xl"
                    ></div>
                    <div
                        class="absolute bottom-0 left-0 w-48 h-48 bg-gradient-to-tr from-cyan-500/10 to-transparent rounded-full blur-3xl"
                    ></div>

                    <div
                        class="relative flex justify-between items-center gap-4 flex-wrap"
                    >
                        <!-- Logo & Status -->
                        <div class="flex items-center gap-4">
                            <div
                                class="w-12 h-12 rounded-2xl bg-gradient-to-br from-zinc-800 to-zinc-900 flex items-center justify-center shadow-lg shadow-orange-500/20 border border-orange-500/20"
                            >
                                <svg
                                    class="w-10 h-10"
                                    viewBox="0 0 200 200"
                                    fill="none"
                                >
                                    <rect
                                        x="40"
                                        y="50"
                                        width="120"
                                        height="100"
                                        rx="30"
                                        stroke="#FF8C00"
                                        stroke-width="8"
                                        fill="white"
                                    />
                                    <path
                                        d="M40 80C40 63.4315 53.4315 50 70 50H130C146.569 50 160 63.4315 160 80V85H40V80Z"
                                        fill="#FF8C00"
                                    />
                                    <path
                                        d="M35 85H165V95C165 97.2091 163.209 99 161 99H39C36.7909 99 35 97.2091 35 95V85Z"
                                        fill="#FF8C00"
                                    />
                                    <rect
                                        x="55"
                                        y="105"
                                        width="90"
                                        height="60"
                                        rx="15"
                                        fill="#333333"
                                    />
                                    <path
                                        d="M75 125C75 122 78 120 81 120C84 120 87 122 87 125"
                                        stroke="#00F0FF"
                                        stroke-width="4"
                                        stroke-linecap="round"
                                    />
                                    <path
                                        d="M113 125C113 122 116 120 119 120C122 120 125 122 125 125"
                                        stroke="#00F0FF"
                                        stroke-width="4"
                                        stroke-linecap="round"
                                    />
                                    <path
                                        d="M85 145C85 145 90 152 100 152C110 152 115 145 115 145"
                                        stroke="#00F0FF"
                                        stroke-width="4"
                                        stroke-linecap="round"
                                    />
                                    <rect
                                        x="25"
                                        y="90"
                                        width="15"
                                        height="40"
                                        rx="7.5"
                                        fill="#FF8C00"
                                    />
                                    <rect
                                        x="160"
                                        y="90"
                                        width="15"
                                        height="40"
                                        rx="7.5"
                                        fill="#FF8C00"
                                    />
                                </svg>
                            </div>
                            <div>
                                <h1
                                    class="text-2xl font-bold bg-gradient-to-r from-white via-zinc-200 to-zinc-400 bg-clip-text text-transparent"
                                >
                                    Leor OS
                                </h1>
                                <div class="flex items-center gap-2 mt-0.5">
                                    {#if getConnected()}
                                        <span class="flex items-center gap-1.5">
                                            <span
                                                class="w-2 h-2 bg-emerald-400 rounded-full animate-pulse shadow-lg shadow-emerald-500/50"
                                            ></span>
                                            <span
                                                class="text-emerald-400 text-xs font-medium"
                                                >Connected</span
                                            >
                                        </span>
                                    {:else}
                                        <span class="flex items-center gap-1.5">
                                            <span
                                                class="w-2 h-2 bg-rose-400 rounded-full"
                                            ></span>
                                            <span
                                                class="text-rose-400 text-xs font-medium"
                                                >Disconnected</span
                                            >
                                        </span>
                                    {/if}
                                </div>
                            </div>
                        </div>

                        <!-- Live Status & Connect -->
                        <div class="flex items-center gap-4">
                            {#if getConnected()}
                                <div
                                    class="hidden sm:flex flex-col items-end gap-1"
                                >
                                    {#if getLastStatus()}
                                        <span
                                            class="text-zinc-500 text-[10px] font-mono"
                                        >
                                            {getLastStatus().split("\n")[0]}
                                        </span>
                                    {/if}
                                    {#if getLastGesture()}
                                        <span
                                            class="text-cyan-400 text-xs font-mono flex items-center gap-1"
                                        >
                                            <svg
                                                class="w-3 h-3"
                                                fill="none"
                                                stroke="currentColor"
                                                viewBox="0 0 24 24"
                                            >
                                                <path
                                                    stroke-linecap="round"
                                                    stroke-linejoin="round"
                                                    stroke-width="2"
                                                    d="M7 11.5V14m0-2.5v-6a1.5 1.5 0 113 0m-3 6a1.5 1.5 0 00-3 0v2a7.5 7.5 0 0015 0v-5a1.5 1.5 0 00-3 0m-6-3V11m0-5.5v-1a1.5 1.5 0 013 0v1m0 0V11m0-5.5a1.5 1.5 0 013 0v3m0 0V11"
                                                />
                                            </svg>
                                            {getLastGesture()}
                                        </span>
                                    {/if}
                                </div>
                            {/if}

                            <button
                                onclick={handleConnect}
                                class="group relative px-5 py-2.5 rounded-xl font-medium transition-all duration-300 {getConnected()
                                    ? 'bg-emerald-500/20 hover:bg-emerald-500/30 border border-emerald-500/30 text-emerald-400'
                                    : 'bg-gradient-to-r from-orange-500 to-amber-500 hover:from-orange-600 hover:to-amber-600 text-white shadow-lg shadow-orange-500/30'}"
                            >
                                <div class="flex items-center gap-2">
                                    {#if getConnected()}
                                        <TvMinimalPlay
                                            class="w-4 h-4 animate-pulse"
                                        />
                                        <span class="text-sm">Connected</span>
                                    {:else}
                                        <TvMinimalPlay class="w-4 h-4" />
                                        <span class="text-sm">Connect</span>
                                    {/if}
                                </div>
                            </button>
                        </div>
                    </div>
                </div>
            </header>

            <!-- Views -->
            {#if activeTab === "home"}
                <div
                    in:fly={{ y: 20, duration: 300 }}
                    class="grid grid-cols-1 lg:grid-cols-12 gap-5"
                >
                    <!-- Expressions Card -->
                    <div class="lg:col-span-12">
                        <div
                            class="bg-gradient-to-br from-indigo-950/40 to-purple-950/30 border border-indigo-500/20 rounded-2xl p-5 backdrop-blur-lg"
                        >
                            <div class="flex items-center gap-3 mb-5">
                                <div
                                    class="w-8 h-8 rounded-lg bg-gradient-to-br from-indigo-500 to-purple-600 flex items-center justify-center"
                                >
                                    <svg
                                        class="w-4 h-4 text-white"
                                        fill="none"
                                        stroke="currentColor"
                                        viewBox="0 0 24 24"
                                    >
                                        <path
                                            stroke-linecap="round"
                                            stroke-linejoin="round"
                                            stroke-width="2"
                                            d="M14.828 14.828a4 4 0 01-5.656 0M9 10h.01M15 10h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z"
                                        />
                                    </svg>
                                </div>
                                <div>
                                    <h2
                                        class="text-white text-sm font-semibold"
                                    >
                                        Expressions
                                    </h2>
                                    <p class="text-indigo-300/60 text-xs">
                                        Tap to trigger
                                    </p>
                                </div>
                            </div>
                            <ExpressionGrid />
                        </div>
                    </div>

                    <!-- Controls Column -->
                    <div class="lg:col-span-8 space-y-5">
                        <!-- Keyboard/Actions -->
                        <div
                            class="bg-gradient-to-br from-pink-950/40 to-rose-950/30 border border-pink-500/20 rounded-2xl p-5 backdrop-blur-lg"
                        >
                            <div class="flex items-center gap-3 mb-5">
                                <div
                                    class="w-8 h-8 rounded-lg bg-gradient-to-br from-pink-500 to-rose-600 flex items-center justify-center"
                                >
                                    <svg
                                        class="w-4 h-4 text-white"
                                        fill="none"
                                        stroke="currentColor"
                                        viewBox="0 0 24 24"
                                    >
                                        <path
                                            stroke-linecap="round"
                                            stroke-linejoin="round"
                                            stroke-width="2"
                                            d="M15 15l-2 5L9 9l11 4-5 2zm0 0l5 5M7.188 2.239l.777 2.897M5.136 7.965l-2.898-.777M13.95 4.05l-2.122 2.122m-5.657 5.656l-2.12 2.122"
                                        />
                                    </svg>
                                </div>
                                <div>
                                    <h2
                                        class="text-white text-sm font-semibold"
                                    >
                                        Quick Actions
                                    </h2>
                                    <p class="text-pink-300/60 text-xs">
                                        Keyboard shortcuts
                                    </p>
                                </div>
                            </div>
                            <ActionButtons />
                        </div>

                        <!-- Mouth Controls -->
                        <div
                            class="bg-gradient-to-br from-amber-950/40 to-orange-950/30 border border-amber-500/20 rounded-2xl p-5 backdrop-blur-lg"
                        >
                            <div class="flex items-center gap-3 mb-5">
                                <div
                                    class="w-8 h-8 rounded-lg bg-gradient-to-br from-amber-500 to-orange-600 flex items-center justify-center"
                                >
                                    <svg
                                        class="w-4 h-4 text-white"
                                        fill="none"
                                        stroke="currentColor"
                                        viewBox="0 0 24 24"
                                    >
                                        <path
                                            stroke-linecap="round"
                                            stroke-linejoin="round"
                                            stroke-width="2"
                                            d="M8 12h.01M12 12h.01M16 12h.01M21 12c0 4.418-4.03 8-9 8a9.863 9.863 0 01-4.255-.949L3 20l1.395-3.72C3.512 15.042 3 13.574 3 12c0-4.418 4.03-8 9-8s9 3.582 9 8z"
                                        />
                                    </svg>
                                </div>
                                <div>
                                    <h2
                                        class="text-white text-sm font-semibold"
                                    >
                                        Mouth
                                    </h2>
                                    <p class="text-amber-300/60 text-xs">
                                        Shape & expression
                                    </p>
                                </div>
                            </div>
                            <MouthControls />
                        </div>
                    </div>

                    <!-- Gaze Control Pad -->
                    <div class="lg:col-span-4">
                        <div
                            class="bg-gradient-to-br from-cyan-950/40 to-teal-950/30 border border-cyan-500/20 rounded-2xl p-5 backdrop-blur-lg h-full flex flex-col"
                        >
                            <div class="flex items-center gap-3 mb-5">
                                <div
                                    class="w-8 h-8 rounded-lg bg-gradient-to-br from-cyan-500 to-teal-600 flex items-center justify-center"
                                >
                                    <svg
                                        class="w-4 h-4 text-white"
                                        fill="none"
                                        stroke="currentColor"
                                        viewBox="0 0 24 24"
                                    >
                                        <path
                                            stroke-linecap="round"
                                            stroke-linejoin="round"
                                            stroke-width="2"
                                            d="M15 12a3 3 0 11-6 0 3 3 0 016 0z"
                                        />
                                        <path
                                            stroke-linecap="round"
                                            stroke-linejoin="round"
                                            stroke-width="2"
                                            d="M2.458 12C3.732 7.943 7.523 5 12 5c4.478 0 8.268 2.943 9.542 7-1.274 4.057-5.064 7-9.542 7-4.477 0-8.268-2.943-9.542-7z"
                                        />
                                    </svg>
                                </div>
                                <div>
                                    <h2
                                        class="text-white text-sm font-semibold"
                                    >
                                        Gaze Control
                                    </h2>
                                    <p class="text-cyan-300/60 text-xs">
                                        Drag to look
                                    </p>
                                </div>
                            </div>
                            <div
                                class="flex-1 flex items-center justify-center"
                            >
                                <LookPad />
                            </div>
                        </div>
                    </div>
                </div>
            {:else if activeTab === "settings"}
                <div
                    in:fly={{ y: 20, duration: 300 }}
                    class="max-w-6xl mx-auto space-y-5"
                >
                    <!-- Top row: Shuffle & Gesture Tuning -->
                    <div class="grid grid-cols-1 lg:grid-cols-2 gap-5">
                        <ShufflePanel />
                        <GestureSettings />
                    </div>

                    <!-- Full width: Appearance -->
                    <AppearanceSettings />

                    <!-- Display Settings -->
                    <DisplaySettings />

                    <BreathingControl />

                    <!-- Power Management -->
                    <PowerSettings />
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
