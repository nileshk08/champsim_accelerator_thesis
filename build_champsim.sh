#!/bin/bash

if [ "$#" -ne 9 ]; then
    echo "Illegal number of parameters"
    echo "Usage: ./build_champsim.sh [branch_pred] [itlb_pref] [dtlb_pref] [stlb_pref] [PTL2_pref] [dtlb_repl][stlb_repl] [PSCL2_repl] [num_accelerators]"
    exit 1
fi

# ChampSim configuration
BRANCH=$1           # branch/*.bpred
#L1D_PREFETCHER=$2   # prefetcher/*.l1d_pref
#L2C_PREFETCHER=$3   # prefetcher/*.l2c_pref
#LLC_PREFETCHER=$4   # prefetcher/*.llc_pref
ITLB_PREFETCHER=$2  # prefetcher/*.itlb_pref
DTLB_PREFETCHER=$3  # prefetcher/*.dtlb_pref
STLB_PREFETCHER=$4  # prefetcher/*.stlb_pref
PSCL2_PREFETCHER=$5  # prefetcher/*.stlb_pref
#LLC_REPLACEMENT=$9  # replacement/*.llc_repl
DTLB_REPLACEMENT=${6} #replacement /*.dtlb_repl
STLB_REPLACEMENT=${7} #replacement /*.stlb_repl
PSCL2_REPLACEMENT=${8} #replacement /*.pscl2_repl
NUM_CORE=0         # tested up to 8-core system
NUM_ACCELERATOR=${9} # number of accelerators

############## Some useful macros ###############
BOLD=$(tput bold)
NORMAL=$(tput sgr0)
#################################################

# Sanity check
if [ ! -f ./branch/${BRANCH}.bpred ]; then
    echo "[ERROR] Cannot find branch predictor"
	echo "[ERROR] Possible branch predictors from branch/*.bpred "
    find branch -name "*.bpred"
    exit 1
fi

#if [ ! -f ./prefetcher/${L1D_PREFETCHER}.l1d_pref ]; then
#    echo "[ERROR] Cannot find L1D prefetcher"
#	echo "[ERROR] Possible L1D prefetchers from prefetcher/*.l1d_pref "
#    find prefetcher -name "*.l1d_pref"
#    exit 1
#fi
#
#if [ ! -f ./prefetcher/${L2C_PREFETCHER}.l2c_pref ]; then
#    echo "[ERROR] Cannot find L2C prefetcher"
#	echo "[ERROR] Possible L2C prefetchers from prefetcher/*.l2c_pref "
#    find prefetcher -name "*.l2c_pref"
#    exit 1
#fi
#
#if [ ! -f ./prefetcher/${LLC_PREFETCHER}.llc_pref ]; then
#    echo "[ERROR] Cannot find LLC prefetcher"
#	echo "[ERROR] Possible LLC prefetchers from prefetcher/*.llc_pref "
#    find prefetcher -name "*.llc_pref"
#    exit 1
#fi

if [ ! -f ./prefetcher/${ITLB_PREFETCHER}.itlb_pref ]; then
    echo "[ERROR] Cannot find ITLB prefetcher"
	echo "[ERROR] Possible ITLB prefetchers from prefetcher/*.itlb_pref "
    find prefetcher -name "*.itlb_pref"
    exit 1
fi

if [ ! -f ./prefetcher/${DTLB_PREFETCHER}.dtlb_pref ]; then
    echo "[ERROR] Cannot find DTLB prefetcher"
        echo "[ERROR] Possible DTLB prefetchers from prefetcher/*.dtlb_pref "
    find prefetcher -name "*.dtlb_pref"
    exit 1
fi

if [ ! -f ./prefetcher/${STLB_PREFETCHER}.stlb_pref ]; then
    echo "[ERROR] Cannot find STLB prefetcher"
        echo "[ERROR] Possible STLB prefetchers from prefetcher/*.stlb_pref "
    find prefetcher -name "*.stlb_pref"
    exit 1
fi

