<script lang="ts">
    import { onMount } from "svelte";
    import {
        sendCommand,
        getConnected,
        bleState,
        getSettingsEw,
        getSettingsEh,
        getSettingsEs,
        getSettingsEr,
        getSettingsMw,
        getSettingsLt,
        getSettingsVt,
        getSettingsBi,
        getSettingsGs,
        getSettingsOs,
        getSettingsSs,
        setSettingsEw,
        setSettingsEh,
        setSettingsEs,
        setSettingsEr,
        setSettingsMw,
        setSettingsLt,
        setSettingsVt,
        setSettingsBi,
        setSettingsGs,
        setSettingsOs,
        setSettingsSs,
    } from "$lib/ble.svelte";

    // Local state for new controls
    let mouthAnimDuration = $state(1000);
    let transitionSpeed = $state(8);

    // Presets
    const presets = [
        {
            name: "Cute",
            icon: "✨",
            ew: 30,
            eh: 35,
            es: 12,
            er: 15,
            mw: 18,
            bi: 4,
            gs: 5,
            os: 12,
            ss: 8,
        },
        {
            name: "Sleepy",
            icon: "😴",
            ew: 40,
            eh: 25,
            es: 8,
            er: 20,
            mw: 15,
            bi: 8,
            gs: 3,
            os: 6,
            ss: 4,
        },
        {
            name: "Energetic",
            icon: "⚡",
            ew: 36,
            eh: 40,
            es: 10,
            er: 8,
            mw: 22,
            bi: 2,
            gs: 12,
            os: 18,
            ss: 14,
        },
        {
            name: "Robotic",
            icon: "🤖",
            ew: 32,
            eh: 32,
            es: 14,
            er: 2,
            mw: 20,
            bi: 5,
            gs: 8,
            os: 15,
            ss: 10,
        },
    ];

    async function applyPreset(preset: (typeof presets)[0]) {
        setSettingsEw(preset.ew);
        setSettingsEh(preset.eh);
        setSettingsEs(preset.es);
        setSettingsEr(preset.er);
        setSettingsMw(preset.mw);
        setSettingsBi(preset.bi);
        setSettingsGs(preset.gs);
        setSettingsOs(preset.os);
        setSettingsSs(preset.ss);
        await applySettings();
    }

    async function applySettings() {
        await sendCommand(`s:ew=${getSettingsEw()},eh=${getSettingsEh()}`);
        await new Promise((r) => setTimeout(r, 50));
        await sendCommand(`s:es=${getSettingsEs()},er=${getSettingsEr()}`);
        await new Promise((r) => setTimeout(r, 50));
        await sendCommand(`s:mw=${getSettingsMw()},bi=${getSettingsBi()}`);
        await new Promise((r) => setTimeout(r, 50));
        await sendCommand(`s:lt=${getSettingsLt()},vt=${getSettingsVt()}`);
        await new Promise((r) => setTimeout(r, 50));
        await sendCommand(
            `s:gs=${getSettingsGs()},os=${getSettingsOs()},ss=${getSettingsSs()}`,
        );
    }

    async function syncSettings() {
        if (getConnected()) {
            await sendCommand("s:");
        }
    }

    // Mouth animation triggers
    async function triggerTalk() {
        await sendCommand(`talk ${mouthAnimDuration}`);
    }
    async function triggerChew() {
        await sendCommand(`chew ${mouthAnimDuration}`);
    }
    async function triggerWobble() {
        await sendCommand(`wobble ${mouthAnimDuration}`);
    }

    // Reset all settings to defaults
    async function resetToDefaults() {
        // Default values matching config.h
        setSettingsEw(36);
        setSettingsEh(36);
        setSettingsEs(10);
        setSettingsEr(12);
        setSettingsMw(20);
        setSettingsBi(3);
        setSettingsLt(1000);
        setSettingsVt(2000);
        setSettingsGs(6);
        setSettingsOs(12);
        setSettingsSs(10);
        mouthAnimDuration = 1000;
        transitionSpeed = 8;

        // Apply to device
        await applySettings();

        // Also send reset command
        await sendCommand("reset");
    }

    onMount(() => {
        if (getConnected()) {
            sendCommand("s:");
        }
    });
</script>

