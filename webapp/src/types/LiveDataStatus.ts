export interface Config {
    serial: number;
    name: string;
    valid: boolean;
}

export interface Update {
    serial: string;
    value: number;
}

export interface Hints {
    time_sync: boolean;
    default_password: boolean;
    radio_problem: boolean;
}

export interface LiveData {
    config: Config[];
    updates: Update[];
    hints: Hints;
}
