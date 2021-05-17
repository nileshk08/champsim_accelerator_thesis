#!/bin/bash

if [ "$#" -lt 20 ] || [ "$#" -gt 20 ]; then
    echo "Illegal number of parameters"
    echo "Usage: ./run_8core.sh [BINARY] [N_WARM] [N_SIM] [N_MIX] [TRACE0] [ASID0] [TRACE1] [ASID1] [TRACE2] [ASID2] [TRACE3] [ASID3] [TRACE4] [ASID4] [TRACE5] [ASID5] [TRACE6] [ASID6] [TRACE7] [ASID7]"
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
TRACE1=${7}
ASID1=${8}
TRACE2=${9}
ASID2=${10}
TRACE3=${11}
ASID3=${12}
TRACE4=${13}
ASID4=${14}
TRACE5=${15}
ASID5=${16}
TRACE6=${17}
ASID6=${18}
TRACE7=${19}
ASID7=${20}
OPTION=0
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

if ! [[ $ASID0 =~ $re ]] || ! [[ $ASID1 =~ $re ]] || ! [[ $ASID2 =~ $re ]] || ! [[ $ASID3 =~ $re ]] || ! [[ $ASID4 =~ $re ]]  || ! [[ $ASID5 =~ $re ]] || ! [[ $ASID6 =~ $re ]] || ! [[ $ASID7 =~ $re ]]  ; then
    echo "[ERROR]: ASID is NOT a number" 
    echo $ASID0 " " $ASID1 " " $ASID2 " " $ASID3 " "
    exit 1
fi


if [ ! -f "$TRACE_DIR/$TRACE0" ] ; then
    echo "[ERROR] Cannot find a trace0 file: $TRACE_DIR/$TRACE0"
    exit 1
fi

if [ ! -f "$TRACE_DIR/$TRACE1" ] ; then
    echo "[ERROR] Cannot find a trace1 file: $TRACE_DIR/$TRACE1"
    exit 1
fi

if [ ! -f "$TRACE_DIR/$TRACE2" ] ; then
    echo "[ERROR] Cannot find a trace2 file: $TRACE_DIR/$TRACE2"
    exit 1
fi

if [ ! -f "$TRACE_DIR/$TRACE3" ] ; then
    echo "[ERROR] Cannot find a trace3 file: $TRACE_DIR/$TRACE3"
    exit 1
fi

if [ ! -f "$TRACE_DIR/$TRACE4" ] ; then
    echo "[ERROR] Cannot find a trace4 file: $TRACE_DIR/$TRACE4"
    exit 1
fi

if [ ! -f "$TRACE_DIR/$TRACE5" ] ; then
    echo "[ERROR] Cannot find a trace1 file: $TRACE_DIR/$TRACE5"
    exit 1
fi

if [ ! -f "$TRACE_DIR/$TRACE6" ] ; then
    echo "[ERROR] Cannot find a trace2 file: $TRACE_DIR/$TRACE6"
    exit 1
fi

if [ ! -f "$TRACE_DIR/$TRACE7" ] ; then
    echo "[ERROR] Cannot find a trace3 file: $TRACE_DIR/$TRACE7"
    exit 1
fi

#if [ ! -f "$CONTEXT_SWITCH_DIR/$CONTEXT_SWITCH" ] ; then
#    echo "[ERROR} Cannot find context switch file: $CONTEXT_SWITCH_DIR/$CONTEXT_SWITCH"
#    exit 1
#fi

#echo "${CONTEXT_SWITCH:15:5}"
mkdir -p results_8core_${N_SIM}M
#echo "(./bin/${BINARY} -warmup_instructions ${N_WARM}000000 -simulation_instructions ${N_SIM}000000 ${OPTION} -traces ${TRACE_DIR}/${TRACE0} ${ASID0} ${TRACE_DIR}/${TRACE1} ${ASID1} ${TRACE_DIR}/${TRACE2} ${ASID2} ${TRACE_DIR}/${TRACE3} ${ASID3} ${TRACE_DIR}/${TRACE4} ${ASID4}${TRACE_DIR}/${TRACE5} ${ASID5} ${TRACE_DIR}/${TRACE6} ${ASID6} ${TRACE_DIR}/${TRACE7} ${ASID7}) &> results_8core_${N_SIM}M/mix${N_MIX}-${BINARY}.txt"

(./bin/${BINARY} -warmup_instructions ${N_WARM}000000 -simulation_instructions ${N_SIM}000000 ${OPTION} -traces ${TRACE_DIR}/${TRACE0} ${ASID0} ${TRACE_DIR}/${TRACE1} ${ASID1} ${TRACE_DIR}/${TRACE2} ${ASID2} ${TRACE_DIR}/${TRACE3} ${ASID3} ${TRACE_DIR}/${TRACE4} ${ASID4} ${TRACE_DIR}/${TRACE5} ${ASID5} ${TRACE_DIR}/${TRACE6} ${ASID6} ${TRACE_DIR}/${TRACE7} ${ASID7}) &> results_8core_${N_SIM}M/mix${N_MIX}-${BINARY}.txt
