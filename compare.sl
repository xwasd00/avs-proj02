#!/bin/bash
#SBATCH -p qcpu_exp
#SBATCH -A DD-23-135
#SBATCH -n 1 
#SBATCH -t 0:05:00
#SBATCH --mail-type END
#SBATCH -J AVS-compare
#SBATCH --output=compare-%j.out
#SBATCH --error=compare-%j.err

cd $SLURM_SUBMIT_DIR

ml intel-compilers/2022.1.0 CMake/3.23.1-GCCcore-11.3.0 Python

[ -d build_compare ] || mkdir build_compare
cd build_compare
rm -f *.obj

CC=icc CXX=icpc cmake ..
make

bash ../scripts/compare.sh
