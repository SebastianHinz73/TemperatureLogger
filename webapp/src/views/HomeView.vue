<template>
    <BasePage :title="$t('home.LiveData')" :isLoading="dataLoading" :isWideScreen="true">
        <HintView :hints="liveData.hints" />
        <SensorInfo :sensorData="liveData.temperatures" /><br />
    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import HintView from '@/components/HintView.vue';
import SensorInfo from '@/components/SensorInfo.vue';
import type { Temperature, LiveData } from '@/types/LiveDataStatus';
import { authHeader, authUrl, handleResponse, isLoggedIn } from '@/utils/authentication';


import { defineComponent } from 'vue';

export default defineComponent({
    components: {
        BasePage,
        HintView,
        SensorInfo
    },
    data() {
        return {
            isLogged: this.isLoggedIn(),

            socket: {} as WebSocket,
            heartInterval: 0,
            dataAgeInterval: 0,
            dataLoading: true,
            liveData: {} as LiveData,
            isFirstFetchAfterConnect: true,
        };
    },
    created() {
        this.getInitialData();
        this.initSocket();
        this.$emitter.on("logged-in", () => {
            this.isLogged = this.isLoggedIn();
        });
        this.$emitter.on("logged-out", () => {
            this.isLogged = this.isLoggedIn();
        });
    },
    mounted() {
    },
    unmounted() {
        this.closeSocket();
    },
    updated() {
    },
    computed: {
        sensorData(): Temperature[] {
            return this.liveData.temperatures;
        }
    },
    methods: {
        isLoggedIn,
        getInitialData() {
            this.dataLoading = true;
            fetch("/api/livedata/status", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.liveData = data;
                    this.dataLoading = false;
                });
        },
        initSocket() {
            console.log("Starting connection to WebSocket Server");

            const { protocol, host } = location;
            const authString = authUrl();
            const webSocketUrl = `${protocol === "https:" ? "wss" : "ws"
                }://${authString}${host}/livedata`;

            this.socket = new WebSocket(webSocketUrl);

            this.socket.onmessage = (event) => {
                if (event.data != "{}") {
                    this.liveData = JSON.parse(event.data);
                    this.dataLoading = false;
                    this.heartCheck(); // Reset heartbeat detection
                } else {
                    // Sometimes it does not recover automatically so have to force a reconnect
                    this.closeSocket();
                    this.heartCheck(10); // Reconnect faster
                }
            };

            this.socket.onopen = function (event) {
                console.log(event);
                console.log("Successfully connected to the echo websocket server...");
            };

            // Listen to window events , When the window closes , Take the initiative to disconnect websocket Connect
            window.onbeforeunload = () => {
                this.closeSocket();
            };
        },
        // Send heartbeat packets regularly * 59s Send a heartbeat
        heartCheck(duration: number = 59) {
            this.heartInterval && clearTimeout(this.heartInterval);
            this.heartInterval = setInterval(() => {
                if (this.socket.readyState === 1) {
                    // Connection status
                    this.socket.send("ping");
                } else {
                    this.initSocket(); // Breakpoint reconnection 5 Time
                }
            }, duration * 1000);
        },
        /** To break off websocket Connect */
        closeSocket() {
            this.socket.close();
            this.heartInterval && clearTimeout(this.heartInterval);
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