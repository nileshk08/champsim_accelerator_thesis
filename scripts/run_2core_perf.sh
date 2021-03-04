TRACE_DIR=~/car3s/spec2017_traces/
binary=${1}
n_warm=${2}
n_sim=${3}
num=${4}

trace1=`sed -n ''$num'p' 2core_workloads.txt | awk '{print $1}'`
trace2=`sed -n ''$num'p' 2core_workloads.txt | awk '{print $2}'`

mkdir -p results_2core
(./bin/${binary} -warmup_instructions ${n_warm}000000 -simulation_instructions ${n_sim}000000 ${option} -traces ${TRACE_DIR}/${trace1}.champsimtrace.xz ${TRACE_DIR}/${trace2}.champsimtrace.xz) &> results_2core/mix${num}-${binary}.txt