if [ ! -f ./prefetcher/${PSCL2_PREFETCHER}.pscl2_pref ]; then
    echo "[ERROR] Cannot find PTL2 prefetcher"
        echo "[ERROR] Possible PTL2 prefetchers from prefetcher/*.pscl2_pref "
    find prefetcher -name "*.pscl2_pref"
    exit 1
fi
#if [ ! -f ./replacement/${LLC_REPLACEMENT}.llc_repl ]; then
#    echo "[ERROR] Cannot find LLC replacement policy"
#	echo "[ERROR] Possible LLC replacement policy from replacement/*.llc_repl"
#    find replacement -name "*.llc_repl"
#    exit 1
#fi
if [ ! -f ./replacement/${PSCL2_REPLACEMENT}.pscl2_repl ]; then
    echo "[ERROR] Cannot find PSCL2 replacement policy"
	echo "[ERROR] Possible PSCL2 replacement policy from replacement/*.llc_repl"
    find replacement -name "*.pscl2_repl"
    exit 1
fi
if [ ! -f ./replacement/${DTLB_REPLACEMENT}.pscl2_repl ]; then
    echo "[ERROR] Cannot find DTLB replacement policy"
	echo "[ERROR] Possible DTLB replacement policy from replacement/*.llc_repl"
    find replacement -name "*.dtlb_repl"
    exit 1
fi
if [ ! -f ./replacement/${STLB_REPLACEMENT}.pscl2_repl ]; then
    echo "[ERROR] Cannot find STLB replacement policy"
	echo "[ERROR] Possible STLB replacement policy from replacement/*.llc_repl"
    find replacement -name "*.stlb_repl"
    exit 1
fi

# Check num_core
re='^[0-9]+$'
if ! [[ $NUM_CORE =~ $re ]] ; then
    echo "[ERROR]: num_core is NOT a number" >&2;
    exit 1
fi

#echo "No of cores $NUM_CORE" 

# Check num_accelerators
if ! [[ $NUM_ACCELERATOR =~ $re ]] ; then
    echo "[ERROR]: num_core is NOT a number" >&2;
    exit 1
fi

#echo "NO of acceleratores $NUM_ACCELERATOR"

# Check for cores and accelerators
if [ "$NUM_CORE" -lt "0" ]; then
        echo "Number of core: $NUM_CORE must be greater or equal than 0"
        exit 1
fi
if [ "$NUM_ACCELERATOR" -lt "0" ]; then
        echo "Number of accelerator: $NUM_ACCELERATOR must be greater or equal than 0"
        exit 1
fi
TOTAL_CORES=$(( $NUM_ACCELERATOR + $NUM_CORE ))

# Check for multi-core
if [ "$TOTAL_CORES" -gt "0" ]; then
    echo "Building multi-core ChampSim..."
    sed -i.bak 's/\<NUM_CPUS 1\>/NUM_CPUS '${TOTAL_CORES}'/g' inc/champsim.h
    sed -i 's/\<ACCELERATOR_START 1\>/ACCELERATOR_START '${NUM_CORE}'/g' inc/champsim.h
#	sed -i.bak 's/\<DRAM_CHANNELS 1\>/DRAM_CHANNELS 2/g' inc/champsim.h
#	sed -i.bak 's/\<DRAM_CHANNELS_LOG2 0\>/DRAM_CHANNELS_LOG2 1/g' inc/champsim.h
else
    echo "Building single-core ChampSim..."
fi

# Change prefetchers and replacement policy
cp branch/${BRANCH}.bpred branch/branch_predictor.cc
#cp prefetcher/${L1D_PREFETCHER}.l1d_pref prefetcher/l1d_prefetcher.cc
#cp prefetcher/${L2C_PREFETCHER}.l2c_pref prefetcher/l2c_prefetcher.cc
#cp prefetcher/${LLC_PREFETCHER}.llc_pref prefetcher/llc_prefetcher.cc
cp prefetcher/${ITLB_PREFETCHER}.itlb_pref prefetcher/itlb_prefetcher.cc
cp prefetcher/${DTLB_PREFETCHER}.dtlb_pref prefetcher/dtlb_prefetcher.cc
cp prefetcher/${STLB_PREFETCHER}.stlb_pref prefetcher/stlb_prefetcher.cc
cp prefetcher/${PSCL2_PREFETCHER}.pscl2_pref prefetcher/pscl2_prefetcher.cc
#cp replacement/${LLC_REPLACEMENT}.llc_repl replacement/llc_replacement.cc
cp replacement/${PSCL2_REPLACEMENT}.pscl2_repl replacement/pscl2_replacement.cc
cp replacement/${DTLB_REPLACEMENT}.dtlb_repl replacement/dtlb_replacement.cc
cp replacement/${STLB_REPLACEMENT}.stlb_repl replacement/stlb_replacement.cc

