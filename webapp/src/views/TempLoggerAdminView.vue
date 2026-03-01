<template>
    <BasePage :title="$t('temploggeradmin.TempLoggerSettings')" :isLoading="dataLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>
        <form @submit="saveTempLoggerConfig">
            <CardElement :text="$t('temploggeradmin.CommonConfiguration')" textVariant="text-bg-primary">
                <InputElement :label="$t('temploggeradmin.DS18B20PollInterval')" v-model="tempLoggerConfigList.pollinterval"
                    type="number" min="5" max="3600" :postfix="$t('temploggeradmin.Seconds')" />
                <InputElement :label="$t('temploggeradmin.DS18B20Fahrenheit')" v-model="tempLoggerConfigList.fahrenheit"
                    type="checkbox" />
            </CardElement>

            <CardElement :text="$t('temploggeradmin.SensorList')" textVariant="text-bg-primary" add-space>
                <div class="table-responsive">
                    <table class="table">
                        <thead>
                            <tr>
                                <th>#</th>
                                <th scope="col">{{ $t('temploggeradmin.Status') }}</th>
                                <th>{{ $t('temploggeradmin.Serial') }}</th>
                                <th>{{ $t('temploggeradmin.Name') }}</th>
                                <th>{{ $t('temploggeradmin.Action') }}</th>
                            </tr>
                        </thead>
                        <tbody ref="invList">
                            <tr v-for="item in tempLoggerConfigList.sensors" v-bind:key="item.serial"
                                :data-id="item.serial">
                                <td>
                                    <BIconGripHorizontal class="drag-handle" />
                                </td>
                                <td>
                                    <span class="icon">
                                        <BIconCheckCircle v-if="item.connected" />
                                    </span>
                                    <span class="icon text-danger">
                                        <BIconExclamationCircle v-if="!item.connected" />
                                    </span>
                                    <span class="icon">
                                        &nbsp;
                                        <BIconEyeSlash v-if="!item.visible"/>
                                    </span>
                                </td>
                                <td>{{ item.serial.toUpperCase() }}</td>
                                <td>{{ item.name }}</td>
                                <td>
                                    <BIconTrash v-if="item.connected" :title="$t('temploggeradmin.DeleteSensor')" />
                                    <a href="#" class="icon text-danger" :title="$t('temploggeradmin.DeleteSensor')">
                                        <BIconTrash v-if="!item.connected" v-on:click="onOpenModal(modalDelete, item)" />
                                    </a>&nbsp;
                                    <a href="#" class="icon" :title="$t('temploggeradmin.EditSensor')">
                                        <BIconPencil v-on:click="onOpenModal(modal, item)" />
                                    </a>
                                </td>
                            </tr>
                        </tbody>
                    </table>
                </div>
            </CardElement>

            <FormFooter @reload="getTempLoggerConfig" />
        </form>

    </BasePage>

    <div class="modal" id="sensorEdit" tabindex="-1">
        <div class="modal-dialog modal-lg">
            <div class="modal-content">
                <div class="modal-header">
                    <h5 v-if="selectedSensorData.serial !== undefined" class="modal-title">{{
                        $t('temploggeradmin.EditSensor') + ' ' + selectedSensorData.serial.toUpperCase() }}</h5>
                    <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="Close"></button>
                </div>
                <div class="modal-body">
                    <InputElement :label="$t('temploggeradmin.Name')" type="text" maxlength="31"
                        v-model="selectedSensorData.name" />
                    <InputElement :label="$t('temploggeradmin.SensorVisible')" type="checkbox"
                        v-model="selectedSensorData.visible" />
                </div>
                <div class="modal-footer">
                    <button type="button" class="btn btn-secondary" @click="onCloseModal(modal)" data-bs-dismiss="modal">{{
                        $t('temploggeradmin.Cancel') }}</button>
                    <button type="button" class="btn btn-primary" @click="onEditSubmit">
                        {{ $t('temploggeradmin.Save') }}</button>
                </div>
            </div>
        </div>
    </div>

    <div class="modal" id="sensorDelete" tabindex="-1">
        <div class="modal-dialog">
            <div class="modal-content">
                <div class="modal-header">
                    <h5 v-if="selectedSensorData.serial !== undefined" class="modal-title">{{
                        $t('temploggeradmin.DeleteSensor') + ' ' + selectedSensorData.serial.toUpperCase() }}</h5>
                    <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="Close"></button>
                </div>

                <div v-if="selectedSensorData.serial !== undefined" class="modal-body">
                    {{ $t('temploggeradmin.DeleteMsg', {
                        name: selectedSensorData.name,
                    })
                    }}
                </div>
                <div class="modal-footer">
                    <button type="button" class="btn btn-secondary" @click="onCloseModal(modalDelete)"
                        data-bs-dismiss="modal">{{ $t('temploggeradmin.Cancel') }}</button>
                    <button type="button" class="btn btn-danger" @click="onDelete">
                        {{ $t('temploggeradmin.Delete') }}</button>
                </div>
            </div>
        </div>
    </div>
