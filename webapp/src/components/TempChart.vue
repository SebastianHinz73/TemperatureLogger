<template>
    <div class="card" >
        <div class="card-header d-flex align-items-center" :class="{ 'text-bg-success': true, 'text-bg-danger': false }">
            <input ref="startDate" class="form-control-sm me-2" type="date" style="padding-top: 0.2rem; padding-bottom: 0.2rem; max-width: 7rem;" />
            <div class="btn-group btn-group-sm me-auto" role="group">
                <input ref="duration30" type="radio" class="btn-check" name="btnradio" id="btnradio1" autocomplete="off" :checked="isTimescale05" :disabled="isDisabled" @click="SetTimescale(0.5)">
                <label class="btn btn-outline-light" for="btnradio1">30min</label>
                <input ref="duration1" type="radio" class="btn-check" name="btnradio" id="btnradio2" autocomplete="off" :checked="isTimescale2" :disabled="isDisabled" @click="SetTimescale(2)">
                <label class="btn btn-outline-light" for="btnradio2">2h</label>
                <input ref="duration6" type="radio" class="btn-check" name="btnradio" id="btnradio3" autocomplete="off" :checked="isTimescale6" :disabled="isDisabled" @click="SetTimescale(6)">
                <label class="btn btn-outline-light" for="btnradio3">6h</label>
                <input ref="duration24" type="radio" class="btn-check" name="btnradio" id="btnradio4" autocomplete="off" :checked="isTimescale24" :disabled="isDisabled" @click="SetTimescale(24)">
                <label class="btn btn-outline-light" for="btnradio4">24h</label>
            </div>
            <span role="button" class="fs-5 me-2" @click="ZoomIn"><BIconZoomIn /></span>
            <span role="button" class="fs-5 me-2" @click="ZoomOut"><BIconZoomOut /></span>
            <span role="button" class="fs-5 me-4" @click="ResetZoom"><BIconArrowCounterclockwise /></span>
            <span role="button" class="fs-5 me-2" @click="IncreaseHeight(50)"><BIconArrowsExpand /></span>
            <span role="button" class="fs-5 me-2" @click="DecreaseHeight(50)"><BIconArrowsCollapse /></span>
            <span role="button" class="fs-5" @click="ResetHeight"><BIconArrowCounterclockwise /></span>
        </div>
        <div class="card-body card-text text-center">
            <div
                class="row row-cols-1 row-cols-md-3 g-3">
                <Scatter ref="tempChart" :data="chartData" :options="chartOptions" :style="{ height: chartHeight + 'px', width: '100%', position:'relative' }"/>
             </div>
        </div>
    </div>

</template>

<script lang="ts">
import type { Config, Update } from '@/types/LiveDataStatus';
import { authHeader, handleBinaryResponse } from '@/utils/authentication';
import { defineComponent, type PropType } from 'vue';
import CardElement from './CardElement.vue';
import { BIconZoomIn, BIconZoomOut, BIconArrowCounterclockwise, BIconArrowsExpand, BIconArrowsCollapse } from 'bootstrap-icons-vue';

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
import type { ChartOptions } from 'chart.js';
import zoomPlugin from 'chartjs-plugin-zoom';
import { Scatter } from 'vue-chartjs';

