<script lang="ts">
    import {
        connect,
        disconnect,
        getConnected,
        isWebBluetoothSupported,
    } from "$lib/ble.svelte";
    import ShimmerButton from "$lib/components/ShimmerButton.svelte";

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
            <ShimmerButton
                shimmerColor="#ff4d4d"
                shimmerDuration="2.5s"
                shimmerSize="0.1em"
                background="rgba(15, 12, 12, 1)"
                class="px-5 py-2 min-w-[140px] shadow-red-500/10 border-white/5"
                onclick={handleConnect}
                disabled={connecting}
            >
                <div class="flex items-center gap-2.5">
                    <svg
                        xmlns="http://www.w3.org/2000/svg"
                        width="20"
                        height="20"
                        viewBox="0 0 24 24"
                        fill="none"
                        stroke="currentColor"
                        stroke-width="2"
                        stroke-linecap="round"
                        stroke-linejoin="round"
                        class="text-red-500"
                    >
                        <path
                            d="M2 8a2 2 0 0 1 2-2h16a2 2 0 0 1 2 2v8a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2Z"
                        />
                        <path d="m10 10 5 3-5 3Z" />
                        <path d="M7 21h10" />
                        <path d="m9 18 1 3" />
                        <path d="m15 18-1 3" />
                    </svg>
                    <span class="text-base font-semibold tracking-tight">
                        {connecting ? "Connecting..." : "Connect"}
                    </span>
                </div>
            </ShimmerButton>
        {/if}
    </div>
{/if}
