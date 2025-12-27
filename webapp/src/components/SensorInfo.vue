<template>
    <div class="row row-cols-1 row-cols-md-3 g-3">
        <div v-for="sensor in config" :key="sensor.serial">
            <div class="card">
                <div class="card-header" :class="{ 'text-bg-success': sensor.valid, 'text-bg-danger': !sensor.valid }">{{
                    sensor.name
                }}</div>
                <div class="card-body card-text text-center">
                    <h2> {{ getSensor(sensor).toFixed(2) }} </h2>
                </div>
            </div>
        </div>
    </div>
</template>

<script lang="ts">
import type { Config, Update } from '@/types/LiveDataStatus';
import { defineComponent, type PropType } from 'vue';

export default defineComponent({
    props: {
        config: { type: Object as PropType<Config[]>, required: true },
        updates: { type: Object as PropType<Update[]>, required: true },
    },
    methods: {
        getSensor(sensor: Config): number {
            if(this.updates === undefined)
            {
                return 0;
            }

            let el = this.updates.find(el => el.serial == sensor.serial.toString());
            if(el !== undefined)
            {
                return el.value;
            }
            return 0;
        },
    },
});
</script>
