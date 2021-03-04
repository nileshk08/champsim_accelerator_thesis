#Neelu: Adding the list of variables here so you'll only need to modify the following to change anything.


parallelismcount=5
echo "Building binaries in executeSERVER, remove if executingSPEC too"
branchPredictor=hashed_perceptron
l1dpref=(final_ipcp final_ipcp no bingo_dpc3 final_ipcp)	
l2cpref=(no ipcp_practical ppf no spp)	
llcpref=(no no no no no)		
llcrepl=lru
numcore=1
numwarmup=50
numsim=100
tracedir=../../IPC-traces
traces=$(cat scripts/server_trace_list.txt)

numofpref=${#l1dpref[@]}


#Neelu: Variables ending now. Kindly add new variables above this line.


#NOT BUILDING HERE AS ALREAD BUILT IN executeSPEC.sh
for((i=0; i<$numofpref; i++))
do
        ./build_champsim.sh $branchPredictor ${l1dpref[i]} ${l2cpref[i]} ${llcpref[i]} $llcrepl $numcore
done

tracenum=0
count=0
iterator=0

#parallelismcount=16

for trace in $traces;
do 
	count=`expr $count + 1`
	tracenum=`expr $tracenum + 1`

	#if [ $tracenum -lt 36 ] && [ $tracenum -gt 2 ]
	#then
	#	continue
	#fi

	if [ $count -eq $parallelismcount ]
	then
		iterator=`expr $iterator + 1`
		for((i=0; i<$numofpref; i++))
		do
			if [ $i -eq `expr $numofpref - 1` ]
                        then
				./run_champsim.sh $branchPredictor-${l1dpref[i]}-${l2cpref[i]}-${llcpref[i]}-$llcrepl-1core $numwarmup $numsim $tracedir $trace 
			else
				./run_champsim.sh $branchPredictor-${l1dpref[i]}-${l2cpref[i]}-${llcpref[i]}-$llcrepl-1core $numwarmup $numsim $tracedir $trace &
			fi
		done
		now=$(date)
		echo "Number of SERVER traces simulated - $count in iteration $iterator at time: $now" 
		#>> results_saved/execution_log.txt
		count=0
	else
		for((i=0; i<$numofpref; i++))
		do
        		./run_champsim.sh $branchPredictor-${l1dpref[i]}-${l2cpref[i]}-${llcpref[i]}-$llcrepl-1core $numwarmup $numsim $tracedir $trace &
		done
	fi


done

echo "Done with count $count"
