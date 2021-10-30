#!/bin/bash

. ./scripts/euler/config.sh

# Number of pending jobs
#   bbjobs -a | grep -o -i PENDING | wc -l
# Number of running jobs
#   bbjobs -a | grep -o -i RUNNING | wc -l
# Number of recently finished jobs
#   bbjobs -a | grep -o -i DONE | wc -l

pendingJobs=$(bbjobs -a | grep -o -i PENDING | wc -l)
runningJobs=$(bbjobs -a | grep -o -i RUNNING | wc -l)
finishedJobs=$(bbjobs -a | grep -o -i DONE | wc -l)
# suspended jobs

echo "$pendingJobs pending, $runningJobs running, $finishedJobs finished"