ChartJS.register(CategoryScale, LinearScale, PointElement, LineElement, Title, Tooltip, Legend);
ChartJS.register(zoomPlugin);

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
            handler(newVal: Update[]) {
                if(!this.isTodaySelected || !this.configData.length) {
                    return;
                }

                const now = Date.now() / 1000;
                const updateMap = new Map<string, number>();
                for (const u of newVal) {
                    updateMap.set(u.serial, u.value);
                }

                let changed = false;
                const newData = this.configData.map(ds => {
                    const value = updateMap.get(ds.serial);
                    if (value !== undefined) {
                        changed = true;
                        return { ...ds, data: [...ds.data, { x: now, y: value }] };
                    }
                    return ds;
                });
                if (changed) {
                    this.configData = newData;
                }
            },
            deep: true,
        },
    },
    components: {
        Scatter,
        CardElement,
        BIconZoomIn,
        BIconZoomOut,
        BIconArrowCounterclockwise,
        BIconArrowsExpand,
        BIconArrowsCollapse,
    },
    data() {
        return {
            dataLoading: false,

            start: {} as Date,
            length: 0 as number,

            // chart height in pixels (can be changed by buttons)
            chartHeight: 400 as number,
            defaultChartHeight: 400 as number,

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
                    tooltip: {
                        callbacks: {
                            label: function (context: any) {
                                // X als Unix-Sekunden -> in ms umwandeln
                                const ts = context.parsed?.x ?? context.raw?.x ?? 0;
                                const d = new Date(ts * 1000);
                                const hh = ("0" + d.getHours()).slice(-2);
                                const mm = ("0" + d.getMinutes()).slice(-2);
                                const ss = ("0" + d.getSeconds()).slice(-2);
                                const y = context.parsed?.y ?? context.raw?.y ?? '';
                                return `${hh}:${mm}:${ss} — ${y}`;
                            },
                        },
                    },
                    zoom: {
                        pan: {
                            enabled: true,
                            mode: 'x',
                        },
                        zoom: {
                            wheel: {
                                enabled: true,
                            },
                            pinch: {
                                enabled: true,
                            },
                            mode: 'x',
                        },
                    },
                },
            } as ChartOptions<'scatter'>,
        };
    },
    created() {
        this.SetTimescale(0.5);
    },
    mounted() {
        let datepicker = this.$refs.startDate as HTMLInputElement
        datepicker.addEventListener('change', async (e: any) => {
            const el = e.target.value.split('-');
            var now = new Date(el[0], el[1]-1, el[2], 0, 0, 0, 0);
            //console.log(now.getTime() / 1000);
            this.start = now;
            this.length = 24*60*60;
            // ensure data is fetched and then update/reset zoom limits to match new day
            await this.fetchData();
            this.ResetZoom();
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
            return { datasets: this.configData };
        },
        isTodaySelected(): boolean {
            if (!(this.start instanceof Date)) return false;
            const now = new Date();
            return now.getFullYear() === this.start.getFullYear() &&
                now.getMonth() === this.start.getMonth() &&
                now.getDate() === this.start.getDate();
        },
        isDisabled(): boolean {
            return this.dataLoading || !this.isTodaySelected;
        },
        isTimescale05(): boolean {
            return this.length === 0.5 * 60 * 60;
        },
        isTimescale2(): boolean {
            return this.length === 2 * 60 * 60;
        },
        isTimescale6(): boolean {
            return this.length === 6 * 60 * 60;
        },
        isTimescale24(): boolean {
            return this.length === 24 * 60 * 60;
        },
    },
    methods: {
        async fetchBinaryData(serial: string): Promise<DataPoint[]> {
            const response = await fetch('/api/livedata/graphdata?id=' + serial + '&start=' + this.start.getTime() / 1000 + '&length=' + this.length, { headers: authHeader() });
            const data = await handleBinaryResponse(response, this.$emitter, this.$router, true);
            const lines = data.split('\n');
            const points: DataPoint[] = new Array(lines.length);
            let count = 0;
            for (let i = 0; i < lines.length; i++) {
                const line = lines[i];
                if (!line) continue;
                const sep = line.indexOf(';');
                if (sep > 0) {
                    points[count++] = { x: parseInt(line.substring(0, sep), 10), y: parseFloat(line.substring(sep + 1)) };
                }
            }
            points.length = count;
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
        async fetchData() {
            const visibleSensors: { sensor: Config; index: number }[] = [];
            for (let i = 0; i < this.sensors.length; i++) {
                const sensor = this.toConfigObject(this.sensors, i);
                if (sensor.connected && sensor.visible) {
                    visibleSensors.push({ sensor, index: i });
                }
            }
            if (!visibleSensors.length) {
                this.configData = [];
                return;
            }

            this.dataLoading = true;
            const sets: IDatasets[] = [];
            for (const { sensor, index } of visibleSensors) {
                sets.push({
                    serial: sensor.serial.toString(16),
                    label: sensor.name,
                    fill: false,
                    borderColor: this.getColor(index, this.sensors.length),
                    backgroundColor: this.getColor(index, this.sensors.length),
                    showLine: true,
                    borderWidth: 2,
                    data: await this.fetchBinaryData(sensor.serial.toString(16)),
                });
            }
            this.dataLoading = false;
            this.configData = sets;
        },
        async SetTimescale(scale: number) {
            const now = new Date();
            switch (scale) {
                case 24:
                    // set to start of day
                    now.setHours(0,0,0,0);
                    this.start = now;
                    this.length = 24*60*60;
                    break;
                default:
                    this.length = scale*60*60;
                    this.start = new Date(now.getTime() - this.length * 1000);
                    break;
            }
            // wait for data to be fetched so the chart exists / has data
            await this.fetchData();
            // reset zoom to the full (and clamped) range for the new timescale
            this.ResetZoom();
        },
        SetZoom(scale: number) {
            const chart = this.getChartInstance();
            if (!chart) return;

            // Versuche aktuelle Achsengrenzen zu ermitteln; fallback auf full range
            const xScale = chart.scales?.x;
            const fallbackMin = this.start ? this.start.getTime() / 1000 : undefined;
            const fallbackMax = (fallbackMin !== undefined) ? (fallbackMin + this.length) : undefined;

            const min = xScale?.min ?? fallbackMin ?? 0;
            const max = xScale?.max ?? fallbackMax ?? (min + this.length || 3600);

            const center = (min + max) / 2;
            const width = (max - min) * scale; // scale <1 = reinzoomen, >1 = rauszoomen
            const newMin = center - width / 2;
            const newMax = center + width / 2;

            // apply with clamping to full day range
            this.setZoomLimits(newMin, newMax);
        },

        ZoomIn() {
            this.SetZoom(0.5);
        },

        ZoomOut() {
            this.SetZoom(2);
        },

        ResetZoom() {
            const fullMin = this.start ? this.start.getTime() / 1000 : undefined;
            const fullMax = (fullMin !== undefined) ? (fullMin + this.length) : undefined;

            if (fullMin !== undefined && fullMax !== undefined) {
                this.setZoomLimits(fullMin, fullMax);
            } else {
                // fallback to plugin resetZoom if available
                const chart = this.getChartInstance();
                if (chart && typeof chart.resetZoom === 'function') {
                    try { chart.resetZoom(); } catch (e) { console.warn(e); }
                }
            }
        },

        // Ensure provided min/max are clamped to the available full range (start..start+length)
        setZoomLimits(min: number, max: number) {
            const chart = this.getChartInstance();
            if (!chart) return;

            const fullMin = this.start ? this.start.getTime() / 1000 : undefined;
            const fullMax = (fullMin !== undefined) ? (fullMin + this.length) : undefined;

            if (fullMin === undefined || fullMax === undefined) {
                // nothing to clamp against
                chart.options.scales.x.min = min;
                chart.options.scales.x.max = max;
                try { chart.update('none'); } catch (e) { /* ignore */ }
                return;
            }

            // clamp
            let clampedMin = Math.max(min, fullMin);
            let clampedMax = Math.min(max, fullMax);

            // Ensure we don't invert or make an empty range; if requested range wider than full range, use full range
            if (clampedMin >= clampedMax) {
                clampedMin = fullMin;
                clampedMax = fullMax;
            }

            //chart.options = chart.options || {};
            //chart.options.scales = chart.options.scales || {};
            //chart.options.scales.x = chart.options.scales.x || {};
            chart.options.scales.x.min = clampedMin;
            chart.options.scales.x.max = clampedMax;
            try { chart.update('none'); } catch (e) { /* ignore */ }
        },

        getChartInstance(): any {
            const chartComp: any = this.$refs.tempChart;
            return chartComp?.chartInstance ?? chartComp?.chart ?? chartComp?.$chart ?? null;
        },
        // Increase chart canvas height by step (px)
        async IncreaseHeight(step: number) {
            this.chartHeight = Math.min(2000, this.chartHeight + step);
            await this.applyChartHeight();
        },
        // Decrease chart canvas height by step (px)
        async DecreaseHeight(step: number) {
            this.chartHeight = Math.max(200, this.chartHeight - step);
            await this.applyChartHeight();
        },
        // Reset chart canvas height to default
        async ResetHeight() {
            this.chartHeight = this.defaultChartHeight;
            await this.applyChartHeight();
        },
        // Apply height change to chart by resizing chart after DOM update
        async applyChartHeight() {
            await this.$nextTick();
            const chart = this.getChartInstance();
            if (!chart) return;
            try {
                if (typeof chart.resize === 'function') {
                    chart.resize();
                } else if (typeof chart.update === 'function') {
                    chart.update();
                }
            } catch (e) { }
        },
        toConfigObject(arr: Config[], index: number) : Config {
            const obj = Object.values(arr).at(index);
            if(obj !== undefined)
            {
                let set: Config = {
                    serial: obj.serial,
                    name: obj.name,
                    connected: obj.connected,
                    visible: obj.visible,
                };
                return set;
            }
            let set: Config = {
                    serial: 0,
                    name: "",
                    connected: false,
                    visible: false,
                };
            return set;
        },
    },
 });
</script>
