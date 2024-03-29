#Neelu: Adding the list of variables here so you'll only need to modify the following to change anything.

numwarmup=5
numsim=10

parallelismcount=5

echo "NOT BUILDING BINARY for INDEX 0 to all. REMEMBER TO DO IT NEXT TIME"


branchPredictor=hashed_perceptron

l1dpref=(ipcp_timely ipcp_timely2 ipcp_timely_v2)

l2cpref=(no no no) #ipcp_paper1)	

llcpref=(no no no)	

llcrepl=lru
numcore=1

tracedir=../../SPEC2017
traces=$(cat scripts/intensive_trace_list.txt)

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

#log2_region_size=15
temp_var=0

#while [ $log2_region_size -lt 21 ]
while [ $temp_var -lt 1 ]
do 
	#degree=3

	num_ips=2

	#while [ $degree -lt 9 ]
	while [ $num_ips -lt 11 ]
	do 

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
						./run_champsim.sh $branchPredictor-${l1dpref[i]}-$num_ips-${l2cpref[i]}-${llcpref[i]}-$llcrepl-1core $numwarmup $numsim $tracedir $trace 
					else
						./run_champsim.sh $branchPredictor-${l1dpref[i]}-$num_ips-${l2cpref[i]}-${llcpref[i]}-$llcrepl-1core $numwarmup $numsim $tracedir $trace &
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


					./run_champsim.sh $branchPredictor-${l1dpref[i]}-$num_ips-${l2cpref[i]}-${llcpref[i]}-$llcrepl-1core $numwarmup $numsim $tracedir $trace &
				done
			fi

		done

		#degree=`expr $degree + 1`
		num_ips=`expr $num_ips + 1`

	done
	#log2_region_size=`expr $log2_region_size + 1`
	temp_var=`expr $temp_var + 1`
done
echo "Done with count $count"
