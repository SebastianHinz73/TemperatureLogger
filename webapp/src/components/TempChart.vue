<template>
    <div class="card">
        <div class="card-header" :class="{ 'text-bg-success': true, 'text-bg-danger': false }">
            <div class="justify-content-center align-self-center">
                Date: <input ref="startDate" class="form-control-sm ms-2" type="date" />
            </div>
        </div>
        <div class="text-left">
            <div class="btn-group ms-1" role="group" aria-label="Basic radio toggle button group">
                <input ref="duration30" type="radio" class="btn-check" name="btnradio" id="btnradio1" autocomplete="off" :checked="IsTimescale(0.5)" :disabled="IsDisabled()" @click="SetTimescale(0.5)">
                <label class="btn btn-outline-success" for="btnradio1">30min</label>

                <input ref="duration1" type="radio" class="btn-check" name="btnradio" id="btnradio2" autocomplete="off" :checked="IsTimescale(1)" :disabled="IsDisabled()" @click="SetTimescale(1)">
                <label class="btn btn-outline-success" for="btnradio2">1h</label>

                <input ref="duration6" type="radio" class="btn-check" name="btnradio" id="btnradio3" autocomplete="off" :checked="IsTimescale(6)" :disabled="IsDisabled()" @click="SetTimescale(6)">
                <label class="btn btn-outline-success" for="btnradio3">6h</label>

                <input ref="duration24" type="radio" class="btn-check" name="btnradio" id="btnradio4" autocomplete="off" :checked="IsTimescale(24)" :disabled="IsDisabled()" @click="SetTimescale(24)">
                <label class="btn btn-outline-success" for="btnradio4">24h</label>
            </div>
        </div>
        <div class="card-body card-text text-center">
            <div
                class="row row-cols-1 row-cols-md-3 g-3">
                <Scatter :data="chartData" :options="chartOptions" />
            </div>
        </div>
    </div>

</template>

<script lang="ts">
import type { Config, Update } from '@/types/LiveDataStatus';
import { authHeader, handleBinaryResponse } from '@/utils/authentication';
import { defineComponent, type PropType } from 'vue';
import CardElement from './CardElement.vue';

import {
    Chart as ChartJS,
    CategoryScale,
    LinearScale,
    PointElement,
    LineElement,
    Title,
    Tooltip,
    Legend,
} from 'chart.js';
import { Scatter } from 'vue-chartjs';

ChartJS.register(CategoryScale, LinearScale, PointElement, LineElement, Title, Tooltip, Legend);

interface IDatasets {
    serial: string;
    label: string;
    fill: boolean;
    borderColor: string;
    backgroundColor: string;
    showLine: boolean;
    borderWidth: number;
    data: DataPoint[];
}

interface DataPoint {
    x: number;
    y: number;
}

