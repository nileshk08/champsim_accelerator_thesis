TRACE_LIST=${1}

n=10
cat $TRACE_LIST | 

#echo "$TRACE_LIST "
while read args; do
	./run_1core.sh   $args 20 50 10  alexnet_500M.trace.gz 0  &
	./run_1core.sh   $args 20 50 11  cifar10_500M.trace.gz 0 &
	./run_1core.sh   $args 20 50 12  inception_500M.trace.gz 0 &
	./run_1core.sh   $args 20 50 13  lenet-5_500M.trace.gz 0 &
	./run_1core.sh   $args 20 50 14  lstm_500M.trace.gz 0  &
	./run_1core.sh   $args 20 50 15  nin_500M.trace.gz 0 &
	./run_1core.sh   $args 20 50 16  resnet-50_500M.trace.gz 0 &
	./run_1core.sh   $args 20 50 17  svhn_500M.trace.gz 0 &
	./run_1core.sh   $args 20 50 18  cc_sv.input-uk-2002.champsim.gz 0 &
	./run_1core.sh   $args 20 50 19  mcf_158B.trace.gz 0 &
	./run_1core.sh   $args 20 50 20  GemsFDTD_716B.trace.gz 0 
	echo $args
done
