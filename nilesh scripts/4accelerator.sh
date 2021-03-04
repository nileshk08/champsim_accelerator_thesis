#oIFS="$IFS" # Save the old input field separator
#IFS=$'\n'   # Set the IFS to a newline
#trace_array=($(<goodtracelist)) # Splitting on newlines, assign the entire file to an array
#size=${#trace_array[@]}
#echo "$size"
#echo "${some_array[0]}" # Get the third element of the array
#echo "${Unix_Array[0]} ${Unix_Array[1]}"
##echo "${#some_array[@]}" # Get the length of the array
#


	BINARY=${1}
	TRACE_LIST=${2}

n=10
cat $TRACE_LIST  | \

echo "$TRACE_LIST "
while read args; do
  # echo "$args "
    echo "./run_2core.sh $BINARY 20 50 $n  $args  &"
#   echo ""
		n=$((n+1))
done

#while [ $n -lt 59 ]; do
#	read args
#	#echo "./run_1core.sh $BINARY 20 50 $n  $args 0  &"
#	if [ $n -lt 31 ]
#	then
#		echo "./run_1core.sh $BINARY 20 50 $n  $args 0  &"
#	else
#		echo "./run_1core.sh $BINARY 20 50 $n  $args 0 -cloudsuite &"
#	fi
#	n=$((n+1))
#done