export default defineComponent({
    props: {
        config: { type: Object as PropType<Config[]>, required: true },
        updates: { type: Object as PropType<Update[]>, required: true },
    },
    watch: {
        updates: {
            handler(newVal: Update[]) { // receive updates from board on websocket
                //console.table(newVal);

                const time = new Date();
                const now = time.getTime() / 1000;

                const copyDataset: IDatasets[] = this.copyDataset(this.configData);

                for (let i = 0; i < this.configData.length; i++) {
                    const serial = this.configData[i]?.serial ?? '';
                    if(serial.length)
                    {
                        const value = newVal.find(el => el.serial == serial)?.value;
                        if(value !== undefined)
                        {
                            let src = this.configData.find(el => (el.serial === serial));
                            let dst = copyDataset.find(el => (el.serial === serial));
                            if (src && dst) {
                                dst.data = [...src.data,   { x: now, y: value } ] ;
                            }
                        }
                    }
                }
                this.configData = copyDataset;
            },
            deep: true,
        },
    },
    components: {
        Scatter,
        CardElement,
    },
    data() {
        return {
            dataLoading: false,

            start: {} as Date,
            length: 0 as number,

            sensors: this.config,

            configData: [] as IDatasets[],

            chartOptions: {
                responsive: true,
                maintainAspectRatio: false,
                elements: {
                    point: {
                        radius: 0,
                    },
                },
                animation: {
                    duration: 0,
                },
                scales: {
                    x: {
                        ticks: {
                            callback: function (value: any) {
                                const time = new Date(value*1000);
                                const text = ("0" + time.getHours()).slice(-2) + ':' + ("0" + time.getMinutes()).slice(-2);
                                return text;
                            },
                        },
                    },
                },
                plugins: {
                    legend: {
                        display: true,
                    },
                },
/*                tooltips: {
                    usePointStyle: true,
                    callbacks: {
                        labelPointStyle: function(context) {
                            return {
                                pointStyle: 'triangle',
                                rotation: 0
                            };
                        }
                    },
                },*/
            },
        };
    },
    // https://medium.com/risan/vue-chart-component-with-chart-js-db85a2d21288
    // https://www.chartjs.org/docs/latest/configuration/tooltip.html#label-callback
    created() {
        this.SetTimescale(0.5);
    },
    mounted() {
        let datepicker = this.$refs.startDate as HTMLInputElement
        datepicker.addEventListener('change',(e: any)=>{
            //console.log(e.target.value);
            const el = e.target.value.split('-');
            var now = new Date(el[0], el[1]-1, el[2], 0, 0, 0, 0);
            //console.log(now.getTime() / 1000);
            this.start = now;
            this.length = 24*60*60;
            this.fetchData();

            //const now = new Date();
            //now.setHours(0,0,0,0);
            //this.start = now;
        })

        const today = new Date();
        datepicker.value = today.toISOString().slice(0, 10);
    },
    unmounted() {
    },
    computed: {
        chartData: function () {

            if(!this.configData.length){
                return {
                    datasets: []
                };
            }
            //console.log('Chart data computed ' + JSON.stringify(this.configData));
            //this.$forceUpdate();
            return { datasets: this.configData };
        },
    },
    methods: {
        copyDataset(src: IDatasets[], copy: boolean = false): IDatasets[] {
            let newSets: IDatasets[] = [];
            for (let i = 0; i < src.length; i++) {
                const config = src[i];
                if(config !== undefined) {
                    let set: IDatasets = {
                        serial: config.serial,
                        label: config.label,
                        fill: false,
                        borderColor: config.borderColor,
                        backgroundColor: config.borderColor,
                        showLine: true,
                        borderWidth: 2,
                        data: copy ? config.data : [],
                    };
                    newSets.push(set);
                }
            }
            return newSets;
        },
        async fetchBinaryData(serial: string): Promise<DataPoint[]> {
            let points: DataPoint[] = [];

            const startOfDay = new Date();
            startOfDay.setHours(0,0,0,0);

            return new Promise((resolve) => {
                fetch('/api/livedata/graphdata?id=' + serial + '&start=' + this.start.getTime() / 1000 + '&length=' + this.length, { headers: authHeader() })
                    .then((response) => handleBinaryResponse(response, this.$emitter, this.$router, true))
                    .then((data) => {
                        //console.log(data.slice(-100));
                        const oldFormat = data.indexOf(':') > -1;
                        data.split('\n').forEach(line => {
                            const el = line.split(';')
                            if(el[0] !== undefined && el[1] !== undefined) {
                                if(oldFormat) {
                                    const t = el[0].split(':');
                                    if(t[0] !== undefined && t[1] !== undefined && t[2] !== undefined)
                                    {
                                        const ti = startOfDay.getTime()/1000 + parseInt(t[0], 10) * 3600 + parseInt(t[1], 10) * 60 + parseInt(t[2], 10);
                                        const dp = { x: ti, y: parseFloat(el[1]) } as DataPoint;
                                        points.push(dp);
                                    }
                                } else {
                                    const dp = { x: parseInt(el[0], 10), y: parseFloat(el[1]) } as DataPoint;
                                    points.push(dp);
                                }
                            }
                        });
                        this.dataLoading = false;
                        resolve(points);
                    });
            });
        },
        getColor(index: number, max: number): string {
            max = Math.floor((max + 1) / 2);
            const h = 360 / max * Math.floor(index/2); // Hue (0-360)
            const s = 1;   // Saturation (0-1)
            const v = (index%2 == 0) ? 1 : 0.8;   // Value (0-1)

            const rgb = this.hsvToRgb(h, s, v);

            return "#" + ("0" + rgb.r.toString(16)).slice(-2) +
                        ("0" + rgb.g.toString(16)).slice(-2) +
                        ("0" + rgb.b.toString(16)).slice(-2);

        },
        hsvToRgb(h: number, s: number, v: number) {
            let r = 0, g = 0, b = 0;
            let i = Math.floor(h / 60);
            let f = h / 60 - i;
            let p = v * (1 - s);
            let q = v * (1 - f * s);
            let t = v * (1 - (1 - f) * s);

            switch (i % 6) {
                case 0: r = v; g = t; b = p; break;
                case 1: r = q; g = v; b = p; break;
                case 2: r = p; g = v; b = t; break;
                case 3: r = p; g = q; b = v; break;
                case 4: r = t; g = p; b = v; break;
                case 5: r = v; g = p; b = q; break;
            }
            return {
                    r: Math.round(r * 255),
                    g: Math.round(g * 255),
                    b: Math.round(b * 255)};
        },
        async fetchData() {
            //console.log(this.sensors);
            let sets: IDatasets[] = [];
            for (let i = 0; i < this.sensors.length; i++) {
                const sensor = this.toConfigObject(this.sensors, i);
                if(!sensor.valid) {
                    continue;
                }

                this.dataLoading = true;

                let set: IDatasets = {
                    serial: sensor.serial.toString(16),
                    label: sensor.name,
                    fill: false,
                    borderColor: this.getColor(i, this.sensors.length),
                    backgroundColor: this.getColor(i, this.sensors.length),
                    showLine: true,
                    borderWidth: 2,
                    data: await this.fetchBinaryData(sensor.serial.toString(16)),
                };
                sets.push(set);
            }
            this.configData = sets;
        },
        SetTimescale(scale: number) {
            const now = new Date();
            switch (scale) {
                case 24:
                    now.setHours(0,0,0,0);
                    this.start = now;
                    this.length = 24*60*60;
                    break;
                default:
                    this.length = scale*60*60;
                    this.start = new Date(now.getTime() - this.length * 1000);
                    break;
            }
            this.fetchData();
        },
        IsTimescale(scale: number) {
            if(scale*60*60 == this.length) {
                return true;
            }
            return false;
        },
        IsDisabled() {
            if(this.dataLoading){
                return true;
            }
            var now = new Date();
            if(now.getFullYear() == this.start.getFullYear() &&
                now.getMonth() == this.start.getMonth() &&
                now.getDay() == this.start.getDay())
            {
                return false;
            }
            return true;
        },
        toConfigObject(arr: Config[], index: number) : Config {
            const obj = Object.values(arr).at(index);
            if(obj !== undefined)
            {
                let set: Config = {
                    serial: obj.serial,
                    name: obj.name,
                    valid: obj.valid,
                };
                return set;
            }
            let set: Config = {
                    serial: 0,
                    name: "",
                    valid: false,
                };
            return set;
        },
    },
 });
</script>
