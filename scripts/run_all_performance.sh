total_workload=14
warm=50
sim=50
policy=lru

for ((i=1;i<=${total_workload};i++));
do
	let a=$i%2
	if [ 0 -eq $a ]
	then
			./run_2core_perf.sh hashed_perceptron-ipcp_crossPages-ipcp_practical-no-no-no-no-lru-2core ${warm} ${sim} ${i} &
			./run_4core_perf.sh hashed_perceptron-ipcp_crossPages-ipcp_practical-no-no-no-no-lru-4core ${warm} ${sim} ${i} & 
			./run_8core_perf.sh hashed_perceptron-ipcp_crossPages-ipcp_practical-no-no-no-no-lru-8core ${warm} ${sim} ${i} 
	else
			./run_2core_perf.sh hashed_perceptron-ipcp_crossPages-ipcp_practical-no-no-no-no-lru-2core ${warm} ${sim} ${i} &
            ./run_4core_perf.sh hashed_perceptron-ipcp_crossPages-ipcp_practical-no-no-no-no-lru-4core ${warm} ${sim} ${i} &
            ./run_8core_perf.sh hashed_perceptron-ipcp_crossPages-ipcp_practical-no-no-no-no-lru-8core ${warm} ${sim} ${i} &
	fi
done


