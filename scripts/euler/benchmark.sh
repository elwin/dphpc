. ./scripts/euler/config.sh

# Constants and variable init
EQUAL_MODE="eq"         # N=M --> only loop through values of N
UNEQUAL_MODE="neq"      # all combinations of N,M
LIN_MODE="linear"    # linear steps in N,M
EXP_MODE="exp"  # exponential steps in N,M
LOCAL_MODE="local"      # execute on local computer
CLUSTER_MODE="cluster"  # execute on cluster
CLEAN=0

############################################################
############################################################
# Parameters                                               #
############################################################
############################################################

# NOTES:
#   - This script has to be executed from the root (dphpc folder)
#   - The whole script can/should be configured only here

# Execution Mode (local node or on cluster)
EXECUTION_MODE=$CLUSTER_MODE
# Number of repeated executions
N_REPETITIONS=2
# name of implementations, can be overwritten by commandline argument
declare -a names=(allreduce)

# Number of threads to run with
declare -a nThreads=()
N_THREADS_START_POWER=2
N_THREADS_STEPS=2
N_THREADS_SCALE=$EXP_MODE
# initialize nThreads
for ((ti = 0; ti < N_THREADS_STEPS; ti += 1)); do
  if [[ $N_THREADS_SCALE == $LIN_MODE ]]; then
    nt=$((2 ** (N_THREADS_START_POWER) * (ti+1)))
    nThreads+=($nt)
  elif [[ $N_THREADS_SCALE == $EXP_MODE ]]; then
    nt=$((2 ** (N_THREADS_START_POWER + ti)))
    nThreads+=($nt)
  fi
done

### Initialize combinations of n,m for running experiments
# Values and number of iterations for N,M
declare -a nValues=()
declare -a mValues=()
# combination mode for n,m. Values {unequal, equal}
nm_mode=$UNEQUAL_MODE
# linear or exponential steps. Values {linear, exp}
nm_mode_scale=$LIN_MODE
# N,M are powers of 2
START_POWER_N=10
STEPS_N=2
START_POWER_M=10
STEPS_M=2
# Initialize nValues, mValues
for ((ni = 0; ni < STEPS_N; ni += 1)); do
  if [[ $nm_mode_scale == $LIN_MODE ]]; then
    n=$((2 ** (START_POWER_N) * (ni+1)))
  elif [[ $nm_mode_scale == $EXP_MODE ]]; then
    n=$((2 ** (START_POWER_N + ni)))
  fi

  # for mode with all combinations
  if [[ $nm_mode == $UNEQUAL_MODE ]]; then
    for ((mi = 0; mi < STEPS_M; mi += 1)); do
      if [[ $nm_mode_scale == $LIN_MODE ]]; then
        val=$((2 ** (START_POWER_M) * (mi+1)))
      elif [[ $nm_mode_scale == $EXP_MODE ]]; then
        val=$((2 ** (START_POWER_M + mi)))
      fi
      nValues+=($n)
      mValues+=("${val}")
    done
  elif [[ $nm_mode == $EQUAL_MODE ]]; then
    nValues+=($n)
    mValues+=($n)
  fi
done

############################################################
# Help                                                     #
############################################################
Help()
{
   # Display Help
   echo "Benchmark Bash Script"
   echo
   echo "Usage: name [-h] [-e $LOCAL_MODE|$CLUSTER_MODE] [-t N_THREADS] [-r N_REPETITIONS] -i NAME [-c]"
   echo "options:"
   echo "h     Print this Help."
   echo "e     Set execution mode. Values are {$LOCAL_MODE, $CLUSTER_MODE} (run on local node or using bsub on euler cluster)."
   echo "t     Number of Threads."
   echo "r     Number of repeated experiments to issue."
   echo "c     Delete old build and build again."
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
echo "[BENCHMARK CONFIGURATION] NM_MODE=$nm_mode, EXECUTION_MODE=$EXECUTION_MODE, N_THREADS=(${nThreads[*]}), N_REPETITIONS=$N_REPETITIONS, IMPLEMENTATIONS=(${names[*]}), clean=$CLEAN"

############################################################
############################################################
# Main program                                             #
############################################################
############################################################

# prepare environment if in cluster mode
if [[ $EXECUTION_MODE == $CLUSTER_MODE ]]; then
  . ./scripts/euler/init.sh
fi

if [ $CLEAN -eq 1 ]; then
  echo "CLEANING UP $code_dir"
  make -C $code_dir clean
  make -C $code_dir build
fi

# check that n,m values are of equal length
if [[ ${#nValues[@]} != ${#mValues[@]} ]]; then
  echo "Input values nValues, mValues are not of equal length"
  exit 1
fi

# different repetitions
for ((rep = 1; rep <= $N_REPETITIONS; rep += 1)); do
  outDir="$bb_output_dir/$rep"
  mkdir -p $outDir

  # run experiment over all implementations
  for IMPLEMENTATION in "${names[@]}"; do

    # input sizes
    for ((i = 0; i < ${#nValues[@]}; i += 1)); do
      n=${nValues[i]}
      m=${mValues[i]}

      # input sizes
      for ((j = 0; j < ${#nThreads[@]}; j += 1)); do
        t=${nThreads[j]}

        # Run locally or on cluster
        if [[ $EXECUTION_MODE == $LOCAL_MODE ]]; then
          OUTPUT_PATH=${bb_output_dir}/${rep}/output_i_${IMPLEMENTATION}_t_${N_THREADS}_n_${n}_m_${m}_rep_${rep}.txt
          echo "Running n_threads=$t, i=$IMPLEMENTATION, n=$n, m=$m, repetition=$rep"

          echo "$(mpirun -np ${t} ${build_dir}/main -n $n -m $m -i $IMPLEMENTATION)" >> $OUTPUT_PATH

        elif [[ $EXECUTION_MODE == $CLUSTER_MODE ]]; then
          jobMsg=$(bsub -oo "${outDir}/%J" -n ${t} "mpirun -np ${t} ${build_dir}/main -n ${n} -m ${m} -i ${IMPLEMENTATION}")
          # jobMsg='asdf<123>asdf<123'

          IFS='>'
          read -a jobMsgSplit <<< "$jobMsg"
          IFS='<'
          read -a jobID <<< "$jobMsgSplit"
          echo "${rep}/${jobID[1]}" >> $job_file

          # Write the overview file
          echo "${jobID[1]}::implementation:$IMPLEMENTATION::n:$n::m:$m::rep:$rep" >> $job_overview_file

          echo "Issued n_threads=$t, i=$IMPLEMENTATION, n=$n, m=$m, repetition=$rep. JOB-ID: ${jobID[1]}"
        fi

      done
    done
  done
done

