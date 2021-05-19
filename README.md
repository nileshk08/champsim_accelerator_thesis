<p align="center">
  <h1 align="center"> ChampSim </h1>
  <p> ChampSim simulator is a trace-based simulator for micro-architecture study. ChampSim has a simple and modular design, easy to understand, making it convenient for use and easy to
modify according to our work needs. ChampSim can simulate single as well as multi-core
systems. It takes application traces as input and generates simulation results. ChampSim
has already provided the traces for a different set of benchmarks. If we want to use traces
for a particular application, ChamSim has the convenience to generate traces for a particular application using the PIN tool. It supports different replacement policies, prefetching
techniques and branch predictors at different levels of cache. It gives the flexibility to
implement new policies/prefetchers/predictors and update the existing ones. This repository is an extension of the original ChampSim repository(https://github.com/ChampSim/ChampSim), where we made some of the changes to simulate it for accelerator-centric architecture. 

  <p>
</p>

# Clone ChampSim repository
```
git clone https://github.com/nileshk08/champsim_accelerator_thesis.git 
```

# Compile

Accelerator version of ChampSim takes nine parameters: Branch predictor, ITLB prefetcher, DTLB prefetcher, STLB prefetcher, PTL2 prefetcher, DTLB replacement, STLB replacement, PTL2 replacement and the number of accelerators. 
For example, `./build_champsim.sh bimodal no no no no lru lru lru 1` builds a simulator for single accelerator with bimodal branch predictor, no ITLB/DTLB/STLB/PTL2 prefetchers, and the baseline LRU replacement policy for the DTLB/STLB/PTL2.
```
$ ./build_champsim.sh bimodal no no no no lru lru lru  1

$ ./build_champsim.sh ${BRANCH} ${ITLB_PREFETCHER} ${DTLB_PREFETCHER} ${STLB_PREFETCHER} ${PTL2_PREFETCHER} ${DTLB_REPLACEMENT} ${STLB_REPLACEMENT} ${PSCL2_REPLACEMENT} ${NUM_ACCELERATOR}

```

# Run simulation

Execute `run_1accelerator.sh` with proper input arguments. The default `TRACE_DIR` in `run_1accelerator.sh` is set to `$PWD/accelerator_traces`. <br>

* Single-accelerator simulation: Run simulation with `run_1accelerator.sh` script.

```
Usage: ./run_1accelerator.sh [BINARY] [N_WARM] [N_MIX] [N_SIM] [TRACE0] [ASID0]
$ ./run_1accelerator.sh bimodal-no-no-no-no-lru-lru-lru-1accelerator 20 50 10  alexnet_500M.trace.gz 0

${BINARY}: ChampSim binary compiled by "build_champsim.sh" (bimodal-no-no-no-no-lru-lru-lru-1accelerator) 
${N_WARM}: number of instructions for warmup (20 million)
${N_SIM}:  number of instructinos for detailed simulation (50 million)
${N_MIX}:  help to store the output with different numbers 
${TRACE0}: trace name (alexnet_500M.trace.gz)
${ASID0}:  trace will run with the asid (0)
```
Simulation results will be stored under "results_${N_SIM}M" as a form of "mix${N_MIX}-${BINARY}.txt".<br> 

* Multi-core simulation: Run simulation with `run_4accelerator.sh` script. <br>
```
Usage: ./run_4accelerator.sh [BINARY] [N_WARM] [N_SIM] [N_MIX] [TRACE0] [ASID0] [TRACE1] [ASID1] [TRACE2] [ASID2] [TRACE3] [ASID3]
$ ./run_4accelerator.sh bimodal-no-no-no-no-lru-lru-lru-1accelerator 20 50 10 alexnet_500M.trace.gz 0 cifar10_500M.trace.gz 1 inception_500M.trace.gz 2 lenet-5_500M.trace.gz 3 
```
Note that we need to specify multiple trace files for `run_4accelerator.sh`. 

Similarly, we can simulate with 8 accelerators by using script `run_8accelerator.sh`.

# Simulate with your own  prefetchers, and replacement policy

Prefetchers which are already available are present in the folder `prefetcher`. If you want to add or edit an existing prefetcher, then you can add or edit the files present in this folder. For our work, we have used the `next_line` and the `constant_stride` prefetcher.

Similarly, various replacement policies are available in folder `replacement`. You can add or edit the existing replacement policy. For our work, we have used SRRIP replacement policy.


**Compile and test**
```
$ ./build_champsim.sh bimodal no no no next_line srrip srrip srrip 4
$ ./run_4accelerator.sh bimodal-no-no-no-next_line-srrip-srrip-srrip-4accelerator 1 1 10 alexnet_500M.trace.gz 0 cifar10_500M.trace.gz 1 inception_500M.trace.gz 2 lenet-5_500M.trace.gz 3
```

# Evaluate Simulation

ChampSim measures the IPC (Instruction Per Cycle) value as a performance metric. <br>
There are some other useful metrics printed out at the end of simulation. <br>