# Build
mkdir -p bin
rm -f bin/champsim
make clean
make

# Sanity check
echo ""
if [ ! -f bin/champsim ]; then
    echo "${BOLD}ChampSim build FAILED!"
    echo ""
    exit 1
fi

echo "${BOLD}ChampSim is successfully built"
echo "Branch Predictor: ${BRANCH}"
#echo "L1D Prefetcher: ${L1D_PREFETCHER}"
#echo "L2C Prefetcher: ${L2C_PREFETCHER}"
#echo "LLC Prefetcher: ${LLC_PREFETCHER}"
echo "ITLB Prefetcher: ${ITLB_PREFETCHER}"
echo "DTLB Prefetcher: ${DTLB_PREFETCHER}"
echo "STLB Prefetcher: ${STLB_PREFETCHER}"
echo "PSCL2 Prefetcher: ${PSCL2_PREFETCHER}"
#echo "LLC Replacement: ${LLC_REPLACEMENT}"
echo "DTLB Replacement: ${DTLB_REPLACEMENT}"
echo "STLB Replacement: ${STLB_REPLACEMENT}"
echo "PSCL2 Replacement: ${PSCL2_REPLACEMENT}"
#echo "CPUs: ${NUM_CORE}"
#echo "CPU Range: 0 - $CPU_RANGE"
echo "Accelerators: ${NUM_ACCELERATOR} "
BINARY_NAME="${BRANCH}-${ITLB_PREFETCHER}-${DTLB_PREFETCHER}-${STLB_PREFETCHER}-${PSCL2_PREFETCHER}-${DTLB_REPLACEMENT}-${STLB_REPLACEMENT}-${PSCL2_REPLACEMENT}-${NUM_ACCELERATOR}accelerator"
echo "Binary: bin/${BINARY_NAME}"
echo " " 
mv bin/champsim bin/${BINARY_NAME}


# Restore to the default configuration
sed -i.bak 's/\<NUM_CPUS '${TOTAL_CORES}'\>/NUM_CPUS 1/g' inc/champsim.h
sed -i 's/\<ACCELERATOR_START '${NUM_CORE}'\>/ACCELERATOR_START 1/g' inc/champsim.h
#sed -i.bak 's/\<DRAM_CHANNELS 2\>/DRAM_CHANNELS 1/g' inc/champsim.h
#sed -i.bak 's/\<DRAM_CHANNELS_LOG2 1\>/DRAM_CHANNELS_LOG2 0/g' inc/champsim.h

cp branch/bimodal.bpred branch/branch_predictor.cc
#cp prefetcher/no.l1d_pref prefetcher/l1d_prefetcher.cc
#cp prefetcher/no.l2c_pref prefetcher/l2c_prefetcher.cc
#cp prefetcher/no.llc_pref prefetcher/llc_prefetcher.cc
cp prefetcher/no.itlb_pref prefetcher/itlb_prefetcher.cc
cp prefetcher/no.dtlb_pref prefetcher/dtlb_prefetcher.cc
cp prefetcher/no.stlb_pref prefetcher/stlb_prefetcher.cc
cp prefetcher/no.pscl2_pref prefetcher/pscl2_prefetcher.cc
#cp replacement/lru.llc_repl replacement/llc_replacement.cc
cp replacement/lru.pscl2_repl replacement/pscl2_replacement.cc
cp replacement/lru.dtlb_repl replacement/dtlb_replacement.cc
cp replacement/lru.stlb_repl replacement/stlb_replacement.cc
