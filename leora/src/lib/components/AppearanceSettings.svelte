<script lang="ts">
    import { onMount } from "svelte";
    import {
        sendCommand,
        getConnected,
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
        // Send in small chunks to avoid MTU limits (20 bytes safe limit)
        // Eyes Dimensions
        await sendCommand(`s:ew=${getSettingsEw()},eh=${getSettingsEh()}`);
        await new Promise((r) => setTimeout(r, 50));

        // Eyes Style (Spacing/Roundness)
        await sendCommand(`s:es=${getSettingsEs()},er=${getSettingsEr()}`);
        await new Promise((r) => setTimeout(r, 50));

        // Mouth & Blink
        await sendCommand(`s:mw=${getSettingsMw()},bi=${getSettingsBi()}`);
        await new Promise((r) => setTimeout(r, 50));

        // Timings
        await sendCommand(`s:lt=${getSettingsLt()},vt=${getSettingsVt()}`);
        await new Promise((r) => setTimeout(r, 50));

        // Animation Speeds
        await sendCommand(
            `s:gs=${getSettingsGs()},os=${getSettingsOs()},ss=${getSettingsSs()}`,
        );
    }

    async function syncSettings() {
        if (getConnected()) {
            console.log("Manual sync requested");
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
    class="bg-zinc-900/40 border border-white/5 rounded-3xl p-6 backdrop-blur-md space-y-6"
>
    <div class="flex items-center justify-between">
        <h3 class="text-zinc-400 text-xs font-bold tracking-widest uppercase">
            Appearance & Behavior
        </h3>
        <button
            onclick={syncSettings}
            class="p-2 rounded-full hover:bg-white/10 text-zinc-400 hover:text-indigo-400 transition-colors"
            title="Refresh Settings from Device"
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
                class="w-4 h-4"
            >
                <path d="M21 12a9 9 0 0 0-9-9 9.75 9.75 0 0 0-6.74 2.74L3 8" />
                <path d="M3 3v5h5" />
                <path d="M3 12a9 9 0 0 0 9 9 9.75 9.75 0 0 0 6.74-2.74L21 16" />
                <path d="M16 21h5v-5" />
            </svg>
        </button>
    </div>

    <div class="grid grid-cols-1 sm:grid-cols-2 gap-6">
        <!-- Eyes -->
        <div class="space-y-4">
            <h4
                class="text-zinc-400 text-xs font-bold uppercase tracking-wider"
            >
                Eye Geometry
            </h4>

            <div class="space-y-2">
                <div class="flex justify-between text-sm">
                    <label class="text-zinc-300">Width (ew)</label>
                    <span class="text-indigo-400 font-mono"
                        >{getSettingsEw()}</span
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
                    class="w-full accent-indigo-500 bg-zinc-700 rounded-lg h-1.5 cursor-pointer"
                />
            </div>

            <div class="space-y-2">
                <div class="flex justify-between text-sm">
                    <label class="text-zinc-300">Height (eh)</label>
                    <span class="text-indigo-400 font-mono"
                        >{getSettingsEh()}</span
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
                    class="w-full accent-indigo-500 bg-zinc-700 rounded-lg h-1.5 cursor-pointer"
                />
            </div>

            <div class="space-y-2">
                <div class="flex justify-between text-sm">
                    <label class="text-zinc-300">Spacing (es)</label>
                    <span class="text-indigo-400 font-mono"
                        >{getSettingsEs()}</span
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
                    class="w-full accent-indigo-500 bg-zinc-700 rounded-lg h-1.5 cursor-pointer"
                />
            </div>

            <div class="space-y-2">
                <div class="flex justify-between text-sm">
                    <label class="text-zinc-300">Roundness (er)</label>
                    <span class="text-indigo-400 font-mono"
                        >{getSettingsEr()}</span
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
                    class="w-full accent-indigo-500 bg-zinc-700 rounded-lg h-1.5 cursor-pointer"
                />
            </div>
        </div>

        <!-- Mouth & Behavior -->
        <div class="space-y-4">
            <h4
                class="text-zinc-400 text-xs font-bold uppercase tracking-wider"
            >
                Mouth & Behavior
            </h4>

            <div class="space-y-2">
                <div class="flex justify-between text-sm">
                    <label class="text-zinc-300">Mouth Width (mw)</label>
                    <span class="text-indigo-400 font-mono"
                        >{getSettingsMw()}</span
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
                    class="w-full accent-indigo-500 bg-zinc-700 rounded-lg h-1.5 cursor-pointer"
                />
            </div>

            <div class="space-y-2">
                <div class="flex justify-between text-sm">
                    <label class="text-zinc-300">Laugh Time (ms)</label>
                    <span class="text-indigo-400 font-mono"
                        >{getSettingsLt()}</span
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
                    class="w-full accent-indigo-500 bg-zinc-700 rounded-lg h-1.5 cursor-pointer"
                />
            </div>

            <div class="space-y-2">
                <div class="flex justify-between text-sm">
                    <label class="text-zinc-300">Love Time (ms)</label>
                    <span class="text-indigo-400 font-mono"
                        >{getSettingsVt()}</span
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
                    class="w-full accent-indigo-500 bg-zinc-700 rounded-lg h-1.5 cursor-pointer"
                />
            </div>

            <div class="space-y-2">
                <div class="flex justify-between text-sm">
                    <label class="text-zinc-300">Blink Interval (s)</label>
                    <span class="text-indigo-400 font-mono"
                        >{getSettingsBi()}</span
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
                    class="w-full accent-indigo-500 bg-zinc-700 rounded-lg h-1.5 cursor-pointer"
                />
            </div>
        </div>
    </div>

    <!-- Animation Speeds -->
    <div class="pt-4 border-t border-white/5">
        <h4
            class="text-zinc-400 text-xs font-bold uppercase tracking-wider mb-4"
        >
            Animation Speeds
        </h4>
        <div class="grid grid-cols-1 sm:grid-cols-3 gap-4">
            <div class="space-y-2">
                <div class="flex justify-between text-sm">
                    <label class="text-zinc-300">Gaze</label>
                    <span class="text-indigo-400 font-mono"
                        >{getSettingsGs()}</span
                    >
                </div>
                <input
                    type="range"
                    min="1"
                    max="20"
                    step="1"
                    value={getSettingsGs()}
                    oninput={(e) =>
                        setSettingsGs(parseInt(e.currentTarget.value))}
                    onchange={applySettings}
                    class="w-full accent-indigo-500 bg-zinc-700 rounded-lg h-1.5 cursor-pointer"
                />
            </div>

            <div class="space-y-2">
                <div class="flex justify-between text-sm">
                    <label class="text-zinc-300">Blink</label>
                    <span class="text-indigo-400 font-mono"
                        >{getSettingsOs()}</span
                    >
                </div>
                <input
                    type="range"
                    min="4"
                    max="30"
                    step="1"
                    value={getSettingsOs()}
                    oninput={(e) =>
                        setSettingsOs(parseInt(e.currentTarget.value))}
                    onchange={applySettings}
                    class="w-full accent-indigo-500 bg-zinc-700 rounded-lg h-1.5 cursor-pointer"
                />
            </div>

            <div class="space-y-2">
                <div class="flex justify-between text-sm">
                    <label class="text-zinc-300">Squish</label>
                    <span class="text-indigo-400 font-mono"
                        >{getSettingsSs()}</span
                    >
                </div>
                <input
                    type="range"
                    min="2"
                    max="20"
                    step="1"
                    value={getSettingsSs()}
                    oninput={(e) =>
                        setSettingsSs(parseInt(e.currentTarget.value))}
                    onchange={applySettings}
                    class="w-full accent-indigo-500 bg-zinc-700 rounded-lg h-1.5 cursor-pointer"
                />
            </div>
        </div>
    </div>
</div>
