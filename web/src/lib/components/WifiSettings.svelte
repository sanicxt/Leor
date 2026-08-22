<script lang="ts">
	import { getWifiSsid, getWifiStatus, saveWifiCredentials, sendWifiSync } from '$lib/ble.svelte';

	let ssid = $derived(getWifiSsid());
	let status = $derived(getWifiStatus());
	let ssidInput = $state('');
	let passInput = $state('');
	let saving = $state(false);
	let syncing = $state(false);

	async function save() {
		saving = true;
		await saveWifiCredentials(ssidInput, passInput);
		saving = false;
	}

	async function syncNow() {
		syncing = true;
		await sendWifiSync();
		setTimeout(() => (syncing = false), 3000);
	}
</script>

<div class="bento-card bg-paper p-4">
	<div class="mb-4 border-b-2 border-bento-border pb-2">
		<h2 class="font-display text-xl uppercase">WiFi Time Sync</h2>
		<p class="text-sm font-bold opacity-80">One-shot NTP sync, then WiFi disconnects</p>
	</div>
	<div class="flex flex-col gap-3">
		<input
			bind:value={ssidInput}
			placeholder="SSID"
			class="rounded-xl border-2 border-bento-border bg-bento-pink px-3 py-2 text-sm font-bold"
		/>
		<input
			bind:value={passInput}
			type="password"
			placeholder="Password"
			class="rounded-xl border-2 border-bento-border bg-bento-pink px-3 py-2 text-sm font-bold"
		/>
		<div class="flex gap-2">
			<button
				class="bento-button bg-bento-blue px-4 py-2 text-sm font-bold uppercase"
				onclick={save}
				disabled={saving}
			>
				{saving ? 'Saving...' : 'Save'}
			</button>
			<button
				class="bento-button bg-bento-peach px-4 py-2 text-sm font-bold uppercase"
				onclick={syncNow}
				disabled={syncing}
			>
				{syncing ? 'Syncing...' : 'Sync now'}
			</button>
		</div>
		<p class="text-xs font-bold uppercase tracking-widest opacity-80">
			SSID: {ssid || '—'} · Status: {status || '—'}
		</p>
	</div>
</div>
