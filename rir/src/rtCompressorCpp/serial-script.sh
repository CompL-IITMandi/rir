#!/bin/bash
#PBS -j oe

#specify queue name
#PBS -q serial

#specify job name
#PBS -N  rshRtCompressor

#specify time required for job completion
#PBS -l walltime=10:00:00

#to locate where it is running
echo "Running on: "
cat ${PBS_NODEFILE}

echo
echo "Program Output begins: "

cd ${PBS_O_WORKDIR}
./runner.sh -d input -h 1 -c 1
/
