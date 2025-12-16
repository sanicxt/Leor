// Svelte 5 state management using $state rune in .svelte.ts file
import { BLE_CONFIG } from './ble-config';

// Reactive state object
export const bleState = $state({
    connected: false,
    lastStatus: '',
    lastGesture: '',
    shuffleEnabled: true,
    shuffleExprMin: 2,
    shuffleExprMax: 5,
    shuffleNeutralMin: 2,
    shuffleNeutralMax: 5,
    settings: {
        ew: 36, eh: 36, es: 10, er: 8,
        mw: 20, lt: 1000, vt: 2000, bi: 3
    }
});

// BLE device and characteristics
let device: BluetoothDevice | null = null;
let commandChar: BluetoothRemoteGATTCharacteristic | null = null;

// Getters
export function getConnected() { return bleState.connected; }
export function getLastStatus() { return bleState.lastStatus; }
export function getLastGesture() { return bleState.lastGesture; }
export function getShuffleEnabled() { return bleState.shuffleEnabled; }
export function getShuffleExprMin() { return bleState.shuffleExprMin; }
export function getShuffleExprMax() { return bleState.shuffleExprMax; }
export function getShuffleNeutralMin() { return bleState.shuffleNeutralMin; }
export function getShuffleNeutralMax() { return bleState.shuffleNeutralMax; }

// Appearance getters
export function getSettingsEw() { return bleState.settings.ew; }
export function getSettingsEh() { return bleState.settings.eh; }
export function getSettingsEs() { return bleState.settings.es; }
export function getSettingsEr() { return bleState.settings.er; }
export function getSettingsMw() { return bleState.settings.mw; }
export function getSettingsLt() { return bleState.settings.lt; }
export function getSettingsVt() { return bleState.settings.vt; }
export function getSettingsBi() { return bleState.settings.bi; }

// Setters
export function setShuffleEnabled(val: boolean) { bleState.shuffleEnabled = val; }
export function setShuffleExprMin(val: number) { bleState.shuffleExprMin = val; }
export function setShuffleExprMax(val: number) { bleState.shuffleExprMax = val; }
export function setShuffleNeutralMin(val: number) { bleState.shuffleNeutralMin = val; }
export function setShuffleNeutralMax(val: number) { bleState.shuffleNeutralMax = val; }

// Appearance setters
export function setSettingsEw(val: number) { bleState.settings.ew = val; }
export function setSettingsEh(val: number) { bleState.settings.eh = val; }
export function setSettingsEs(val: number) { bleState.settings.es = val; }
export function setSettingsEr(val: number) { bleState.settings.er = val; }
export function setSettingsMw(val: number) { bleState.settings.mw = val; }
export function setSettingsLt(val: number) { bleState.settings.lt = val; }
export function setSettingsVt(val: number) { bleState.settings.vt = val; }
export function setSettingsBi(val: number) { bleState.settings.bi = val; }

export async function connect(): Promise<boolean> {
    try {
        device = await navigator.bluetooth.requestDevice({
            filters: [
                { name: BLE_CONFIG.DEVICE_NAME },
                { services: [BLE_CONFIG.SERVICE_UUID] }
            ],
            optionalServices: [BLE_CONFIG.SERVICE_UUID],
        });

        const server = await device.gatt!.connect();
        const service = await server.getPrimaryService(BLE_CONFIG.SERVICE_UUID);

        // Get characteristics
        commandChar = await service.getCharacteristic(BLE_CONFIG.COMMAND_CHAR_UUID);
        const statusChar = await service.getCharacteristic(BLE_CONFIG.STATUS_CHAR_UUID);
        const gestureChar = await service.getCharacteristic(BLE_CONFIG.GESTURE_CHAR_UUID);

        // Subscribe to status notifications
        await statusChar.startNotifications();
        statusChar.addEventListener('characteristicvaluechanged', (e) => {
            const value = new TextDecoder().decode((e.target as BluetoothRemoteGATTCharacteristic).value!);
            console.log('[BLE RX]', value);
            bleState.lastStatus = value;

            // Parse settings from response
            const settingsParams = ['ew', 'eh', 'es', 'er', 'mw', 'lt', 'vt', 'bi'];
            if (value.includes('=')) {
                settingsParams.forEach(param => {
                    const match = value.match(new RegExp(`${param}=(\\d+(\\.\\d+)?)`));
                    if (match) {
                        const val = parseFloat(match[1]);
                        // safely update nested state
                        (bleState.settings as any)[param] = val;
                    }
                });
            }

            // Robust Shuffle Parsing with Regex
            // Matches "Shuffle: ON" or "Shuffle: OFF" (case insensitive)
            const shuffleMatch = value.match(/shuffle:\s*(on|off)/i);
            if (shuffleMatch) {
                const status = shuffleMatch[1].toUpperCase();
                console.log('Shuffle Parsed:', status);
                bleState.shuffleEnabled = (status === 'ON');
            }

            // Parse expr range: expr=2-5
            const exprMatch = value.match(/expr=(\d+)-(\d+)/);
            if (exprMatch) {
                bleState.shuffleExprMin = parseInt(exprMatch[1]);
                bleState.shuffleExprMax = parseInt(exprMatch[2]);
            }

            // Parse neutral range: neutral=2-5
            const neuMatch = value.match(/neutral=(\d+)-(\d+)/);
            if (neuMatch) {
                bleState.shuffleNeutralMin = parseInt(neuMatch[1]);
                bleState.shuffleNeutralMax = parseInt(neuMatch[2]);
            }
        });

        // Subscribe to gesture notifications
        await gestureChar.startNotifications();
        gestureChar.addEventListener('characteristicvaluechanged', (e) => {
            const value = new TextDecoder().decode((e.target as BluetoothRemoteGATTCharacteristic).value!);
            bleState.lastGesture = value;
        });

        // Handle disconnect
        device.addEventListener('gattserverdisconnected', () => {
            bleState.connected = false;
        });

        bleState.connected = true;

        // Initial sync with safe delays (using short commands)
        setTimeout(async () => {
            console.log('Requesting Shuffle Settings...');
            await sendCommand('sh:');

            // Wait for shuffle response chunks to arrive
            setTimeout(async () => {
                console.log('Requesting Appearance Settings...');
                await sendCommand('s:');
            }, 300);
        }, 300);

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
    bleState.connected = false;
}

export async function sendCommand(cmd: string): Promise<void> {
    if (!commandChar) {
        console.error('Not connected');
        return;
    }
    await commandChar.writeValue(new TextEncoder().encode(cmd));
    console.log('[BLE TX]', cmd);
}

// Check if Web Bluetooth is supported
export function isWebBluetoothSupported(): boolean {
    return typeof navigator !== 'undefined' && 'bluetooth' in navigator;
}
