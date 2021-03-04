#!/bin/bash


#./run_5core.sh bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator 0 50 3 inception_500M.trace.gz 0 lenet-5_500M.trace.gz 1 resnet-50_500M.trace.gz 2 vgg-16_500M.trace.gz 3 vgg-s_500M.trace.gz 4 
#cp results_4core_50M/mix3-bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator.txt results_4core_50M/1cpu-4accelerator-4diff-01

#echo "1st completed"
#./build_champsim.sh bimodal no no no no no no lru 1 4

#./run_5core.sh bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator 20 50 3 inception_500M.trace.gz 0 alexnet_500M.trace.gz 1 cifar10_500M.trace.gz 2 inception_500M.trace.gz 3 lenet-5_500M.trace.gz 4 
./build_champsim.sh bimodal no no no no next_line no lru 1 4
./run_5core.sh bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator 20 50 3 inception_500M.trace.gz 0 alexnet_500M.trace.gz 1 cifar10_500M.trace.gz 2 inception_500M.trace.gz 3 lenet-5_500M.trace.gz 4 
cp results_4core_50M/mix3-bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator.txt results_4core_50M/1cpu-4accelerator-4diff-01_pref

echo "1st completed"


./run_5core.sh bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator 20 50 3 lenet-5_500M.trace.gz 0 lstm_500M.trace.gz 1 nin_500M.trace.gz 2 resnet-50_500M.trace.gz 3 svhn_500M.trace.gz 4 
cp results_4core_50M/mix3-bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator.txt results_4core_50M/1cpu-4accelerator-4diff-02_pref

echo "2 nd completed"

./run_5core.sh bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator 20 50 3 vgg-19_500M.trace.gz 0 squeezenet_500M.trace.gz 1 vgg-m_500M.trace.gz 2 vgg-s_500M.trace.gz 3 vgg-16_500M.trace.gz 4 
cp results_4core_50M/mix3-bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator.txt results_4core_50M/1cpu-4accelerator-4diff-03_pref

echo "3rd completed"

