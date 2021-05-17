#!/bin/bash

if [ "$#" -lt 6 ] || [ "$#" -gt 7 ]; then
    echo "Illegal number of parameters"
    echo "Usage: ./run_2core.sh [BINARY] [N_WARM] [N_SIM] [N_MIX] [TRACE0] [ASID0] [OPTION]" 
    exit 1
fi

TRACE_DIR=$PWD/accelerator_traces
CONTEXT_SWITCH_DIR=$PWD/context_switch
BINARY=${1}
N_WARM=${2}
N_SIM=${3}
N_MIX=${4}
TRACE0=${5}
ASID0=${6}
OPTION=${7}
#CONTEXT_SWITCH=${9}
#OPTION=${10}

#echo ${CONTEXT_SWITCH}
#echo ${OPTION}
# Sanity check
if [ -z $TRACE_DIR ] || [ ! -d "$TRACE_DIR" ] ; then
    echo "[ERROR] Cannot find a trace directory: $TRACE_DIR"
    exit 1
fi

if [ ! -f "bin/$BINARY" ] ; then
    echo "[ERROR] Cannot find a ChampSim binary: bin/$BINARY"
    exit 1
fi

re='^[0-9]+$'
if ! [[ $N_WARM =~ $re ]] || [ -z $N_WARM ] ; then
    echo "[ERROR]: Number of warmup instructions is NOT a number" >&2;
    exit 1
fi

re='^[0-9]+$'
if ! [[ $N_SIM =~ $re ]] || [ -z $N_SIM ] ; then
    echo "[ERROR]: Number of simulation instructions is NOT a number" >&2;
    exit 1
fi

if ! [[ $ASID0 =~ $re ]] ; then
    echo "[ERROR]: ASID is NOT a number" 
    echo $ASID0 " " $ASID1 " " $ASID2 " " $ASID3 " "
    exit 1
fi


if [ ! -f "$TRACE_DIR/$TRACE0" ] ; then
    echo "[ERROR] Cannot find a trace0 file: $TRACE_DIR/$TRACE0"
    exit 1
fi



#if [ ! -f "$CONTEXT_SWITCH_DIR/$CONTEXT_SWITCH" ] ; then
#    echo "[ERROR} Cannot find context switch file: $CONTEXT_SWITCH_DIR/$CONTEXT_SWITCH"
#    exit 1
#fi

#echo "${CONTEXT_SWITCH:15:5}"
mkdir -p results_1core_${N_SIM}M
(./bin/${BINARY} -warmup_instructions ${N_WARM}000000 -simulation_instructions ${N_SIM}000000 ${OPTION} -traces ${TRACE_DIR}/${TRACE0} ${ASID0} ) &> results_1core_${N_SIM}M/mix${N_MIX}-${BINARY}.txt
