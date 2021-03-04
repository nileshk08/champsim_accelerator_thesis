./run_1core.sh print_pattern 30 15 10  alexnet_500M.trace.gz 0  &
./run_1core.sh print_pattern 30 15 11  lenet-5_500M.trace.gz 0  &
./run_1core.sh print_pattern 30 15 12  lstm_500M.trace.gz 0  &
./run_1core.sh print_pattern 30 15 13  svhn_500M.trace.gz 0  &
./run_1core.sh print_pattern 30 15 14  cc_sv.input-hugebubbles-00020.champsim.gz 0  &
./run_1core.sh print_pattern 30 15 15  cc_sv.input-uk-2002.champsim.gz 0  &
./run_1core.sh print_pattern 30 15 16  randomizer.input-hugebubbles-00020.champsim.gz 0  &
./run_1core.sh print_pattern 30 15 17  randomizer.input-uk-2002.champsim.gz 0  &
./run_1core.sh print_pattern 30 15 18  mcf_158B.trace.gz 0  &
./run_1core.sh print_pattern 30 15 19  mcf_250B.trace.gz 0  &
./run_1core.sh print_pattern 30 15 20  mcf_46B.trace.gz 0  &
./run_1core.sh print_pattern 30 15 21  GemsFDTD_716B.trace.gz 0  &


./run_1core.sh reuse_distance_PTL2 20 50 10  alexnet_500M.trace.gz 0  &
./run_1core.sh reuse_distance_PTL2 20 50 11  lenet-5_500M.trace.gz 0  &
./run_1core.sh reuse_distance_PTL2 20 50 12  lstm_500M.trace.gz 0  &
./run_1core.sh reuse_distance_PTL2 20 50 13  svhn_500M.trace.gz 0  &
./run_1core.sh reuse_distance_PTL2 20 50 14  cc_sv.input-hugebubbles-00020.champsim.gz 0  &
./run_1core.sh reuse_distance_PTL2 20 50 15  cc_sv.input-uk-2002.champsim.gz 0  &
./run_1core.sh reuse_distance_PTL2 20 50 16  randomizer.input-hugebubbles-00020.champsim.gz 0  &
./run_1core.sh reuse_distance_PTL2 20 50 17  randomizer.input-uk-2002.champsim.gz 0  &
./run_1core.sh reuse_distance_PTL2 20 50 18  mcf_158B.trace.gz 0  &
./run_1core.sh reuse_distance_PTL2 20 50 19  mcf_250B.trace.gz 0  &
./run_1core.sh reuse_distance_PTL2 20 50 20  mcf_46B.trace.gz 0  &
./run_1core.sh reuse_distance_PTL2 20 50 21  GemsFDTD_716B.trace.gz 0  &
