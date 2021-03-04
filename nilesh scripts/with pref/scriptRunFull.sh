#!/bin/bash

./build_champsim.sh bimodal no no no no next_line no lru 5 0

./run_5core.sh bimodal-no-no-no-no-next_line-no-lru-5cpu-0accelerator 20 50 5 alexnet_500M.trace.gz 0 lenet-5_500M.trace.gz 1 resnet-50_500M.trace.gz 2 squeezenet_500M.trace.gz 3 vgg-19_500M.trace.gz 4

cp results_4core_50M/mix5-bimodal-no-no-no-no-next_line-no-lru-5cpu-0accelerator.txt results_4core_50M/full_tace_with_cpu01_pref
echo "1st completed"

./run_5core.sh bimodal-no-no-no-no-next_line-no-lru-5cpu-0accelerator 20 50 5 cifar10_500M.trace.gz 0 lstm_500M.trace.gz 1 svhn_500M.trace.gz 2 vgg-m_500M.trace.gz 3 inception_500M.trace.gz 4
cp results_4core_50M/mix5-bimodal-no-no-no-no-next_line-no-lru-5cpu-0accelerator.txt results_4core_50M/full_tace_with_cpu02_pref
echo "2st completed"



./run_5core.sh bimodal-no-no-no-no-next_line-no-lru-5cpu-0accelerator 20 50 5 vgg-16_500M.trace.gz 0 vgg-s_500M.trace.gz 1 nin_500M.trace.gz 2 server_028.champsimtrace.xz 3 server_014.champsimtrace.xz 4
cp results_4core_50M/mix5-bimodal-no-no-no-no-next_line-no-lru-5cpu-0accelerator.txt results_4core_50M/full_tace_with_cpu03_pref
echo "3st completed"

#./run_4core.sh bimodal-no-no-no-no-no-no-lru-2cpu-2accelerator 50 50 5 lenet-5_500M.trace.gz 0 cifar10_500M.trace.gz 1 cifar10_500M.trace.gz 2 inception_500M.trace.gz 3

