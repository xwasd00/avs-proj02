#!/bin/bash
#SBATCH -p qcpu_exp
#SBATCH -A DD-23-135
#SBATCH -n 1 
#SBATCH -t 0:05:00
#SBATCH --mail-type END
#SBATCH -J AVS-run
#SBATCH --output=run-%j.out
#SBATCH --error=run-%j.err

cd $SLURM_SUBMIT_DIR

ml intel-compilers/2022.1.0 CMake/3.23.1-GCCcore-11.3.0

[ -d build ] || mkdir build
cd build
rm -f *.obj

CC=icc CXX=icpc cmake ..
make

#for threads in 18 36; do
for threads in 18; do
    #for builder in "ref" "loop" "tree"; do
    for builder in "loop"; do
        # ref Elapsed Time: cca 23.5 s
        # loop 18 : cca 1.3 s
        # loop 36 : cca 0.7 s
        ./PMC --builder ${builder} -t ${threads} --grid 128 ../data/bun_zipper_res3.pts ${builder}-${threads}-bun_zipper_res3.obj
    done
done