</template>


<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from "@/components/BootstrapAlert.vue";
import CardElement from '@/components/CardElement.vue';
import FormFooter from '@/components/FormFooter.vue';
import InputElement from '@/components/InputElement.vue';
import type { DS18B20Info, TempLoggerConfig } from "@/types/TempLoggerConfig";
import Sortable from 'sortablejs';
import { authHeader, handleResponse } from '@/utils/authentication';
import * as bootstrap from 'bootstrap';
import {
    BIconPencil,
    BIconTrash,
    BIconEyeSlash,
    BIconGripHorizontal,
    BIconExclamationCircle,
    BIconCheckCircle
} from 'bootstrap-icons-vue';
import { defineComponent } from 'vue';

declare interface AlertResponse {
    message: string;
    type: string;
    code: number;
    show: boolean;
}

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
        CardElement,
        FormFooter,
        InputElement,
        BIconPencil,
        BIconTrash,
        BIconEyeSlash,
        BIconGripHorizontal,
        BIconExclamationCircle,
        BIconCheckCircle
    },
    data() {
        return {
            modal: {} as bootstrap.Modal,
            modalDelete: {} as bootstrap.Modal,
            tempLoggerConfigList: {} as TempLoggerConfig,
            selectedSensorData: {} as DS18B20Info,
            dataLoading: true,
            alertMessage: "",
            alertType: "info",
            showAlert: false,
            alert: {} as AlertResponse,
            sortable: {} as Sortable,
        };
    },
    mounted() {
        this.modal = new bootstrap.Modal('#sensorEdit');
        this.modalDelete = new bootstrap.Modal('#sensorDelete');
    },
    created() {
        this.getTempLoggerConfig();
    },
    methods: {
        getTempLoggerConfig() {
            this.dataLoading = true;
            fetch("/api/templogger/config", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.tempLoggerConfigList = data;
                    this.dataLoading = false;
                    this.$nextTick(() => {
                        const table = this.$refs.invList as HTMLElement;

                        this.sortable = Sortable.create(table, {
                            sort: true,
                            handle: '.drag-handle',
                            animation: 150,
                            draggable: 'tr',
                        });
                    });
                });
        },
        saveTempLoggerConfig(e: Event) {
            e.preventDefault();
            const sortArray = this.sortable.toArray();

            this.tempLoggerConfigList.sensors.sort((a, b) => {
                return sortArray.indexOf(a.serial) - sortArray.indexOf(b.serial);
            });

            const formData = new FormData();
            formData.append("data", JSON.stringify(this.tempLoggerConfigList));

            fetch("/api/templogger/config", {
                method: "POST",
                headers: authHeader(),
                body: formData,
            })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then(
                    (response) => {
                        this.alertMessage = this.$t('apiresponse.' + response.code, response.param);
                        this.alertType = response.type;
                        this.showAlert = true;
                    }
                );
        },
        onDelete() {
            const found = this.tempLoggerConfigList.sensors.find((element) => element.serial === this.selectedSensorData.serial);
            if (found !== undefined) {
                const index = this.tempLoggerConfigList.sensors.indexOf(found);
                if (index > -1) { // only splice array when item is found
                    this.tempLoggerConfigList.sensors.splice(index, 1); // 2nd parameter means remove one item only
                }
            }
            this.onCloseModal(this.modalDelete);
        },
        onEditSubmit() {
            const found = this.tempLoggerConfigList.sensors.find((element) => element.serial === this.selectedSensorData.serial);
            if (found !== undefined) {
                const index = this.tempLoggerConfigList.sensors.indexOf(found);
                if (index > -1 && this.tempLoggerConfigList.sensors[index] !== undefined) { // only splice array when item is found
                    this.tempLoggerConfigList.sensors[index].name = this.selectedSensorData.name;
                    this.tempLoggerConfigList.sensors[index].visible = this.selectedSensorData.visible;
                }
            }

            this.onCloseModal(this.modal);
        },
        onOpenModal(modal: bootstrap.Modal, sensor: DS18B20Info) {
            // deep copy ds18b20 object for editing/deleting
            this.selectedSensorData = JSON.parse(JSON.stringify(sensor)) as DS18B20Info;
            modal.show();
        },
        onCloseModal(modal: bootstrap.Modal) {
            modal.hide();
        },
    },
});
</script>

<style>
.drag-handle {
    cursor: grab;
}
</style>
