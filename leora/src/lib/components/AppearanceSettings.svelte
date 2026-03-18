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
    <div class="bento-card bg-bento-yellow p-6 space-y-4">
        <div class="mb-4 border-b-2 border-bento-border pb-2">
            <h2 class="text-xl font-black uppercase">Quick Presets</h2>
        </div>
        <div class="grid grid-cols-4 gap-3">
            {#each presets as preset}
                <button
                    onclick={() => applyPreset(preset)}
                    disabled={!bleState.connected}
                    class="bento-button group flex flex-col items-center gap-2 p-3 bg-paper transition-all duration-200 disabled:opacity-50"
                >
                    <span class="text-2xl group-hover:scale-110 transition-transform">{preset.icon}</span>
                    <span class="text-ink text-xs font-bold">{preset.name}</span>
                </button>
            {/each}
        </div>

        <!-- Reset Button -->
        <button
            onclick={resetToDefaults}
            disabled={!bleState.connected}
            class="bento-button w-full mt-3 py-2.5 bg-bento-peach text-ink text-xs font-bold transition-all disabled:opacity-50 flex items-center justify-center gap-2"
        >
            <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15" />
            </svg>
            Reset to Defaults
        </button>
    </div>

    <!-- Eye Appearance Card -->
    <div class="bento-card bg-bento-blue p-6 space-y-6">
        <!-- Header -->
        <div class="mb-4 border-b-2 border-bento-border pb-2 flex items-center justify-between">
            <div>
                <h2 class="text-xl font-black uppercase">Eye Appearance</h2>
                <p class="text-sm font-bold opacity-80">
                    Customize geometry & style
                </p>
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
            <div class="space-y-2 p-3 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl">
                <div class="flex items-center justify-between">
                    <div class="flex items-center gap-2">
                        <svg class="w-3.5 h-3.5 text-ink" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M4 8V4m0 0h4M4 4l5 5m11-1V4m0 0h-4m4 0l-5 5M4 16v4m0 0h4m-4 0l5-5m11 5l-5-5m5 5v-4m0 4h-4" />
                        </svg>
                        <span class="text-ink/80 text-xs font-bold uppercase tracking-wider">Width</span>
                    </div>
                    <span class="text-ink font-mono font-bold text-xs px-2 py-0.5 bg-paper border-2 border-bento-border rounded-lg shadow-[2px_2px_0px_0px_var(--color-bento-border)]">{getSettingsEw()}px</span>
                </div>
                <input
                    type="range"
                    min="10"
                    max="100"
                    value={getSettingsEw()}
                    oninput={(e) => setSettingsEw(parseInt(e.currentTarget.value))}
                    onchange={applySettings}
                    disabled={!bleState.connected}
                    class="w-full h-2 bg-paper border-2 border-bento-border rounded-full appearance-none cursor-pointer disabled:opacity-50
                           [&::-webkit-slider-thumb]:appearance-none
                           [&::-webkit-slider-thumb]:w-4
                           [&::-webkit-slider-thumb]:h-4
                           [&::-webkit-slider-thumb]:rounded-sm
                           [&::-webkit-slider-thumb]:bg-bento-yellow
                           [&::-webkit-slider-thumb]:shadow-[2px_2px_0px_0px_var(--color-bento-border)]
                           [&::-webkit-slider-thumb]:cursor-pointer
                           [&::-webkit-slider-thumb]:border-2
                           [&::-webkit-slider-thumb]:border-bento-border
                           [&::-webkit-slider-thumb]:transition-transform
                           [&::-webkit-slider-thumb]:active:scale-125
                           [&::-webkit-slider-thumb]:active:shadow-none
                           [&::-webkit-slider-thumb]:active:translate-y-[2px]
                           [&::-webkit-slider-thumb]:active:translate-x-[2px]"
                />
            </div>

            <!-- Height -->
            <div class="space-y-2 p-3 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl">
                <div class="flex items-center justify-between">
                    <div class="flex items-center gap-2">
                        <svg class="w-3.5 h-3.5 text-ink" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M7 16V4m0 0L3 8m4-4l4 4m6 0v12m0 0l4-4m-4 4l-4-4" />
                        </svg>
                        <span class="text-ink/80 text-xs font-bold uppercase tracking-wider">Height</span>
                    </div>
                    <span class="text-ink font-mono font-bold text-xs px-2 py-0.5 bg-paper border-2 border-bento-border rounded-lg shadow-[2px_2px_0px_0px_var(--color-bento-border)]">{getSettingsEh()}px</span>
                </div>
                <input
                    type="range"
                    min="10"
                    max="100"
                    value={getSettingsEh()}
                    oninput={(e) => setSettingsEh(parseInt(e.currentTarget.value))}
                    onchange={applySettings}
                    disabled={!bleState.connected}
                    class="w-full h-2 bg-paper border-2 border-bento-border rounded-full appearance-none cursor-pointer disabled:opacity-50
                           [&::-webkit-slider-thumb]:appearance-none
                           [&::-webkit-slider-thumb]:w-4
                           [&::-webkit-slider-thumb]:h-4
                           [&::-webkit-slider-thumb]:rounded-sm
                           [&::-webkit-slider-thumb]:bg-bento-yellow
                           [&::-webkit-slider-thumb]:shadow-[2px_2px_0px_0px_var(--color-bento-border)]
                           [&::-webkit-slider-thumb]:cursor-pointer
                           [&::-webkit-slider-thumb]:border-2
                           [&::-webkit-slider-thumb]:border-bento-border
                           [&::-webkit-slider-thumb]:transition-transform
                           [&::-webkit-slider-thumb]:active:scale-125
                           [&::-webkit-slider-thumb]:active:shadow-none
                           [&::-webkit-slider-thumb]:active:translate-y-[2px]
                           [&::-webkit-slider-thumb]:active:translate-x-[2px]"
                />
            </div>

            <!-- Spacing -->
            <div class="space-y-2 p-3 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl">
                <div class="flex items-center justify-between">
                    <div class="flex items-center gap-2">
                        <svg class="w-3.5 h-3.5 text-ink" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M8 9l4-4 4 4m0 6l-4 4-4-4" />
                        </svg>
                        <span class="text-ink/80 text-xs font-bold uppercase tracking-wider">Spacing</span>
                    </div>
                    <span class="text-ink font-mono font-bold text-xs px-2 py-0.5 bg-paper border-2 border-bento-border rounded-lg shadow-[2px_2px_0px_var(--color-bento-border)]">{getSettingsEs()}px</span>
                </div>
                <input
                    type="range"
                    min="0"
                    max="50"
                    value={getSettingsEs()}
                    oninput={(e) => setSettingsEs(parseInt(e.currentTarget.value))}
                    onchange={applySettings}
                    disabled={!bleState.connected}
                    class="w-full h-2 bg-paper border-2 border-bento-border rounded-full appearance-none cursor-pointer disabled:opacity-50
                           [&::-webkit-slider-thumb]:appearance-none
                           [&::-webkit-slider-thumb]:w-4
                           [&::-webkit-slider-thumb]:h-4
                           [&::-webkit-slider-thumb]:rounded-sm
                           [&::-webkit-slider-thumb]:bg-bento-yellow
                           [&::-webkit-slider-thumb]:shadow-[2px_2px_0px_0px_var(--color-bento-border)]
                           [&::-webkit-slider-thumb]:cursor-pointer
                           [&::-webkit-slider-thumb]:border-2
                           [&::-webkit-slider-thumb]:border-bento-border
                           [&::-webkit-slider-thumb]:transition-transform
                           [&::-webkit-slider-thumb]:active:scale-125
                           [&::-webkit-slider-thumb]:active:shadow-none
                           [&::-webkit-slider-thumb]:active:translate-y-[2px]
                           [&::-webkit-slider-thumb]:active:translate-x-[2px]"
                />
            </div>

            <!-- Roundness -->
            <div class="space-y-2 p-3 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl">
                <div class="flex items-center justify-between">
                    <div class="flex items-center gap-2">
                        <svg class="w-3.5 h-3.5 text-ink" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M9 12l2 2 4-4m6 2a9 9 0 11-18 0 9 9 0 0118 0z" />
                        </svg>
                        <span class="text-ink/80 text-xs font-bold uppercase tracking-wider">Roundness</span>
                    </div>
                    <span class="text-ink font-mono font-bold text-xs px-2 py-0.5 bg-paper border-2 border-bento-border rounded-lg shadow-[2px_2px_0px_var(--color-bento-border)]">{getSettingsEr()}px</span>
                </div>
                <input
                    type="range"
                    min="0"
                    max="50"
                    value={getSettingsEr()}
                    oninput={(e) => setSettingsEr(parseInt(e.currentTarget.value))}
                    onchange={applySettings}
                    disabled={!bleState.connected}
                    class="w-full h-2 bg-paper border-2 border-bento-border rounded-full appearance-none cursor-pointer disabled:opacity-50
                           [&::-webkit-slider-thumb]:appearance-none
                           [&::-webkit-slider-thumb]:w-4
                           [&::-webkit-slider-thumb]:h-4
                           [&::-webkit-slider-thumb]:rounded-sm
                           [&::-webkit-slider-thumb]:bg-bento-yellow
                           [&::-webkit-slider-thumb]:shadow-[2px_2px_0px_0px_var(--color-bento-border)]
                           [&::-webkit-slider-thumb]:cursor-pointer
                           [&::-webkit-slider-thumb]:border-2
                           [&::-webkit-slider-thumb]:border-bento-border
                           [&::-webkit-slider-thumb]:transition-transform
                           [&::-webkit-slider-thumb]:active:scale-125
                           [&::-webkit-slider-thumb]:active:shadow-none
                           [&::-webkit-slider-thumb]:active:translate-y-[2px]
                           [&::-webkit-slider-thumb]:active:translate-x-[2px]"
                />
            </div>
        </div>
    </div>

    <!-- Mouth & Animations Card -->
    <div class="bento-card bg-bento-pink p-6 space-y-5">
        <div class="mb-4 border-b-2 border-bento-border pb-2">
            <h2 class="text-xl font-black uppercase">Mouth & Animations</h2>
            <p class="text-sm font-bold opacity-80">
                Control expressions & timing
            </p>
        </div>

        <!-- Mouth Width -->
        <div class="space-y-2 p-3 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl">
            <div class="flex items-center justify-between">
                <span class="text-ink/80 text-xs font-bold uppercase tracking-wider">Mouth Width</span>
                <span class="text-ink font-mono font-bold text-xs px-2 py-0.5 bg-paper border-2 border-bento-border rounded-lg shadow-[2px_2px_0px_0px_var(--color-bento-border)]">{getSettingsMw()}px</span>
            </div>
            <input
                type="range"
                min="5"
                max="100"
                value={getSettingsMw()}
                oninput={(e) => setSettingsMw(parseInt(e.currentTarget.value))}
                onchange={applySettings}
                disabled={!bleState.connected}
                class="w-full h-2 bg-paper border-2 border-bento-border rounded-full appearance-none cursor-pointer disabled:opacity-50
                       [&::-webkit-slider-thumb]:appearance-none
                       [&::-webkit-slider-thumb]:w-4
                       [&::-webkit-slider-thumb]:h-4
                       [&::-webkit-slider-thumb]:rounded-sm
                       [&::-webkit-slider-thumb]:bg-bento-yellow
                       [&::-webkit-slider-thumb]:shadow-[2px_2px_0px_0px_var(--color-bento-border)]
                       [&::-webkit-slider-thumb]:cursor-pointer
                       [&::-webkit-slider-thumb]:border-2
                       [&::-webkit-slider-thumb]:border-bento-border
                       [&::-webkit-slider-thumb]:transition-transform
                       [&::-webkit-slider-thumb]:active:scale-125
                       [&::-webkit-slider-thumb]:active:shadow-none
                       [&::-webkit-slider-thumb]:active:translate-y-[2px]
                       [&::-webkit-slider-thumb]:active:translate-x-[2px]"
            />
        </div>

        <!-- Mouth Animation Triggers -->
        <div class="space-y-3">
            <div class="space-y-2 p-3 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl">
                <div class="flex items-center justify-between">
                    <span class="text-ink/80 text-xs font-bold uppercase tracking-wider">Animation Duration</span>
                    <span class="text-ink font-mono font-bold text-xs px-2 py-0.5 bg-paper border-2 border-bento-border rounded-lg shadow-[2px_2px_0px_0px_var(--color-bento-border)]"
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
                    class="w-full h-2 bg-paper border-2 border-bento-border rounded-full appearance-none cursor-pointer disabled:opacity-50
                           [&::-webkit-slider-thumb]:appearance-none
                           [&::-webkit-slider-thumb]:w-4
                           [&::-webkit-slider-thumb]:h-4
                           [&::-webkit-slider-thumb]:rounded-sm
                           [&::-webkit-slider-thumb]:bg-bento-yellow
                           [&::-webkit-slider-thumb]:shadow-[2px_2px_0px_0px_var(--color-bento-border)]
                           [&::-webkit-slider-thumb]:cursor-pointer
                           [&::-webkit-slider-thumb]:border-2
                           [&::-webkit-slider-thumb]:border-bento-border
                           [&::-webkit-slider-thumb]:transition-transform
                           [&::-webkit-slider-thumb]:active:scale-125
                           [&::-webkit-slider-thumb]:active:shadow-none
                           [&::-webkit-slider-thumb]:active:translate-y-[2px]
                           [&::-webkit-slider-thumb]:active:translate-x-[2px]"
                />
            </div>
            <div class="grid grid-cols-3 gap-2">
                <button
                    onclick={triggerTalk}
                    disabled={!bleState.connected}
                    class="bento-button py-2 bg-paper text-ink transition-all disabled:opacity-50 text-xs"
                >
                    💬 Talk
                </button>
                <button
                    onclick={triggerChew}
                    disabled={!bleState.connected}
                    class="bento-button py-2 bg-paper text-ink transition-all disabled:opacity-50 text-xs"
                >
                    🍔 Chew
                </button>
                <button
                    onclick={triggerWobble}
                    disabled={!bleState.connected}
                    class="bento-button py-2 bg-paper text-ink transition-all disabled:opacity-50 text-xs"
                >
                    〰️ Wobble
                </button>
            </div>
        </div>

        <!-- Blink & Expression Timing -->
        <div class="grid grid-cols-2 gap-4 pt-2">
            <div class="space-y-2 p-3 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl">
                <div class="flex items-center justify-between">
                    <span class="text-ink/80 text-xs font-bold uppercase tracking-wider">Blink Interval</span>
                    <span class="text-ink font-mono font-bold text-xs px-2 py-0.5 bg-paper border-2 border-bento-border rounded-lg shadow-[2px_2px_0px_0px_var(--color-bento-border)]"
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
                    class="w-full h-2 bg-paper border-2 border-bento-border rounded-full appearance-none cursor-pointer disabled:opacity-50
                           [&::-webkit-slider-thumb]:appearance-none
                           [&::-webkit-slider-thumb]:w-4
                           [&::-webkit-slider-thumb]:h-4
                           [&::-webkit-slider-thumb]:rounded-sm
                           [&::-webkit-slider-thumb]:bg-bento-yellow
                           [&::-webkit-slider-thumb]:shadow-[2px_2px_0px_0px_var(--color-bento-border)]
                           [&::-webkit-slider-thumb]:cursor-pointer
                           [&::-webkit-slider-thumb]:border-2
                           [&::-webkit-slider-thumb]:border-bento-border
                           [&::-webkit-slider-thumb]:transition-transform
                           [&::-webkit-slider-thumb]:active:scale-125
                           [&::-webkit-slider-thumb]:active:shadow-none
                           [&::-webkit-slider-thumb]:active:translate-y-[2px]
                           [&::-webkit-slider-thumb]:active:translate-x-[2px]"
                />
            </div>
            <div class="space-y-2 p-3 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl">
                <div class="flex items-center justify-between">
                    <span class="text-ink/80 text-xs font-bold uppercase tracking-wider">Transition Speed</span>
                    <span class="text-ink font-mono font-bold text-xs px-2 py-0.5 bg-paper border-2 border-bento-border rounded-lg shadow-[2px_2px_0px_0px_var(--color-bento-border)]"
                        >{transitionSpeed}</span
                    >
                </div>
                <input
                    type="range"
                    min="1"
                    max="20"
                    bind:value={transitionSpeed}
                    disabled={!bleState.connected}
                    class="w-full h-2 bg-paper border-2 border-bento-border rounded-full appearance-none cursor-pointer disabled:opacity-50
                           [&::-webkit-slider-thumb]:appearance-none
                           [&::-webkit-slider-thumb]:w-4
                           [&::-webkit-slider-thumb]:h-4
                           [&::-webkit-slider-thumb]:rounded-sm
                           [&::-webkit-slider-thumb]:bg-bento-yellow
                           [&::-webkit-slider-thumb]:shadow-[2px_2px_0px_0px_var(--color-bento-border)]
                           [&::-webkit-slider-thumb]:cursor-pointer
                           [&::-webkit-slider-thumb]:border-2
                           [&::-webkit-slider-thumb]:border-bento-border
                           [&::-webkit-slider-thumb]:transition-transform
                           [&::-webkit-slider-thumb]:active:scale-125
                           [&::-webkit-slider-thumb]:active:shadow-none
                           [&::-webkit-slider-thumb]:active:translate-y-[2px]
                           [&::-webkit-slider-thumb]:active:translate-x-[2px]"
                />
            </div>
        </div>
    </div>

    <!-- Animation Speeds Card -->
    <div class="bento-card bg-bento-green p-6 space-y-5">
        <div class="mb-4 border-b-2 border-bento-border pb-2">
            <h2 class="text-xl font-black uppercase">Animation Speeds</h2>
            <p class="text-sm font-bold opacity-80">How fast things move</p>
        </div>

        <div class="grid grid-cols-3 gap-3">
            <div class="space-y-2 p-3 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl text-center">
                <span class="text-ink/80 text-[10px] uppercase font-bold tracking-wide">Gaze</span>
                <input
                    type="range"
                    min="1"
                    max="20"
                    value={getSettingsGs()}
                    oninput={(e) => setSettingsGs(parseInt(e.currentTarget.value))}
                    onchange={applySettings}
                    disabled={!bleState.connected}
                    class="w-full h-2 bg-paper border-2 border-bento-border rounded-full appearance-none cursor-pointer disabled:opacity-50
                           [&::-webkit-slider-thumb]:appearance-none
                           [&::-webkit-slider-thumb]:w-4
                           [&::-webkit-slider-thumb]:h-4
                           [&::-webkit-slider-thumb]:rounded-sm
                           [&::-webkit-slider-thumb]:bg-bento-yellow
                           [&::-webkit-slider-thumb]:shadow-[2px_2px_0px_0px_var(--color-bento-border)]
                           [&::-webkit-slider-thumb]:cursor-pointer
                           [&::-webkit-slider-thumb]:border-2
                           [&::-webkit-slider-thumb]:border-bento-border
                           [&::-webkit-slider-thumb]:transition-transform
                           [&::-webkit-slider-thumb]:active:scale-125
                           [&::-webkit-slider-thumb]:active:shadow-none
                           [&::-webkit-slider-thumb]:active:translate-y-[2px]
                           [&::-webkit-slider-thumb]:active:translate-x-[2px]"
                />
                <span class="text-ink font-mono text-xs font-bold block bg-paper border-2 border-bento-border rounded-lg py-0.5 mx-auto w-10 shadow-[2px_2px_0px_0px_var(--color-bento-border)]">{getSettingsGs()}</span>
            </div>
            <div class="space-y-2 p-3 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl text-center">
                <span class="text-ink/80 text-[10px] uppercase font-bold tracking-wide">Blink</span>
                <input
                    type="range"
                    min="4"
                    max="30"
                    value={getSettingsOs()}
                    oninput={(e) => setSettingsOs(parseInt(e.currentTarget.value))}
                    onchange={applySettings}
                    disabled={!bleState.connected}
                    class="w-full h-2 bg-paper border-2 border-bento-border rounded-full appearance-none cursor-pointer disabled:opacity-50
                           [&::-webkit-slider-thumb]:appearance-none
                           [&::-webkit-slider-thumb]:w-4
                           [&::-webkit-slider-thumb]:h-4
                           [&::-webkit-slider-thumb]:rounded-sm
                           [&::-webkit-slider-thumb]:bg-bento-yellow
                           [&::-webkit-slider-thumb]:shadow-[2px_2px_0px_0px_var(--color-bento-border)]
                           [&::-webkit-slider-thumb]:cursor-pointer
                           [&::-webkit-slider-thumb]:border-2
                           [&::-webkit-slider-thumb]:border-bento-border
                           [&::-webkit-slider-thumb]:transition-transform
                           [&::-webkit-slider-thumb]:active:scale-125
                           [&::-webkit-slider-thumb]:active:shadow-none
                           [&::-webkit-slider-thumb]:active:translate-y-[2px]
                           [&::-webkit-slider-thumb]:active:translate-x-[2px]"
                />
                <span class="text-ink font-mono text-xs font-bold block bg-paper border-2 border-bento-border rounded-lg py-0.5 mx-auto w-10 shadow-[2px_2px_0px_0px_var(--color-bento-border)]">{getSettingsOs()}</span>
            </div>
            <div class="space-y-2 p-3 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl text-center">
                <span class="text-ink/80 text-[10px] uppercase font-bold tracking-wide">Squish</span>
                <input
                    type="range"
                    min="2"
                    max="20"
                    value={getSettingsSs()}
                    oninput={(e) => setSettingsSs(parseInt(e.currentTarget.value))}
                    onchange={applySettings}
                    disabled={!bleState.connected}
                    class="w-full h-2 bg-paper border-2 border-bento-border rounded-full appearance-none cursor-pointer disabled:opacity-50
                           [&::-webkit-slider-thumb]:appearance-none
                           [&::-webkit-slider-thumb]:w-4
                           [&::-webkit-slider-thumb]:h-4
                           [&::-webkit-slider-thumb]:rounded-sm
                           [&::-webkit-slider-thumb]:bg-bento-yellow
                           [&::-webkit-slider-thumb]:shadow-[2px_2px_0px_0px_var(--color-bento-border)]
                           [&::-webkit-slider-thumb]:cursor-pointer
                           [&::-webkit-slider-thumb]:border-2
                           [&::-webkit-slider-thumb]:border-bento-border
                           [&::-webkit-slider-thumb]:transition-transform
                           [&::-webkit-slider-thumb]:active:scale-125
                           [&::-webkit-slider-thumb]:active:shadow-none
                           [&::-webkit-slider-thumb]:active:translate-y-[2px]
                           [&::-webkit-slider-thumb]:active:translate-x-[2px]"
                />
                <span class="text-ink font-mono text-xs font-bold block bg-paper border-2 border-bento-border rounded-lg py-0.5 mx-auto w-10 shadow-[2px_2px_0px_0px_var(--color-bento-border)]">{getSettingsSs()}</span>
            </div>
        </div>
        <p class="text-ink/60 font-bold text-[10px] text-center">
            Higher values = faster animations
        </p>
    </div>
</div>
