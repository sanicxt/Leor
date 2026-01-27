// Svelte 5 state management using $state rune in .svelte.ts file
import { BLE_CONFIG } from './ble-config';

// Gesture mapping type
export type GestureMapping = { name: string; action: string };

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
    breathingEnabled: true,
    breathingIntensity: 0.08,
    breathingSpeed: 0.3,
    settings: {
        ew: 36, eh: 36, es: 10, er: 8,
        mw: 20, lt: 1000, vt: 2000, bi: 3,
        gs: 6, os: 12, ss: 10  // gaze speed, openness speed, squish speed
    },
    display: {
        type: 'sh1106',  // sh1106 or ssd1306
        addr: '0x3c'     // I2C address
    },
    gestureMatching: false,
    gestureReactionTime: 1500,
    gestureConfidence: 70,
    gestureCooldown: 2000,
    gestureMappings: [] as GestureMapping[],  // synced from ESP32
    bleLowPowerMode: false  // BLE power saving mode
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
export function getBreathingEnabled() { return bleState.breathingEnabled; }
export function getBreathingIntensity() { return bleState.breathingIntensity; }
export function getBreathingSpeed() { return bleState.breathingSpeed; }

// Appearance getters
export function getSettingsEw() { return bleState.settings.ew; }
export function getSettingsEh() { return bleState.settings.eh; }
export function getSettingsEs() { return bleState.settings.es; }
export function getSettingsEr() { return bleState.settings.er; }
export function getSettingsMw() { return bleState.settings.mw; }
export function getSettingsLt() { return bleState.settings.lt; }
export function getSettingsVt() { return bleState.settings.vt; }
export function getSettingsBi() { return bleState.settings.bi; }
export function getSettingsGs() { return bleState.settings.gs; }
export function getSettingsOs() { return bleState.settings.os; }
export function getSettingsSs() { return bleState.settings.ss; }

export function getGestureMatching() { return bleState.gestureMatching; }
export function getGestureReactionTime() { return bleState.gestureReactionTime; }
export function getGestureConfidence() { return bleState.gestureConfidence; }
export function getGestureCooldown() { return bleState.gestureCooldown; }
export function getGestureMappings() { return bleState.gestureMappings; }

// Setters
export function setShuffleEnabled(val: boolean) { bleState.shuffleEnabled = val; }
export function setShuffleExprMin(val: number) { bleState.shuffleExprMin = val; }
export function setShuffleExprMax(val: number) { bleState.shuffleExprMax = val; }
export function setShuffleNeutralMin(val: number) { bleState.shuffleNeutralMin = val; }
export function setShuffleNeutralMax(val: number) { bleState.shuffleNeutralMax = val; }
export function setBreathingEnabled(val: boolean) { bleState.breathingEnabled = val; }
export function setBreathingIntensity(val: number) { bleState.breathingIntensity = val; }
export function setBreathingSpeed(val: number) { bleState.breathingSpeed = val; }

// Appearance setters
export function setSettingsEw(val: number) { bleState.settings.ew = val; }
export function setSettingsEh(val: number) { bleState.settings.eh = val; }
export function setSettingsEs(val: number) { bleState.settings.es = val; }
export function setSettingsEr(val: number) { bleState.settings.er = val; }
export function setSettingsMw(val: number) { bleState.settings.mw = val; }
export function setSettingsLt(val: number) { bleState.settings.lt = val; }
export function setSettingsVt(val: number) { bleState.settings.vt = val; }
export function setSettingsBi(val: number) { bleState.settings.bi = val; }
export function setSettingsGs(val: number) { bleState.settings.gs = val; }
export function setSettingsOs(val: number) { bleState.settings.os = val; }
export function setSettingsSs(val: number) { bleState.settings.ss = val; }

