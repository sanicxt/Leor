/**
 * ml.ts - TensorFlow.js model for gesture recognition
 * 
 * Model: 75 inputs (25 samples × 3 axes) → 24 hidden → 5 outputs
 * Trains on collected gyro data and exports weights for ESP32
 */

import * as tf from '@tensorflow/tfjs';

// Model configuration (must match ESP32 gesture_trainer.h)
const INPUT_SIZE = 75;  // 25 samples * 3 axes
const HIDDEN_SIZE = 24;
const OUTPUT_SIZE = 5;  // Max gesture classes (must match ESP32)

// Global model instance
let model: tf.Sequential | null = null;

// Training data storage
interface GestureSample {
    data: number[];  // Flattened [x0,x1...x24, y0...y24, z0...z24]
    label: number;   // Class index (0-4)
}

let trainingSamples: GestureSample[] = [];
let gestureLabels: string[] = [];

/**
 * Create the neural network model
 */
export function createModel(): tf.Sequential {
    model = tf.sequential();

    // Hidden layer with ReLU
    model.add(tf.layers.dense({
        inputShape: [INPUT_SIZE],
        units: HIDDEN_SIZE,
        activation: 'relu',
        kernelInitializer: 'glorotUniform'
    }));

    // Output layer with Softmax
    model.add(tf.layers.dense({
        units: OUTPUT_SIZE,
        activation: 'softmax'
    }));

    model.compile({
        optimizer: tf.train.adam(0.01),
        loss: 'categoricalCrossentropy',
        metrics: ['accuracy']
    });

    console.log('[ML] Model created');
    model.summary();

    return model;
}

/**
 * Add a training sample
 */
export function addSample(data: number[], gestureName: string) {
    // Find or create label index
    let labelIndex = gestureLabels.indexOf(gestureName);
    if (labelIndex === -1) {
        if (gestureLabels.length >= OUTPUT_SIZE) {
            console.error('[ML] Max gestures reached');
            return;
        }
        labelIndex = gestureLabels.length;
        gestureLabels.push(gestureName);
    }

    trainingSamples.push({ data, label: labelIndex });
    console.log(`[ML] Added sample for "${gestureName}" (class ${labelIndex}), total: ${trainingSamples.length}`);
}

/**
 * Get current training data stats
 */
export function getStats() {
    const counts: Record<string, number> = {};
    for (const sample of trainingSamples) {
        const name = gestureLabels[sample.label];
        counts[name] = (counts[name] || 0) + 1;
    }
    return {
        totalSamples: trainingSamples.length,
        gestures: gestureLabels,
        samplesPerGesture: counts
    };
}

/**
 * Train the model on collected data
 */
export async function trainModel(
    onProgress?: (epoch: number, loss: number, accuracy: number) => void
): Promise<{ loss: number; accuracy: number }> {
    if (!model) createModel();
    if (trainingSamples.length < 2) {
        throw new Error('Need at least 2 samples to train');
    }

    // Prepare tensors
    const xs = tf.tensor2d(trainingSamples.map(s => s.data));
    const ys = tf.tidy(() => {
        const labels = trainingSamples.map(s => s.label);
        return tf.oneHot(tf.tensor1d(labels, 'int32'), OUTPUT_SIZE);
    });

    console.log('[ML] Training on', trainingSamples.length, 'samples...');

    // Train
    const history = await model!.fit(xs, ys, {
        epochs: 100,
        batchSize: Math.min(32, trainingSamples.length),
        shuffle: true,
        validationSplit: 0.2,
        callbacks: {
            onEpochEnd: (epoch, logs) => {
                if (onProgress && logs) {
                    onProgress(epoch, logs.loss, logs.acc || 0);
                }
            }
        }
    });

    // Cleanup
    xs.dispose();
    ys.dispose();

    const finalLoss = history.history.loss[history.history.loss.length - 1] as number;
    const finalAcc = (history.history.acc?.[history.history.acc.length - 1] || 0) as number;

    console.log(`[ML] Training complete. Loss: ${finalLoss.toFixed(4)}, Accuracy: ${(finalAcc * 100).toFixed(1)}%`);

    return { loss: finalLoss, accuracy: finalAcc };
}

