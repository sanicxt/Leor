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

    onMount(() => {
        if (getConnected()) {
            sendCommand("s:");
        }
    });
</script>

<div
    class="bg-zinc-900/40 border border-white/5 rounded-3xl p-6 backdrop-blur-md space-y-5"
>
    <!-- Header -->
    <div class="flex items-center justify-between">
        <div>
            <h3
                class="text-zinc-400 text-xs font-bold tracking-widest uppercase"
            >
                Appearance
            </h3>
            <p class="text-zinc-600 text-[10px] mt-1">
                Customize eye geometry and animation
            </p>
        </div>
        <button
            onclick={syncSettings}
            disabled={!bleState.connected}
            class="p-2 rounded-full hover:bg-white/10 text-zinc-400 hover:text-indigo-400 transition-colors disabled:opacity-50"
            title="Refresh from device"
        >
            <svg
                xmlns="http://www.w3.org/2000/svg"
                width="16"
                height="16"
                viewBox="0 0 24 24"
                fill="none"
                stroke="currentColor"
                stroke-width="2"
                stroke-linecap="round"
                stroke-linejoin="round"
            >
                <path d="M21 12a9 9 0 0 0-9-9 9.75 9.75 0 0 0-6.74 2.74L3 8" />
                <path d="M3 3v5h5" />
                <path d="M3 12a9 9 0 0 0 9 9 9.75 9.75 0 0 0 6.74-2.74L21 16" />
                <path d="M16 21h5v-5" />
            </svg>
        </button>
    </div>

    <!-- Eye Geometry -->
    <div class="space-y-4">
        <h4
            class="text-zinc-500 text-[10px] font-bold uppercase tracking-wider"
        >
            Eye Geometry
        </h4>

        <div class="grid grid-cols-1 lg:grid-cols-2 gap-x-8 gap-y-4">
            <div class="space-y-2">
                <div class="flex items-center justify-between">
                    <label class="text-zinc-300 text-sm">Width</label>
                    <span class="text-indigo-400 font-mono text-sm"
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
                    class="w-full h-2 bg-zinc-700 rounded-lg appearance-none cursor-pointer accent-indigo-500 disabled:opacity-50"
                />
            </div>

            <div class="space-y-2">
                <div class="flex items-center justify-between">
                    <label class="text-zinc-300 text-sm">Height</label>
                    <span class="text-indigo-400 font-mono text-sm"
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
                    class="w-full h-2 bg-zinc-700 rounded-lg appearance-none cursor-pointer accent-indigo-500 disabled:opacity-50"
                />
            </div>

            <div class="space-y-2">
                <div class="flex items-center justify-between">
                    <label class="text-zinc-300 text-sm">Spacing</label>
                    <span class="text-indigo-400 font-mono text-sm"
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
                    class="w-full h-2 bg-zinc-700 rounded-lg appearance-none cursor-pointer accent-indigo-500 disabled:opacity-50"
                />
            </div>

            <div class="space-y-2">
                <div class="flex items-center justify-between">
                    <label class="text-zinc-300 text-sm">Roundness</label>
                    <span class="text-indigo-400 font-mono text-sm"
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
                    class="w-full h-2 bg-zinc-700 rounded-lg appearance-none cursor-pointer accent-indigo-500 disabled:opacity-50"
                />
            </div>
        </div>
    </div>

    <!-- Mouth & Timings -->
    <div class="space-y-4 pt-4 border-t border-white/5">
        <h4
            class="text-zinc-500 text-[10px] font-bold uppercase tracking-wider"
        >
            Mouth & Timings
        </h4>

        <div class="grid grid-cols-1 lg:grid-cols-2 gap-x-8 gap-y-4">
            <div class="space-y-2">
                <div class="flex items-center justify-between">
                    <label class="text-zinc-300 text-sm">Mouth Width</label>
                    <span class="text-indigo-400 font-mono text-sm"
                        >{getSettingsMw()}px</span
                    >
                </div>
                <input
                    type="range"
                    min="5"
                    max="100"
                    value={getSettingsMw()}
                    oninput={(e) =>
                        setSettingsMw(parseInt(e.currentTarget.value))}
                    onchange={applySettings}
                    disabled={!bleState.connected}
                    class="w-full h-2 bg-zinc-700 rounded-lg appearance-none cursor-pointer accent-indigo-500 disabled:opacity-50"
                />
            </div>

            <div class="space-y-2">
                <div class="flex items-center justify-between">
                    <label class="text-zinc-300 text-sm">Blink Interval</label>
                    <span class="text-indigo-400 font-mono text-sm"
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
                    class="w-full h-2 bg-zinc-700 rounded-lg appearance-none cursor-pointer accent-indigo-500 disabled:opacity-50"
                />
                <p class="text-zinc-600 text-[10px]">
                    Time between auto-blinks
                </p>
            </div>

            <div class="space-y-2">
                <div class="flex items-center justify-between">
                    <label class="text-zinc-300 text-sm">Laugh Duration</label>
                    <span class="text-indigo-400 font-mono text-sm"
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
                    class="w-full h-2 bg-zinc-700 rounded-lg appearance-none cursor-pointer accent-indigo-500 disabled:opacity-50"
                />
                <p class="text-zinc-600 text-[10px]">
                    How long laugh expression plays
                </p>
            </div>

            <div class="space-y-2">
                <div class="flex items-center justify-between">
                    <label class="text-zinc-300 text-sm">Love Duration</label>
                    <span class="text-indigo-400 font-mono text-sm"
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
                    class="w-full h-2 bg-zinc-700 rounded-lg appearance-none cursor-pointer accent-indigo-500 disabled:opacity-50"
                />
                <p class="text-zinc-600 text-[10px]">
                    How long love expression plays
                </p>
            </div>
        </div>
    </div>

    <!-- Animation Speeds -->
    <div class="space-y-4 pt-4 border-t border-white/5">
        <h4
            class="text-zinc-500 text-[10px] font-bold uppercase tracking-wider"
        >
            Animation Speed
        </h4>

        <div class="grid grid-cols-3 gap-4">
            <div class="space-y-2">
                <div class="flex items-center justify-between">
                    <label class="text-zinc-300 text-sm">Gaze</label>
                    <span class="text-indigo-400 font-mono text-sm"
                        >{getSettingsGs()}</span
                    >
                </div>
                <input
                    type="range"
                    min="1"
                    max="20"
                    value={getSettingsGs()}
                    oninput={(e) =>
                        setSettingsGs(parseInt(e.currentTarget.value))}
                    onchange={applySettings}
                    disabled={!bleState.connected}
                    class="w-full h-2 bg-zinc-700 rounded-lg appearance-none cursor-pointer accent-indigo-500 disabled:opacity-50"
                />
            </div>

            <div class="space-y-2">
                <div class="flex items-center justify-between">
                    <label class="text-zinc-300 text-sm">Blink</label>
                    <span class="text-indigo-400 font-mono text-sm"
                        >{getSettingsOs()}</span
                    >
                </div>
                <input
                    type="range"
                    min="4"
                    max="30"
                    value={getSettingsOs()}
                    oninput={(e) =>
                        setSettingsOs(parseInt(e.currentTarget.value))}
                    onchange={applySettings}
                    disabled={!bleState.connected}
                    class="w-full h-2 bg-zinc-700 rounded-lg appearance-none cursor-pointer accent-indigo-500 disabled:opacity-50"
                />
            </div>

            <div class="space-y-2">
                <div class="flex items-center justify-between">
                    <label class="text-zinc-300 text-sm">Squish</label>
                    <span class="text-indigo-400 font-mono text-sm"
                        >{getSettingsSs()}</span
                    >
                </div>
                <input
                    type="range"
                    min="2"
                    max="20"
                    value={getSettingsSs()}
                    oninput={(e) =>
                        setSettingsSs(parseInt(e.currentTarget.value))}
                    onchange={applySettings}
                    disabled={!bleState.connected}
                    class="w-full h-2 bg-zinc-700 rounded-lg appearance-none cursor-pointer accent-indigo-500 disabled:opacity-50"
                />
            </div>
        </div>
        <p class="text-zinc-600 text-[10px]">Higher = faster animations</p>
    </div>
</div>
