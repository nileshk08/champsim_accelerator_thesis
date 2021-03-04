#!/bin/bash



./build_champsim.sh bimodal no no no no no no lru 1 1

./run_2core.sh bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator 20 50 1 alexnet_500M.trace.gz 0 alexnet_500M.trace.gz 1 
cp results_2core_50M/mix1-bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator.txt results_2core_50M/1cpu-1accelerator-32_dtlb-01_pref

echo "1st completed"
./run_2core.sh bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator 20 50 1 cifar10_500M.trace.gz 0 cifar10_500M.trace.gz 1 
cp results_2core_50M/mix1-bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator.txt results_2core_50M/1cpu-1accelerator-32_dtlb-02_pref

echo "2st completed"
./run_2core.sh bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator 20 50 1 inception_500M.trace.gz 0  inception_500M.trace.gz 1 
cp results_2core_50M/mix1-bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator.txt results_2core_50M/1cpu-1accelerator-32_dtlb-03_pref

echo "3st completed"
./run_2core.sh bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator 20 50 1 lenet-5_500M.trace.gz 0  lenet-5_500M.trace.gz 4 
cp results_2core_50M/mix1-bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator.txt results_2core_50M/1cpu-1accelerator-32_dtlb-04_pref

echo "4st completed"

./run_2core.sh bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator 20 50 1 lstm_500M.trace.gz 0 lstm_500M.trace.gz 1 
cp results_2core_50M/mix1-bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator.txt results_2core_50M/1cpu-1accelerator-32_dtlb-05_pref

echo "5 nd completed"
./run_2core.sh bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator 20 50 1 nin_500M.trace.gz 0  nin_500M.trace.gz 2 
cp results_2core_50M/mix1-bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator.txt results_2core_50M/1cpu-1accelerator-32_dtlb-06_pref

echo "6 nd completed"
./run_2core.sh bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator 20 50 1 resnet-50_500M.trace.gz 0  resnet-50_500M.trace.gz 3 
cp results_2core_50M/mix1-bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator.txt results_2core_50M/1cpu-1accelerator-32_dtlb-07_pref

echo "7 nd completed"

./run_2core.sh bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator 20 50 1 svhn_500M.trace.gz 0   svhn_500M.trace.gz 4 
cp results_2core_50M/mix1-bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator.txt results_2core_50M/1cpu-1accelerator-32_dtlb-08_pref

echo "8 nd completed"

./run_2core.sh bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator 20 50 1 squeezenet_500M.trace.gz 0 squeezenet_500M.trace.gz 1 
cp results_2core_50M/mix1-bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator.txt results_2core_50M/1cpu-1accelerator-32_dtlb-09_pref

echo "9rd completed"
./run_2core.sh bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator 20 50 1 vgg-m_500M.trace.gz 0  vgg-m_500M.trace.gz 2 
cp results_2core_50M/mix1-bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator.txt results_2core_50M/1cpu-1accelerator-32_dtlb-10_pref

echo "10d completed"

./run_2core.sh bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator 20 50 1 vgg-s_500M.trace.gz 0  vgg-s_500M.trace.gz 3 
cp results_2core_50M/mix1-bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator.txt results_2core_50M/1cpu-1accelerator-32_dtlb-11_pref

echo "11d completed"
./run_2core.sh bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator 20 50 1 vgg-16_500M.trace.gz 0   vgg-16_500M.trace.gz 4 
cp results_2core_50M/mix1-bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator.txt results_2core_50M/1cpu-1accelerator-32_dtlb-12_pref

echo "12d completed"
./run_2core.sh bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator 20 50 1 vgg-19_500M.trace.gz 0   vgg-19_500M.trace.gz 1 
cp results_2core_50M/mix1-bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator.txt results_2core_50M/1cpu-1accelerator-32_dtlb-13_pref

echo "13d completed"