/**
 * Export model weights as base64 for ESP32
 * 
 * Weight layout (must match ESP32):
 * - W1: [INPUT_SIZE × HIDDEN_SIZE] = 75 × 24 = 1800 floats
 * - B1: [HIDDEN_SIZE] = 24 floats
 * - W2: [HIDDEN_SIZE × OUTPUT_SIZE] = 24 × 5 = 120 floats
 * - B2: [OUTPUT_SIZE] = 5 floats
 * Total: 1949 floats
 */
export async function exportWeights(): Promise<string> {
    if (!model) throw new Error('No model to export');

    const weights: number[] = [];

    // Get weights from layers
    const layers = model.layers;

    // Layer 0: Dense (hidden)
    const w1 = layers[0].getWeights()[0];  // Kernel
    const b1 = layers[0].getWeights()[1];  // Bias

    // Layer 1: Dense (output)
    const w2 = layers[1].getWeights()[0];
    const b2 = layers[1].getWeights()[1];

    // Flatten all weights in order
    const w1Data = await w1.data();
    const b1Data = await b1.data();
    const w2Data = await w2.data();
    const b2Data = await b2.data();

    weights.push(...Array.from(w1Data));  // 1800 floats
    weights.push(...Array.from(b1Data));  // 24 floats
    weights.push(...Array.from(w2Data));  // 192 floats
    weights.push(...Array.from(b2Data));  // 8 floats

    console.log(`[ML] Exporting ${weights.length} weights`);

    // Validate weights
    for (let i = 0; i < weights.length; i++) {
        if (!Number.isFinite(weights[i])) {
            console.error(`[ML] Bad weight at index ${i}: ${weights[i]}`);
            throw new Error(`Model training failed: Weight at index ${i} is ${weights[i]}`);
        }
    }

    // Convert to Float32Array and then to base64
    const float32 = new Float32Array(weights);
    const uint8 = new Uint8Array(float32.buffer);

    // Base64 encode
    let binary = '';
    for (let i = 0; i < uint8.length; i++) {
        binary += String.fromCharCode(uint8[i]);
    }
    const base64 = btoa(binary);

    console.log(`[ML] Weights encoded: ${base64.length} chars`);

    return base64;
}

/**
 * Get gesture labels for ESP32
 */
export function getGestureLabels(): string[] {
    return [...gestureLabels];
}

/**
 * Clear all training data
 */
export function clearData() {
    trainingSamples = [];
    gestureLabels = [];
    model = null;
    createModel();  // Recreate fresh model
    console.log('[ML] Training data cleared');
}

/**
 * Delete all samples for a specific gesture
 */
export function deleteGesture(gestureName: string) {
    const labelIndex = gestureLabels.indexOf(gestureName);
    if (labelIndex === -1) return;

    // Remove all samples for this gesture
    trainingSamples = trainingSamples.filter(s => s.label !== labelIndex);

    // Re-index remaining samples if needed
    gestureLabels = gestureLabels.filter((_, i) => i !== labelIndex);
    for (const sample of trainingSamples) {
        if (sample.label > labelIndex) {
            sample.label--;
        }
    }

    console.log(`[ML] Deleted gesture: ${gestureName}`);
}

/**
 * Run inference on a sample (for testing in browser)
 */
export async function predict(data: number[]): Promise<{ label: string; confidence: number }> {
    if (!model) throw new Error('No model');

    const input = tf.tensor2d([data]);
    const output = model.predict(input) as tf.Tensor;
    const predictions = await output.data();

    input.dispose();
    output.dispose();

    let maxIdx = 0;
    let maxVal = predictions[0];
    for (let i = 1; i < predictions.length; i++) {
        if (predictions[i] > maxVal) {
            maxVal = predictions[i];
            maxIdx = i;
        }
    }

    return {
        label: gestureLabels[maxIdx] || `class_${maxIdx}`,
        confidence: maxVal
    };
}

// Initialize model on import
createModel();
