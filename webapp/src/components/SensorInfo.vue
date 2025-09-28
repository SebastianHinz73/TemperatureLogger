<template>
    <div class="row row-cols-1 row-cols-md-3 g-3">
        <div v-for="sensor in  sensorData" :key="sensor.serial">
            <div class="card">
                <div class="card-header" :class="{ 'text-bg-success': sensor.valid, 'text-bg-danger': !sensor.valid }">{{
                    sensor.name
                }}</div>
                <div class="card-body card-text text-center">
                    <h2>
                        {{ sensor.value.toFixed(2) }}
                    </h2>
                </div>
            </div>
        </div>
    </div>
</template>

<script lang="ts">
import type { Temperature } from '@/types/LiveDataStatus';
import { defineComponent, type PropType } from 'vue';

export default defineComponent({
    props: {
        sensorData: { type: Object as PropType<Temperature[]>, required: true },
    },
});
</script>
