# leor Web App - Implementation Guide

> **For AI/Developer**: Build a SvelteKit web app to control leor robot eyes via Web Bluetooth API.

## Tech Stack

- **SvelteKit** with Bun runtime
- **shadcn-svelte** for UI components
- **Web Bluetooth API** for BLE communication
- **TypeScript** for type safety
- **Tailwind CSS** for styling

---

## Setup Commands

```bash
# Create SvelteKit project with Bun
bunx sv create leor-app
cd leor-app

# Install dependencies
bun install

# Add shadcn-svelte
bunx shadcn-svelte@latest init

# Add required components
bunx shadcn-svelte@latest add button card switch slider badge label separator
```

---

## BLE Configuration

```typescript
// src/lib/ble-config.ts
export const BLE_CONFIG = {
  SERVICE_UUID: '4fafc201-1fb5-459e-8fcc-c5c9c331914b',
  COMMAND_CHAR_UUID: 'beb5483e-36e1-4688-b7f5-ea07361b26a8',
  STATUS_CHAR_UUID: '1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e',
  GESTURE_CHAR_UUID: 'd1e5f0a1-2b3c-4d5e-6f7a-8b9c0d1e2f3a',
  DEVICE_NAME: 'leor',
};
```

---

## Project Structure

```text
src/
├── routes/
│   ├── +page.svelte           # Main control page
│   └── +layout.svelte         # App layout with connection status
├── lib/
│   ├── ble-service.ts         # Web Bluetooth service
│   ├── ble-config.ts          # BLE UUIDs
│   └── stores/
│       └── leor.ts           # Svelte stores for state
├── components/
│   ├── ExpressionGrid.svelte  # Expression buttons
│   ├── MouthControls.svelte   # Mouth type buttons
│   ├── LookPad.svelte         # 3x3 directional pad
│   ├── ShufflePanel.svelte    # Shuffle mode controls
│   ├── SettingsPanel.svelte   # Eye/mouth settings
│   └── ConnectionButton.svelte # Connect/disconnect button
└── components/ui/             # shadcn components
```

---

## Web Bluetooth Service

```typescript
// src/lib/ble-service.ts
import { BLE_CONFIG } from './ble-config';
import { connected, lastStatus, lastGesture } from './stores/leor';

let device: BluetoothDevice | null = null;
let commandChar: BluetoothRemoteGATTCharacteristic | null = null;

export async function connect(): Promise<boolean> {
  try {
    device = await navigator.bluetooth.requestDevice({
      filters: [{ name: BLE_CONFIG.DEVICE_NAME }],
      optionalServices: [BLE_CONFIG.SERVICE_UUID],
    });

    const server = await device.gatt!.connect();
    const service = await server.getPrimaryService(BLE_CONFIG.SERVICE_UUID);

    // Get characteristics
    commandChar = await service.getCharacteristic(BLE_CONFIG.COMMAND_CHAR_UUID);
    const statusChar = await service.getCharacteristic(BLE_CONFIG.STATUS_CHAR_UUID);
    const gestureChar = await service.getCharacteristic(BLE_CONFIG.GESTURE_CHAR_UUID);

    // Subscribe to notifications
    await statusChar.startNotifications();
    statusChar.addEventListener('characteristicvaluechanged', (e) => {
      const value = new TextDecoder().decode((e.target as BluetoothRemoteGATTCharacteristic).value);
      lastStatus.set(value);
    });

    await gestureChar.startNotifications();
    gestureChar.addEventListener('characteristicvaluechanged', (e) => {
      const value = new TextDecoder().decode((e.target as BluetoothRemoteGATTCharacteristic).value);
      lastGesture.set(value);
    });

    // Handle disconnect
    device.addEventListener('gattserverdisconnected', () => {
      connected.set(false);
    });

    connected.set(true);
    return true;
  } catch (error) {
    console.error('BLE connection failed:', error);
    return false;
  }
}

export async function disconnect(): Promise<void> {
  if (device?.gatt?.connected) {
    device.gatt.disconnect();
  }
  connected.set(false);
}

export async function sendCommand(cmd: string): Promise<void> {
  if (!commandChar) throw new Error('Not connected');
  await commandChar.writeValue(new TextEncoder().encode(cmd));
}
```

---

## Svelte Stores

```typescript
// src/lib/stores/leor.ts
import { writable } from 'svelte/store';

export const connected = writable(false);
export const lastStatus = writable('');
export const lastGesture = writable('');

export const shuffleEnabled = writable(true);
export const shuffleExpr = writable([2, 5]);
export const shuffleNeutral = writable([2, 5]);
```

---

## Main Components

### ExpressionGrid.svelte

```svelte
<script lang="ts">
  import { Button } from '$lib/components/ui/button';
  import { sendCommand } from '$lib/ble-service';

  const expressions = [
    'happy', 'sad', 'angry', 'love', 'surprised', 'confused',
    'sleepy', 'curious', 'nervous', 'knocked', 'neutral', 'idle'
  ];
</script>

<div class="grid grid-cols-3 gap-2 p-4">
  {#each expressions as expr}
    <Button variant="outline" on:click={() => sendCommand(expr)}>
      {expr}
    </Button>
  {/each}
</div>
```

### ShufflePanel.svelte

