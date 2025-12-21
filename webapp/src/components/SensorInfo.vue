<template>
    <div class="row row-cols-1 row-cols-md-3 g-3">
        <div v-for="sensor in  sensorData" :key="sensor.serial">
            <div class="card">
                <div class="card-header" :class="{ 'text-bg-success': sensor.valid, 'text-bg-danger': !sensor.valid }">{{
                    sensor.name
                }}</div>
                <div class="card-body card-text text-center">
                    <h2> {{ getValue(sensor).toFixed(2) }} </h2>
                </div>
            </div>
        </div>
    </div>
</template>

<script lang="ts">
import type { Temperature, UpdateMap } from '@/types/LiveDataStatus';
import { defineComponent, type PropType } from 'vue';

export default defineComponent({
    props: {
        sensorData: { type: Object as PropType<Temperature[]>, required: true },
        updates: { type: Object as PropType<UpdateMap>, required: true },
    },
    watch: {
        updates: {
            handler(newVal: UpdateMap) { // receive updates from board on websocket
                //if (JSON.stringify(newVal) == JSON.stringify(oldVal) )
                {
                    //return;
                }

                this.values = newVal;
                //console.log(oldVal);
                //console.log(newVal);
            },
            deep: true,
        },
    },
    data() {
        return {
             values: {} as UpdateMap,
         };
    },
    methods: {
        getValue(temperature: Temperature): number {
            let el = Array.from(this.values).find(row => row[0] == temperature.serial.toString())
            if(el !== undefined)
            {
                return el[1];
            }
            return 0;
        },
    },
});
</script>
