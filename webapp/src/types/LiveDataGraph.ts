export interface SingleConfigGraph {
    name: string;
    color: string;
}

export interface ConfigGraph {
    config: SingleConfigGraph[];
}

export interface DPoint {
    x: number;
    y: number;
}

export interface SensorDataList {
    SingleSensor: DPoint[];
}

export type UpdateMap = Map<string, number>;

/*
export const validDataNames = {
    data_pro3em: 1,
    data_pro3em_min: 2,
    data_pro3em_max: 3,
    data_plugs: 4,
    data_plugs_min: 5,
    data_plugs_max: 6,
    data_calculated_limit: 7,
    data_limit: 8,
};
*/
/*
export interface SingleGraph {
    data_name: keyof typeof validDataNames;
    label: string;
    color: string;
}
    */
/*
export interface ShellyCard {
    pro3em_value: number;
    pro3em_debug: string;
    plugs_value: number;
    plugs_debug: string;
    limit_value: number;
    limit_debug: string;

}

export interface LiveDataGraph {
    view_option: number;
    timestamp: string;
    interval: number;

    cards: ShellyCard;

    diagram_pro3em: SingleGraph[];
    diagram_plugs: SingleGraph[];
    diagram_limit: SingleGraph[];
    diagram_all: SingleGraph[];

    data_pro3em: string;
    data_pro3em_min: string;
    data_pro3em_max: string;

    data_plugs: string;
    data_plugs_min: string;
    data_plugs_max: string;

    data_calculated_limit: string;
    data_limit: string;
}
*/
