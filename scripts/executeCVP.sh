#Neelu: Adding the list of variables here so you'll only need to modify the following to change anything.

numwarmup=50
numsim=100

parallelismcount=16

echo "NOT BUILDING BINARY for INDEX 0 to all. REMEMBER TO DO IT NEXT TIME"


branchPredictor=hashed_perceptron

l1dpref=(no)  #(ipcp_16KBreg1 ipcp_16KBreg2 ipcp_512KBreg1 ipcp_512KBreg2 ipcp_1MBreg1 ipcp_1MBreg2) # final_ipcp)	#(ipcp_256KBreg1 ipcp_256KBreg4 ipcp_64KBreg1 ipcp_64KBreg2 ipcp_32KBreg1 ipcp_32KBreg2)

l2cpref=(no) # no no no no) # isb_ideal)	# no no no no) #ipcp_paper1)	

llcpref=(no) # no no no no)	

llcrepl=lru
numcore=1

tracedir=../../Converted_CVP_Traces
traces=$(cat scripts/cvp_trace_list.txt)

numofpref=${#l1dpref[@]}

#paralleslismcount=`expr 16 / $numofpref`
#echo "PCount: $paralleslismcount"
#Neelu: Variables ending now. Kindly add new variables above this line.


#for((i=0; i<$numofpref; i++))
#do 
#	./build_champsim.sh $branchPredictor ${l1dpref[i]} ${l2cpref[i]} ${llcpref[i]} $llcrepl $numcore || exit
#done

count=0
iterator=0
totalcount=0


for trace in $traces;
do 
	count=`expr $count + 1`
	#totalcount=`expr $totalcount + 1`

	#if [ $totalcount -lt 5 ]
	#	continue
	#temp_parallelismcount=$parallelismcount
	if [ $count -eq $parallelismcount ]
	then
		iterator=`expr $iterator + 1`
		for((i=0; i<$numofpref; i++))
		do
			#GREP OR NOT START
#			if [ $i -eq `expr $numofpref - 1` ]
#			then
#				if grep -q -r "CPU 0 cumulative IPC" results_100M/$trace-$branchPredictor-${l1dpref[i]}-${l2cpref[i]}-${llcpref[i]}-$llcrepl-1core.txt
#				then
 #       				echo "NOT EMPTY"
#				else
 #       				echo "EMPTY"
#					./run_champsim.sh $branchPredictor-${l1dpref[i]}-${l2cpref[i]}-${llcpref[i]}-$llcrepl-1core $numwarmup $numsim $tracedir $trace
#				fi

#			else
#				if grep -q -r "CPU 0 cumulative IPC" results_100M/$trace-$branchPredictor-${l1dpref[i]}-${l2cpref[i]}-${llcpref[i]}-$llcrepl-1core.txt
#				then
 #       				echo "NOT EMPTY"
#				else
 #       				echo "EMPTY"
#					./run_champsim.sh $branchPredictor-${l1dpref[i]}-${l2cpref[i]}-${llcpref[i]}-$llcrepl-1core $numwarmup $numsim $tracedir $trace &
#				fi
#			fi
			#GREP OR NOT END

			if [ $i -eq `expr $numofpref - 1` ]
			then
				./run_champsim.sh $branchPredictor-${l1dpref[i]}-${l2cpref[i]}-${llcpref[i]}-$llcrepl-1core $numwarmup $numsim $tracedir $trace 
			else
				./run_champsim.sh $branchPredictor-${l1dpref[i]}-${l2cpref[i]}-${llcpref[i]}-$llcrepl-1core $numwarmup $numsim $tracedir $trace &
			fi
		done
		now=$(date)
		echo "Number of SPEC traces simulated - $count in iteration $iterator at time: $now" 
		#>> results_saved/execution_log.txt
		count=0
	else
		for((i=0; i<$numofpref; i++))
                do
			#GREP OR NOT START
        #                if grep -q -r "CPU 0 cumulative IPC" results_100M/$trace-$branchPredictor-${l1dpref[i]}-${l2cpref[i]}-${llcpref[i]}-$llcrepl-1core.txt
	#		then
         #                       echo "NOT EMPTY"
          #              else    
           #                     echo "EMPTY"
            #                    ./run_champsim.sh $branchPredictor-${l1dpref[i]}-${l2cpref[i]}-${llcpref[i]}-$llcrepl-1core $numwarmup $numsim $tracedir $trace &
             #           fi

			#GREP OR NOT END


                        ./run_champsim.sh $branchPredictor-${l1dpref[i]}-${l2cpref[i]}-${llcpref[i]}-$llcrepl-1core $numwarmup $numsim $tracedir $trace &
                done
	fi


done

echo "Done with count $count"
