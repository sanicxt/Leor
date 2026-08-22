<script lang="ts">
  import { bleState } from "$lib/ble.svelte";

  const items = [
    { key: 'display', label: 'Display' },
    { key: 'gyro', label: 'Gyro' },
    { key: 'buzzer', label: 'Buzzer' },
    { key: 'touch', label: 'Touch' },
    { key: 'power', label: 'Power' },
  ] as const;

  function statusText(v: number) {
    return v === 0 ? 'OK' : v === 1 ? 'Absent' : 'Failed';
  }
  function statusClass(v: number) {
    return v === 0 ? 'bg-bento-green' : 'bg-bento-pink';
  }
</script>

<div class="bento-card bg-paper p-4">
  <div class="mb-4 border-b-2 border-bento-border pb-2">
    <h2 class="font-display text-xl uppercase">Hardware</h2>
    <p class="text-sm font-bold opacity-80">Detected peripherals</p>
  </div>
  <div class="grid grid-cols-2 md:grid-cols-5 gap-3">
    {#each items as item}
      <div class="flex flex-col items-center gap-1 rounded-xl border-2 border-bento-border p-3">
        <span class="w-3 h-3 rounded-full {statusClass(bleState.hardware[item.key])}"></span>
        <span class="text-xs font-bold uppercase">{item.label}</span>
        <span class="text-[10px] font-bold opacity-70">{statusText(bleState.hardware[item.key])}</span>
      </div>
    {/each}
  </div>
</div>
