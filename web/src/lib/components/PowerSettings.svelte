<script lang="ts">
    import {
        sendCommand,
        bleState,
        getSettingsTd,
        setSettingsTd,
        getSettingsWp,
        setSettingsWp,
        getSettingsPp,
        setSettingsPp,
    } from "$lib/ble.svelte";

    let touchHoldDelay = $derived(getSettingsTd());
    let wakePin = $derived(getSettingsWp());
    let pwrPin = $derived(getSettingsPp());

    // ESP32-C3 RTC-capable GPIO pins (0-5)
    const rtcPins = [0, 1, 2, 3, 4, 5];

    async function toggleLowPowerMode() {
        const newVal = !bleState.bleLowPowerMode;
        bleState.bleLowPowerMode = newVal;
        await sendCommand(`ble:lp=${newVal ? "1" : "0"}`);
    }

    async function updateTouchHoldDelay(value: number) {
        setSettingsTd(value);
        await sendCommand(`s:td=${value}`);
    }

    async function updateWakePin(value: number) {
        setSettingsWp(value);
        await sendCommand(`s:wp=${value}`);
    }

    async function updatePwrPin(value: number) {
        setSettingsPp(value);
        await sendCommand(`s:pp=${value}`);
    }

    let rebooting = $state(false);

    async function rebootDevice() {
        rebooting = true;
        await sendCommand("restart");
        // Device disconnects on reboot; show spinner briefly
        setTimeout(() => {
            rebooting = false;
        }, 3000);
    }

    // BLE Device Name — synced from bleState (populated when ble:name response arrives)
    let deviceName = $state('');
    let deviceNameSaved = $state(false);
    let deviceNameLoading = $state(false);

    let nameInitialized = false;

    // When connected, request the current name
    $effect(() => {
        if (bleState.connected && !nameInitialized) {
            sendCommand('ble:name'); 
        }
    });

    // Only copy from bleState to local deviceName ONCE 
    // This allows the user to delete/clear the input without it popping back.
    $effect(() => {
        if (bleState.bleDeviceName && !nameInitialized) {
            deviceName = bleState.bleDeviceName;
            nameInitialized = true;
        }
    });

    async function saveDeviceName() {
        if (!deviceName.trim() || !bleState.connected) return;
        deviceNameLoading = true;
        deviceNameSaved = false;
        await sendCommand(`ble:name=${deviceName.trim()}`);
        deviceNameLoading = false;
        deviceNameSaved = true;
        // Update local bleState to reflect the new saved name
        bleState.bleDeviceName = deviceName.trim();
        setTimeout(() => deviceNameSaved = false, 3000);
    }
</script>

