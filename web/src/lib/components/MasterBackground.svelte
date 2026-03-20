<script lang="ts">
    import { bleState } from "$lib/ble.svelte";
</script>

<div class="fixed inset-0 z-0 w-full h-full bg-paper transition-colors duration-500 pointer-events-none">
    <!-- Subtle dot grid for bento aesthetics -->
    <div 
        class="absolute inset-0 opacity-[0.06] dark:opacity-20" 
        style="background-image: radial-gradient(var(--color-ink) 2px, transparent 2px); background-size: 32px 32px;">
    </div>
</div>
