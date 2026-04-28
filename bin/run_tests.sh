#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DATA_DIR="$SCRIPT_DIR/../data"

EXECUTABLES=(stereo_baseline stereo_vbee stereo_vbee_ransac)
DATASETS=(all_days all_machine_halls)
MAX_JOBS=3

wait_for_slot() {
    while [ "$(jobs -rp | wc -l)" -ge "$MAX_JOBS" ]; do
        wait -n 2>/dev/null || sleep 0.2
    done
}

run_slam() {
    local exe="$1"
    local dataset="$2"
    local iter="$3"
    local work_dir="$DATA_DIR/$dataset/$exe/$iter"

    mkdir -p "$work_dir"
    echo "[$(date '+%H:%M:%S')] START  $exe / $dataset / $iter"
    (
        cd "$work_dir"
        "$SCRIPT_DIR/$exe" "$dataset" >"stdout.log" 2>"stderr.log"
    )
    echo "[$(date '+%H:%M:%S')] DONE   $exe / $dataset / $iter (exit $?)"
}

for dataset in "${DATASETS[@]}"; do
    for exe in "${EXECUTABLES[@]}"; do
        for iter in $(seq 1 5); do
            wait_for_slot
            run_slam "$exe" "$dataset" "$iter" &
        done
    done
done

wait
echo "All runs complete."
