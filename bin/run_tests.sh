#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DATA_DIR="$SCRIPT_DIR/../data"

EXECUTABLES=(stereo_baseline stereo_vbee stereo_vbee_ransac)
DATASETS=(all_days all_machine_halls)
MAX_JOBS=5

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

    sleep $((RANDOM % 121))
    mkdir -p "$work_dir"
    echo "[$(date '+%H:%M:%S')] START  $exe / $dataset / $iter"
    (
        cd "$work_dir"
        echo "Start time: $(date '+%Y-%m-%d %H:%M:%S')" >"stdout.log"
        "$SCRIPT_DIR/$exe" "$dataset" >>"stdout.log" 2>"stderr.log"
    )
    echo "[$(date '+%H:%M:%S')] DONE   $exe / $dataset / $iter (exit $?)"
}

jobs_list=()
for dataset in "${DATASETS[@]}"; do
    for exe in "${EXECUTABLES[@]}"; do
        for iter in $(seq 1 15); do
            jobs_list+=("$dataset $exe $iter")
        done
    done
done

while IFS= read -r job; do
    dataset="${job%% *}"; rest="${job#* }"
    exe="${rest%% *}"; iter="${rest##* }"
    if [ -f "$DATA_DIR/$dataset/$exe/$iter/CameraTrajectory.txt" ]; then
        echo "Skipping $exe / $dataset / $iter (already exists)"
    else
        wait_for_slot
        run_slam "$exe" "$dataset" "$iter" &
    fi
done < <(printf '%s\n' "${jobs_list[@]}" | shuf)

wait
echo "All runs complete."
