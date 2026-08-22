<script lang="ts">
	import {
		getWifiSsid,
		getWifiStatus,
		getWifiMode,
		getWifiPass,
		getWifiAps,
		setWifiMode,
		saveWifiCredentials,
		sendWifiScan,
		fetchWifiCredentials,
		sendCommand
	} from '$lib/ble.svelte';
	import { bleState } from '$lib/ble.svelte';

	let ssid = $derived(getWifiSsid());
	let status = $derived(getWifiStatus());
	let wifiMode = $derived(getWifiMode());
	let aps = $derived(getWifiAps());
	let selectedSsid = $state('');
	let passInput = $state('');
	let saving = $state(false);
	let scanning = $state(false);
	let showPass = $state(false);

	function pickAp(apSsid: string) {
		selectedSsid = apSsid;
	}

	async function scanNow() {
		scanning = true;
		await sendWifiScan();
		setTimeout(() => (scanning = false), 3000);
	}

	async function save() {
		saving = true;
		await saveWifiCredentials(selectedSsid, passInput);
		saving = false;
	}

	async function syncNow() {
		if (wifiMode !== 1) {
			setWifiMode(1);
		}
		await sendCommand('restart');
	}

	async function revealPass() {
		showPass = !showPass;
		if (showPass) {
			await fetchWifiCredentials();
			setTimeout(() => {
				passInput = getWifiPass();
			}, 600);
		}
	}

	const modeOptions = [
		{ label: 'OFF', value: 0 },
		{ label: 'ON', value: 1 }
	];
</script>

<div class="bento-card bg-paper p-4">
	<div class="mb-4 border-b-2 border-bento-border pb-2">
		<h2 class="font-display text-xl uppercase">WiFi Time Sync</h2>
		<p class="text-sm font-bold opacity-80">One-shot NTP sync, then WiFi disconnects</p>
	</div>
	<div class="flex flex-col gap-3">
		<div class="flex flex-col gap-1.5">
			<span class="text-xs font-bold uppercase tracking-widest opacity-80">Enabled</span>
			<div class="flex gap-1">
				{#each modeOptions as mode (mode.value)}
					<button
						class="flex-1 h-9 rounded-lg border-2 border-bento-border text-xs font-black uppercase transition-all disabled:opacity-50 {wifiMode ===
						mode.value
							? 'bg-bento-green text-ink shadow-[2px_2px_0px_0px_var(--color-bento-border)]'
							: 'bg-paper text-ink hover:bg-zinc-100 dark:hover:bg-zinc-800 hover:shadow-[1px_1px_0px_0px_var(--color-bento-border)]'}"
						onclick={() => setWifiMode(mode.value)}
						disabled={!bleState.connected}
					>
						{mode.label}
					</button>
				{/each}
			</div>
		</div>
		<div class="flex flex-col gap-1.5">
			<span class="text-xs font-bold uppercase tracking-widest opacity-80">Network</span>
			<button
				class="bento-button bg-bento-blue px-4 py-2 text-sm font-bold uppercase"
				onclick={scanNow}
				disabled={scanning || !bleState.connected}
			>
				{scanning ? 'Scanning...' : 'Scan networks'}
			</button>
			{#if aps.length > 0}
				<div class="flex max-h-40 flex-col gap-1 overflow-y-auto rounded-xl border-2 border-bento-border bg-bento-pink p-2">
					{#each aps as ap (ap.ssid + ap.rssi)}
						<button
							class="flex items-center justify-between rounded-lg px-3 py-2 text-left text-sm font-bold transition-all {selectedSsid ===
							ap.ssid
								? 'bg-bento-green text-ink shadow-[2px_2px_0px_0px_var(--color-bento-border)]'
								: 'bg-paper text-ink hover:bg-zinc-100 dark:hover:bg-zinc-800'}"
							onclick={() => pickAp(ap.ssid)}
						>
							<span class="truncate">{ap.ssid}</span>
							<span class="text-xs opacity-60">{ap.rssi} dBm</span>
						</button>
					{/each}
				</div>
			{:else if scanning}
				<p class="text-xs font-bold opacity-60">Scanning for networks...</p>
			{/if}
		</div>
		<div class="flex flex-col gap-1.5">
			<span class="text-xs font-bold uppercase tracking-widest opacity-80">Selected</span>
			<input
				bind:value={selectedSsid}
				placeholder="Pick a network above"
				readonly
				class="rounded-xl border-2 border-bento-border bg-bento-pink px-3 py-2 text-sm font-bold"
			/>
		</div>
		<div class="flex gap-2">
			<input
				bind:value={passInput}
				type={showPass ? 'text' : 'password'}
				placeholder="Password"
				class="flex-1 rounded-xl border-2 border-bento-border bg-bento-pink px-3 py-2 text-sm font-bold"
			/>
			<button
				class="bento-button bg-bento-yellow px-3 py-2 text-sm font-bold uppercase"
				onclick={revealPass}
				disabled={!bleState.connected}
			>
				{showPass ? 'Hide' : 'Show'}
			</button>
		</div>
		<div class="flex gap-2">
			<button
				class="bento-button bg-bento-blue px-4 py-2 text-sm font-bold uppercase"
				onclick={save}
				disabled={saving || !selectedSsid || !bleState.connected}
			>
				{saving ? 'Saving...' : 'Save'}
			</button>
			<button
				class="bento-button bg-bento-peach px-4 py-2 text-sm font-bold uppercase"
				onclick={syncNow}
				disabled={!bleState.connected}
			>
				Restart & sync
			</button>
		</div>
		<p class="text-xs font-bold uppercase tracking-widest opacity-80">
			SSID: {ssid || '—'} · Status: {status || '—'}
		</p>
	</div>
</div>
