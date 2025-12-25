<template>
    <div
        class="row row-cols-1 row-cols-md-3 g-3">
        <Scatter :data="chartData" :options="chartOptions" />
    </div>
</template>

<script lang="ts">
import type { UpdateMap } from '@/types/LiveDataStatus';
import { authHeader, handleResponse, handleBinaryResponse } from '@/utils/authentication';
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
        updates: { type: Object as PropType<UpdateMap>, required: true },
    },
    watch: {
        updates: {
            handler(newVal: UpdateMap) { // receive updates from board on websocket
                //console.table(newVal);

                const time = new Date();
                const now = time.getTime() / 1000;

                //const text = ("0" + time.getHours()).slice(-2) + ':' + ("0" + time.getMinutes()).slice(-2) + ':' + ("0" + time.getSeconds()).slice(-2);
                //console.log(text);

                const copyDataset: IDatasets[] = this.copyDataset(this.configData);

                for (let i = 0; i < this.configData.length; i++) {
                    const serial = this.configData[i]?.serial ?? '';
                    if(serial.length)
                    {
                        const value = newVal.get(serial);
                        if(value !== undefined)
                        {
                            let src = this.configData.find(el => (el.serial === serial));
                            let dst = copyDataset.find(el => (el.serial === serial));
                            if (src && dst) {
                                //dst.data = [...src.data, ...JSON.parse(value) as DataPoint[]];
                                dst.data = [...src.data,   { x: now, y: value } ] ;
                                //dst.data = dst.data.slice(-15); // keep only last 500 data points
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
                                const text = ("0" + time.getHours()).slice(-2) + ':' + ("0" + time.getMinutes()).slice(-2) + ':' + ("0" + time.getSeconds()).slice(-2);
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
            },
        };
    },
    created() {
        this.fetchData();
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
        fetchBinaryData(serial: string): DataPoint[] {
            let points: DataPoint[] = [];
            const now = new Date().getTime() / 1000;

            fetch('/api/livedata/graphdata?id=' + serial + '&start=' + (now-30*60) + '&length=' + (30*60), { headers: authHeader() })
                .then((response) => handleBinaryResponse(response, this.$emitter, this.$router, true))
                .then((data) => {
                    //console.log(data.slice(-100));
                    data.split('\n').forEach(line => {
                        const el = line.split(';')
                        if(el[0] !== undefined && el[1] !== undefined)
                        {
                            const dp = { x: parseInt(el[0], 10), y: parseFloat(el[1]) } as DataPoint;
                            points.push(dp);
                        }
                    });
                });

            return points;
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
        fetchData() {

        //this.dataLoading = true;
            fetch('/api/livedata/graph', { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router, true))
                .then((data) => {
                    let sets: IDatasets[] = [];

                    if (data['config'] !== undefined) {
                        const serialList = Object.keys(data['config']);
                        for (let i = 0; i < serialList.length; i++) {
                            const serial = serialList[i];
                            if(serial !== undefined) {
                                const config = data['config'][serial];
                                let set: IDatasets = {
                                    serial: serial,
                                    label: config.name,
                                    fill: false,
                                    borderColor: this.getColor(i, serialList.length),
                                    backgroundColor: this.getColor(i, serialList.length),
                                    showLine: true,
                                    borderWidth: 2,
                                    data: this.fetchBinaryData(serial),
                                };
                                //console.log(JSON.stringify(set.data));
                                sets.push(set);
                            }
                        }
                    }

                    this.configData = sets;
                });
        },

    },
 });
</script>
