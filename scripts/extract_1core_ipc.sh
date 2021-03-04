configs=(noninclusive inclusive seclusive)
cpus=(2 4 8)
total_workload=46

rm -f 1core_results.txt

#echo -e 'Trace\tNInc_2\tInc_2\tSec_2\tNInc_4\tInc_4\tSec_4\tNInc_8\tInc_8\tSec_8' >> 1core_results.txt

for ((i=1;i<=${total_workload};i++));
do
	trace=`sed -n ''${i}'p' scripts/intensive_trace_list.txt | awk '{print $1}'`
	output=${trace::-17}
	binary=hashed_perceptron-no-no-no-no-no-no-lru-1core
	file_name=results_10M/${trace}-${binary}.txt
	ipc=`grep "CPU 0 cumulative IPC:" ${file_name}  | awk '{printf("%.6f", $5)}'`
	output=$output','$ipc
	
	binary=hashed_perceptron-final_ipcp-ipcp_practical-no-no-no-no-lru-1core
    file_name=results_10M/${trace}-${binary}.txt
    ipc=`grep "CPU 0 cumulative IPC:" ${file_name}  | awk '{printf("%.6f", $5)}'`
    output=$output','$ipc

	binary=hashed_perceptron-ipcp_crossPages-ipcp_practical-no-no-no-no-lru-1core
    file_name=results_10M/${trace}-${binary}.txt
    ipc=`grep "CPU 0 cumulative IPC:" ${file_name}  | awk '{printf("%.6f", $5)}'`
    output=$output','$ipc

	binary=hashed_perceptron-no-spp_tuned_aggr-no-no-no-no-lru-1core
    file_name=results_10M/${trace}-${binary}.txt
    ipc=`grep "CPU 0 cumulative IPC:" ${file_name}  | awk '{printf("%.6f", $5)}'`
    output=$output','$ipc


	echo $output
	#echo -e $output >> 1core_results.txt
done

