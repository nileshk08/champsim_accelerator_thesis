

arr=(  "2" "4" "8" "16" "32" "64" "128" "256" "512" "1024")

for i in "${arr[@]}"
do
#	echo "./run_4core.sh new_pref_p"$i"_4acc 20 50 10 alexnet_500M.trace.gz 0 cifar10_500M.trace.gz 1 inception_500M.trace.gz 2 lenet-5_500M.trace.gz 3 &"
	echo "./run_1core.sh  new_pref_p"$i"_1acc 20 50 10  alexnet_500M.trace.gz 0 & "
	echo "./run_1core.sh  new_pref_p"$i"_1acc 20 50 11  cifar10_500M.trace.gz 0 &"
	echo "./run_1core.sh  new_pref_p"$i"_1acc 20 50 12  inception_500M.trace.gz 0 &"
	echo "./run_1core.sh  new_pref_p"$i"_1acc 20 50 13  lenet-5_500M.trace.gz 0 &"
	echo "./run_1core.sh  new_pref_p"$i"_1acc 20 50 14  lstm_500M.trace.gz 0 &"
	echo "./run_1core.sh  new_pref_p"$i"_1acc 20 50 15  nin_500M.trace.gz 0   &"
	echo "./run_1core.sh  new_pref_p"$i"_1acc 20 50 16  resnet-50_500M.trace.gz 0  &"
	echo "./run_1core.sh  new_pref_p"$i"_1acc 20 50 17  svhn_500M.trace.gz 0 &"
	echo "./run_1core.sh  new_pref_p"$i"_1acc 20 50 18  cc_sv.input-uk-2002.champsim.gz 0 &"
	echo "./run_1core.sh  new_pref_p"$i"_1acc 20 50 19  mcf_158B.trace.gz 0 "
	echo "./run_1core.sh  new_pref_p"$i"_1acc 20 50 20  GemsFDTD_716B.trace.gz 0 &"

	echo "./run_1core.sh  new_pref_m"$i"_1acc 20 50 10  alexnet_500M.trace.gz 0 & "
	echo "./run_1core.sh  new_pref_m"$i"_1acc 20 50 11  cifar10_500M.trace.gz 0 &"
	echo "./run_1core.sh  new_pref_m"$i"_1acc 20 50 12  inception_500M.trace.gz 0 &"
	echo "./run_1core.sh  new_pref_m"$i"_1acc 20 50 13  lenet-5_500M.trace.gz 0 &"
	echo "./run_1core.sh  new_pref_m"$i"_1acc 20 50 14  lstm_500M.trace.gz 0 &"
	echo "./run_1core.sh  new_pref_m"$i"_1acc 20 50 15  nin_500M.trace.gz 0   &"
	echo "./run_1core.sh  new_pref_m"$i"_1acc 20 50 16  resnet-50_500M.trace.gz 0  &"
	echo "./run_1core.sh  new_pref_m"$i"_1acc 20 50 17  svhn_500M.trace.gz 0 &"
	echo "./run_1core.sh  new_pref_m"$i"_1acc 20 50 18  cc_sv.input-uk-2002.champsim.gz 0 &"
	echo "./run_1core.sh  new_pref_m"$i"_1acc 20 50 19  mcf_158B.trace.gz 0 "
	echo "./run_1core.sh  new_pref_m"$i"_1acc 20 50 20  GemsFDTD_716B.trace.gz 0 &"
done
