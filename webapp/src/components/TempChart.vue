<template>
    <div
        class="row row-cols-1 row-cols-md-3 g-3">
        <Scatter :data="chartData" :options="chartOptions" />
    </div>
</template>

<script lang="ts">
import type { UpdatePoint } from '@/types/LiveDataGraph';
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
        updates: { type: Object as PropType<UpdatePoint[]>, required: true },
    },
    watch: {
        updates: {
            handler(newVal: any) { // TODO: type Map?
                const serialList = Object.keys(newVal);
                for (let i = 0; i < serialList.length; i++) {
                    const serial = serialList[i];
                    if(serial !== undefined) {
                        const config = newVal[serial];
                        console.log('Updates for serial ' + serial + ' data ' + JSON.stringify(config));
                        //this.addDataAsDataPoint(serial, [{ x: Date.now()/1000, y: config }]);
                    }
                }
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

        //if (this.updateInterval) {
        //    clearTimeout(this.updateInterval);
        //}
    },
    unmounted() {
       // clearInterval(this.updateInterval);
    },
    computed: {
        chartData: function () {

            if(!this.configData.length){
                return {
                    datasets: []
                };
            }
            console.log('Chart data computed ' + JSON.stringify(this.configData));
            return { datasets: this.configData };
        },
    },
    methods: {
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
                                    data: [ {x: 10,y: 22},  {x: 20,y: 25} ] ,
                                    //data: [] ,
                                };
                                console.log(JSON.stringify(set));
                                sets.push(set);
                            }

                        }
                        this.configData = sets;
                    }

                 /*   if (data['data'] !== undefined)
                    {
                        console.log('Graph data received ' + JSON.stringify(data['data']));
                        const serialList = Object.keys(data['data']);
                        for (let i = 0; i < serialList.length; i++) {
                            const serial = serialList[i];
                            if(serial !== undefined){
                                const readings = data['data'][serial];
                                this.setData(serial, readings);
                            }
                        }
                    }
*/
                });

        },
        repeateData__remove(serial: string, data: string) {

            console.log("TempChart fetchData interval" + serial + data);

            //this.updateInterval = setTimeout(() => {
            //            this.repeateData();
            //        }, 5);
        },
        addDataAsDataPoint(serial: string, data: DataPoint[]) {
            //const id = this.configData.findIndex(el => (el.serial === serial));
            //console.log('Found index ' + id + ' for serial ' + serial);

            let obj = this.configData.find(el => (el.serial === serial));
            if (obj) {
                //console.log('Found');
                obj.data = [...obj.data, ...data];
                //console.log(JSON.stringify(this.configData));
            }



            //this.configData[id].data = JSON.parse(data) as DataPoint[];

            //if (initialLoadiing) {
            //    this.data[name] = JSON.parse(this.liveData[name]);
            ///} else {
            //    this.data[name] = [...this.data[name], ...JSON.parse(this.liveData[name])];
            //}
        },
        setData(serial: string, data: string) {

            let obj = this.configData.find(el => (el.serial === serial));
            if (obj) {
                //console.log('Found');
                obj.data = JSON.parse(data) as DataPoint[];
                //console.log(JSON.stringify(this.configData));
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
