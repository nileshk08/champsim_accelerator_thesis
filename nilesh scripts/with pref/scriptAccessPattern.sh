#!/bin/bash


./build_champsim.sh bimodal no no no no no no lru 1 1

./run_2core.sh bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator 20 5 0 inception_500M.trace.gz 0 alexnet_500M.trace.gz 1 
cp results_2core_5M/mix0-bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator.txt results_2core_5M/1.txt

echo "1st completed"
./run_2core.sh bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator 20 5 0 inception_500M.trace.gz 0 cifar10_500M.trace.gz 2 
cp results_2core_5M/mix0-bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator.txt results_2core_5M/2.txt

echo "2st completed"
./run_2core.sh bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator 20 5 0 inception_500M.trace.gz 0  inception_500M.trace.gz 3 
cp results_2core_5M/mix0-bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator.txt results_2core_5M/3.txt

echo "3st completed"
./run_2core.sh bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator 20 5 0 inception_500M.trace.gz 0  lenet-5_500M.trace.gz 4 
cp results_2core_5M/mix0-bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator.txt results_2core_5M/4.txt

echo "4st completed"

./run_2core.sh bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator 20 5 0 lenet-5_500M.trace.gz 0 lstm_500M.trace.gz 1 
cp results_2core_5M/mix0-bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator.txt results_2core_5M/5.txt

echo "5 nd completed"
./run_2core.sh bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator 20 5 0 lenet-5_500M.trace.gz 0  nin_500M.trace.gz 2 
cp results_2core_5M/mix0-bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator.txt results_2core_5M/6.txt

echo "6 nd completed"
./run_2core.sh bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator 20 5 0 lenet-5_500M.trace.gz 0  resnet-50_500M.trace.gz 3 
cp results_2core_5M/mix0-bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator.txt results_2core_5M/7.txt

echo "7 nd completed"

./run_2core.sh bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator 20 5 0 lenet-5_500M.trace.gz 0   svhn_500M.trace.gz 4 
cp results_2core_5M/mix0-bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator.txt results_2core_5M/8.txt

echo "8 nd completed"

./run_2core.sh bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator 20 5 0 vgg-19_500M.trace.gz 0 squeezenet_500M.trace.gz 1 
cp results_2core_5M/mix0-bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator.txt results_2core_5M/9.txt

echo "9rd completed"
./run_2core.sh bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator 20 5 0 vgg-19_500M.trace.gz 0  vgg-m_500M.trace.gz 2 
cp results_2core_5M/mix0-bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator.txt results_2core_5M/10.txt

echo "10d completed"

./run_2core.sh bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator 20 5 0 vgg-19_500M.trace.gz 0  vgg-s_500M.trace.gz 3 
cp results_2core_5M/mix0-bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator.txt results_2core_5M/11.txt

echo "11d completed"
./run_2core.sh bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator 20 5 0 vgg-19_500M.trace.gz 0   vgg-16_500M.trace.gz 4 
cp results_2core_5M/mix0-bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator.txt results_2core_5M/12.txt

echo "12d completed"
./run_2core.sh bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator 20 5 0 vgg-19_500M.trace.gz 0   vgg-19_500M.trace.gz 1 
cp results_2core_5M/mix0-bimodal-no-no-no-no-no-no-lru-1cpu-1accelerator.txt results_2core_5M/13.txt


echo "13d completed"