```svelte
<script lang="ts">
  import { Card, CardHeader, CardTitle, CardContent } from '$lib/components/ui/card';
  import { Switch } from '$lib/components/ui/switch';
  import { Slider } from '$lib/components/ui/slider';
  import { Label } from '$lib/components/ui/label';
  import { sendCommand } from '$lib/ble-service';
  import { shuffleEnabled, shuffleExpr, shuffleNeutral } from '$lib/stores/leor';

  async function toggleShuffle(checked: boolean) {
    $shuffleEnabled = checked;
    await sendCommand(`shuffle:${checked ? 'on' : 'off'}`);
  }

  async function updateExprTiming() {
    await sendCommand(`shuffle:expr=${$shuffleExpr[0]}-${$shuffleExpr[1]}`);
  }

  async function updateNeutralTiming() {
    await sendCommand(`shuffle:neutral=${$shuffleNeutral[0]}-${$shuffleNeutral[1]}`);
  }
</script>

<Card class="bg-zinc-900 text-white">
  <CardHeader>
    <CardTitle>Shuffle Mode</CardTitle>
  </CardHeader>
  <CardContent class="space-y-4">
    <div class="flex items-center justify-between">
      <Label>Enable Shuffle</Label>
      <Switch checked={$shuffleEnabled} onCheckedChange={toggleShuffle} />
    </div>

    <div class="space-y-2">
      <Label>Expression: {$shuffleExpr[0]}-{$shuffleExpr[1]}s</Label>
      <Slider min={1} max={10} bind:value={$shuffleExpr} onValueCommit={updateExprTiming} />
    </div>

    <div class="space-y-2">
      <Label>Neutral: {$shuffleNeutral[0]}-{$shuffleNeutral[1]}s</Label>
      <Slider min={1} max={10} bind:value={$shuffleNeutral} onValueCommit={updateNeutralTiming} />
    </div>
  </CardContent>
</Card>
```

### LookPad.svelte

```svelte
<script lang="ts">
  import { Button } from '$lib/components/ui/button';
  import { sendCommand } from '$lib/ble-service';

  const directions = [
    ['nw', 'n', 'ne'],
    ['w', 'center', 'e'],
    ['sw', 's', 'se']
  ];
</script>

<div class="grid grid-cols-3 gap-1 w-40">
  {#each directions as row}
    {#each row as dir}
      <Button
        variant={dir === 'center' ? 'default' : 'outline'}
        size="icon"
        class="w-12 h-12"
        on:click={() => sendCommand(dir)}
      >
        {dir === 'center' ? '●' : dir.toUpperCase()}
      </Button>
    {/each}
  {/each}
</div>
```

### ConnectionButton.svelte

```svelte
<script lang="ts">
  import { Button } from '$lib/components/ui/button';
  import { Badge } from '$lib/components/ui/badge';
  import { connect, disconnect } from '$lib/ble-service';
  import { connected } from '$lib/stores/leor';

  let connecting = false;

  async function handleConnect() {
    connecting = true;
    await connect();
    connecting = false;
  }
</script>

<div class="flex items-center gap-2">
  <Badge variant={$connected ? 'default' : 'destructive'}>
    {$connected ? '● Connected' : '○ Disconnected'}
  </Badge>

  {#if $connected}
    <Button variant="outline" on:click={disconnect}>Disconnect</Button>
  {:else}
    <Button on:click={handleConnect} disabled={connecting}>
      {connecting ? 'Connecting...' : 'Connect'}
    </Button>
  {/if}
</div>
```

---

## Main Page

```svelte
<!-- src/routes/+page.svelte -->
<script lang="ts">
  import ConnectionButton from '$lib/components/ConnectionButton.svelte';
  import ExpressionGrid from '$lib/components/ExpressionGrid.svelte';
  import ShufflePanel from '$lib/components/ShufflePanel.svelte';
  import LookPad from '$lib/components/LookPad.svelte';
  import { connected } from '$lib/stores/leor';
</script>

<div class="min-h-screen bg-zinc-950 text-white p-6">
  <header class="flex justify-between items-center mb-8">
    <h1 class="text-2xl font-bold">leor Control</h1>
    <ConnectionButton />
  </header>

  {#if $connected}
    <div class="grid md:grid-cols-2 gap-6">
      <div>
        <h2 class="text-lg font-semibold mb-4">Expressions</h2>
        <ExpressionGrid />
      </div>

      <div class="space-y-6">
        <ShufflePanel />
        <div>
          <h2 class="text-lg font-semibold mb-4">Look Direction</h2>
          <LookPad />
        </div>
      </div>
    </div>
  {:else}
    <div class="text-center py-20 text-zinc-500">
      <p class="text-xl">Click "Connect" to pair with leor</p>
      <p class="text-sm mt-2">Make sure leor is powered on and Bluetooth is enabled</p>
    </div>
  {/if}
</div>
```

---

## Command Reference

| Category | Commands |
|----------|----------|
| Expressions | `happy`, `sad`, `angry`, `love`, `surprised`, `confused`, `sleepy`, `curious`, `nervous`, `knocked`, `neutral`, `idle` |
| Mouth | `smile`, `frown`, `open`, `ooo`, `flat`, `talk`, `chew`, `wobble` |
| Actions | `blink`, `wink`, `winkr`, `laugh`, `cry` |
| Direction | `n`, `ne`, `e`, `se`, `s`, `sw`, `w`, `nw`, `center` |
| Shuffle | `shuffle:on`, `shuffle:off`, `shuffle:expr=N-M`, `shuffle:neutral=N-M` |
| Settings | `set:ew=N,eh=N,es=N,er=N,mw=N,bi=N` |

---

## Browser Requirements

Web Bluetooth only works in:

- Chrome/Edge (desktop & Android)
- Chrome on macOS
- **NOT supported**: Firefox, Safari, iOS browsers

Must be served over HTTPS (or localhost for development).

---

## Run Development Server

```bash
bun run dev
```

---

## Reference Files

- **BLE API Details**: See `BLE_CONTROL_GUIDE.md`
- **Firmware Source**: `pio.ino`, `commands.h`
