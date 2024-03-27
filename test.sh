#!/bin/bash
#author: andrei dodot

NPROCS=5

NSTEPS1=(1 2 3 4 5 6 7 8 9 10 11)
NSTEPS2=(1 2 3 4 5 6 7 8 9 10 37 38)
NSTEPS3=1450

INPUT_DIR="./in"

input_file="in_2.txt"
input_file_id=$(echo $input_file | cut -c 4)

run_test() {
    for steps in "${NSTEPS3[@]}"; do
        output_file="./myresults/out_${input_file_id}_${steps}.txt"
        echo "Testing $input_file with $steps steps..."
        mpirun -np $NPROCS ./homework "${INPUT_DIR}/${input_file}" "${output_file}" $steps
        diff -q "${output_file}" "./out/out_${input_file_id}_${steps}.txt"
        if [ $? -eq 0 ]; then
            echo "Test passed for $input_file with $steps steps"
        else
            echo "Test failed for $input_file with $steps steps"
        fi
    done
}

run_test 