<div class="bento-card bg-bento-green p-6 space-y-5">
    <!-- Header -->
    <div class="mb-4 border-b-2 border-bento-border pb-2">
        <h2 class="text-xl font-black uppercase">Power Settings</h2>
        <p class="text-sm font-bold opacity-80">BLE power management</p>
    </div>

    <!-- BLE Device Name -->
    <div class="p-4 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl space-y-3">
        <div class="flex items-center gap-3">
            <div class="w-10 h-10 rounded-lg border-2 border-bento-border bg-bento-pink flex items-center justify-center">
                <svg class="w-5 h-5 text-ink" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                    <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M7 7h.01M7 3h5c.512 0 1.024.195 1.414.586l7 7a2 2 0 010 2.828l-7 7a2 2 0 01-2.828 0l-7-7A2 2 0 013 12V7a4 4 0 014-4z" />
                </svg>
            </div>
            <div>
                <span class="text-ink text-sm font-black uppercase tracking-wider">Device Name</span>
                <p class="text-ink/60 text-xs font-bold">Shown when scanning Bluetooth</p>
            </div>
        </div>
        <div class="flex gap-2">
            <input
                type="text"
                bind:value={deviceName}
                maxlength={30}
                placeholder="e.g. Leor"
                disabled={!bleState.connected}
                class="flex-1 px-3 py-2 bg-paper border-2 border-bento-border rounded-xl text-ink text-sm font-bold font-mono
                       placeholder:text-ink/40 focus:outline-none focus:shadow-[2px_2px_0px_0px_var(--color-bento-border)]
                       disabled:opacity-50 shadow-[2px_2px_0px_0px_var(--color-bento-border)]"
            />
            <button
                onclick={saveDeviceName}
                disabled={!bleState.connected || deviceNameLoading || !deviceName.trim()}
                class="bento-button px-4 py-2 bg-bento-yellow text-ink text-sm disabled:opacity-50"
            >
                {#if deviceNameLoading}
                    Saving…
                {:else if deviceNameSaved}
                    ✓ Saved
                {:else}
                    Save
                {/if}
            </button>
        </div>
        {#if deviceNameSaved}
            <p class="text-ink/60 font-bold text-[10px]">Name saved — restart device to apply</p>
        {/if}
    </div>

    <!-- Touch Hold Delay -->
    <div class="mt-4 p-4 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl space-y-3">
        <div class="flex items-center justify-between">
            <div class="flex items-center gap-3">
                <div class="w-10 h-10 rounded-lg border-2 border-bento-border bg-bento-blue flex items-center justify-center">
                    <svg class="w-5 h-5 text-ink" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                        <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M9 11V6a3 3 0 016 0v5m-6 0h6m-6 0v5a3 3 0 006 0v-5" />
                    </svg>
                </div>
                <div>
                    <div class="text-ink font-black uppercase text-sm">Touch Hold to Sleep</div>
                    <div class="text-ink/60 font-bold text-xs">Long touch duration before deep sleep</div>
                </div>
            </div>
            <span class="text-ink font-mono font-bold text-xs px-2 py-0.5 bg-paper border-2 border-bento-border rounded-lg shadow-[2px_2px_0px_0px_var(--color-bento-border)]">
                {(touchHoldDelay / 1000).toFixed(1)}s
            </span>
        </div>

        <input
            type="range"
            min="1000"
            max="15000"
            step="500"
            value={touchHoldDelay}
            onchange={(e) => updateTouchHoldDelay(parseInt(e.currentTarget.value))}
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

        <div class="flex justify-between text-[10px] text-ink/60 font-bold px-1">
            <span>1s</span>
            <span>15s</span>
        </div>
    </div>

    <!-- Pin Configuration -->
    <div class="mt-3 p-4 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl space-y-4">
        <div class="flex items-center gap-3 mb-1">
            <div class="w-10 h-10 rounded-lg border-2 border-bento-border bg-bento-yellow flex items-center justify-center">
                <svg class="w-5 h-5 text-ink" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                    <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M9 3H5a2 2 0 00-2 2v4m6-6h10a2 2 0 012 2v4M9 3v18m0 0h10a2 2 0 002-2V9M9 21H5a2 2 0 01-2-2V9m0 0h18" />
                </svg>
            </div>
            <div>
                <div class="text-ink font-black uppercase text-sm">GPIO Pin Assignment</div>
                <div class="text-ink/60 font-bold text-xs">RTC-capable only (0–5) · Restart required</div>
            </div>
        </div>

        <!-- Wake Pin -->
        <div class="flex items-center justify-between">
            <div>
                <div class="text-ink font-bold text-sm">Touch / Wake pin</div>
                <div class="text-ink/60 font-bold text-xs">Active-HIGH touch module input</div>
            </div>
            <div class="flex gap-1">
                {#each rtcPins as pin}
                    <button
                        class="w-8 h-8 rounded-lg border-2 border-bento-border text-xs font-mono font-bold transition-all disabled:opacity-50 {wakePin === pin ? 'bg-bento-yellow text-ink shadow-[2px_2px_0px_0px_var(--color-bento-border)]' : 'bg-paper text-ink hover:bg-zinc-100 dark:hover:bg-zinc-800 hover:shadow-[1px_1px_0px_0px_var(--color-bento-border)]'}"
                        onclick={() => updateWakePin(pin)}
                        disabled={!bleState.connected || pin === pwrPin}
                    >{pin}</button>
                {/each}
            </div>
        </div>

        <!-- Power Control Pin -->
        <div class="flex items-center justify-between">
            <div>
                <div class="text-ink font-bold text-sm">Power control pin</div>
                <div class="text-ink/60 font-bold text-xs">PNP base · LOW=on, HIGH=off</div>
            </div>
            <div class="flex gap-1">
                {#each rtcPins as pin}
                    <button
                        class="w-8 h-8 rounded-lg border-2 border-bento-border text-xs font-mono font-bold transition-all disabled:opacity-50 {pwrPin === pin ? 'bg-bento-pink text-ink shadow-[2px_2px_0px_0px_var(--color-bento-border)]' : 'bg-paper text-ink hover:bg-zinc-100 dark:hover:bg-zinc-800 hover:shadow-[1px_1px_0px_0px_var(--color-bento-border)]'}"
                        onclick={() => updatePwrPin(pin)}
                        disabled={!bleState.connected || pin === wakePin}
                    >{pin}</button>
                {/each}
            </div>
        </div>

        <p class="text-[10px] text-ink/60 font-bold flex items-center gap-1">
            <svg class="w-3 h-3 shrink-0" fill="currentColor" viewBox="0 0 20 20">
                <path fill-rule="evenodd" d="M8.257 3.099c.765-1.36 2.722-1.36 3.486 0l5.58 9.92c.75 1.334-.213 2.98-1.742 2.98H4.42c-1.53 0-2.493-1.646-1.743-2.98l5.58-9.92zM11 13a1 1 0 11-2 0 1 1 0 012 0zm-1-8a1 1 0 00-1 1v3a1 1 0 002 0V6a1 1 0 00-1-1z" clip-rule="evenodd" />
            </svg>
            Changes take effect after device restart. Wake and power pins cannot be the same.
        </p>

        <!-- Reboot Button -->
        <button
            onclick={rebootDevice}
            disabled={!bleState.connected || rebooting}
            class="bento-button w-full flex items-center justify-center gap-2 py-2.5 {bleState.connected && !rebooting ? 'bg-bento-peach text-ink' : 'bg-paper text-ink'} transition-all disabled:opacity-50"
        >
            {#if rebooting}
                <span class="flex items-center justify-center gap-2">
                    <svg class="w-4 h-4 animate-spin" fill="none" viewBox="0 0 24 24">
                        <circle class="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" stroke-width="4" />
                        <path class="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8v8z" />
                    </svg>
                    Rebooting...
                </span>
            {:else}
                Reboot Device
            {/if}
        </button>
    </div>

    <!-- Low Power Toggle -->
    <div class="flex items-center justify-between p-4 bg-paper border-2 border-bento-border shadow-[2px_2px_0px_0px_var(--color-bento-border)] rounded-xl">
        <div class="flex items-center gap-3">
            <div class="w-10 h-10 rounded-lg border-2 border-bento-border bg-bento-pink flex items-center justify-center">
                <svg class="w-5 h-5 text-ink" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                    <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M9 3v2m6-2v2M9 19v2m6-2v2M5 9H3m2 6H3m18-6h-2m2 6h-2M7 19h10a2 2 0 002-2V7a2 2 0 00-2-2H7a2 2 0 00-2 2v10a2 2 0 002 2zM9 9h6v6H9V9z" />
                </svg>
            </div>
            <div>
                <div class="text-ink font-black uppercase text-sm">Low Power Mode</div>
                <div class="text-ink/60 font-bold text-xs">Reduce TX power & slower advertising</div>
            </div>
        </div>

        <button
            class="w-14 h-8 rounded-full border-2 border-bento-border transition-all duration-300 relative focus:outline-none disabled:opacity-50 {bleState.bleLowPowerMode ? 'bg-bento-yellow shadow-[2px_2px_0px_0px_var(--color-bento-border)]' : 'bg-paper shadow-[2px_2px_0px_0px_var(--color-bento-border)]'}"
            onclick={toggleLowPowerMode}
            disabled={!bleState.connected}
            aria-label="Toggle low power mode"
        >
            <span class="absolute left-1 top-0.5 w-6 h-6 bg-paper border-[1.5px] border-bento-border rounded-full transition-transform duration-300 flex items-center justify-center {bleState.bleLowPowerMode ? 'translate-x-6' : 'translate-x-0'}">
                {#if bleState.bleLowPowerMode}
                    <svg class="w-3 h-3 text-ink" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                        <path stroke-linecap="round" stroke-linejoin="round" stroke-width="3" d="M5 13l4 4L19 7" />
                    </svg>
                {:else}
                    <svg class="w-3 h-3 text-ink" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                        <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M6 18L18 6M6 6l12 12" />
                    </svg>
                {/if}
            </span>
        </button>
    </div>

    <!-- Power Info -->
    <div class="mt-3 text-xs text-ink/60 font-bold flex items-center gap-2">
        <svg class="w-4 h-4 text-ink" fill="none" stroke="currentColor" viewBox="0 0 24 24">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M13 16h-1v-4h-1m1-4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z" />
        </svg>
        <span>
            {#if bleState.bleLowPowerMode}
                TX Power: -3dBm • Range: ~10m • Slower advertising
            {:else}
                TX Power: +9dBm • Range: ~30m • Fast advertising
            {/if}
        </span>
    </div>
</div>
