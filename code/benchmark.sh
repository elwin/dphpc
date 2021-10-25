#!/bin/bash

set -e # Exit on first failure
# Constants and variable init
EQUAL_MODE="eq"
UNEQUAL_MODE="neq"
CLEAN=0

############################################################
############################################################
# Parameters                                               #
############################################################
############################################################

# Start values and number of iterations for N,M
# N,M are powers of 2
START_POWER_N=2
STEPS_N=2
START_POWER_M=2
STEPS_M=2
# name of implementations, can be overwritten by commandline argument
declare -a names=(allreduce allreduce2)
# combination mode for n,m. Values {unequal, equal}
mode=$UNEQUAL_MODE

############################################################
# Help                                                     #
############################################################
Help()
{
   # Display Help
   echo "Benchmark Bash Script"
   echo
   echo "Usage: name [-h] [-m MODE] -i NAME [-c]"
   echo "options:"
   echo "h     Print this Help."
   echo "c     Delete old build before building again."
   echo "m     Set mode. Values are {$EQUAL_MODE, $UNEQUAL_MODE} (either equal (N=M) or unequal (all combinations))."
   echo "i     Can set implementation name, when wanting to run a single implementation"
   echo
   exit 1;
}

while getopts ":h:m:i:c" option; do
   case $option in
      h) # display Help
          Help
          exit;;
      m) # set mode
          mode=${OPTARG}
          if [[ ! ( $mode == $EQUAL_MODE || $mode == $UNEQUAL_MODE ) ]]; then
            Help
          fi
          ;;
      i) # Enter an implementation name
          names=("${OPTARG}");;
      c) # Clean old build
          CLEAN=$[1]
          ;;
      \?) # Invalid option
          echo "Error: Invalid option"
          Help
          exit;;
   esac
done

############################################################
############################################################
# Main program                                             #
############################################################
############################################################

# Experiment Configuration
BUILD_DIR=build_output
OUTPUT_DIR=results
OUTPUT_PATH=${OUTPUT_DIR}/output.csv

if [ $CLEAN -eq 1 ]; then
  echo "CLEANING UP"
  rm -rf ${BUILD_DIR}
  rm -rf cmake-build-debug
  rm -rf ${OUTPUT_DIR}
fi

# build
mkdir -p ${OUTPUT_DIR}
cmake -S . -B ${BUILD_DIR}
cmake --build ${BUILD_DIR}

# run experiment over all implementations
for IMPLEMENTATION in "${names[@]}"; do
  n=$START_N
  for ((ni = 1; ni <= $STEPS_N; ni += 1)); do
    n=$[2 ** ($START_N + $ni)]
    declare -a mValues=()

    # for mode with all combinations
    if [[ $mode == $UNEQUAL_MODE ]]; then
      echo "UNEQUAL MODE"
      for ((mi = 1; mi <= $STEPS_M; mi += 1)); do

        # val=$[$START_M ** $mi]
        val=$[2 ** ($START_M + $mi)]
        mValues+=("${val}")

      done
    elif [[ $mode == $EQUAL_MODE ]]; then
      mValues+=($n)
    fi

    # Execute
    for m in "${mValues[@]}"; do
      echo "Running n=$n, m=$m, i=$IMPLEMENTATION"
      # TODO: Make output fitting to the output format
      echo "Executing: mpiexec ${BUILD_DIR}/main -n $n -m $m -i $IMPLEMENTATION"
      echo "$(mpiexec ${BUILD_DIR}/main -n $n -m $m -i $IMPLEMENTATION)" >>$OUTPUT_PATH
    done
  done
done

