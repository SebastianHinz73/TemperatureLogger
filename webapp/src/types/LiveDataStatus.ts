import type { UpdateMap } from '@/types/LiveDataGraph';

export interface Temperature {
    serial: number;
    name: string;
    time: number;
    value: number;
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
