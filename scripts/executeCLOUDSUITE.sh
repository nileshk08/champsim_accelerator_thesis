#Neelu: Adding the list of variables here so you'll only need to modify the following to change anything.

parallelismcount=16

branchPredictor=hashed_perceptron
l1dpref=(no) # final_ipcp final_ipcp no no bingo_dpc3 final_ipcp no)       
l2cpref=(no) # no ipcp_practical spp ppf no spp spp_tuned_aggr)    
llcpref=(no) # no no no no no no no) 

llcrepl=lru
numcore=1
numwarmup=50
numsim=100
tracedir=../../cloudsuite_traces
traces=$(cat scripts/cloudsuite_trace_list.txt)

numofpref=${#l1dpref[@]}


#Neelu: Variables ending now. Kindly add new variables above this line.

#NOT BUILDING HERE AS BUILT IN executeSPEC.sh


count=0
iterator=0

#parallelismcount=16

for trace in $traces;
do 
	count=`expr $count + 1`
	
	if [ $count -eq $parallelismcount ]
	then
		iterator=`expr $iterator + 1`
		for((i=0; i<$numofpref; i++))
		do
			if [ $i -eq `expr $numofpref - 1` ]
                        then
				./run_champsim.sh $branchPredictor-${l1dpref[i]}-${l2cpref[i]}-${llcpref[i]}-$llcrepl-1core $numwarmup $numsim $tracedir $trace -cloudsuite
			else
				./run_champsim.sh $branchPredictor-${l1dpref[i]}-${l2cpref[i]}-${llcpref[i]}-$llcrepl-1core $numwarmup $numsim $tracedir $trace -cloudsuite &
			fi
		done
		now=$(date)
		echo "Number of CLOUDSUITE traces simulated - $count in iteration $iterator at time: $now" 
		#>> results_saved/execution_log.txt
		count=0
	else
		for((i=0; i<$numofpref; i++))
		do
	        	./run_champsim.sh $branchPredictor-${l1dpref[i]}-${l2cpref[i]}-${llcpref[i]}-$llcrepl-1core $numwarmup $numsim $tracedir $trace -cloudsuite &
		done
	fi

done

echo "Done with count $count"
