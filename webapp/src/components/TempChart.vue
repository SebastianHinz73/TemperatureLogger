<template>
    <div
        class="row row-cols-1 row-cols-md-3 g-3">
        <Scatter :data="chartData" :options="chartOptions" />
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
                const serialList = Object.keys(newVal);
                const valueList = Object.values(newVal);
                for (let i = 0; i < serialList.length; i++) {
                    const serial = serialList[i];
                    const value = valueList[i];
                    if(serial !== undefined && value !== undefined) {
                        //this.addDataAsDataPoint(serial, [{ x: Date.now()/1000, y: value }]);
                        //this.addDataAsDataPoint(serial, [{ x: this.dummyCnt*10, y: this.dummyCnt + 50 }]);
                         //this.dummyCnt++;
                    }
                }
                //console.log(this.configData);

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
            updateInterval: 0,
            dummyCnt: 0,
            dummyData: [] as DataPoint[],

            configData: {} as IDatasets[],

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

        if (this.updateInterval) {
            clearTimeout(this.updateInterval);
        }
    },
    unmounted() {
        clearInterval(this.updateInterval); // TODO clearTimeout / clearInterval
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
        fetchData() {

        //this.dataLoading = true;
            fetch('/api/livedata/graph', { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.dummyCnt++;

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
                                    //data: [ {x: 10,y: 22},  {x: 20,y: this.dummyCnt} ] ,
                                    data: [] ,
                                };
                                console.log(JSON.stringify(set.data));
                                sets.push(set);
                            }

                        }
                        this.configData = sets;
                    }
                    console.log('TempChart fetchData completed ' + this.dummyCnt);
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
                                //console.table(JSON.parse(value));
                                this.setData(serial, JSON.parse(value) as DataPoint[]);
                            }
                        }
                        //console.log(this.configData);
                    }

                    this.updateInterval = setTimeout(() => {
                        //console.log("TempChart fetchData interval");
                        //this.fetchData();
                    }, 1000);

                });

        },
        addDataAsDataPoint(serial: string, data: DataPoint[]) {
            //const id = this.configData.findIndex(el => (el.serial === serial));
            //console.log('Found index ' + id + ' for serial ' + serial);

            let obj = this.configData.find(el => (el.serial === serial));
            if (obj) {
                //console.log('Found');
                console.log(typeof data);
                //obj.data = [...obj.data, ...data];

                this.updateInterval = setTimeout(() => {
                        //console.log("TempChart fetchData interval");
                        //console.table(this.configData['data']);
                        //this.fetchData();
                        obj.data = [{"x":10,"y":22},{"x":20,"y":27}] as DataPoint[];

                    }, 1000);
                //console.log(JSON.stringify(this.configData));
            }

            //this.$forceUpdate();

            //this.configData[id].data = JSON.parse(data) as DataPoint[];

            //if (initialLoadiing) {
            //    this.data[name] = JSON.parse(this.liveData[name]);
            ///} else {
            //    this.data[name] = [...this.data[name], ...JSON.parse(this.liveData[name])];
            //}
        },
        setData(serial: string, data: DataPoint[]) {
            let obj = this.configData.find(el => (el.serial === serial)) as IDatasets;
            if (obj) {
                obj.data = data;
            }
            //this.configData[id].data = JSON.parse(data) as DataPoint[];

            //if (initialLoadiing) {
            //    this.data[name] = JSON.parse(this.liveData[name]);
            ///} else {
            //    this.data[name] = [...this.data[name], ...JSON.parse(this.liveData[name])];
            //}
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
