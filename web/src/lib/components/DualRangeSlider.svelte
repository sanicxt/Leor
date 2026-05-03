<script lang="ts">
    interface Props {
        min?: number;
        max?: number;
        step?: number;
        valueMin: number;
        valueMax: number;
        onchange?: (min: number, max: number) => void;
        disabled?: boolean;
        unit?: string;
    }

    let {
        min = 1,
        max = 10,
        step = 1,
        valueMin = $bindable(2),
        valueMax = $bindable(5),
        onchange,
        disabled = false,
        unit = "s",
    }: Props = $props();

    function handleMinInput(e: Event) {
        const val = parseInt((e.target as HTMLInputElement).value);
        if (val <= valueMax) {
            valueMin = val;
            onchange?.(valueMin, valueMax);
        } else {
            // Prevent going past max
            (e.target as HTMLInputElement).value = valueMax.toString();
        }
    }

    function handleMaxInput(e: Event) {
        const val = parseInt((e.target as HTMLInputElement).value);
        if (val >= valueMin) {
            valueMax = val;
            onchange?.(valueMin, valueMax);
        } else {
            // Prevent going past min
            (e.target as HTMLInputElement).value = valueMin.toString();
        }
    }

    let leftPercent = $derived(((valueMin - min) / (max - min)) * 100);
    let rightPercent = $derived(((valueMax - min) / (max - min)) * 100);
</script>

<div class="relative h-6 {disabled ? 'opacity-50' : ''}">
    <!-- Track background -->
    <div class="absolute bottom-2 left-0 right-0 h-2 bg-paper border-2 border-bento-border rounded-full"></div>

    <!-- Highlighted range track -->
    <div class="absolute bottom-2 h-2 bg-bento-peach border-y-2 border-bento-border" style="left: {leftPercent}%; right: {100 - rightPercent}%"></div>

    <!-- Min slider -->
    <input
        type="range"
        {min}
        {max}
        {step}
        value={valueMin}
        oninput={handleMinInput}
        {disabled}
        class="absolute bottom-0 w-full h-6 slider-thumb-only"
    />

    <!-- Max slider -->
    <input
        type="range"
        {min}
        {max}
        {step}
        value={valueMax}
        oninput={handleMaxInput}
        {disabled}
        class="absolute bottom-0 w-full h-6 slider-thumb-only"
    />
</div>
