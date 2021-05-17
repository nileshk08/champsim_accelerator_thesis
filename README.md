<p align="center">
  <h1 align="center"> ChampSim </h1>
  <p> ChampSim simulator is a trace-based simulator for micro-architecture study. ChampSim has a simple and modular design, easy to understand, making it convenient for use and easy to
modify according to our work needs. ChampSim can simulate single as well as multi-core
systems. It takes application traces as input and generates simulation results. ChampSim
has already provided the traces for a different set of benchmarks. If we want to use traces
for a particular application, ChamSim has the convenience to generate traces for a partic-
ular application using the PIN tool. It supports different replacement policies, prefetching
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

Execute `run_champsim.sh` with proper input arguments. The default `TRACE_DIR` in `run_1accelerator.sh` is set to `$PWD/accelerator_traces`. <br>

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


# Add your own branch predictor, data prefetchers, and replacement policy
**Copy an empty template**
```
$ cp branch/branch_predictor.cc prefetcher/mybranch.bpred
$ cp prefetcher/l1d_prefetcher.cc prefetcher/mypref.l1d_pref
$ cp prefetcher/l2c_prefetcher.cc prefetcher/mypref.l2c_pref
$ cp prefetcher/llc_prefetcher.cc prefetcher/mypref.llc_pref
$ cp replacement/llc_replacement.cc replacement/myrepl.llc_repl
```

**Work on your algorithms with your favorite text editor**
```
$ vim branch/mybranch.bpred
$ vim prefetcher/mypref.l1d_pref
$ vim prefetcher/mypref.l2c_pref
$ vim prefetcher/mypref.llc_pref
$ vim replacement/myrepl.llc_repl
```

**Compile and test**
```
$ ./build_champsim.sh mybranch mypref mypref mypref myrepl 1
$ ./run_champsim.sh mybranch-mypref-mypref-mypref-myrepl-1core 1 10 bzip2_183B
```

# How to create traces

We have included only 4 sample traces, taken from SPEC CPU 2006. These 
traces are short (10 million instructions), and do not necessarily cover the range of behaviors your 
replacement algorithm will likely see in the full competition trace list (not
included).  We STRONGLY recommend creating your own traces, covering
a wide variety of program types and behaviors.

The included Pin Tool champsim_tracer.cpp can be used to generate new traces.
We used Pin 3.2 (pin-3.2-81205-gcc-linux), and it may require 
installing libdwarf.so, libelf.so, or other libraries, if you do not already 
have them. Please refer to the Pin documentation (https://software.intel.com/sites/landingpage/pintool/docs/81205/Pin/html/)
for working with Pin 3.2.

Get this version of Pin:
```
wget http://software.intel.com/sites/landingpage/pintool/downloads/pin-3.2-81205-gcc-linux.tar.gz
```

**Use the Pin tool like this**
```
pin -t obj-intel64/champsim_tracer.so -- <your program here>
```

The tracer has three options you can set:
```
-o
Specify the output file for your trace.
The default is default_trace.champsim

-s <number>
Specify the number of instructions to skip in the program before tracing begins.
The default value is 0.

-t <number>
The number of instructions to trace, after -s instructions have been skipped.
The default value is 1,000,000.
```
For example, you could trace 200,000 instructions of the program ls, after
skipping the first 100,000 instructions, with this command:
```
pin -t obj/champsim_tracer.so -o traces/ls_trace.champsim -s 100000 -t 200000 -- ls
```
Traces created with the champsim_tracer.so are approximately 64 bytes per instruction,
but they generally compress down to less than a byte per instruction using xz compression.

# Evaluate Simulation

ChampSim measures the IPC (Instruction Per Cycle) value as a performance metric. <br>
There are some other useful metrics printed out at the end of simulation. <br>

Good luck and be a champion! <br>
