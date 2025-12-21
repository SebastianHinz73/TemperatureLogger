<template>
    <BasePage
        :title="$t('home.LiveData')"
        :isLoading="dataLoading"
        :isWideScreen="true"
        :showWebSocket="true"
        :isWebsocketConnected="isWebsocketConnected"
        @reload="reloadData"
    >
        <HintView :hints="liveData.hints" />
        <SensorInfo :updates="liveData.updates" :sensorData="liveData.temperatures" /><br />
        <TempChart :updates="liveData.updates"/><br />
    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import TempChart from '@/components/TempChart.vue';
import HintView from '@/components/HintView.vue';
import SensorInfo from '@/components/SensorInfo.vue';
import type { Temperature, LiveData } from '@/types/LiveDataStatus';
import { authHeader, authUrl, handleResponse, isLoggedIn } from '@/utils/authentication';
import { defineComponent } from 'vue';

export default defineComponent({
    components: {
        BasePage,
        HintView,
        TempChart,
        SensorInfo,
    },
    data() {
        return {
            isLogged: isLoggedIn(),

            socket: {} as WebSocket,
            heartInterval: 0,
            dataAgeInterval: 0,
            dataLoading: true,
            liveData: {} as LiveData,
            isFirstFetchAfterConnect: true,
            isWebsocketConnected: false,
        };
    },
    created() {
        this.getInitialData();
        this.initSocket();
        this.$emitter.on('logged-in', () => {
            this.isLogged = this.isLoggedIn();
        });
        this.$emitter.on('logged-out', () => {
            this.isLogged = this.isLoggedIn();
        });
    },
    mounted() {
    },
    unmounted() {
        this.closeSocket();
    },
    updated() {
       //console.log('Updated');
    },
    computed: {
        sensorData(): Temperature[] {
            return this.liveData.temperatures;
        },
    },
    methods: {
        isLoggedIn,
        getInitialData(triggerLoading: boolean = true) {
            if (triggerLoading) {
                this.dataLoading = true;
            }
            fetch('/api/livedata/status', { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.liveData = data;
                    if (triggerLoading) {
                        this.dataLoading = false;
                    }
                });
        },
        reloadData() {
            this.closeSocket();

            setTimeout(() => {
                this.getInitialData(false);
                this.initSocket();
            }, 1000);
        },
        initSocket() {
            console.log('Starting connection to WebSocket Server');

            const { protocol, host } = location;
            const authString = authUrl();
            const webSocketUrl = `${protocol === 'https:' ? 'wss' : 'ws'}://${authString}${host}/livedata`;

            this.socket = new WebSocket(webSocketUrl);

            this.socket.onmessage = (event) => {
                //console.log(event);
                if (event.data != '{}') {
                    const newData = JSON.parse(event.data);
                    //console.log(newData);

                    Object.assign(this.liveData.temperatures, newData.temperatures);
                    //Object.assign(this.liveData.updates, newData.updates);
                   // Object.assign(this.liveData.updates2, newData.updates2);
                    Object.assign(this.liveData.hints, newData.hints);

                    let m = new Map<string, number>();

                    const serialList = Object.keys(newData.updates);
                    const valueList = Object.values(newData.updates) as number[];

                    for (let i = 0; i < serialList.length; i++) {
                        const serial = serialList[i];
                        const value = valueList[i];

                        if(serial !== undefined && value !== undefined) {
                            m.set(serial, value);
                        }
                    }
                    this.liveData.updates = m;
                    //this.liveData.updates2 = newData.updates;

                    //console.log(m);

                    this.dataLoading = false;
                    this.heartCheck(); // Reset heartbeat detection
                } else {
                    // Sometimes it does not recover automatically so have to force a reconnect
                    this.closeSocket();
                    this.heartCheck(10); // Reconnect faster
                }
            };

            this.socket.onopen = (event) => {
                console.log(event);
                console.log('Successfully connected to the echo websocket server...');
                this.isWebsocketConnected = true;
            };

            this.socket.onclose = () => {
                console.log('Connection to websocket closed...');
                this.isWebsocketConnected = false;
            };

            // Listen to window events , When the window closes , Take the initiative to disconnect websocket Connect
            window.onbeforeunload = () => {
                this.closeSocket();
            };
        },
        // Send heartbeat packets regularly * 59s Send a heartbeat
        heartCheck(duration: number = 59) {
            if (this.heartInterval) {
                clearTimeout(this.heartInterval);
            }
            this.heartInterval = setInterval(() => {
                if (this.socket.readyState === 1) {
                    // Connection status
                    this.socket.send('ping');
                } else {
                    this.initSocket(); // Breakpoint reconnection 5 Time
                }
            }, duration * 1000);
        },
        /** To break off websocket Connect */
        closeSocket() {
            this.socket.close();
            if (this.heartInterval) {
                clearTimeout(this.heartInterval);
            }
            this.isFirstFetchAfterConnect = true;
        },
    },
});
</script>

<style>
.btn-group {
    border-radius: var(--bs-border-radius);
    margin-top: 0.25rem;
}
</style>
