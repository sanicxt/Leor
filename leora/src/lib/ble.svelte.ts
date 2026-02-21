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
        gs: 6, os: 12, ss: 10, td: 3000, // gaze speed, openness speed, squish speed, touch hold delay (ms)
        wp: 0, pp: 1  // wake GPIO pin, power-control GPIO pin (RTC-capable, 0-5 on ESP32-C3)
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
// OTA characteristics (populated on connect if OTA service is present)
let otaControlChar: BluetoothRemoteGATTCharacteristic | null = null;
let otaDataChar: BluetoothRemoteGATTCharacteristic | null = null;

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
export function getSettingsTd() { return bleState.settings.td; }
export function getSettingsWp() { return bleState.settings.wp; }
export function getSettingsPp() { return bleState.settings.pp; }

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
export function setSettingsTd(val: number) { bleState.settings.td = val; }
export function setSettingsWp(val: number) { bleState.settings.wp = val; }
export function setSettingsPp(val: number) { bleState.settings.pp = val; }

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
            optionalServices: [BLE_CONFIG.SERVICE_UUID, BLE_CONFIG.OTA_SERVICE_UUID],
        });

        const server = await device.gatt!.connect();
        const service = await server.getPrimaryService(BLE_CONFIG.SERVICE_UUID);

        // Get main characteristics
        commandChar = await service.getCharacteristic(BLE_CONFIG.COMMAND_CHAR_UUID);
        const statusChar = await service.getCharacteristic(BLE_CONFIG.STATUS_CHAR_UUID);
        const gestureChar = await service.getCharacteristic(BLE_CONFIG.GESTURE_CHAR_UUID);

        // Try to get OTA service (non-fatal if not present)
        try {
            const otaService = await server.getPrimaryService(BLE_CONFIG.OTA_SERVICE_UUID);
            otaControlChar = await otaService.getCharacteristic(BLE_CONFIG.OTA_CONTROL_UUID);
            otaDataChar = await otaService.getCharacteristic(BLE_CONFIG.OTA_DATA_UUID);
            console.log('[BLE] OTA service ready');
        } catch (_) {
            otaControlChar = null;
            otaDataChar = null;
            console.warn('[BLE] OTA service not available on this firmware');
        }

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
                            if (data.shuffle) {
                                if ('emin' in data.shuffle) bleState.shuffleExprMin = data.shuffle.emin;
                                if ('emax' in data.shuffle) bleState.shuffleExprMax = data.shuffle.emax;
                                if ('nmin' in data.shuffle) bleState.shuffleNeutralMin = data.shuffle.nmin;
                                if ('nmax' in data.shuffle) bleState.shuffleNeutralMax = data.shuffle.nmax;
                            }
                            if (data.breathing) {
                                if ('on' in data.breathing) bleState.breathingEnabled = (data.breathing.on === 1);
                                if ('i' in data.breathing) bleState.breathingIntensity = parseFloat(data.breathing.i);
                                if ('s' in data.breathing) bleState.breathingSpeed = parseFloat(data.breathing.s);
                            }
                            if (data.ble) {
                                if ('lp' in data.ble) bleState.bleLowPowerMode = (data.ble.lp === 1);
                            }
                            if (data.gesture) {
                                if ('gm' in data.gesture) bleState.gestureMatching = (data.gesture.gm === 1);
                                if ('rt' in data.gesture) bleState.gestureReactionTime = data.gesture.rt;
                                if ('cf' in data.gesture) bleState.gestureConfidence = data.gesture.cf;
                                if ('cd' in data.gesture) bleState.gestureCooldown = data.gesture.cd;
                                if (data.gesture.map) {
                                    bleState.gestureMappings = data.gesture.map.map((m: { n: string, a: string }) => ({
                                        name: m.n,
                                        action: m.a
                                    }));
                                }
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
            const params = ['ew', 'eh', 'es', 'er', 'mw', 'lt', 'vt', 'bi', 'gs', 'os', 'ss', 'td', 'wp', 'pp', 'rt', 'cf', 'cd', 'gm'];
            if (value.includes('=') || value.includes('gs:')) {
                params.forEach(param => {
                    const match = value.match(new RegExp(`${param}=(\\d+(\\.\\d+)?)`));
                    if (match) {
                        const val = parseFloat(match[1]);
                        if (['ew', 'eh', 'es', 'er', 'mw', 'lt', 'vt', 'bi', 'gs', 'os', 'ss', 'td', 'wp', 'pp'].includes(param)) {
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

        // Single unified sync — device returns all state in one JSON payload
        setTimeout(async () => {
            console.log('Requesting full device sync...');
            await sendCommand('s:');
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

// ==================== OTA firmware update ====================

const OTA_CTRL_REQUEST = new Uint8Array([0x01]);
const OTA_CTRL_DONE = new Uint8Array([0x04]);
const OTA_CTRL_REQUEST_ACK = 0x02;
const OTA_CTRL_REQUEST_NAK = 0x03;
const OTA_CTRL_DONE_ACK = 0x05;
const OTA_CTRL_DONE_NAK = 0x06;

export async function sendOTA(
    firmware: ArrayBuffer,
    onProgress: (percent: number, status: string) => void
): Promise<boolean> {
    if (!otaControlChar || !otaDataChar) {
        onProgress(0, 'OTA service not available — flash firmware first.');
        return false;
    }

    // Auto-boost BLE power for OTA if in low power mode.
    // -3dBm can cause packet loss over BLE; switch to +9dBm for the transfer.
    const wasLowPower = bleState.bleLowPowerMode;
    if (wasLowPower) {
        console.log('[OTA] Boosting BLE power for OTA transfer');
        bleState.bleLowPowerMode = false;
        await sendCommand('ble:lp=0');
        // Short delay to let the device apply the new TX power
        await new Promise(r => setTimeout(r, 200));
    }

    // 509 = ATT_MTU(512) - 3 byte ATT header — maximum per-packet payload.
    // Flow control uses OTA_CTRL_CREDIT (0x07) notifications: device ACKs every
    // CREDIT_BATCH packets so the browser can send the next burst without waiting
    // for a per-packet ATT response. This is 10-20× faster than writeValueWithResponse.
    const packetSize = 509;
    const CREDIT_BATCH = 32;  // must match OTA_CREDIT_BATCH in ota_manager.h

    // ── Disconnection watchdog ────────────────────────────────────────────────
    let aborted = false;
    let rejectPending: ((e: Error) => void) | null = null;
    // Credit counter — declared here so onDisconnect can reference it safely
    let credits = 0; // initialised to CREDIT_BATCH once CREDIT_BATCH is in scope below
    let creditWaiters: Array<() => void> = [];
    const onDisconnect = () => {
        aborted = true;
        rejectPending?.(new Error('BLE disconnected during OTA'));
        rejectPending = null;
        // Wake all credit waiters so consumeCredit detects aborted=true and throws
        creditWaiters.splice(0).forEach(w => w());
    };
    device?.addEventListener('gattserverdisconnected', onDisconnect);

    // ── ACK / Credit notification router ─────────────────────────────────────
    // Control char notifies two kinds of messages:
    //   • OTA_CTRL_CREDIT (0x07) — device processed a batch; send next burst
    //   • everything else        — protocol ACK (REQUEST_ACK, DONE_ACK, …)
    let resolveAck: ((val: number) => void) | null = null;
    const OTA_CTRL_CREDIT = 0x07;
    // Seed initial credits so the very first burst sends immediately (no wait).
    credits = CREDIT_BATCH;

    const handler = (e: Event) => {
        const code = (e.target as BluetoothRemoteGATTCharacteristic).value!.getUint8(0);
        console.log('[OTA] Control notify:', code);
        if (code === OTA_CTRL_CREDIT) {
            credits += CREDIT_BATCH;
            // Wake any waiter that was blocked on credits == 0
            const waiters = creditWaiters.splice(0);
            waiters.forEach(w => w());
        } else if (resolveAck) {
            const r = resolveAck; resolveAck = null; rejectPending = null; r(code);
        }
    };

    // Consume one credit, waiting if none are available.
    // Timeout is 15s to account for flash sector erases (~100-200ms each)
    // that can delay the device's credit notification.
    let creditTimeouts = 0;
    const MAX_CREDIT_TIMEOUTS = 3; // give up after 3 consecutive timeouts
    const consumeCredit = (): Promise<void> => {
        if (aborted) return Promise.reject(new Error('BLE disconnected during OTA'));
        if (credits > 0) { credits--; creditTimeouts = 0; return Promise.resolve(); }
        return new Promise<void>((res, rej) => {
            let timeoutId: ReturnType<typeof setTimeout>;
            let check: () => void;
            check = () => {
                if (aborted) { clearTimeout(timeoutId); rej(new Error('BLE disconnected during OTA')); return; }
                if (credits > 0) { credits--; creditTimeouts = 0; clearTimeout(timeoutId); res(); }
                else creditWaiters.push(check); // re-queue if still no credits
            };
            timeoutId = setTimeout(() => {
                const idx = creditWaiters.indexOf(check);
                if (idx !== -1) creditWaiters.splice(idx, 1);
                creditTimeouts++;
                if (creditTimeouts >= MAX_CREDIT_TIMEOUTS) {
                    rej(new Error('Credit timeout — device stopped responding'));
                } else {
                    // Grant a free credit to unstick the pipeline — the device
                    // may have sent a notification we missed (BLE is lossy).
                    console.warn(`[OTA] Credit timeout #${creditTimeouts}/${MAX_CREDIT_TIMEOUTS}, retrying...`);
                    credits += CREDIT_BATCH;
                    creditTimeouts = 0; // reset after granting
                    res();
                }
            }, 15_000);
            creditWaiters.push(check);
        });
    };

    // Must set resolveAck BEFORE the write that triggers the notification
    const waitAck = (timeoutMs: number): Promise<number> =>
        new Promise<number>((res, rej) => {
            if (aborted) return rej(new Error('disconnected'));
            resolveAck = res;
            rejectPending = rej;
            setTimeout(() => {
                if (resolveAck) { resolveAck = null; rejectPending = null; rej(new Error('ACK timeout')); }
            }, timeoutMs);
        });

    const cleanup = async () => {
        device?.removeEventListener('gattserverdisconnected', onDisconnect);
        otaControlChar?.removeEventListener('characteristicvaluechanged', handler);
        try { await otaControlChar!.stopNotifications(); } catch (_) { }
    };

    try {
        // 1. Subscribe to control notifications FIRST (before any writes)
        await otaControlChar.startNotifications();
        otaControlChar.addEventListener('characteristicvaluechanged', handler);

        // 2. Notify device of our chunk size (device uses it for logging only)
        onProgress(0, 'Starting...');
        await otaDataChar.writeValueWithResponse(
            new Uint8Array([packetSize & 0xFF, (packetSize >> 8) & 0xFF])
        );

        // 3. REQUEST — setup ACK waiter BEFORE writing to avoid race
        onProgress(1, 'Requesting OTA...');
        const reqAckPromise = waitAck(10_000);
        await otaControlChar.writeValueWithResponse(OTA_CTRL_REQUEST);
        const reqAck = await reqAckPromise;

        if (reqAck !== OTA_CTRL_REQUEST_ACK) {
            onProgress(0, `OTA request rejected (code ${reqAck}).`);
            await cleanup();
            return false;
        }
        onProgress(2, 'Acknowledged. Sending firmware…');

        // 4. Stream firmware — credit-based writeValueWithoutResponse bursts
        //    • Start with CREDIT_BATCH initial credits (first burst goes immediately)
        //    • After each batch the device notifies OTA_CTRL_CREDIT (0x07) → +8 credits
        //    • consumeCredit() waits only when credits run out (no per-packet round trip)
        //    This lets BLE send multiple PDUs per connection interval (10-20× faster
        //    than writeValueWithResponse while still preventing NimBLE buffer overrun).
        const data = new Uint8Array(firmware);
        const total = data.byteLength;
        const totalPackets = Math.ceil(total / packetSize);
        let offset = 0;
        let packet = 0;

        while (offset < total) {
            if (aborted) throw new Error('BLE disconnected during OTA');

            await consumeCredit(); // rate-limit to device capacity

            const slice = data.slice(offset, offset + packetSize);
            await otaDataChar!.writeValueWithoutResponse(slice);
            offset += slice.byteLength;
            packet++;

            const pct = 2 + Math.floor((offset / total) * 93);
            if (packet % 8 === 0 || offset >= total) {
                const kbDone = (offset / 1024).toFixed(1);
                const kbTotal = (total / 1024).toFixed(1);
                onProgress(pct, `${kbDone} / ${kbTotal} KB  (${packet}/${totalPackets} pkts)`);
            }
        }

        // Give the device 300ms to flush remaining writeValueWithoutResponse packets
        // from the NimBLE host buffer to flash before we send the DONE command.
        await new Promise(r => setTimeout(r, 300));

        // 5. DONE — wait for firmware validation on device side (can take a few seconds)
        onProgress(96, 'Validating firmware…');
        const doneAckPromise = waitAck(30_000);
        // The device may reboot before the ATT Write Response arrives,
        // causing writeValueWithResponse to throw. That's OK — we rely on
        // the DONE_ACK notification (received via doneAckPromise) instead.
        try {
            await otaControlChar.writeValueWithResponse(OTA_CTRL_DONE);
        } catch (_) {
            // GATT error expected if device rebooted — fall through to check ACK
        }
        const doneAck = await doneAckPromise;

        await cleanup();

        if (doneAck === OTA_CTRL_DONE_ACK) {
            onProgress(100, 'OTA complete — device rebooting…');
            return true;
        } else {
            onProgress(96, `Validation failed on device (code ${doneAck}).`);
            return false;
        }

    } catch (err: any) {
        await cleanup();
        // Restore low power if we boosted it and the device is still connected
        if (wasLowPower && !aborted) {
            try {
                bleState.bleLowPowerMode = true;
                await sendCommand('ble:lp=1');
            } catch (_) { /* device may have disconnected */ }
        }
        onProgress(0, `OTA error: ${err?.message ?? err}`);
        return false;
    }
}