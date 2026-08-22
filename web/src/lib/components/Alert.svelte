<script lang="ts">
  import type { Snippet } from 'svelte';

  let {
    variant = 'error',
    dismissable = true,
    ondismiss,
    children,
  }: {
    variant?: 'error' | 'warning' | 'success';
    dismissable?: boolean;
    ondismiss?: () => void;
    children: Snippet;
  } = $props();

  const variants = {
    error: { bg: 'bg-bento-pink', icon: '⚠', label: 'Error' },
    warning: { bg: 'bg-bento-yellow', icon: '⚠', label: 'Warning' },
    success: { bg: 'bg-bento-green', icon: '✓', label: 'Success' },
  } as const;

  const v = $derived(variants[variant]);
</script>

<div class="bento-card {v.bg} p-3 flex items-start gap-2">
  <span class="text-lg font-black leading-none mt-0.5 shrink-0">{v.icon}</span>
  <div class="flex-1 min-w-0 pt-0.5">
    {@render children()}
  </div>
  {#if dismissable}
    <button
      type="button"
      class="shrink-0 font-black text-lg leading-none opacity-60 hover:opacity-100 transition-opacity"
      aria-label="Dismiss"
      onclick={ondismiss}
    >×</button>
  {/if}
</div>