<div class="space-y-6">
    <!-- Quick Presets -->
    <div
        class="bg-gradient-to-br from-indigo-950/40 to-purple-950/30 border border-indigo-500/20 rounded-2xl p-5 backdrop-blur-lg"
    >
        <div class="flex items-center gap-2 mb-4">
            <svg
                class="w-4 h-4 text-indigo-400"
                fill="none"
                stroke="currentColor"
                viewBox="0 0 24 24"
            >
                <path
                    stroke-linecap="round"
                    stroke-linejoin="round"
                    stroke-width="2"
                    d="M5 3v4M3 5h4M6 17v4m-2-2h4m5-16l2.286 6.857L21 12l-5.714 2.143L13 21l-2.286-6.857L5 12l5.714-2.143L13 3z"
                />
            </svg>
            <h3 class="text-indigo-300 text-sm font-semibold">Quick Presets</h3>
        </div>
        <div class="grid grid-cols-4 gap-3">
            {#each presets as preset}
                <button
                    onclick={() => applyPreset(preset)}
                    disabled={!bleState.connected}
                    class="group flex flex-col items-center gap-2 p-3 rounded-xl bg-white/5 hover:bg-indigo-500/20 border border-white/5 hover:border-indigo-500/30 transition-all duration-200 disabled:opacity-40 disabled:cursor-not-allowed"
                >
                    <span
                        class="text-2xl group-hover:scale-110 transition-transform"
                        >{preset.icon}</span
                    >
                    <span class="text-zinc-300 text-xs font-medium"
                        >{preset.name}</span
                    >
                </button>
            {/each}
        </div>

        <!-- Reset Button -->
        <button
            onclick={resetToDefaults}
            disabled={!bleState.connected}
            class="w-full mt-3 py-2.5 rounded-xl bg-rose-500/10 hover:bg-rose-500/20 border border-rose-500/20 hover:border-rose-500/40 text-rose-400 text-xs font-medium transition-all disabled:opacity-40 flex items-center justify-center gap-2"
        >
            <svg
                class="w-4 h-4"
                fill="none"
                stroke="currentColor"
                viewBox="0 0 24 24"
            >
                <path
                    stroke-linecap="round"
                    stroke-linejoin="round"
                    stroke-width="2"
                    d="M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15"
                />
            </svg>
            Reset to Defaults
        </button>
    </div>

    <!-- Main Settings Card -->
    <div
        class="bg-zinc-900/50 border border-white/5 rounded-2xl p-5 backdrop-blur-md space-y-6"
    >
        <!-- Header -->
        <div class="flex items-center justify-between">
            <div class="flex items-center gap-3">
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
                    <h3 class="text-white text-sm font-semibold">
                        Eye Appearance
                    </h3>
                    <p class="text-zinc-500 text-xs">
                        Customize geometry & style
                    </p>
                </div>
            </div>
            <button
                onclick={syncSettings}
                disabled={!bleState.connected}
                class="p-2 rounded-lg hover:bg-white/10 text-zinc-400 hover:text-indigo-400 transition-all disabled:opacity-50"
                title="Sync from device"
            >
                <svg
                    class="w-4 h-4"
                    fill="none"
                    stroke="currentColor"
                    viewBox="0 0 24 24"
                >
                    <path
                        stroke-linecap="round"
                        stroke-linejoin="round"
                        stroke-width="2"
                        d="M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15"
                    />
                </svg>
            </button>
        </div>

        <!-- Eye Geometry Grid -->
        <div class="grid grid-cols-2 gap-4">
            <!-- Width -->
            <div class="space-y-2 p-3 rounded-xl bg-white/5">
                <div class="flex items-center justify-between">
                    <div class="flex items-center gap-2">
                        <svg
                            class="w-3.5 h-3.5 text-zinc-500"
                            fill="none"
                            stroke="currentColor"
                            viewBox="0 0 24 24"
                        >
                            <path
                                stroke-linecap="round"
                                stroke-linejoin="round"
                                stroke-width="2"
                                d="M4 8V4m0 0h4M4 4l5 5m11-1V4m0 0h-4m4 0l-5 5M4 16v4m0 0h4m-4 0l5-5m11 5l-5-5m5 5v-4m0 4h-4"
                            />
                        </svg>
                        <span class="text-zinc-400 text-xs">Width</span>
                    </div>
                    <span class="text-indigo-400 font-mono text-xs font-bold"
                        >{getSettingsEw()}px</span
                    >
                </div>
                <input
                    type="range"
                    min="10"
                    max="100"
                    value={getSettingsEw()}
                    oninput={(e) =>
                        setSettingsEw(parseInt(e.currentTarget.value))}
                    onchange={applySettings}
                    disabled={!bleState.connected}
                    class="w-full h-1.5 bg-zinc-700 rounded-full appearance-none cursor-pointer accent-indigo-500 disabled:opacity-50"
                />
            </div>

            <!-- Height -->
            <div class="space-y-2 p-3 rounded-xl bg-white/5">
                <div class="flex items-center justify-between">
                    <div class="flex items-center gap-2">
                        <svg
                            class="w-3.5 h-3.5 text-zinc-500"
                            fill="none"
                            stroke="currentColor"
                            viewBox="0 0 24 24"
                        >
                            <path
                                stroke-linecap="round"
                                stroke-linejoin="round"
                                stroke-width="2"
                                d="M7 16V4m0 0L3 8m4-4l4 4m6 0v12m0 0l4-4m-4 4l-4-4"
                            />
                        </svg>
                        <span class="text-zinc-400 text-xs">Height</span>
                    </div>
                    <span class="text-indigo-400 font-mono text-xs font-bold"
                        >{getSettingsEh()}px</span
                    >
                </div>
                <input
                    type="range"
                    min="10"
                    max="100"
                    value={getSettingsEh()}
                    oninput={(e) =>
                        setSettingsEh(parseInt(e.currentTarget.value))}
                    onchange={applySettings}
                    disabled={!bleState.connected}
                    class="w-full h-1.5 bg-zinc-700 rounded-full appearance-none cursor-pointer accent-indigo-500 disabled:opacity-50"
                />
            </div>

            <!-- Spacing -->
            <div class="space-y-2 p-3 rounded-xl bg-white/5">
                <div class="flex items-center justify-between">
                    <div class="flex items-center gap-2">
                        <svg
                            class="w-3.5 h-3.5 text-zinc-500"
                            fill="none"
                            stroke="currentColor"
                            viewBox="0 0 24 24"
                        >
                            <path
                                stroke-linecap="round"
                                stroke-linejoin="round"
                                stroke-width="2"
                                d="M8 9l4-4 4 4m0 6l-4 4-4-4"
                            />
                        </svg>
                        <span class="text-zinc-400 text-xs">Spacing</span>
                    </div>
                    <span class="text-indigo-400 font-mono text-xs font-bold"
                        >{getSettingsEs()}px</span
                    >
                </div>
                <input
                    type="range"
                    min="0"
                    max="50"
                    value={getSettingsEs()}
                    oninput={(e) =>
                        setSettingsEs(parseInt(e.currentTarget.value))}
                    onchange={applySettings}
                    disabled={!bleState.connected}
                    class="w-full h-1.5 bg-zinc-700 rounded-full appearance-none cursor-pointer accent-indigo-500 disabled:opacity-50"
                />
            </div>

            <!-- Roundness -->
            <div class="space-y-2 p-3 rounded-xl bg-white/5">
                <div class="flex items-center justify-between">
                    <div class="flex items-center gap-2">
                        <svg
                            class="w-3.5 h-3.5 text-zinc-500"
                            fill="none"
                            stroke="currentColor"
                            viewBox="0 0 24 24"
                        >
                            <path
                                stroke-linecap="round"
                                stroke-linejoin="round"
                                stroke-width="2"
                                d="M9 12l2 2 4-4m6 2a9 9 0 11-18 0 9 9 0 0118 0z"
                            />
                        </svg>
                        <span class="text-zinc-400 text-xs">Roundness</span>
                    </div>
                    <span class="text-indigo-400 font-mono text-xs font-bold"
                        >{getSettingsEr()}px</span
                    >
                </div>
                <input
                    type="range"
                    min="0"
                    max="50"
                    value={getSettingsEr()}
                    oninput={(e) =>
                        setSettingsEr(parseInt(e.currentTarget.value))}
                    onchange={applySettings}
                    disabled={!bleState.connected}
                    class="w-full h-1.5 bg-zinc-700 rounded-full appearance-none cursor-pointer accent-indigo-500 disabled:opacity-50"
                />
            </div>
        </div>
    </div>

    <!-- Mouth & Animations Card -->
    <div
        class="bg-zinc-900/50 border border-white/5 rounded-2xl p-5 backdrop-blur-md space-y-5"
    >
        <div class="flex items-center gap-3">
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
                        d="M14.828 14.828a4 4 0 01-5.656 0M9 10h.01M15 10h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z"
                    />
                </svg>
            </div>
            <div>
                <h3 class="text-white text-sm font-semibold">
                    Mouth & Animations
                </h3>
                <p class="text-zinc-500 text-xs">
                    Control expressions & timing
                </p>
            </div>
        </div>

        <!-- Mouth Width -->
        <div class="space-y-2 p-3 rounded-xl bg-white/5">
            <div class="flex items-center justify-between">
                <span class="text-zinc-400 text-xs">Mouth Width</span>
                <span class="text-pink-400 font-mono text-xs font-bold"
                    >{getSettingsMw()}px</span
                >
            </div>
            <input
                type="range"
                min="5"
                max="100"
                value={getSettingsMw()}
                oninput={(e) => setSettingsMw(parseInt(e.currentTarget.value))}
                onchange={applySettings}
                disabled={!bleState.connected}
                class="w-full h-1.5 bg-zinc-700 rounded-full appearance-none cursor-pointer accent-pink-500 disabled:opacity-50"
            />
        </div>

        <!-- Mouth Animation Triggers -->
        <div class="space-y-3">
            <div class="flex items-center justify-between">
                <span class="text-zinc-400 text-xs">Animation Duration</span>
                <span class="text-pink-400 font-mono text-xs font-bold"
                    >{mouthAnimDuration}ms</span
                >
            </div>
            <input
                type="range"
                min="500"
                max="5000"
                step="100"
                bind:value={mouthAnimDuration}
                disabled={!bleState.connected}
                class="w-full h-1.5 bg-zinc-700 rounded-full appearance-none cursor-pointer accent-pink-500 disabled:opacity-50"
            />
            <div class="grid grid-cols-3 gap-2 pt-2">
                <button
                    onclick={triggerTalk}
                    disabled={!bleState.connected}
                    class="px-3 py-2 rounded-lg bg-pink-500/20 hover:bg-pink-500/30 border border-pink-500/30 text-pink-300 text-xs font-medium transition-all disabled:opacity-40"
                >
                    💬 Talk
                </button>
                <button
                    onclick={triggerChew}
                    disabled={!bleState.connected}
                    class="px-3 py-2 rounded-lg bg-pink-500/20 hover:bg-pink-500/30 border border-pink-500/30 text-pink-300 text-xs font-medium transition-all disabled:opacity-40"
                >
                    🍔 Chew
                </button>
                <button
                    onclick={triggerWobble}
                    disabled={!bleState.connected}
                    class="px-3 py-2 rounded-lg bg-pink-500/20 hover:bg-pink-500/30 border border-pink-500/30 text-pink-300 text-xs font-medium transition-all disabled:opacity-40"
                >
                    〰️ Wobble
                </button>
            </div>
        </div>

        <!-- Blink & Expression Timing -->
        <div class="grid grid-cols-2 gap-4 pt-2">
            <div class="space-y-2 p-3 rounded-xl bg-white/5">
                <div class="flex items-center justify-between">
                    <span class="text-zinc-400 text-xs">Blink Interval</span>
                    <span class="text-pink-400 font-mono text-xs font-bold"
                        >{getSettingsBi()}s</span
                    >
                </div>
                <input
                    type="range"
                    min="1"
                    max="10"
                    step="0.5"
                    value={getSettingsBi()}
                    oninput={(e) =>
                        setSettingsBi(parseFloat(e.currentTarget.value))}
                    onchange={applySettings}
                    disabled={!bleState.connected}
                    class="w-full h-1.5 bg-zinc-700 rounded-full appearance-none cursor-pointer accent-pink-500 disabled:opacity-50"
                />
            </div>
            <div class="space-y-2 p-3 rounded-xl bg-white/5">
                <div class="flex items-center justify-between">
                    <span class="text-zinc-400 text-xs">Transition Speed</span>
                    <span class="text-pink-400 font-mono text-xs font-bold"
                        >{transitionSpeed}</span
                    >
                </div>
                <input
                    type="range"
                    min="1"
                    max="20"
                    bind:value={transitionSpeed}
                    disabled={!bleState.connected}
                    class="w-full h-1.5 bg-zinc-700 rounded-full appearance-none cursor-pointer accent-pink-500 disabled:opacity-50"
                />
            </div>
        </div>
    </div>

    <!-- Animation Speeds Card -->
    <div
        class="bg-zinc-900/50 border border-white/5 rounded-2xl p-5 backdrop-blur-md space-y-5"
    >
        <div class="flex items-center gap-3">
            <div
                class="w-8 h-8 rounded-lg bg-gradient-to-br from-emerald-500 to-teal-600 flex items-center justify-center"
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
                        d="M13 10V3L4 14h7v7l9-11h-7z"
                    />
                </svg>
            </div>
            <div>
                <h3 class="text-white text-sm font-semibold">
                    Animation Speeds
                </h3>
                <p class="text-zinc-500 text-xs">How fast things move</p>
            </div>
        </div>

        <div class="grid grid-cols-3 gap-3">
            <div class="space-y-2 p-3 rounded-xl bg-white/5 text-center">
                <span class="text-zinc-400 text-[10px] uppercase tracking-wide"
                    >Gaze</span
                >
                <input
                    type="range"
                    min="1"
                    max="20"
                    value={getSettingsGs()}
                    oninput={(e) =>
                        setSettingsGs(parseInt(e.currentTarget.value))}
                    onchange={applySettings}
                    disabled={!bleState.connected}
                    class="w-full h-1.5 bg-zinc-700 rounded-full appearance-none cursor-pointer accent-emerald-500 disabled:opacity-50"
                />
                <span class="text-emerald-400 font-mono text-xs font-bold block"
                    >{getSettingsGs()}</span
                >
            </div>
            <div class="space-y-2 p-3 rounded-xl bg-white/5 text-center">
                <span class="text-zinc-400 text-[10px] uppercase tracking-wide"
                    >Blink</span
                >
                <input
                    type="range"
                    min="4"
                    max="30"
                    value={getSettingsOs()}
                    oninput={(e) =>
                        setSettingsOs(parseInt(e.currentTarget.value))}
                    onchange={applySettings}
                    disabled={!bleState.connected}
                    class="w-full h-1.5 bg-zinc-700 rounded-full appearance-none cursor-pointer accent-emerald-500 disabled:opacity-50"
                />
                <span class="text-emerald-400 font-mono text-xs font-bold block"
                    >{getSettingsOs()}</span
                >
            </div>
            <div class="space-y-2 p-3 rounded-xl bg-white/5 text-center">
                <span class="text-zinc-400 text-[10px] uppercase tracking-wide"
                    >Squish</span
                >
                <input
                    type="range"
                    min="2"
                    max="20"
                    value={getSettingsSs()}
                    oninput={(e) =>
                        setSettingsSs(parseInt(e.currentTarget.value))}
                    onchange={applySettings}
                    disabled={!bleState.connected}
                    class="w-full h-1.5 bg-zinc-700 rounded-full appearance-none cursor-pointer accent-emerald-500 disabled:opacity-50"
                />
                <span class="text-emerald-400 font-mono text-xs font-bold block"
                    >{getSettingsSs()}</span
                >
            </div>
        </div>
        <p class="text-zinc-600 text-[10px] text-center">
            Higher values = faster animations
        </p>
    </div>

    <!-- Expression Durations -->
    <div
        class="bg-zinc-900/50 border border-white/5 rounded-2xl p-5 backdrop-blur-md space-y-4"
    >
        <div class="flex items-center gap-3">
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
                        d="M12 8v4l3 3m6-3a9 9 0 11-18 0 9 9 0 0118 0z"
                    />
                </svg>
            </div>
            <div>
                <h3 class="text-white text-sm font-semibold">
                    Expression Durations
                </h3>
                <p class="text-zinc-500 text-xs">
                    How long special expressions play
                </p>
            </div>
        </div>

        <div class="grid grid-cols-2 gap-4">
            <div class="space-y-2 p-3 rounded-xl bg-white/5">
                <div class="flex items-center justify-between">
                    <span class="text-zinc-400 text-xs">😂 Laugh</span>
                    <span class="text-amber-400 font-mono text-xs font-bold"
                        >{getSettingsLt()}ms</span
                    >
                </div>
                <input
                    type="range"
                    min="500"
                    max="5000"
                    step="100"
                    value={getSettingsLt()}
                    oninput={(e) =>
                        setSettingsLt(parseInt(e.currentTarget.value))}
                    onchange={applySettings}
                    disabled={!bleState.connected}
                    class="w-full h-1.5 bg-zinc-700 rounded-full appearance-none cursor-pointer accent-amber-500 disabled:opacity-50"
                />
            </div>
            <div class="space-y-2 p-3 rounded-xl bg-white/5">
                <div class="flex items-center justify-between">
                    <span class="text-zinc-400 text-xs">❤️ Love</span>
                    <span class="text-amber-400 font-mono text-xs font-bold"
                        >{getSettingsVt()}ms</span
                    >
                </div>
                <input
                    type="range"
                    min="500"
                    max="5000"
                    step="100"
                    value={getSettingsVt()}
                    oninput={(e) =>
                        setSettingsVt(parseInt(e.currentTarget.value))}
                    onchange={applySettings}
                    disabled={!bleState.connected}
                    class="w-full h-1.5 bg-zinc-700 rounded-full appearance-none cursor-pointer accent-amber-500 disabled:opacity-50"
                />
            </div>
        </div>
    </div>
</div>
