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
    <div
        class="absolute bottom-2 left-0 right-0 h-1.5 bg-zinc-700/50 rounded-full"
    ></div>

    <!-- Highlighted range track -->
    <div
        class="absolute bottom-2 h-1.5 bg-indigo-500 rounded-full"
        style="left: {leftPercent}%; right: {100 - rightPercent}%"
    ></div>

    <!-- Min slider -->
    <input
        type="range"
        {min}
        {max}
        {step}
        value={valueMin}
        oninput={handleMinInput}
        {disabled}
        class="absolute bottom-0 w-full h-6 appearance-none bg-transparent pointer-events-none
               [&::-webkit-slider-thumb]:pointer-events-auto
               [&::-webkit-slider-thumb]:appearance-none
               [&::-webkit-slider-thumb]:w-4
               [&::-webkit-slider-thumb]:h-4
               [&::-webkit-slider-thumb]:rounded-full
               [&::-webkit-slider-thumb]:bg-white
               [&::-webkit-slider-thumb]:shadow-lg
               [&::-webkit-slider-thumb]:cursor-pointer
               [&::-webkit-slider-thumb]:border-2
               [&::-webkit-slider-thumb]:border-indigo-500
               [&::-webkit-slider-thumb]:transition-transform
               [&::-webkit-slider-thumb]:active:scale-125"
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
        class="absolute bottom-0 w-full h-6 appearance-none bg-transparent pointer-events-none
               [&::-webkit-slider-thumb]:pointer-events-auto
               [&::-webkit-slider-thumb]:appearance-none
               [&::-webkit-slider-thumb]:w-4
               [&::-webkit-slider-thumb]:h-4
               [&::-webkit-slider-thumb]:rounded-full
               [&::-webkit-slider-thumb]:bg-white
               [&::-webkit-slider-thumb]:shadow-lg
               [&::-webkit-slider-thumb]:cursor-pointer
               [&::-webkit-slider-thumb]:border-2
               [&::-webkit-slider-thumb]:border-indigo-500
               [&::-webkit-slider-thumb]:transition-transform
               [&::-webkit-slider-thumb]:active:scale-125"
    />
</div>
