<template>
    <div
        class="row row-cols-1 row-cols-md-3 g-3">
        <Scatter ref="mychart" :data="chartData" :options="chartOptions" />
    </div>
</template>

<script lang="ts">
import type { UpdateMap } from '@/types/LiveDataGraph';
import { authHeader, handleResponse } from '@/utils/authentication';
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
                console.table(newVal);

                const copyDataset: IDatasets[] = this.copyDataset();

                const serialList = Object.keys(newVal);
                const valueList = Object.values(newVal);
                for (let i = 0; i < serialList.length; i++) {
                    const serial = serialList[i];
                    const value = valueList[i];
                    if(serial !== undefined && value !== undefined) {
                        let src = this.configData.find(el => (el.serial === serial));
                        let dst = copyDataset.find(el => (el.serial === serial));
                        if (src && dst) {
                            //dst.data = [...src.data, ...JSON.parse(value) as DataPoint[]];
                            dst.data = [...src.data, JSON.parse(value) as DataPoint ] ;
                        }

                        //this.addDataAsDataPoint(serial, [{ x: Date.now()/1000, y: value }]);
                        //this.addDataAsDataPoint(serial, [{ x: this.dummyCnt*10, y: this.dummyCnt + 50 }]);
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
                        radius: 0,
                    },
                },
                animation: {
                    duration: 0,
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
        //this.repeateData();

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
                        //data: [ {x: 10,y: 22},  {x: 20,y: this.dummyCnt} ] ,
                        data: [] ,
                    };
                    //console.log(JSON.stringify(set.data));
                    newSets.push(set);
                }
            }
            return newSets;
            //return JSON.parse(JSON.stringify(this.configData));
        },
        fetchData() {

        //this.dataLoading = true;
            fetch('/api/livedata/graph', { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
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
                                    data: [ {x: 10,y: 22},  {x: 20,y: 30} ] ,
                                    //data: [] ,
                                };
                                console.log(JSON.stringify(set.data));
                                sets.push(set);
                            }

                        }
                        this.configData = sets;
                    }

                    if (data['data'] !== undefined)
                    {
                        //console.log('Graph data received ' + JSON.stringify(data['data']));
                        //console.table(data['data']);
                        const serialList = Object.keys(data['data']);
                        const valueList = Object.values(data['data']) as string[];

                        for (let i = 0; i < serialList.length; i++) {
                            const serial = serialList[i];
                            const value = valueList[i];

                            if(serial !== undefined && value !== undefined) {
                                const obj = this.configData.find(el => (el.serial === serial)) as IDatasets;
                                if (obj) {
                                    obj.data = JSON.parse(value) as DataPoint[];
                                }
                            }
                        }
                        //console.log(this.configData);
                    }
                });

        },
        handleLoading() {
            //if (initialLoading) {
            //    this.data[name] = JSON.parse(this.liveData[name]);
            ///} else {
            //    this.data[name] = [...this.data[name], ...JSON.parse(this.liveData[name])];
            //}
        },
    },
});
</script>
