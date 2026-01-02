<template>
    <BasePage :title="$t('about.AboutOpendtu')">
        <div class="accordion" id="accordionExample">
            <div class="accordion-item">
                <h2 class="accordion-header" id="headingOne">
                    <button
                        class="accordion-button collapsed"
                        type="button"
                        data-bs-toggle="collapse"
                        data-bs-target="#collapseOne"
                        aria-expanded="false"
                        aria-controls="collapseOne"
                    >
                        <span class="badge text-bg-secondary"> <BIconInfoCircle class="fs-4" /> </span>&nbsp;{{
                            $t('about.ProjectOrigin')
                        }}
                    </button>
                </h2>
                <div
                    id="collapseOne"
                    class="accordion-collapse collapse"
                    aria-labelledby="headingOne"
                    data-bs-parent="#accordionExample"
                >
                    <div class="accordion-body">
                        <p class="fw-normal" v-html="$t('about.ProjectOriginBody1')"></p>
                        <p class="fw-normal" v-html="$t('about.ProjectOriginBody2')"></p>
                        <p class="fw-normal" v-html="$t('about.ProjectOriginBody3')"></p>
                        <p class="fw-normal" v-html="$t('about.ProjectOriginBody4')"></p>
                    </div>
                </div>
            </div>

        </div>

    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import { BIconInfoCircle } from 'bootstrap-icons-vue';
import { defineComponent } from 'vue';

export default defineComponent({
    components: {
        BasePage,
        BIconInfoCircle,
    },
});
</script>