export function setGestureMatching(val: boolean) { bleState.gestureMatching = val; }
export function setGestureReactionTime(val: number) { bleState.gestureReactionTime = val; }
export function setGestureConfidence(val: number) { bleState.gestureConfidence = val; }
export function setGestureCooldown(val: number) { bleState.gestureCooldown = val; }

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

        // Buffer for chunked status data
        let statusBuffer = '';

        // Subscribe to status notifications
        await statusChar.startNotifications();
        statusChar.addEventListener('characteristicvaluechanged', (e) => {
            const chunk = new TextDecoder().decode((e.target as BluetoothRemoteGATTCharacteristic).value!);
            console.log('[BLE RX Chunk]', chunk);

            // If it starts with '{' or we have stuff in the buffer, we are in a chunked JSON message
            if (chunk.startsWith('{') || (statusBuffer.startsWith('{') && !statusBuffer.endsWith('}'))) {
                statusBuffer += chunk;

                // Try to parse if it looks complete
                if (statusBuffer.endsWith('}')) {
                    try {
                        const data = JSON.parse(statusBuffer);
                        console.log('[BLE RX JSON Sync]', data);

                        if (data.type === 'sync') {
                            if (data.settings) {
                                Object.keys(data.settings).forEach(key => {
                                    if (key in bleState.settings) {
                                        (bleState.settings as any)[key] = data.settings[key];
                                    } else if (key === 'bi') {
                                        bleState.settings.bi = data.settings[key];
                                    }
                                });
                            }
                            if (data.display) {
                                if (data.display.type) bleState.display.type = data.display.type;
                                if (data.display.addr) bleState.display.addr = data.display.addr;
                            }
                            if (data.state) {
                                if ('shuf' in data.state) bleState.shuffleEnabled = (data.state.shuf === 1);
                            }
                        }

                        bleState.lastStatus = 'Sync complete';
                        statusBuffer = ''; // Clear buffer
                        return;
                    } catch (err) {
                        // Not complete yet or invalid JSON, keep buffering
                        console.log('Incomplete JSON chunk...');
                    }
                } else {
                    return; // Wait for more chunks
                }
            }

            // Parse gesture info response: [{"n":"neutral","a":""},{"n":"patpat","a":"happy"},...]
            if (chunk.startsWith('[{"n":')) {
                try {
                    const mappings = JSON.parse(chunk);
                    bleState.gestureMappings = mappings.map((m: { n: string, a: string }) => ({
                        name: m.n,
                        action: m.a
                    }));
                    console.log('[BLE] Gesture mappings synced:', bleState.gestureMappings);
                    return;
                } catch (err) {
                    console.error('Failed to parse gesture mappings:', err);
                }
            }

            // Fallback to legacy line-by-line parsing for non-JSON or broken messages
            const value = chunk;
            bleState.lastStatus = value;

            // Parse appearance and gesture settings
            const params = ['ew', 'eh', 'es', 'er', 'mw', 'lt', 'vt', 'bi', 'gs', 'os', 'ss', 'rt', 'cf', 'cd', 'gm'];
            if (value.includes('=') || value.includes('gs:')) {
                params.forEach(param => {
                    const match = value.match(new RegExp(`${param}=(\\d+(\\.\\d+)?)`));
                    if (match) {
                        const val = parseFloat(match[1]);
                        if (['ew', 'eh', 'es', 'er', 'mw', 'lt', 'vt', 'bi', 'gs', 'os', 'ss'].includes(param)) {
                            (bleState.settings as any)[param] = val;
                        } else if (param === 'rt') {
                            bleState.gestureReactionTime = val;
                        } else if (param === 'cf') {
                            bleState.gestureConfidence = val;
                        } else if (param === 'cd') {
                            bleState.gestureCooldown = val;
                        } else if (param === 'gm') {
                            bleState.gestureMatching = (val === 1);
                        }
                    }
                });
            }

            // Parse BLE low power mode (ble:lp=0 or ble:lp=1)
            const bleLpMatch = value.match(/ble:lp=(\d)/);
            if (bleLpMatch) {
                bleState.bleLowPowerMode = (bleLpMatch[1] === '1');
                console.log('[BLE] Low power mode:', bleState.bleLowPowerMode ? 'ON' : 'OFF');
            }

            // Robust Shuffle Parsing with Regex
            const shuffleMatch = value.match(/shuffle:\s*(on|off)/i);
            if (shuffleMatch) {
                const status = shuffleMatch[1].toUpperCase();
                bleState.shuffleEnabled = (status === 'ON');
            }

            // Parse ranges
            const exprMatch = value.match(/expr=(\d+)-(\d+)/);
            if (exprMatch) {
                bleState.shuffleExprMin = parseInt(exprMatch[1]);
                bleState.shuffleExprMax = parseInt(exprMatch[2]);
            }

            const neuMatch = value.match(/neutral=(\d+)-(\d+)/);
            if (neuMatch) {
                bleState.shuffleNeutralMin = parseInt(neuMatch[1]);
                bleState.shuffleNeutralMax = parseInt(neuMatch[2]);
            }

            // Parse breathing status (br:on/off i=0.08 s=0.3)
            const breathMatch = value.match(/br:(on|off)\s+i=([\d.]+)\s+s=([\d.]+)/);
            if (breathMatch) {
                bleState.breathingEnabled = (breathMatch[1] === 'on');
                bleState.breathingIntensity = parseFloat(breathMatch[2]);
                bleState.breathingSpeed = parseFloat(breathMatch[3]);
                console.log('[BLE] Breathing:', breathMatch[1], 'intensity:', breathMatch[2], 'speed:', breathMatch[3]);
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

                // Request Gesture Settings
                setTimeout(async () => {
                    console.log('Requesting Gesture Settings...');
                    await sendCommand('gs:');

                    // Request Gesture Mappings (gi = gesture info)
                    setTimeout(async () => {
                        console.log('Requesting Gesture Mappings...');
                        await sendCommand('gi');

                        // Request BLE Power Status
                        setTimeout(async () => {
                            console.log('Requesting BLE Power Status...');
                            await sendCommand('ble:');

                            // Request Breathing Status
                            setTimeout(async () => {
                                console.log('Requesting Breathing Status...');
                                await sendCommand('br:');
                            }, 300);
                        }, 300);
                    }, 300);
                }, 300);
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
