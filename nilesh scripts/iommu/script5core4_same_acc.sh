#!/bin/bash



#./build_champsim.sh bimodal no no no no no no lru 1 4

./run_5core.sh bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator 20 50 4 inception_500M.trace.gz 0 alexnet_500M.trace.gz 1 alexnet_500M.trace.gz 1  alexnet_500M.trace.gz 1 alexnet_500M.trace.gz 1
cp results_4core_50M/mix4-bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator.txt results_4core_50M/1cpu-4accelerator-all_same-01_pref

echo "1st completed"
./run_5core.sh bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator 20 50 4 inception_500M.trace.gz 0 cifar10_500M.trace.gz 2 cifar10_500M.trace.gz 2    cifar10_500M.trace.gz 2 cifar10_500M.trace.gz 2   
cp results_4core_50M/mix4-bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator.txt results_4core_50M/1cpu-4accelerator-all_same-02_pref

echo "2st completed"
./run_5core.sh bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator 20 50 4 inception_500M.trace.gz 0  inception_500M.trace.gz 3 inception_500M.trace.gz 3 inception_500M.trace.gz 3 inception_500M.trace.gz 3
cp results_4core_50M/mix4-bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator.txt results_4core_50M/1cpu-4accelerator-all_same-03_pref

echo "3st completed"
./run_5core.sh bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator 20 50 4 inception_500M.trace.gz 0  lenet-5_500M.trace.gz 4 lenet-5_500M.trace.gz 4 lenet-5_500M.trace.gz 4 lenet-5_500M.trace.gz 4 
cp results_4core_50M/mix4-bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator.txt results_4core_50M/1cpu-4accelerator-all_same-04_pref

echo "4st completed"

./run_5core.sh bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator 20 50 4 lenet-5_500M.trace.gz 0 lstm_500M.trace.gz 1 lstm_500M.trace.gz 1 lstm_500M.trace.gz 1 lstm_500M.trace.gz 1
cp results_4core_50M/mix4-bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator.txt results_4core_50M/1cpu-4accelerator-all_same-05_pref

echo "5 nd completed"
./run_5core.sh bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator 20 50 4 lenet-5_500M.trace.gz 0  nin_500M.trace.gz 2 nin_500M.trace.gz 2 nin_500M.trace.gz 2 nin_500M.trace.gz 2 
cp results_4core_50M/mix4-bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator.txt results_4core_50M/1cpu-4accelerator-all_same-06_pref

echo "6 nd completed"
./run_5core.sh bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator 20 50 4 lenet-5_500M.trace.gz 0  resnet-50_500M.trace.gz 3 resnet-50_500M.trace.gz 3 resnet-50_500M.trace.gz 3 resnet-50_500M.trace.gz 3
cp results_4core_50M/mix4-bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator.txt results_4core_50M/1cpu-4accelerator-all_same-07_pref

echo "7 nd completed"

./run_5core.sh bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator 20 50 4 lenet-5_500M.trace.gz 0   svhn_500M.trace.gz 4 svhn_500M.trace.gz 4 svhn_500M.trace.gz 4 svhn_500M.trace.gz 4 
cp results_4core_50M/mix4-bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator.txt results_4core_50M/1cpu-4accelerator-all_same-08_pref

echo "8 nd completed"

./run_5core.sh bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator 20 50 4 vgg-19_500M.trace.gz 0 squeezenet_500M.trace.gz 1 squeezenet_500M.trace.gz 1 squeezenet_500M.trace.gz 1 squeezenet_500M.trace.gz 1
cp results_4core_50M/mix4-bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator.txt results_4core_50M/1cpu-4accelerator-all_same-09_pref

echo "9rd completed"
./run_5core.sh bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator 20 50 4 vgg-19_500M.trace.gz 0  vgg-m_500M.trace.gz 2 vgg-m_500M.trace.gz 2 vgg-m_500M.trace.gz 2 vgg-m_500M.trace.gz 2 
cp results_4core_50M/mix4-bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator.txt results_4core_50M/1cpu-4accelerator-all_same-10_pref

echo "10d completed"

./run_5core.sh bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator 20 50 4 vgg-19_500M.trace.gz 0  vgg-s_500M.trace.gz 3 vgg-s_500M.trace.gz 3 vgg-s_500M.trace.gz 3 vgg-s_500M.trace.gz 3
cp results_4core_50M/mix4-bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator.txt results_4core_50M/1cpu-4accelerator-all_same-11_pref

echo "11d completed"
./run_5core.sh bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator 20 50 4 vgg-19_500M.trace.gz 0   vgg-16_500M.trace.gz 4 vgg-16_500M.trace.gz 4 vgg-16_500M.trace.gz 4 vgg-16_500M.trace.gz 4 
cp results_4core_50M/mix4-bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator.txt results_4core_50M/1cpu-4accelerator-all_same-12_pref

echo "12d completed"
./run_5core.sh bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator 20 50 4 vgg-19_500M.trace.gz 0   vgg-19_500M.trace.gz 1 vgg-19_500M.trace.gz 1 vgg-19_500M.trace.gz 1 vgg-19_500M.trace.gz 1

cp results_4core_50M/mix4-bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator.txt results_4core_50M/1cpu-4accelerator-all_same-13_pref

echo "13d completed"