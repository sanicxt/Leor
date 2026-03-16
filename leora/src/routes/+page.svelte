<script lang="ts">
  import { fly } from "svelte/transition";
  import { onMount } from "svelte";
  import MasterBackground from "$lib/components/MasterBackground.svelte";
  import AnimatedTabs from "$lib/components/AnimatedTabs.svelte";
  
  import Wifi from "lucide-svelte/icons/wifi";
  import Moon from "lucide-svelte/icons/moon";
  import Sun from "lucide-svelte/icons/sun";

  // Components
  import ExpressionGrid from "$lib/components/ExpressionGrid.svelte";
  import MouthControls from "$lib/components/MouthControls.svelte";
  import ActionButtons from "$lib/components/ActionButtons.svelte";
  import LookPad from "$lib/components/LookPad.svelte";
  import ShufflePanel from "$lib/components/ShufflePanel.svelte";
  import AppearanceSettings from "$lib/components/AppearanceSettings.svelte";
  import DisplaySettings from "$lib/components/DisplaySettings.svelte";
  import GestureManager from "$lib/components/GestureManager.svelte";
  import GestureSettings from "$lib/components/GestureSettings.svelte";
  import PowerSettings from "$lib/components/PowerSettings.svelte";
  import BreathingControl from "$lib/components/BreathingControl.svelte";
  import OtaPanel from "$lib/components/OtaPanel.svelte";
  
  import {
    getConnected,
    getLastStatus,
    getLastGesture,
    connect,
    disconnect,
    bleState,
  } from "$lib/ble.svelte";

  let activeTab = $state("home"); // 'home' | 'settings' | 'gestures'
  let isDarkMode = $state(false);

  onMount(() => {
    // Initialize dark mode based on existing class or system preference
    isDarkMode = document.documentElement.classList.contains("dark") || 
      (!document.documentElement.classList.contains("light") && window.matchMedia("(prefers-color-scheme: dark)").matches);
    
    // Apply explicitly on mount to ensure variables are set
    if (isDarkMode) {
      document.documentElement.classList.add("dark");
      document.documentElement.classList.remove("light");
    } else {
      document.documentElement.classList.remove("dark");
      document.documentElement.classList.add("light");
    }
  });

  function toggleDarkMode() {
    isDarkMode = !isDarkMode;
    if (isDarkMode) {
      document.documentElement.classList.add("dark");
      document.documentElement.classList.remove("light");
    } else {
      document.documentElement.classList.remove("dark");
      document.documentElement.classList.add("light");
    }
  }

  async function handleConnect() {
    if (getConnected()) {
      await disconnect();
    } else {
      await connect();
    }
  }
</script>

<svelte:head>
  <title>Leor OS</title>
</svelte:head>

