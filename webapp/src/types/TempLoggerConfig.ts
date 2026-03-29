
export interface DS18B20Info {
    serial: string;
    connected: boolean;
    visible: boolean;
    name: string;
}

export interface TempLoggerConfig {
    pollinterval: number;
    fahrenheit: boolean;
    sensors: Array<DS18B20Info>;
}

