#!/bin/bash



#./build_champsim.sh bimodal no no no no no no lru 1 4

./run_5core.sh bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator 20 50 2 inception_500M.trace.gz 0 alexnet_500M.trace.gz 1 alexnet_500M.trace.gz 1  cifar10_500M.trace.gz 2 cifar10_500M.trace.gz 2   
cp results_4core_50M/mix2-bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator.txt results_4core_50M/1cpu-4accelerator-2diff-01_pref

echo "1st completed"
./run_5core.sh bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator 20 50 2 inception_500M.trace.gz 0  inception_500M.trace.gz 3 inception_500M.trace.gz 3 lenet-5_500M.trace.gz 4 lenet-5_500M.trace.gz 4 
cp results_4core_50M/mix2-bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator.txt results_4core_50M/1cpu-4accelerator-2diff-02_pref

echo "2st completed"

./run_5core.sh bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator 20 50 2 lenet-5_500M.trace.gz 0 lstm_500M.trace.gz 1 lstm_500M.trace.gz 1 nin_500M.trace.gz 2 nin_500M.trace.gz 2 
cp results_4core_50M/mix2-bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator.txt results_4core_50M/1cpu-4accelerator-2diff-03_pref

echo "3 nd completed"

./run_5core.sh bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator 20 50 2 lenet-5_500M.trace.gz 0  resnet-50_500M.trace.gz 3 resnet-50_500M.trace.gz 3 svhn_500M.trace.gz 4 svhn_500M.trace.gz 4 
cp results_4core_50M/mix2-bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator.txt results_4core_50M/1cpu-4accelerator-2diff-04_pref

echo "4 nd completed"

./run_5core.sh bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator 20 50 2 vgg-19_500M.trace.gz 0 squeezenet_500M.trace.gz 1 squeezenet_500M.trace.gz 1 vgg-m_500M.trace.gz 2 vgg-m_500M.trace.gz 2 
cp results_4core_50M/mix2-bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator.txt results_4core_50M/1cpu-4accelerator-2diff-05_pref

echo "5rd completed"

./run_5core.sh bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator 20 50 2 vgg-19_500M.trace.gz 0  vgg-s_500M.trace.gz 3 vgg-s_500M.trace.gz 3 vgg-16_500M.trace.gz 4 vgg-16_500M.trace.gz 4 
cp results_4core_50M/mix2-bimodal-no-no-no-no-next_line-no-lru-1cpu-4accelerator.txt results_4core_50M/1cpu-4accelerator-2diff-06_pref

echo "6rd completed"
