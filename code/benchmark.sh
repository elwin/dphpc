#!/bin/bash

set -e # Exit on first failure

# Constants and variable init
EQUAL_MODE="eq"         # N=M --> only loop through values of N
UNEQUAL_MODE="neq"      # all combinations of N,M
LOCAL_MODE="local"      # execute on local computer
CLUSTER_MODE="cluster"  # execute on cluster
CLEAN=0

############################################################
############################################################
# Parameters                                               #
############################################################
############################################################

# Number of threads to run with
N_THREADS=8
# Execution Mode (local node or on cluster)
EXECUTION_MODE=$LOCAL_MODE
# Number of repeated executions
N_REPETITIONS=2
# Start values and number of iterations for N,M
# N,M are powers of 2
START_POWER_N=10
STEPS_N=2
START_POWER_M=10
STEPS_M=2
# name of implementations, can be overwritten by commandline argument
declare -a names=(allreduce)
# combination mode for n,m. Values {unequal, equal}
nm_mode=$UNEQUAL_MODE

############################################################
# Help                                                     #
############################################################
Help()
{
   # Display Help
   echo "Benchmark Bash Script"
   echo
   echo "Usage: name [-h] [-m NM_MODE] [-e EXECUTION_MODE] [-t N_THREADS] [-r N_REPETITIONS] -i NAME [-c]"
   echo "options:"
   echo "h     Print this Help."
   echo "m     Set input size mode. Values are {$EQUAL_MODE, $UNEQUAL_MODE} (either equal (N=M) or unequal (all combinations))."
   echo "e     Set execution mode. Values are {$LOCAL_MODE, $CLUSTER_MODE} (run on local node or using bsub on euler cluster)."
   echo "t     Number of Threads."
   echo "r     Number of repeated experiments to issue."
   echo "c     Delete old build before building again."
   echo "i     Can set implementation name, when wanting to run a single implementation"
   echo
   exit 1;
}

while getopts "hm:e:t:r:i:c" option; do
   case $option in
      h) # display Help
          Help
          exit;;
      m) # set mode
          nm_mode=${OPTARG}
          if [[ ! ( $nm_mode == $EQUAL_MODE || $nm_mode == $UNEQUAL_MODE ) ]]; then
            Help
          fi
          ;;
      e) # execution mode
          EXECUTION_MODE=${OPTARG}
          if [[ ! ( $EXECUTION_MODE == $LOCAL_MODE || $EXECUTION_MODE == $CLUSTER_MODE ) ]]; then
            Help
          fi
          ;;
      t) # number of threads
          N_THREADS=("${OPTARG}");;
      r) # repetitions
          N_REPETITIONS=("${OPTARG}");;
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
echo "[BENCHMARK CONFIGURATION] NM_MODE=$nm_mode, EXECUTION_MODE=$EXECUTION_MODE, N_THREADS=$N_THREADS, N_REPETITIONS=$N_REPETITIONS, IMPLEMENTATIONS=(${names[*]}), clean=$CLEAN"

############################################################
############################################################
# Main program                                             #
############################################################
############################################################

# Experiment Configuration
BUILD_DIR=build_output
OUTPUT_DIR=results/$(date "+%Y.%m.%d-%H.%M.%S")
mkdir -p results
mkdir -p ${OUTPUT_DIR}
JOB_OVERVIEW_FILE="$OUTPUT_DIR/jobs.txt"

# prepare environment if in cluster mode
if [[ $EXECUTION_MODE == $CLUSTER_MODE ]]; then
  source /cluster/apps/local/env2lmod.sh
  module load gcc/8.2.0 cmake/3.20.3 openmpi/4.0.2
fi

if [ $CLEAN -eq 1 ]; then
  echo "CLEANING UP"
  make clean
fi

# build
make build

# run experiment over all implementations
for IMPLEMENTATION in "${names[@]}"; do
  n=$START_N
  for ((ni = 1; ni <= $STEPS_N; ni += 1)); do
    n=$[2 ** ($START_N + $ni)]
    declare -a mValues=()

    # for mode with all combinations
    if [[ $nm_mode == $UNEQUAL_MODE ]]; then
      for ((mi = 1; mi <= $STEPS_M; mi += 1)); do

        # val=$[$START_M ** $mi]
        val=$[2 ** ($START_M + $mi)]
        mValues+=("${val}")

      done
    elif [[ $nm_mode == $EQUAL_MODE ]]; then
      mValues+=($n)
    fi

    # Execute
    for m in "${mValues[@]}"; do
      # Execute different repetitions
      for ((rep = 1; rep <= $N_REPETITIONS; rep += 1)); do
        OUTPUT_PATH=${OUTPUT_DIR}/output_i_${IMPLEMENTATION}_t_${N_THREADS}_n_${n}_m_${m}_rep_${rep}.txt

        # Run locally or on cluster
        if [[ $EXECUTION_MODE == $LOCAL_MODE ]]; then
          echo "Running n_threads=$N_THREADS, i=$IMPLEMENTATION, n=$n, m=$m, repetition=$rep"

          echo "$(mpirun -np ${N_THREADS} ${BUILD_DIR}/main -n $n -m $m -i $IMPLEMENTATION)" >>$OUTPUT_PATH

        elif [[ $EXECUTION_MODE == $CLUSTER_MODE ]]; then
          jobMsg=$(bsub -o ${OUTPUT_PATH} -n ${N_THREADS} mpirun -np ${N_THREADS} ${BUILD_DIR}/main -n $n -m $m -i $IMPLEMENTATION)

          IFS='>'
          read -a jobMsgSplit <<< "$jobMsg"
          IFS='<'
          read -a jobID <<< "$jobMsgSplit"
          echo "${jobID[1]}}" >> $JOB_OVERVIEW_FILE

          echo "Issued n_threads=$N_THREADS, i=$IMPLEMENTATION, n=$n, m=$m, repetition=$rep"
        fi

      done
    done
  done
done

