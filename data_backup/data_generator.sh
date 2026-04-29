#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DATA_DIR="$SCRIPT_DIR/"

EXECUTABLES=(stereo_baseline stereo_vbee stereo_vbee_ransac)
DATASETS=(all_days all_machine_halls)

rm -rf "$DATA_DIR/processed"
rm -rf "$DATA_DIR/output"

for dataset in "${DATASETS[@]}"; do
    for exe in "${EXECUTABLES[@]}"; do
        for iter in $(seq 1 5); do
            mkdir -p "$DATA_DIR/processed/$dataset/$exe/$iter"
            mkdir -p "$DATA_DIR/output"
            python3 fix_timestamps.py "$DATA_DIR/$dataset/$exe/$iter/CameraTrajectory.txt" "$DATA_DIR/processed/$dataset/$exe/$iter/CameraTrajectory.tum"
            python3 fix_timestamps.py "$DATA_DIR/$dataset/$exe/$iter/KeyFrameTrajectory.txt" "$DATA_DIR/processed/$dataset/$exe/$iter/KeyFrameTrajectory.tum"

            evo_ape tum "$DATASET_DIR/$dataset/mav0/state_groundtruth_estimate0/data.tum" "$DATA_DIR/processed/$dataset/$exe/$iter/CameraTrajectory.tum" --align -s --save_results "$DATA_DIR/output/${dataset}_${exe}_${iter}_cam_ape.zip"
            evo_ape tum "$DATASET_DIR/$dataset/mav0/state_groundtruth_estimate0/data.tum" "$DATA_DIR/processed/$dataset/$exe/$iter/KeyFrameTrajectory.tum" --align -s --save_results "$DATA_DIR/output/${dataset}_${exe}_${iter}_kf_ape.zip"

            evo_rpe tum "$DATASET_DIR/$dataset/mav0/state_groundtruth_estimate0/data.tum" "$DATA_DIR/processed/$dataset/$exe/$iter/CameraTrajectory.tum" --align -s --save_results "$DATA_DIR/output/${dataset}_${exe}_${iter}_cam_rpe.zip"
            evo_rpe tum "$DATASET_DIR/$dataset/mav0/state_groundtruth_estimate0/data.tum" "$DATA_DIR/processed/$dataset/$exe/$iter/KeyFrameTrajectory.tum" --align -s --save_results "$DATA_DIR/output/${dataset}_${exe}_${iter}_kf_rpe.zip"
        done
    done
done

wait
echo "All runs complete."
