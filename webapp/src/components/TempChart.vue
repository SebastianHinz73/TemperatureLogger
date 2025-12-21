<template>
    <div
        class="row row-cols-1 row-cols-md-3 g-3">
        <Scatter ref="mychart" :data="chartData" :options="chartOptions" />
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

interface BlobData {
    data: Int8Array;
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

                const copyDataset: IDatasets[] = this.copyDataset();

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
            configData: {} as IDatasets[],
            chartOptions123: {
                responsive: true,
                maintainAspectRatio: false,
                elements: {
                    point: {
                        radius: 2,
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
        chartOptions: function () {
            return this.chartOptions123;
        },
    },
    methods: {
        copyDataset(): IDatasets[] {
            let newSets: IDatasets[] = [];

            for (let i = 0; i < this.configData.length; i++) {
                const config = this.configData[i];

                if(config !== undefined) {
                    let set: IDatasets = {
                        serial: config.serial,
                        label: config.label,
                        fill: false,
                        borderColor: config.borderColor,
                        backgroundColor: config.borderColor,
                        showLine: true,
                        borderWidth: 2,
                        data: [] ,
                    };
                    newSets.push(set);
                }
            }
            return newSets;
        },
        fetchData() {

        //this.dataLoading = true;
            fetch('/api/livedata/graph', { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router, true))
                .then((data) => {
                    if (data['config'] !== undefined) {
                        let sets: IDatasets[] = [];

                        const serialList = Object.keys(data['config']);
                        for (let i = 0; i < serialList.length; i++) {
                            const serial = serialList[i];
                            if(serial !== undefined) {
                                const config = data['config'][serial];
                                let set: IDatasets = {
                                    serial: serial,
                                    label: config.name,
                                    fill: false,
                                    borderColor: config.color,
                                    backgroundColor: config.color,
                                    showLine: true,
                                    borderWidth: 2,
                                    data: [] ,
                                };
                                //console.log(JSON.stringify(set.data));
                                sets.push(set);
                            }
                        }
                        this.configData = sets;
                    }

                    //fetch('/api/livedata/graphdata?serial=30377&timestamp=1&interval=60', { headers: authHeader() })
                    fetch('/api/file?id=76a9', { headers: authHeader() })
                        .then((response) => handleBinaryResponse(response, this.$emitter, this.$router, true))
                        .then((data) => {
                            //console.log(data);
                            return;
                            const arr = [] as DataPoint[];
                            const now = new Date().getTime() / 1000;

                            data.split('\n').splice(-10).forEach(line => {
                                //console.log(line);
                                const el = line.split(';')
                                const t = el[0]?.split(':');
                                if(t !== undefined && t[0] != undefined && t[1] != undefined && t[2] != undefined && el[1] !== undefined)
                                {
                                    const time = parseInt(t[0], 10)*60*60 + parseInt(t[1], 10) * 60 + parseInt(t[2], 10) ;
                                    const value = parseFloat(el[1]);
                                    //console.log(t);
                                    //console.log(value);
                                    const dp = { x: now+time, y: value } as DataPoint;
                                    arr.push(dp);
                                }
                            });
                            //console.log(arr);

                            const obj = this.configData.find(el => (el.serial === '76a9')) as IDatasets;
                            if (obj) {
                                obj.data = arr;
                               console.log(arr);

                            }

                            //const arr = data.split(';')
                            //console.log(arr);
                            //var int8view = new Uint8Array(data, 8);
                            //console.log(int8view);
                            //console.log(data.type);
                            //const ff = data['data'] as BlobData;
                            //if(ff !== undefined)
                            {
                              //  console.log(ff);
                            }
                        });


                    if (data['data'] !== undefined)
                    {
                        ///console.log('Graph data received ' + JSON.stringify(data['data']));
                        //console.table(data['data']);
                        //console.log(Array.from(data['data']));
                        //console.log(Object.fromEntries(data['data']));

                        const serialList = Object.keys(data['data']);
                        const valueList = Object.values(data['data']) as string[];

                        for (let i = 0; i < serialList.length; i++) {
                            const serial = serialList[i];
                            const value = valueList[i];

                            if(serial !== undefined && value !== undefined) {
                                const obj = this.configData.find(el => (el.serial === serial)) as IDatasets;
                                if (obj) {
                                    obj.data = JSON.parse(value) as DataPoint[];
                                    console.log(obj.data);
                                }
                            }
                        }
                        //console.log(this.configData);
                    }
                });

        },
    },
});
</script>
