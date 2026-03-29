<template>
    <div class="row row-cols-3 row-cols-md-5 g-2">
        <div v-for="sensor in visibleSensors" :key="sensor.serial">
            <div class="card">
                <div class="card-header" :class="{ 'text-bg-success': sensor.connected, 'text-bg-danger': !sensor.connected }">{{
                    sensor.name
                }}</div>
                <div class="card-body card-text text-center">
                    <h3> {{ getSensor(sensor).toFixed(2) }} </h3>
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
    computed: {
        visibleSensors() {
            return this.config.filter(s => !!s.visible);
        },
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
