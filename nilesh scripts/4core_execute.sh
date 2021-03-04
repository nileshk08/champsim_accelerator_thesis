BINARY=${1}
TRACE_LIST=${2}

n=10
cat $TRACE_LIST  | \

#echo "$TRACE_LIST "
#while read args; do
   # echo "$args "
#    echo "./run_2core.sh $BINARY 20 50 $n  $args -cloudsuite &"
 #   echo ""
#		n=$((n+1))
#done

while [ $n -lt 22 ]; do
	read args
	#echo "./run_1core.sh $BINARY 20 50 $n  $args 0  &"
	if [ $n -lt 31 ]
	then
		echo "./run_1core.sh $BINARY 20 50 $n  $args 0  &"
	else
		echo "./run_1core.sh $BINARY 20 50 $n  $args 0 -cloudsuite &"
	fi
	n=$((n+1))
done


