<script lang="ts">
    import {
        connect,
        disconnect,
        getConnected,
        isWebBluetoothSupported,
    } from "$lib/ble.svelte";

    let connecting = $state(false);

    async function handleConnect() {
        connecting = true;
        await connect();
        connecting = false;
    }
</script>

{#if !isWebBluetoothSupported()}
    <button
        class="px-4 py-1.5 bg-zinc-800 text-zinc-500 rounded-full text-sm font-medium border border-zinc-700 cursor-not-allowed opacity-75"
        disabled
        title="Web Bluetooth API is not available in this browser"
    >
        Not Supported
    </button>
{:else}
    <div class="flex items-center gap-3">
        <div
            class="flex items-center gap-2 px-3 py-1.5 rounded-full bg-zinc-900/50 border border-white/5 backdrop-blur-sm transition-colors
                {getConnected()
                ? 'border-emerald-500/20 bg-emerald-500/5'
                : ''}"
        >
            <span class="relative flex h-2 w-2">
                {#if getConnected()}
                    <span
                        class="animate-ping absolute inline-flex h-full w-full rounded-full bg-emerald-400 opacity-75"
                    ></span>
                {/if}
                <span
                    class="relative inline-flex rounded-full h-2 w-2 {getConnected()
                        ? 'bg-emerald-400 shadow-[0_0_8px_rgba(52,211,153,0.5)]'
                        : 'bg-zinc-600'}"
                ></span>
            </span>
            <span
                class="text-xs font-medium {getConnected()
                    ? 'text-emerald-400'
                    : 'text-zinc-500'}"
            >
                {getConnected() ? "Connected" : "Disconnected"}
            </span>
        </div>

        {#if getConnected()}
            <button
                class="px-4 py-1.5 bg-white/5 hover:bg-white/10 text-zinc-300 hover:text-white rounded-full
               border border-white/5 hover:border-white/20 text-sm transition-all shadow-sm backdrop-blur-sm"
                onclick={disconnect}
            >
                Disconnect
            </button>
        {:else}
            <button
                class="px-5 py-1.5 bg-gradient-to-r from-indigo-500 to-purple-500 hover:from-indigo-400 hover:to-purple-400
               text-white rounded-full text-sm font-medium transition-all shadow-lg shadow-indigo-500/20
               hover:shadow-indigo-500/30 hover:-translate-y-0.5 disabled:opacity-50 disabled:cursor-not-allowed"
                onclick={handleConnect}
                disabled={connecting}
            >
                {connecting ? "Connecting..." : "Connect"}
            </button>
        {/if}
    </div>
{/if}
