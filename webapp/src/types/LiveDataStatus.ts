export interface Temperature {
    serial: number;
    name: string;
    valid: boolean;
}

export interface Hints {
    time_sync: boolean;
    default_password: boolean;
    radio_problem: boolean;
}

export interface LiveData {
    temperatures: Temperature[];
    updates: UpdateMap;
    hints: Hints;
}

export type UpdateMap = Map<string, number>;
