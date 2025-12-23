/**
 * ml.ts - 1D Conv Gesture Recognition
 * 
 * Simple 1D Convolution model that can be easily inferenced on ESP32.
 * Architecture: Input(150) -> Dense(16) -> ReLU -> Dense(N) -> Softmax
 * 
 * This simple dense network can be implemented as matrix multiply on ESP32.
 * Weights are exported as: [W1: 150x16, B1: 16, W2: 16xN, B2: N]
 */

import * as tf from '@tensorflow/tfjs';

// Model configuration (must match ESP32 gesture_trainer.h)
export const SAMPLE_COUNT = 50;   // 2 seconds @ 40ms interval
export const INPUT_SIZE = 150;    // 50 samples * 3 axes
export const HIDDEN_SIZE = 16;    // Hidden layer size
export const OUTPUT_SIZE = 5;     // Max gesture classes

// Training data storage
interface GestureSample {
    data: number[];  // Flattened [x0..x49, y0..y49, z0..z49]
    label: number;   // Class index (0-4)
}

let trainingSamples: GestureSample[] = [];
let gestureLabels: string[] = [];
let model: tf.Sequential | null = null;

/**
 * Create a simple dense network for gesture classification
 * This architecture is easy to implement in pure C++ on ESP32
 */
export function createModel(numClasses: number = OUTPUT_SIZE) {
    model = tf.sequential({
        layers: [
            tf.layers.dense({
                inputShape: [INPUT_SIZE],
                units: HIDDEN_SIZE,
                activation: 'relu',
                name: 'hidden'
            }),
            tf.layers.dense({
                units: numClasses,
                activation: 'softmax',
                name: 'output'
            })
        ]
    });

    model.compile({
        optimizer: tf.train.adam(0.001),
        loss: 'categoricalCrossentropy',
        metrics: ['accuracy']
    });

    console.log(`[ML] Simple dense model created: ${INPUT_SIZE} -> ${HIDDEN_SIZE} -> ${numClasses}`);
    return model;
}

/**
 * Add a training sample
 */
export function addSample(data: number[], gestureName: string) {
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
 * Train the neural network model
 */
export async function trainModel(
    onProgress?: (epoch: number, loss: number, accuracy: number) => void
): Promise<{ loss: number; accuracy: number }> {
    if (trainingSamples.length < 2) {
        throw new Error('Need at least 2 samples to train');
    }

    const numClasses = gestureLabels.length;

    // Dispose old model if exists
    if (model) {
        model.dispose();
        model = null;
    }

    // Create simple dense model
    createModel(numClasses);

    // Prepare training data
    const xs = tf.tensor2d(trainingSamples.map(s => s.data));
    const ys = tf.oneHot(tf.tensor1d(trainingSamples.map(s => s.label), 'int32'), numClasses);

    console.log(`[ML] Training on ${trainingSamples.length} samples, ${numClasses} classes`);

    // Train the model
    const history = await model!.fit(xs, ys, {
        epochs: 100,
        batchSize: Math.min(32, trainingSamples.length),
        validationSplit: 0.2,
        shuffle: true,
        callbacks: {
            onEpochEnd: (epoch, logs) => {
                if (onProgress && logs) {
                    onProgress(epoch + 1, logs.loss as number, logs.acc as number);
                }
            }
        }
    });

    // Cleanup tensors
    xs.dispose();
    ys.dispose();

    const finalLoss = history.history.loss[history.history.loss.length - 1] as number;
    const finalAcc = history.history.acc[history.history.acc.length - 1] as number;

    console.log(`[ML] Training complete. Loss: ${finalLoss.toFixed(4)}, Accuracy: ${(finalAcc * 100).toFixed(1)}%`);

    return { loss: finalLoss, accuracy: finalAcc };
}

/**
 * Export model weights as base64 for ESP32
 * Format: [W1 (150*16), B1 (16), W2 (16*N), B2 (N)]
 * All as Float32 little-endian
 */
export async function exportWeights(): Promise<string> {
    if (!model) {
        throw new Error('No model trained yet');
    }

    const weights = model.getWeights();
    // weights[0] = W1 (150, 16), weights[1] = B1 (16)
    // weights[2] = W2 (16, N),  weights[3] = B2 (N)

    const allWeights: number[] = [];

    for (const w of weights) {
        const data = await w.data();
        allWeights.push(...Array.from(data));
    }

    const numClasses = gestureLabels.length;
    const expectedSize = (INPUT_SIZE * HIDDEN_SIZE) + HIDDEN_SIZE + (HIDDEN_SIZE * numClasses) + numClasses;

    console.log(`[ML] Exporting ${allWeights.length} weights (expected ${expectedSize})`);
    console.log(`[ML] W1: ${INPUT_SIZE}x${HIDDEN_SIZE}, B1: ${HIDDEN_SIZE}, W2: ${HIDDEN_SIZE}x${numClasses}, B2: ${numClasses}`);

    // Convert to Float32Array and then to base64
    const float32 = new Float32Array(allWeights);
    const uint8 = new Uint8Array(float32.buffer);

    let binary = '';
    for (let i = 0; i < uint8.length; i++) {
        binary += String.fromCharCode(uint8[i]);
    }

    return btoa(binary);
}

/**
 * Get gesture labels for ESP32
 */
export function getGestureLabels(): string[] {
    return [...gestureLabels];
}

/**
 * Clear all training data and reset model
 */
export function clearData() {
    trainingSamples = [];
    gestureLabels = [];
    if (model) {
        model.dispose();
        model = null;
    }
    console.log('[ML] Data and model cleared');
}

/**
 * Delete all samples for a specific gesture
 */
export function deleteGesture(gestureName: string) {
    const labelIndex = gestureLabels.indexOf(gestureName);
    if (labelIndex === -1) return;

    trainingSamples = trainingSamples.filter(s => s.label !== labelIndex);
    gestureLabels = gestureLabels.filter((_, i) => i !== labelIndex);

    // Re-index remaining samples
    for (const sample of trainingSamples) {
        if (sample.label > labelIndex) {
            sample.label--;
        }
    }

    console.log(`[ML] Deleted gesture: ${gestureName}`);
}

/**
 * Run inference using the trained neural network
 */
export async function predict(data: number[]): Promise<{ label: string; confidence: number }> {
    if (!model) throw new Error('No model trained');

    const input = tf.tensor2d([data]);
    const prediction = model.predict(input) as tf.Tensor;
    const probabilities = await prediction.data();

    let maxProb = 0;
    let bestClass = 0;
    for (let i = 0; i < probabilities.length; i++) {
        if (probabilities[i] > maxProb) {
            maxProb = probabilities[i];
            bestClass = i;
        }
    }

    input.dispose();
    prediction.dispose();

    return {
        label: gestureLabels[bestClass] || `class_${bestClass}`,
        confidence: maxProb
    };
}

// Initialize on import
console.log('[ML] Simple 1D gesture recognition ready');
console.log(`[ML] Architecture: ${INPUT_SIZE} -> ${HIDDEN_SIZE} -> N classes`);
console.log('[ML] Sampling: 50 samples @ 40ms = 2 sec window');