<div class="min-h-screen bg-paper text-ink font-sans selection:bg-bento-pink overflow-hidden">
  <MasterBackground />

  <main class="relative z-10 h-screen overflow-y-auto pb-32">
    <div class="p-4 md:p-8 max-w-7xl mx-auto min-h-full">
      
      <!-- Top Bento Header -->
      <header class="bento-card bg-paper p-5 mb-8 flex justify-between items-center flex-wrap gap-4">
        <div class="flex items-center gap-4">
          <div class="w-14 h-14 bg-bento-peach border-4 border-ink rounded-2xl flex items-center justify-center shadow-[2px_2px_0px_0px_var(--color-ink)]">
             <!-- Simplified smiling robot avatar -->
             <svg class="w-8 h-8 text-ink" viewBox="0 0 24 24" fill="currentColor" stroke="currentColor" stroke-width="2">
                <rect x="3" y="6" width="18" height="12" rx="3" fill="white" />
                <circle cx="8" cy="11" r="1.5" fill="var(--color-ink)" />
                <circle cx="16" cy="11" r="1.5" fill="var(--color-ink)" />
                <path d="M9 15 Q12 17 15 15" fill="none" />
                <path d="M6 6 V4 M18 6 V4" stroke-width="2"/>
             </svg>
          </div>
          <div>
            <h1 class="text-2xl font-black uppercase tracking-tight">Leor OS</h1>
            <div class="flex items-center gap-2 mt-1">
              {#if getConnected()}
                <span class="w-3 h-3 bg-bento-green border-2 border-ink rounded-full animate-pulse"></span>
                <span class="text-sm font-bold">Connected</span>
              {:else}
                <span class="w-3 h-3 bg-bento-pink border-2 border-ink rounded-full"></span>
                <span class="text-sm font-bold opacity-70">Disconnected</span>
              {/if}
            </div>
          </div>
        </div>

        <div class="flex items-center gap-4">
            {#if getConnected() && getLastGesture()}
                <div class="hidden sm:flex bg-bento-yellow border-2 border-ink px-3 py-1 rounded-xl">
                    <span class="font-bold text-sm">Gaze: {getLastGesture()}</span>
                </div>
            {/if}
            <button onclick={toggleDarkMode} class="bento-button bg-paper px-3 py-3 flex items-center justify-center text-ink w-[52px]" aria-label="Toggle Dark Mode">
                {#if isDarkMode}
                    <Sun class="w-5 h-5" />
                {:else}
                    <Moon class="w-5 h-5" />
                {/if}
            </button>
            <button onclick={handleConnect} class="bento-button bg-bento-blue px-6 py-3 flex items-center gap-2 text-ink">
                <Wifi class="w-5 h-5" />
                <span>{getConnected() ? 'Connected' : 'Connect'}</span>
            </button>
        </div>
      </header>

      {#if activeTab === "home"}
        <div in:fly={{ y: 20, duration: 300 }} class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6">
          
          <!-- Expressions (Spans 2 columns) -->
          <div class="bento-card bg-bento-pink p-6 lg:col-span-2">
             <div class="mb-4 border-b-2 border-ink pb-2">
                <h2 class="text-xl font-black uppercase">Expressions</h2>
                <p class="text-sm font-bold opacity-80">Tap to trigger</p>
             </div>
             <!-- The nested component will be updated next to remove its inner backgrounds -->
             <ExpressionGrid />
          </div>

          <!-- Quick Actions -->
          <div class="bento-card bg-bento-yellow p-6 md:col-span-1">
             <div class="mb-4 border-b-2 border-ink pb-2">
                 <h2 class="text-xl font-black uppercase">Quick Actions</h2>
                 <p class="text-sm font-bold opacity-80">Overrides</p>
             </div>
             <ActionButtons />
          </div>

          <!-- Gaze Control -->
          <div class="bento-card bg-bento-green p-6 md:col-span-1 lg:row-span-2 flex flex-col">
             <div class="mb-4 border-b-2 border-ink pb-2">
                 <h2 class="text-xl font-black uppercase">Gaze Control</h2>
                 <p class="text-sm font-bold opacity-80">Drag to look</p>
             </div>
             <div class="flex-1 flex items-center justify-center">
                 <LookPad />
             </div>
          </div>

          <!-- Mouth Control Array -->
          <div class="bento-card bg-bento-blue p-6 lg:col-span-3">
             <div class="mb-4 border-b-2 border-ink pb-2">
                 <h2 class="text-xl font-black uppercase">Mouth Control</h2>
                 <p class="text-sm font-bold opacity-80">Shape & Type</p>
             </div>
             <MouthControls />
          </div>

        </div>
      {:else if activeTab === "settings"}
        <div in:fly={{ y: 20, duration: 300 }} class="space-y-6">
          <div class="grid grid-cols-1 md:grid-cols-2 gap-6">
            <ShufflePanel />
            <GestureSettings />
          </div>
          <AppearanceSettings />
          <DisplaySettings />
          <BreathingControl />
          <PowerSettings />
        </div>
      {:else if activeTab === "gestures"}
        <div in:fly={{ y: 20, duration: 300 }}>
          <GestureManager />
        </div>
      {:else if activeTab === "ota"}
        <div in:fly={{ y: 20, duration: 300 }} class="max-w-2xl mx-auto">
          <OtaPanel />
        </div>
      {/if}

      <!-- Bottom Spacer -->
      <div class="h-32"></div>
    </div>
  </main>

  <!-- Navigation Tabs -->
  <div class="fixed bottom-8 left-1/2 -translate-x-1/2 z-50 pointer-events-none">
    <div class="pointer-events-auto">
      <AnimatedTabs
        bind:activeTab
        tabs={[
          { id: "home", title: "Home" },
          { id: "settings", title: "Settings" },
          { id: "gestures", title: "Gestures" },
          { id: "ota", title: "Update" },
        ]}
      />
    </div>
  </div>
</div>
