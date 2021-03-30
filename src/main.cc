#define _BSD_SOURCE

#include <getopt.h>
#include "ooo_cpu.h"
#include "uncore.h"
#include <fstream>

uint8_t warmup_complete[NUM_CPUS], 
        simulation_complete[NUM_CPUS], 
        all_warmup_complete = 0, 
        all_simulation_complete = 0,
        MAX_INSTR_DESTINATIONS = NUM_INSTR_DESTINATIONS,
        knob_cloudsuite = 0,
        knob_low_bandwidth = 0,
	knob_context_switch = 0;

uint64_t warmup_instructions     = 1000000,
         simulation_instructions = 10000000,
         champsim_seed;

FILE *context_switch_file;
char context_switch_string[1024];
time_t start_time;

struct CS_file{
	int index;
	long cycle;
	uint8_t swap_cpu[2];
} ;

//Data-structure used to store the contents of context-switch file
struct CS_file cs_file[CONTEXT_SWITCH_FILE_SIZE];

// PAGE TABLE
uint32_t PAGE_TABLE_LATENCY = 0, SWAP_LATENCY = 0;
#ifndef INS_PAGE_TABLE_WALKER
queue <uint64_t > page_queue;
map <uint64_t, uint64_t> page_table, inverse_table, recent_page, unique_cl[NUM_CPUS];
#endif
uint64_t previous_ppage, num_adjacent_page, num_cl[NUM_CPUS], allocated_pages, num_page[NUM_CPUS], minor_fault[NUM_CPUS], major_fault[NUM_CPUS];

void record_roi_stats(uint32_t cpu, CACHE *cache)
{
    for (uint32_t i=0; i<NUM_TYPES; i++) {
        cache->roi_access[cpu][i] = cache->sim_access[cpu][i];
        cache->roi_hit[cpu][i] = cache->sim_hit[cpu][i];
        cache->roi_miss[cpu][i] = cache->sim_miss[cpu][i];
    }
}

void print_roi_stats(uint32_t cpu, CACHE *cache)
{
    uint64_t TOTAL_ACCESS = 0, TOTAL_HIT = 0, TOTAL_MISS = 0;

    for (uint32_t i=0; i<NUM_TYPES; i++) {
        TOTAL_ACCESS += cache->roi_access[cpu][i];
        TOTAL_HIT += cache->roi_hit[cpu][i];
        TOTAL_MISS += cache->roi_miss[cpu][i];
    }


/*
    cout << cache->NAME;
    cout << " TOTAL     ACCESS: " << setw(10) << TOTAL_ACCESS << "  HIT: " << setw(10) << TOTAL_HIT <<  "  MISS: " << setw(10) << TOTAL_MISS << "  HIT %: " << setw(10) << ((double)TOTAL_HIT*100/TOTAL_ACCESS) << "  MISS %: " << setw(10) << ((double)TOTAL_MISS*100/TOTAL_ACCESS) << endl;
    cout << cache->NAME;
    cout << " LOAD      ACCESS: " << setw(10) << cache->roi_access[cpu][0] << "  HIT: " << setw(10) << cache->roi_hit[cpu][0] << "  MISS: " << setw(10) << cache->roi_miss[cpu][0] << "  HIT %: " << setw(10) << ((double)cache->roi_hit[cpu][0]*100/cache->roi_access[cpu][0]) << "  MISS %: " << setw(10) << ((double)cache->roi_miss[cpu][0]*100/cache->roi_access[cpu][0]) << endl;

    cout << cache->NAME;
    cout << " RFO       ACCESS: " << setw(10) << cache->roi_access[cpu][1] << "  HIT: " << setw(10) << cache->roi_hit[cpu][1] << "  MISS: " << setw(10) << cache->roi_miss[cpu][1] << "  HIT %: " << setw(10) << ((double)cache->roi_hit[cpu][1]*100/cache->roi_access[cpu][1]) << "  MISS %: " << setw(10) << ((double)cache->roi_miss[cpu][1]*100/cache->roi_access[cpu][1]) << endl;

    cout << cache->NAME;
    cout << " PREFETCH  ACCESS: " << setw(10) << cache->roi_access[cpu][2] << "  HIT: " << setw(10) << cache->roi_hit[cpu][2] << "  MISS: " << setw(10) << cache->roi_miss[cpu][2] << "  HIT %: " << setw(10) << ((double)cache->roi_hit[cpu][2]*100/cache->roi_access[cpu][2]) << "  MISS %: " << setw(10) << ((double)cache->roi_miss[cpu][2]*100/cache->roi_access[cpu][2]) <<  endl;

    cout << cache->NAME;
    cout << " WRITEBACK ACCESS: " << setw(10) << cache->roi_access[cpu][3] << "  HIT: " << setw(10) << cache->roi_hit[cpu][3] << "  MISS: " << setw(10) << cache->roi_miss[cpu][3] << "  HIT %: " << setw(10) << ((double)cache->roi_hit[cpu][3]*100/cache->roi_access[cpu][3]) << "  MISS %: " << setw(10) << ((double)cache->roi_miss[cpu][3]*100/cache->roi_access[cpu][3]) << endl ;

    cout << cache->NAME;
    cout << " PREFETCH  REQUESTED: " << setw(10) << cache->pf_requested << "  ISSUED: " << setw(10) << cache->pf_issued;
    cout << "  USEFUL: " << setw(10) << cache->pf_useful << "  USELESS: " << setw(10) << cache->pf_useless << endl;

    cout << cache->NAME;
    cout << " AVERAGE MISS LATENCY: " << (1.0*(cache->total_miss_latency))/TOTAL_MISS << " cycles" << endl;
    
    //if(cache->NAME=="ITLB" || cache->NAME=="DTLB" || cache->NAME=="STLB")
    //{
    	cout << cache->NAME;
    	cout << " USEFUL LOAD PREFETCHES: " << setw(10) << cache->pf_useful << " PREFETCH ISSUED TO LOWER LEVEL: " << setw(10) << cache->pf_lower_level << "  ACCURACY: " << ((double)cache->pf_useful*100/cache->pf_lower_level) << endl;
    
    	cout << cache->NAME;
    	cout << " TIMELY PREFETCHES: " << setw(10) << cache->pf_useful << " LATE PREFETCHES: " << cache->pf_late << endl;  
    //}
    cout << endl;
*/
    cout << "\n\n TOTal latency:= " << cache->sim_latency[cpu] << endl;
        cout << " Total latency request " << cache->total_lat_req[cpu]  << " " << cache->check_stlb_counter[cpu]<< endl;

    uint64_t num_instrs = ooo_cpu[cpu].finish_sim_instr;

    if(TOTAL_ACCESS != 0) 
	{
			cout << cache->NAME;
			cout << " TOTAL     ACCESS: " << setw(10) << TOTAL_ACCESS << "  HIT: " << setw(10) << TOTAL_HIT <<  "  MISS: " << setw(10) << TOTAL_MISS << "  HIT %: " << setw(10) << ((double)TOTAL_HIT*100/TOTAL_ACCESS) << "  MISS %: " << setw(10) << ((double)TOTAL_MISS*100/TOTAL_ACCESS) << "   MPKI: " <<  ((double)TOTAL_MISS*1000/num_instrs)<<" Avg Lat: "<< setw(10) <<fixed << (cache->sim_latency[cpu] * 1.0 / TOTAL_ACCESS) << endl;
	}
	
	if(cache->roi_access[cpu][0])
	{
			cout << cache->NAME;
			cout << " LOAD      ACCESS: " << setw(10) << cache->roi_access[cpu][0] << "  HIT: " << setw(10) << cache->roi_hit[cpu][0] << "  MISS: " << setw(10) << cache->roi_miss[cpu][0] << "  HIT %: " << setw(10) << ((double)cache->roi_hit[cpu][0]*100/cache->roi_access[cpu][0]) << "  MISS %: " << setw(10) << ((double)cache->roi_miss[cpu][0]*100/cache->roi_access[cpu][0]) << "   MPKI: " <<  ((double)cache->roi_miss[cpu][0]*1000/num_instrs) << endl; // << " T_ACCESS: " << setw(10) << cache->ACCESS[0] << " T_HIT: " << setw(10) << cache->HIT[0] << " T_MISS: " << setw(10) << cache->MISS[0] << " T_MSHR_MERGED: " << cache->MSHR_MERGED[0] << endl; //@Vishal: MSHR merged will give correct result for 1 core, multi_core will give whole sim
	}

	if(cache->roi_access[cpu][1])
    {
			cout << cache->NAME;
			cout << " RFO       ACCESS: " << setw(10) << cache->roi_access[cpu][1] << "  HIT: " << setw(10) << cache->roi_hit[cpu][1] << "  MISS: " << setw(10) << cache->roi_miss[cpu][1] << "  HIT %: " << setw(10) << ((double)cache->roi_hit[cpu][1]*100/cache->roi_access[cpu][1]) << "  MISS %: " << setw(10) << ((double)cache->roi_miss[cpu][1]*100/cache->roi_access[cpu][1]) << "   MPKI: " <<  ((double)cache->roi_miss[cpu][1]*1000/num_instrs) << endl; //<< " T_ACCESS: " << setw(10) << cache->ACCESS[1] << " T_HIT: " << setw(10) << cache->HIT[1] << " T_MISS: " << setw(10) << cache->MISS[1] << " T_MSHR_MERGED: " << cache->MSHR_MERGED[1] << endl;
	}

	if(cache->roi_access[cpu][2])
    {
			cout << cache->NAME;
			cout << " PREFETCH  ACCESS: " << setw(10) << cache->roi_access[cpu][2] << "  HIT: " << setw(10) << cache->roi_hit[cpu][2] << "  MISS: " << setw(10) << cache->roi_miss[cpu][2] << "  HIT %: " << setw(10) << ((double)cache->roi_hit[cpu][2]*100/cache->roi_access[cpu][2]) << "  MISS %: " << setw(10) << ((double)cache->roi_miss[cpu][2]*100/cache->roi_access[cpu][2]) << "   MPKI: " <<  ((double)cache->roi_miss[cpu][2]*1000/num_instrs) << endl; //<< " T_ACCESS: " << setw(10) << cache->ACCESS[2] << " T_HIT: " << setw(10) << cache->HIT[2] << " T_MISS: " << setw(10) << cache->MISS[2] << " T_MSHR_MERGED: " << cache->MSHR_MERGED[2] << endl;
	}

	if(cache->roi_access[cpu][3])
    {
			cout << cache->NAME;
			cout << " WRITEBACK ACCESS: " << setw(10) << cache->roi_access[cpu][3] << "  HIT: " << setw(10) << cache->roi_hit[cpu][3] << "  MISS: " << setw(10) << cache->roi_miss[cpu][3] << "  HIT %: " << setw(10) << ((double)cache->roi_hit[cpu][3]*100/cache->roi_access[cpu][3]) << "  MISS %: " << setw(10) << ((double)cache->roi_miss[cpu][3]*100/cache->roi_access[cpu][3]) << "   MPKI: " <<  ((double)cache->roi_miss[cpu][3]*1000/num_instrs) << endl; //<< " T_ACCESS: " << setw(10) << cache->ACCESS[3] << " T_HIT: " << setw(10) << cache->HIT[3] << " T_MISS: " << setw(10) << cache->MISS[3] <<" T_MSHR_MERGED: " << cache->MSHR_MERGED[3] << endl;
	}
    
	if(cache->roi_access[cpu][4])
    {
			cout << cache->NAME;
			cout << " TRANSLATION ACCESS: " << setw(10) << cache->roi_access[cpu][4] << "  HIT: " << setw(10) << cache->roi_hit[cpu][4] << "  MISS: " << setw(10) << cache->roi_miss[cpu][4] << "  HIT %: " << setw(10) << ((double)cache->roi_hit[cpu][4]*100/cache->roi_access[cpu][4]) << "  MISS %: " << setw(10) << ((double)cache->roi_miss[cpu][4]*100/cache->roi_access[cpu][4]) << "   MPKI: " <<  ((double)cache->roi_miss[cpu][4]*1000/num_instrs) << endl; //<< " T_ACCESS: " << setw(10) << cache->ACCESS[4] << " T_HIT: " << setw(10) << cache->HIT[4] << " T_MISS: " << setw(10) << cache->MISS[4] <<" T_MSHR_MERGED: " << cache->MSHR_MERGED[4] << endl;
	}

	if(cache->pf_requested)
	{
			cout << cache->NAME;
			cout << " PREFETCH  REQUESTED: " << setw(10) << cache->pf_requested << "  ISSUED: " << setw(10) << cache->pf_issued;
			cout << "  USEFUL: " << setw(10) << cache->pf_useful << "  USELESS: " << setw(10) << cache->pf_useless << endl;
	
			 cout << cache->NAME;
 		         cout << " USEFUL LOAD PREFETCHES: " << setw(10) << cache->pf_useful << " PREFETCH ISSUED TO LOWER LEVEL: " << setw(10) << cache->pf_lower_level << "  ACCURACY: " <<      ((double)cache->pf_useful*100/cache->pf_lower_level) << endl;
  		         cout << " TIMELY PREFETCHES: " << setw(10) << cache->pf_useful << " LATE PREFETCHES: " << cache->pf_late << endl;

	}

	if(cache->cache_type == IS_SCRATCHPAD){
	
			cout << cache->NAME;
			cout << " TOTAL     ACCESS: " << setw(10) << TOTAL_ACCESS << "  HIT: " << setw(10) << TOTAL_HIT <<  "  MISS: " << setw(10) << TOTAL_MISS << "  HIT %: " << setw(10) << ((double)TOTAL_HIT*100/TOTAL_ACCESS) << "  MISS %: " << setw(10) << ((double)TOTAL_MISS*100/TOTAL_ACCESS) << "   MPKI: " <<  ((double)TOTAL_MISS*1000/num_instrs)<<" Avg Lat: "<< setw(10) <<fixed << (cache->sim_latency[cpu] * 1.0 / TOTAL_ACCESS) << endl;
	}
    if(cache->cache_type == IS_PSCL5 || cache->cache_type == IS_PSCL4 || cache->cache_type == IS_PSCL3 || cache->cache_type == IS_PSCL2)
	{
	}
	else	
	{
			cout << cache->NAME;
			cout << " AVERAGE MISS LATENCY: " << (1.0*(cache->total_miss_latency))/TOTAL_MISS << " cycles" << endl;
	}

    //@Vishal: Will work only for 1 core, for multi-core this will give sim_result not roi_result
	if(cache->RQ.ACCESS)
    	cout << " RQ	ACCESS: "<< setw(10) << cache->RQ.ACCESS << "	FORWARD: " << setw(10) << cache->RQ.FORWARD << "	MERGED: " << setw(10) << cache->RQ.MERGED << "	TO_CACHE: " << setw(10) << cache->RQ.TO_CACHE << endl;
	
	if(cache->WQ.ACCESS)
    	cout << " WQ	ACCESS: "<< setw(10) << cache->WQ.ACCESS << "	FORWARD: " << setw(10) << cache->WQ.FORWARD << "	MERGED: " << setw(10) << cache->WQ.MERGED << "	TO_CACHE: " << setw(10) << cache->WQ.TO_CACHE << endl;
	
	if(cache->PQ.ACCESS)
    	cout << " PQ	ACCESS: "<< setw(10) << cache->PQ.ACCESS << "	FORWARD: " << setw(10) << cache->PQ.FORWARD << "	MERGED: " << setw(10) << cache->PQ.MERGED << "	TO_CACHE: " << setw(10) << cache->PQ.TO_CACHE << endl;
	
	cout << endl;



	//Neelu: addition ideal spatial region stats
	if(cache->cache_type == IS_L1D)
	{
		cout<<"UNIQUE REGIONS ACCESSED: "<<cache->unique_region_count<<endl;
		cout<<"REGIONS CONFLICTS:"<<cache->region_conflicts<<endl;
	}

	static int flag = 0;
	
	#ifdef PUSH_DTLB_PB
	if(cache->cache_type == IS_DTLB_PB && flag == 0)
	{	
		flag = 1;
		cout <<"DTLB_PB PREFETCH USEFUL - " << cache->pf_useful << " USELESS PREFETCHES: " << cache->pf_useless << endl;
	}
	#endif
}


void print_sim_stats(uint32_t cpu, CACHE *cache)
{
    uint64_t TOTAL_ACCESS = 0, TOTAL_HIT = 0, TOTAL_MISS = 0;

    for (uint32_t i=0; i<NUM_TYPES; i++) {
        TOTAL_ACCESS += cache->sim_access[cpu][i];
        TOTAL_HIT += cache->sim_hit[cpu][i];
        TOTAL_MISS += cache->sim_miss[cpu][i];
    }

    cout << "\n\n TOTal latency:= " << cache->sim_latency[cpu] << endl;
	cout << " Total latency request " << cache->total_lat_req[cpu]  << " " << cache->check_stlb_counter[cpu]<< endl;
    uint64_t num_instrs = ooo_cpu[cpu].num_retired - ooo_cpu[cpu].begin_sim_instr;

    if(cache->cache_type == IS_DTLB){
    	cout << " DTLB cold miss = " << cache->coldMiss_DTLB.size()  << endl;
    }
    cout << cache->NAME;
    cout << " TOTAL     ACCESS: " << setw(10) << TOTAL_ACCESS << "  HIT: " << setw(10) << TOTAL_HIT <<  "  MISS: " << setw(10) << TOTAL_MISS << "  HIT %: " << setw(10) << ((double)TOTAL_HIT*100/TOTAL_ACCESS) << "  MISS %: " << setw(10) << ((double)TOTAL_MISS*100/TOTAL_ACCESS) << "   MPKI: " <<  ((double)TOTAL_MISS*1000/num_instrs) << " Avg Lat: "<< setw(10) << fixed << (cache->sim_latency[cpu]*1.0/ cache->total_lat_req[cpu])<< endl;

    cout << cache->NAME;
    cout << " LOAD      ACCESS: " << setw(10) << cache->sim_access[cpu][0] << "  HIT: " << setw(10) << cache->sim_hit[cpu][0] << "  MISS: " << setw(10) << cache->sim_miss[cpu][0] << "  HIT %: " << setw(10) << ((double)cache->sim_hit[cpu][0]*100/cache->sim_access[cpu][0]) << "  MISS %: " << setw(10) << ((double)cache->sim_miss[cpu][0]*100/cache->sim_access[cpu][0]) << "   MPKI: " <<  ((double)cache->sim_miss[cpu][0]*1000/num_instrs) << endl;

    cout << cache->NAME;
    cout << " RFO       ACCESS: " << setw(10) << cache->sim_access[cpu][1] << "  HIT: " << setw(10) << cache->sim_hit[cpu][1] << "  MISS: " << setw(10) << cache->sim_miss[cpu][1] << "  HIT %: " << setw(10) << ((double)cache->sim_hit[cpu][1]*100/cache->sim_access[cpu][1]) << "  MISS %: " << setw(10) << ((double)cache->sim_miss[cpu][1]*100/cache->sim_access[cpu][1]) << "   MPKI: " <<  ((double)cache->sim_miss[cpu][1]*1000/num_instrs) << endl;

    cout << cache->NAME;
    cout << " PREFETCH  ACCESS: " << setw(10) << cache->sim_access[cpu][2] << "  HIT: " << setw(10) << cache->sim_hit[cpu][2] << "  MISS: " << setw(10) << cache->sim_miss[cpu][2] << "  HIT %: " << setw(10) << ((double)cache->sim_hit[cpu][2]*100/cache->sim_access[cpu][2]) << "  MISS %: " << setw(10) << ((double)cache->sim_miss[cpu][2]*100/cache->sim_access[cpu][2]) << "   MPKI: " <<  ((double)cache->sim_miss[cpu][2]*1000/num_instrs) << endl;

    cout << cache->NAME;
    cout << " WRITEBACK ACCESS: " << setw(10) << cache->sim_access[cpu][3] << "  HIT: " << setw(10) << cache->sim_hit[cpu][3] << "  MISS: " << setw(10) << cache->sim_miss[cpu][3] << "  HIT %: " << setw(10) << ((double)cache->sim_hit[cpu][3]*100/cache->sim_access[cpu][3]) << "  MISS %: " << setw(10) << ((double)cache->sim_miss[cpu][3]*100/cache->sim_access[cpu][3]) << "   MPKI: " <<  ((double)cache->sim_miss[cpu][3]*1000/num_instrs) << endl;
	
	#ifdef PUSH_DTLB_PB
	if(cache->cache_type == IS_DTLB_PB)
                cout <<"DTLB_PB PREFETCH USEFUL: " << cache->pf_useful << endl;
	#endif
}

void print_branch_stats()
{
    for (uint32_t i=0; i<NUM_CPUS; i++) {
        cout << endl << "CPU " << i << " Branch Prediction Accuracy: ";
        cout << (100.0*(ooo_cpu[i].num_branch - ooo_cpu[i].branch_mispredictions)) / ooo_cpu[i].num_branch;
        cout << "% MPKI: " << (1000.0*ooo_cpu[i].branch_mispredictions)/(ooo_cpu[i].num_retired - ooo_cpu[i].warmup_instructions);
	cout << " Average ROB Occupancy at Mispredict: " << (1.0*ooo_cpu[i].total_rob_occupancy_at_branch_mispredict)/ooo_cpu[i].branch_mispredictions << endl;
    }
}

void print_dram_stats()
{
    cout << endl;
    cout << "DRAM Statistics" << endl;
    cout << "No of PTW : " << uncore.DRAM.no_of_PTW << endl;
    for (uint32_t i=0; i<DRAM_CHANNELS; i++) {
        cout << " CHANNEL " << i << endl;
        cout << " RQ ROW_BUFFER_HIT: " << setw(10) << uncore.DRAM.RQ[i].ROW_BUFFER_HIT << "  ROW_BUFFER_MISS: " << setw(10) << uncore.DRAM.RQ[i].ROW_BUFFER_MISS << endl;
        cout << " DBUS_CONGESTED: " << setw(10) << uncore.DRAM.dbus_congested[NUM_TYPES][NUM_TYPES] << endl; 
        cout << " WQ ROW_BUFFER_HIT: " << setw(10) << uncore.DRAM.WQ[i].ROW_BUFFER_HIT << "  ROW_BUFFER_MISS: " << setw(10) << uncore.DRAM.WQ[i].ROW_BUFFER_MISS;
        cout << "  FULL: " << setw(10) << uncore.DRAM.WQ[i].FULL << endl; 
        cout << endl;
    }

    uint64_t total_congested_cycle = 0;
    for (uint32_t i=0; i<DRAM_CHANNELS; i++)
        total_congested_cycle += uncore.DRAM.dbus_cycle_congested[i];
    if (uncore.DRAM.dbus_congested[NUM_TYPES][NUM_TYPES])
        cout << " AVG_CONGESTED_CYCLE: " << (total_congested_cycle / uncore.DRAM.dbus_congested[NUM_TYPES][NUM_TYPES]) << endl;
    else
        cout << " AVG_CONGESTED_CYCLE: -" << endl;
}

void reset_cache_stats(uint32_t cpu, CACHE *cache)
{
    for (uint32_t i=0; i<NUM_TYPES; i++) {
        cache->ACCESS[i] = 0;
        cache->HIT[i] = 0;
        cache->MISS[i] = 0;
        cache->MSHR_MERGED[i] = 0;
        cache->STALL[i] = 0;

        cache->sim_access[cpu][i] = 0;
        cache->sim_hit[cpu][i] = 0;
        cache->sim_miss[cpu][i] = 0;
    }
    if(cache->cache_type == IS_DTLB)
	    cache->coldMiss_DTLB.clear();

    cache->total_miss_latency = 0;

    cache->RQ.ACCESS = 0;
    cache->RQ.MERGED = 0;
    cache->RQ.TO_CACHE = 0;
    cache->RQ.FORWARD = 0;
    cache->RQ.FULL = 0;
    
    cache->WQ.ACCESS = 0;
    cache->WQ.MERGED = 0;
    cache->WQ.TO_CACHE = 0;
    cache->WQ.FORWARD = 0;
    cache->WQ.FULL = 0;
    
    //reset_prefetch_stats
    cache->pf_requested = 0;
    cache->pf_issued = 0;
    cache->pf_useful = 0;
    cache->pf_useless = 0;
    cache->pf_fill = 0;
    cache->pf_late = 0;
    cache->pf_lower_level = 0;

    cache->PQ.ACCESS = 0;
    cache->PQ.MERGED = 0;
    cache->PQ.TO_CACHE = 0;
    cache->PQ.FORWARD = 0;
    cache->PQ.FULL = 0;
}

void finish_warmup()
{
    uint64_t elapsed_second = (uint64_t)(time(NULL) - start_time),
             elapsed_minute = elapsed_second / 60,
             elapsed_hour = elapsed_minute / 60;
    elapsed_minute -= elapsed_hour*60;
    elapsed_second -= (elapsed_hour*3600 + elapsed_minute*60);

    // eeset core latency
    SCHEDULING_LATENCY = 6;
    EXEC_LATENCY = 1;
    PAGE_TABLE_LATENCY = 100;
    SWAP_LATENCY = 100000;

    cout << endl;
    for (uint32_t i=0; i<NUM_CPUS; i++) {
        cout << "Warmup complete CPU " << i << " instructions: " << ooo_cpu[i].num_retired << " cycles: " << current_core_cycle[i];
        cout << " (Simulation time: " << elapsed_hour << " hr " << elapsed_minute << " min " << elapsed_second << " sec) " << endl;

        ooo_cpu[i].begin_sim_cycle = current_core_cycle[i]; 
        ooo_cpu[i].begin_sim_instr = ooo_cpu[i].num_retired;

        // reset branch stats
        ooo_cpu[i].num_branch = 0;
        ooo_cpu[i].branch_mispredictions = 0;
	ooo_cpu[i].total_rob_occupancy_at_branch_mispredict = 0;
	

	//@Vishal: reset cpu stats
	ooo_cpu[i].sim_RAW_hits = 0;
	ooo_cpu[i].sim_load_gen = 0;
	ooo_cpu[i].sim_load_sent = 0;
	ooo_cpu[i].sim_store_gen = 0;
	ooo_cpu[i].sim_store_sent = 0;


	reset_cache_stats(i, &ooo_cpu[i].ITLB);
        reset_cache_stats(i, &ooo_cpu[i].DTLB);
        reset_cache_stats(i, &ooo_cpu[i].STLB);
	reset_cache_stats(i, &ooo_cpu[i].SCRATCHPAD);
        reset_cache_stats(i, &ooo_cpu[i].L1I);
        reset_cache_stats(i, &ooo_cpu[i].L1D);
        reset_cache_stats(i, &ooo_cpu[i].L2C);
        reset_cache_stats(i, &uncore.LLC);

	//@Vishal: Reset MMU cache stats
	reset_cache_stats(i, &ooo_cpu[i].PTW.PSCL5);
	reset_cache_stats(i, &ooo_cpu[i].PTW.PSCL4);
	reset_cache_stats(i, &ooo_cpu[i].PTW.PSCL3);
	reset_cache_stats(i, &ooo_cpu[i].PTW.PSCL2);

	#ifdef PUSH_DTLB_PB	
	reset_cache_stats(i, &ooo_cpu[i].DTLB_PB);
        #endif
    }
    cout << endl;

    // reset DRAM stats
    for (uint32_t i=0; i<DRAM_CHANNELS; i++) {
        uncore.DRAM.RQ[i].ROW_BUFFER_HIT = 0;
        uncore.DRAM.RQ[i].ROW_BUFFER_MISS = 0;
        uncore.DRAM.WQ[i].ROW_BUFFER_HIT = 0;
        uncore.DRAM.WQ[i].ROW_BUFFER_MISS = 0;
    }

    // set actual cache latency
    for (uint32_t i=0; i<NUM_CPUS; i++) {
        ooo_cpu[i].ITLB.LATENCY = ITLB_LATENCY;
        ooo_cpu[i].DTLB.LATENCY = DTLB_LATENCY;
        ooo_cpu[i].STLB.LATENCY = STLB_LATENCY;
        ooo_cpu[i].L1I.LATENCY  = L1I_LATENCY;
        ooo_cpu[i].L1D.LATENCY  = L1D_LATENCY;
        ooo_cpu[i].L2C.LATENCY  = L2C_LATENCY;
//        ooo_cpu[i].PTW.LATENCY  = MMU_CACHE_LATENCY;
    }
    uncore.LLC.LATENCY = LLC_LATENCY;
}

void print_deadlock(uint32_t i)
{
	int k = i;
	for(int i=0;i<NUM_CPUS;i++){
    cout << "DEADLOCK! CPU " << i << " instr_id: " << ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].instr_id;
    cout << " translated: " << +ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].translated;
    cout << " fetched: " << +ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].fetched;
    cout << " scheduled: " << +ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].scheduled;
    cout << " executed: " << +ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].executed;
    cout << " is_memory: " << +ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].is_memory;
    cout << " event: " << ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].event_cycle;
    cout << " current: " << current_core_cycle[i] << endl;
	}
	i = k;
	cout << "deadlock for cpu " << i << endl;

	cout << "Scratchpad stats RQ " ;
	ooo_cpu[i].SCRATCHPAD.RQ.queuePrint();
       cout	<< " MSHR Trans \n";
       ooo_cpu[i].SCRATCHPAD.MSHR_TRANSLATION.queuePrint();
       cout	<< " MSHR DATA \n";
       ooo_cpu[i].SCRATCHPAD.MSHR_DATA.queuePrint();
	cout << "ITLB stats RQ \n";
	ooo_cpu[i].ITLB.RQ.queuePrint();
        cout << " MSHR \n" ;
       	ooo_cpu[i].ITLB.MSHR.queuePrint();
	cout << "DTLB stats RQ \n";
       	ooo_cpu[i].DTLB.RQ.queuePrint();
       cout<< " MSHR \n";
       ooo_cpu[i].DTLB.MSHR.queuePrint();
	cout << "STLB stats RQ \n";
       	ooo_cpu[ACCELERATOR_START].STLB.RQ.queuePrint();
       cout 	<< " MSHR \n";
       ooo_cpu[ACCELERATOR_START].STLB.MSHR.queuePrint();
	cout << "dram stats RQ" << endl;
	uncore.DRAM.RQ[0].queuePrint();
	cout << "WQ" << endl;
	uncore.DRAM.WQ[0].queuePrint();


    // print ROB entry
    cout << endl << "ROB Entries" << endl;
    for (uint32_t j=0; j<ROB_SIZE; j++) {
	cout <<  "rob_index = " << j <<" instr_id: " <<ooo_cpu[i].ROB.entry[j].instr_id <<" translated: " << (int)ooo_cpu[i].ROB.entry[j].translated << " fetched: " << (int)ooo_cpu[i].LQ.entry[i].fetched << " scheduled: " << (int)ooo_cpu[i].ROB.entry[j].scheduled << " executed: " << (int)ooo_cpu[i].ROB.entry[j].executed << endl;
    }
    // print LQ entry
    cout << endl << "Load Queue Entry" << endl;
    for (uint32_t j=0; j<LQ_SIZE; j++) {
        cout << "[LQ] entry: " << j << " instr_id: " << ooo_cpu[i].LQ.entry[j].instr_id << " address: " << hex << ooo_cpu[i].LQ.entry[j].physical_address << dec << " translated: " << +ooo_cpu[i].LQ.entry[j].translated << " fetched: " << +ooo_cpu[i].LQ.entry[i].fetched << endl;
    }

    // print SQ entry
    cout << endl << "Store Queue Entry" << endl;
    for (uint32_t j=0; j<SQ_SIZE; j++) {
        cout << "[SQ] entry: " << j << " instr_id: " << ooo_cpu[i].SQ.entry[j].instr_id << " address: " << hex << ooo_cpu[i].SQ.entry[j].physical_address << dec << " translated: " << +ooo_cpu[i].SQ.entry[j].translated << " fetched: " << +ooo_cpu[i].SQ.entry[i].fetched << endl;
    }

    // print L1D MSHR entry
    PACKET_QUEUE *queue;
    queue = &ooo_cpu[i].L1D.MSHR;
    cout << endl << queue->NAME << " Entry" << endl;
    for (uint32_t j=0; j<queue->SIZE; j++) {
        cout << "[" << queue->NAME << "] entry: " << j << " instr_id: " << queue->entry[j].instr_id << " rob_index: " << queue->entry[j].rob_index;
        cout << " address: " << hex << queue->entry[j].address << " full_addr: " << queue->entry[j].full_addr << dec << " type: " << +queue->entry[j].type;
        cout << " fill_level: " << queue->entry[j].fill_level << " lq_index: " << queue->entry[j].lq_index << " sq_index: " << queue->entry[j].sq_index << endl; 
    }

    assert(0);
}

void signal_handler(int signal) 
{
	cout << "Caught signal: " << signal << endl;
	exit(1);
}

// log base 2 function from efectiu
int lg2(int n)
{
    int i, m = n, c = -1;
    for (i=0; m; i++) {
        m /= 2;
        c++;
    }
    return c;
}

uint64_t rotl64 (uint64_t n, unsigned int c)
{
    const unsigned int mask = (CHAR_BIT*sizeof(n)-1);

    assert ( (c<=mask) &&"rotate by type width or more");
    c &= mask;  // avoid undef behaviour with NDEBUG.  0 overhead for most types / compilers
    return (n<<c) | (n>>( (-c)&mask ));
}

uint64_t rotr64 (uint64_t n, unsigned int c)
{
    const unsigned int mask = (CHAR_BIT*sizeof(n)-1);

    assert ( (c<=mask) &&"rotate by type width or more");
    c &= mask;  // avoid undef behaviour with NDEBUG.  0 overhead for most types / compilers
    return (n>>c) | (n<<( (-c)&mask ));
}

RANDOM champsim_rand(champsim_seed);

#ifndef INS_PAGE_TABLE_WALKER
uint64_t va_to_pa(uint32_t cpu, uint64_t instr_id, uint64_t va, uint64_t unique_vpage)
{
#ifdef SANITY_CHECK
    if (va == 0) 
        assert(0);
#endif
	//DP ( if (warmup_complete[cpu]) {
          //      cout << "va = " << hex << va << "unique_vpage = " << unique_vpage << endl; });
    uint8_t  swap = 0;
    uint64_t high_bit_mask = rotr64(cpu, lg2(NUM_CPUS)),
             unique_va = va | high_bit_mask;
    //DP ( if (warmup_complete[cpu]) {
      //      cout <<  hex << "unique_va = " << unique_va << "highBitMask = " << high_bit_mask << endl; });
    //uint64_t vpage = unique_va >> LOG2_PAGE_SIZE,
    uint64_t vpage = unique_vpage | high_bit_mask,
             voffset = unique_va & ((1<<LOG2_PAGE_SIZE) - 1);
    //DP ( if (warmup_complete[cpu]) {
      //          cout << "voffset = " << hex << voffset << "vpage = " << vpage << endl; });
    // smart random number generator
    uint64_t random_ppage;

    map <uint64_t, uint64_t>::iterator pr = page_table.begin();
    map <uint64_t, uint64_t>::iterator ppage_check = inverse_table.begin();

    // check unique cache line footprint
    map <uint64_t, uint64_t>::iterator cl_check = unique_cl[cpu].find(unique_va >> LOG2_BLOCK_SIZE);
    if (cl_check == unique_cl[cpu].end()) { // we've never seen this cache line before
        unique_cl[cpu].insert(make_pair(unique_va >> LOG2_BLOCK_SIZE, 0));
        num_cl[cpu]++;
    }
    else
        cl_check->second++;

    pr = page_table.find(vpage);
    if (pr == page_table.end()) { // no VA => PA translation found 

        if (allocated_pages >= DRAM_PAGES) { // not enough memory

            // TODO: elaborate page replacement algorithm
            // here, ChampSim randomly selects a page that is not recently used and we only track 32K recently accessed pages
            uint8_t  found_NRU = 0;
            uint64_t NRU_vpage = 0; // implement it
            map <uint64_t, uint64_t>::iterator pr2 = recent_page.begin();
            for (pr = page_table.begin(); pr != page_table.end(); pr++) {

                NRU_vpage = pr->first;
                if (recent_page.find(NRU_vpage) == recent_page.end()) {
                    found_NRU = 1;
                    break;
                }
            }
#ifdef SANITY_CHECK
            if (found_NRU == 0)
                assert(0);

            if (pr == page_table.end())
                assert(0);
#endif
            DP ( if (warmup_complete[cpu]) {
            cout << "[SWAP] update page table NRU_vpage: " << hex << pr->first << " new_vpage: " << vpage << " ppage: " << pr->second << dec << endl; });

            // update page table with new VA => PA mapping
            // since we cannot change the key value already inserted in a map structure, we need to erase the old node and add a new node
            uint64_t mapped_ppage = pr->second;
            page_table.erase(pr);
            page_table.insert(make_pair(vpage, mapped_ppage));

            // update inverse table with new PA => VA mapping
            ppage_check = inverse_table.find(mapped_ppage);
#ifdef SANITY_CHECK
            if (ppage_check == inverse_table.end())
                assert(0);
#endif
            ppage_check->second = vpage;

            DP ( if (warmup_complete[cpu]) {
            cout << "[SWAP] update inverse table NRU_vpage: " << hex << NRU_vpage << " new_vpage: ";
            cout << ppage_check->second << " ppage: " << ppage_check->first << dec << endl; });

            // update page_queue
            page_queue.pop();
            page_queue.push(vpage);

            // invalidate corresponding vpage and ppage from the cache hierarchy
            ooo_cpu[cpu].ITLB.invalidate_entry(NRU_vpage);
            ooo_cpu[cpu].DTLB.invalidate_entry(NRU_vpage);
            ooo_cpu[cpu].STLB.invalidate_entry(NRU_vpage);
            for (uint32_t i=0; i<BLOCK_SIZE; i++) {
                uint64_t cl_addr = (mapped_ppage << 6) | i;
                ooo_cpu[cpu].L1I.invalidate_entry(cl_addr);
                ooo_cpu[cpu].L1D.invalidate_entry(cl_addr);
                ooo_cpu[cpu].L2C.invalidate_entry(cl_addr);
                uncore.LLC.invalidate_entry(cl_addr);
            }

            // swap complete
            swap = 1;
        } else {
            uint8_t fragmented = 0;
            if (num_adjacent_page > 0)
                random_ppage = ++previous_ppage;
            else {
                random_ppage = champsim_rand.draw_rand();
                fragmented = 1;
            }

            // encoding cpu number 
            // this allows ChampSim to run homogeneous multi-programmed workloads without VA => PA aliasing
            // (e.g., cpu0: astar  cpu1: astar  cpu2: astar  cpu3: astar...)
            //random_ppage &= (~((NUM_CPUS-1)<< (32-LOG2_PAGE_SIZE)));
            //random_ppage |= (cpu<<(32-LOG2_PAGE_SIZE)); 

            while (1) { // try to find an empty physical page number
                ppage_check = inverse_table.find(random_ppage); // check if this page can be allocated 
                if (ppage_check != inverse_table.end()) { // random_ppage is not available
                    DP ( if (warmup_complete[cpu]) {
                    cout << "vpage: " << hex << ppage_check->first << " is already mapped to ppage: " << random_ppage << dec << endl; }); 
                    
                    if (num_adjacent_page > 0)
                        fragmented = 1;

                    // try one more time
                    random_ppage = champsim_rand.draw_rand();
                    
                    // encoding cpu number 
                    //random_ppage &= (~((NUM_CPUS-1)<<(32-LOG2_PAGE_SIZE)));
                    //random_ppage |= (cpu<<(32-LOG2_PAGE_SIZE)); 
                }
                else
                    break;
            }

            // insert translation to page tables
            //printf("Insert  num_adjacent_page: %u  vpage: %lx  ppage: %lx\n", num_adjacent_page, vpage, random_ppage);
            page_table.insert(make_pair(vpage, random_ppage));
            inverse_table.insert(make_pair(random_ppage, vpage));
            page_queue.push(vpage);
            previous_ppage = random_ppage;
            num_adjacent_page--;
            num_page[cpu]++;
            allocated_pages++;

            // try to allocate pages contiguously
            if (fragmented) {
                num_adjacent_page = 1 << (rand() % 10);
                DP ( if (warmup_complete[cpu]) {
                cout << "Recalculate num_adjacent_page: " << num_adjacent_page << endl; });
            }
        }

        if (swap)
            major_fault[cpu]++;
        else
            minor_fault[cpu]++;
    }
    else {
        //printf("Found  vpage: %lx  random_ppage: %lx\n", vpage, pr->second);
    }

    pr = page_table.find(vpage);
#ifdef SANITY_CHECK
    if (pr == page_table.end())
        assert(0);
#endif
    uint64_t ppage = pr->second;

    uint64_t pa = ppage << LOG2_PAGE_SIZE;
    pa |= voffset;

    DP ( if (warmup_complete[cpu]) {
    cout << "[PAGE_TABLE] instr_id: " << instr_id << " vpage: " << hex << vpage;
    cout << " => ppage: " << (pa >> LOG2_PAGE_SIZE) << " vadress: " << unique_va << " paddress: " << pa << dec << endl; });

    
    if (swap)
        stall_cycle[cpu] = current_core_cycle[cpu] + SWAP_LATENCY;
    else
        stall_cycle[cpu] = current_core_cycle[cpu] + PAGE_TABLE_LATENCY;

    //cout << "cpu: " << cpu << " allocated unique_vpage: " << hex << unique_vpage << " to ppage: " << ppage << dec << endl;

    return pa;
}
#endif

void swap_context(uint8_t swap_cpu_0, uint8_t swap_cpu_1)
{
	//swap file descriptor
	FILE *swap_file;
	swap_file = ooo_cpu[swap_cpu_0].trace_file;
	ooo_cpu[swap_cpu_0].trace_file = ooo_cpu[swap_cpu_1].trace_file;
	ooo_cpu[swap_cpu_1].trace_file = swap_file;

	//fetch_stall should be zero after context-switch
	//Since context-switch penalty is not included, this needs to be done, once it is added, fetch_stall will be 0 as branch mispredict penalty is only 20, while context-switch penalty is in miliseconds 
	ooo_cpu[swap_cpu_0].fetch_stall = 0;
	ooo_cpu[swap_cpu_1].fetch_stall = 0;

	cout << "FLUSHING TLB \n\n\n" << endl;
	//TLB flush
	
	ooo_cpu[swap_cpu_0].ITLB.flush_TLB();
	ooo_cpu[swap_cpu_1].ITLB.flush_TLB();
	ooo_cpu[swap_cpu_0].DTLB.flush_TLB();
	ooo_cpu[swap_cpu_1].DTLB.flush_TLB();
	ooo_cpu[swap_cpu_0].STLB.flush_TLB();
	ooo_cpu[swap_cpu_1].STLB.flush_TLB();

}

void check_prefetch_status_ptl2(){
	for(int i=1;i<NUM_CPUS; i++){
		CACHE *cache = &ooo_cpu[ACCELERATOR_START].PTW.PSCL2;
		cache->is_prefetch[i] = false;
		if((cache->sim_hit[i][TRANSLATION]*100 / cache->sim_access[i][TRANSLATION]) >= PREFETCH_THRESHOLD){
			cache->is_prefetch[i] = true;	
		}    
		else
			cache->is_prefetch[i] = false;
		cout << "cpu " << i << " sim hit " << cache->sim_hit[i][TRANSLATION] << " sim access " << cache->sim_access[i][TRANSLATION] << 
			" THreshold " << PREFETCH_THRESHOLD << " is prefech " << cache->is_prefetch << endl;
	}
}
int main(int argc, char** argv)
{
	
	// interrupt signal hanlder
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = signal_handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, NULL);

    cout << endl << "*** ChampSim Multicore Out-of-Order Simulator ***" << endl << endl;

    // initialize knobs
    uint8_t show_heartbeat = 1;

    uint32_t seed_number = 0;

    // check to see if knobs changed using getopt_long()
    int c, index;
    while (1) {
        static struct option long_options[] =
        {
            {"warmup_instructions", required_argument, 0, 'w'},
            {"simulation_instructions", required_argument, 0, 'i'},
            {"hide_heartbeat", no_argument, 0, 'h'},
            {"cloudsuite", no_argument, 0, 'c'},
            {"low_bandwidth",  no_argument, 0, 'b'},
            {"traces",  no_argument, 0, 't'},
	    // {0, 0, 0, 0},
      	    {"context_switch", required_argument, 0, 's'},
            {0,0,0,0}	    
        };

        int option_index = 0;

        c = getopt_long_only(argc, argv, "wihsb", long_options, &option_index);

        // no more option characters
        if (c == -1)
            break;

        int traces_encountered = 0;

        switch(c) {
            case 'w':
                warmup_instructions = atol(optarg);
                break;
            case 'i':
                simulation_instructions = atol(optarg);
                break;
            case 'h':
                show_heartbeat = 0;
                break;
            case 'c':
                knob_cloudsuite = 1;
                MAX_INSTR_DESTINATIONS = NUM_INSTR_DESTINATIONS_SPARC;
                break;
            case 'b':
                knob_low_bandwidth = 1;
                break;
            case 't':
                traces_encountered = 1;
                break;
	    case 's':

		cout << "knob updated\n";
		knob_context_switch = 1;
		break;
            default:
                abort();
        }

        if (traces_encountered == 1)
            break;
    }

    // consequences of knobs
    cout << "Warmup Instructions: " << warmup_instructions << endl;
    cout << "Simulation Instructions: " << simulation_instructions << endl;
    //cout << "Scramble Loads: " << (knob_scramble_loads ? "ture" : "false") << endl;
    cout << "Number of CPUs: " << NUM_CPUS << endl;
    cout << "LLC sets: " << LLC_SET << endl;
    cout << "LLC ways: " << LLC_WAY << endl;

    if (knob_low_bandwidth)
        DRAM_MTPS = DRAM_IO_FREQ/4;
    else
        DRAM_MTPS = DRAM_IO_FREQ;

    // DRAM access latency
    tRP  = (uint32_t)((1.0 * tRP_DRAM_NANOSECONDS  * CPU_FREQ) / 1000); 
    tRCD = (uint32_t)((1.0 * tRCD_DRAM_NANOSECONDS * CPU_FREQ) / 1000); 
    tCAS = (uint32_t)((1.0 * tCAS_DRAM_NANOSECONDS * CPU_FREQ) / 1000); 
    
    

    // default: 16 = (64 / 8) * (3200 / 1600)
    // it takes 16 CPU cycles to tranfser 64B cache block on a 8B (64-bit) bus 
    // note that dram burst length = BLOCK_SIZE/DRAM_CHANNEL_WIDTH
    DRAM_DBUS_RETURN_TIME = (BLOCK_SIZE / DRAM_CHANNEL_WIDTH) * (CPU_FREQ / DRAM_MTPS);

    printf("Off-chip DRAM Size: %u MB Channels: %u Width: %u-bit Data Rate: %u MT/s\n",
            DRAM_SIZE, DRAM_CHANNELS, 8*DRAM_CHANNEL_WIDTH, DRAM_MTPS);

    // end consequence of knobs

    // search through the argv for "-traces"
    int found_traces = 0;
    int count_traces = 0;
    
    for (int i=0; i<argc; i++) {	    
        if (found_traces) {
        	
            printf("CPU %d runs %s with asid %s\n", count_traces, argv[i],argv[i+1]);

            sprintf(ooo_cpu[count_traces].trace_string, "%s", argv[i]);
	    ooo_cpu[count_traces].core_asid = stoi(argv[i+1]);

            char *full_name = ooo_cpu[count_traces].trace_string,
                 *last_dot = strrchr(ooo_cpu[count_traces].trace_string, '.');
		cout << last_dot << endl;
			ifstream test_file(full_name);
			if(!test_file.good()){
				printf("TRACE FILE DOES NOT EXIST\n");
				assert(false);
			}
				

            if (full_name[last_dot - full_name + 1] == 'g') // gzip format
                sprintf(ooo_cpu[count_traces].gunzip_command, "gunzip -c %s", argv[i]);
            else if (full_name[last_dot - full_name + 1] == 'x') // xz
                sprintf(ooo_cpu[count_traces].gunzip_command, "xz -dc %s", argv[i]);
            else {
                cout << "ChampSim does not support traces other than gz or xz compression!" << endl; 
                assert(0);
            }

            char *pch[100];
            int count_str = 0;
            pch[0] = strtok (argv[i], " /,.-");
            while (pch[count_str] != NULL) {
                //printf ("%s %d\n", pch[count_str], count_str);
                count_str++;
                pch[count_str] = strtok (NULL, " /,.-");
            }

            //printf("max count_str: %d\n", count_str);
            //printf("application: %s\n", pch[count_str-3]);

            int j = 0;
            while (pch[count_str-3][j] != '\0') {
                seed_number += pch[count_str-3][j];
                //printf("%c %d %d\n", pch[count_str-3][j], j, seed_number);
                j++;
            }

            ooo_cpu[count_traces].trace_file = popen(ooo_cpu[count_traces].gunzip_command, "r");
            if (ooo_cpu[count_traces].trace_file == NULL) {
                printf("\n*** Trace file not found: %s ***\n\n", argv[i]);
                assert(0);
            }

           count_traces++;
	   i++;
	   cout <<" count_traces = " << count_traces << endl; 
	   if(count_traces == NUM_CPUS)
		   found_traces = 0;
            if (count_traces > NUM_CPUS) {
                printf("\n*** Too many traces for the configured number of cores ***\n\n");
                assert(0);
            }
        }
		
        else if(strcmp(argv[i],"-traces") == 0) {
            found_traces = 1;
        }
	/* Below code is required so that, further arguements are not treated as tracefile */
 	if(i+1<argc && (strcmp(argv[i+1], "-context_switch") == 0 || strcmp(argv[i+1], "-s") == 0))
	{
		found_traces = 0;
		knob_context_switch = 1;
		cout << "knob on" << endl;
	}
    }
    
    if(knob_context_switch == 1)
    {
	     
//	for(int i=0; i<count_traces; i++)
//	{
	sprintf(context_switch_string, "%s", argv[argc-1]);
	
	ifstream test_file(context_switch_string);
	if(!test_file.good()){
		printf("CONTEXT SWITCH FILE DOES NOT EXIST\n");
		assert(false);
	}
	else
		cout << "CONTEXT SWITCH FILE EXIST\n";
	        //ooo_cpu[i].context_switch_file = popen(ooo_cpu[i].context_switch_string, "r");
	context_switch_file = fopen(context_switch_string, "r");

	if (context_switch_file == NULL) {
        	printf("\n*** Context switch file not found: %s ***\n\n", argv[argc-1]);
                assert(0);
        }
	else
		cout << "Context_switch file found\n\n" ;
        index=0;
	while((fscanf(context_switch_file, "%ld %d %d", &cs_file[index].cycle, &cs_file[index].swap_cpu[0], &cs_file[index].swap_cpu[1]))!=EOF)
	{
		cs_file[index].index = index;
		cout << "print file:" << cs_file[index].index << " " << cs_file[index].cycle <<"- "<<  cs_file[index].swap_cpu[0] << cs_file[index].swap_cpu[1] <<  endl;
		index++;
	}
	
//	}
    }

    if (count_traces != NUM_CPUS) {
        printf("\n*** Not enough traces for the configured number of cores ***\n\n");
        assert(0);
    }
    // end trace file setup
    // TODO: can we initialize these variables from the class constructor?
    srand(seed_number);
    champsim_seed = seed_number;
    for (int i=0; i<NUM_CPUS; i++) {

	
        ooo_cpu[i].cpu = i; 
        ooo_cpu[i].warmup_instructions = warmup_instructions;
        ooo_cpu[i].simulation_instructions = simulation_instructions;
        ooo_cpu[i].begin_sim_cycle = 0; 
        ooo_cpu[i].begin_sim_instr = warmup_instructions;

        // ROB
        ooo_cpu[i].ROB.cpu = i;

        // BRANCH PREDICTOR
        ooo_cpu[i].initialize_branch_predictor();

        // TLBs
        ooo_cpu[i].ITLB.cpu = i;
        ooo_cpu[i].ITLB.cache_type = IS_ITLB;
        ooo_cpu[i].ITLB.fill_level = FILL_L1;
	if(i >= ACCELERATOR_START){
		ooo_cpu[i].ITLB.lower_level = &ooo_cpu[ACCELERATOR_START].STLB; 
		ooo_cpu[i].ITLB.extra_interface = &ooo_cpu[i].SCRATCHPAD;
	}
	else{
		ooo_cpu[i].ITLB.lower_level = &ooo_cpu[i].STLB; 
		ooo_cpu[i].ITLB.extra_interface = &ooo_cpu[i].L1I;
	}
        ooo_cpu[i].ITLB.itlb_prefetcher_initialize();

        ooo_cpu[i].DTLB.cpu = i;
        ooo_cpu[i].DTLB.cache_type = IS_DTLB;
        ooo_cpu[i].DTLB.MAX_READ = (2 > MAX_READ_PER_CYCLE) ? MAX_READ_PER_CYCLE : 2;
        ooo_cpu[i].DTLB.fill_level = FILL_L1;
	if(i >= ACCELERATOR_START)
		ooo_cpu[i].DTLB.extra_interface = &ooo_cpu[i].SCRATCHPAD;
	else
		ooo_cpu[i].DTLB.extra_interface = &ooo_cpu[i].L1D;
	if(i >= ACCELERATOR_START){
		ooo_cpu[i].DTLB.lower_level = &ooo_cpu[ACCELERATOR_START].STLB;
	}
	else
		ooo_cpu[i].DTLB.lower_level = &ooo_cpu[i].STLB;
        ooo_cpu[i].DTLB.dtlb_prefetcher_initialize();
	ooo_cpu[i].DTLB.dtlb_initialize_replacement();

	#ifdef PUSH_DTLB_PB
	ooo_cpu[i].DTLB_PB.cpu = i;
	ooo_cpu[i].DTLB_PB.cache_type = IS_DTLB_PB;
	ooo_cpu[i].DTLB_PB.fill_level = FILL_L1;
	#endif

        ooo_cpu[i].STLB.cpu = i;
        ooo_cpu[i].STLB.cache_type = IS_STLB;
        ooo_cpu[i].STLB.fill_level = FILL_L2;
	if(i >= ACCELERATOR_START){
		ooo_cpu[ACCELERATOR_START].STLB.upper_level_icache[i] = &ooo_cpu[i].ITLB;
		ooo_cpu[ACCELERATOR_START].STLB.upper_level_dcache[i] = &ooo_cpu[i].DTLB; 
	}
	else{
	      ooo_cpu[i].STLB.upper_level_icache[i] = &ooo_cpu[i].ITLB;
		ooo_cpu[i].STLB.upper_level_dcache[i] = &ooo_cpu[i].DTLB;
	}
        ooo_cpu[i].STLB.stlb_prefetcher_initialize();
#ifdef INS_PAGE_TABLE_WALKER
	ooo_cpu[i].STLB.lower_level = &ooo_cpu[i].PTW;
#endif

	ooo_cpu[i].PTW.cpu = i;
        ooo_cpu[i].PTW.cache_type = IS_PTW;
        ooo_cpu[i].PTW.upper_level_icache[i] = &ooo_cpu[i].STLB;
        ooo_cpu[i].PTW.upper_level_dcache[i] = &ooo_cpu[i].STLB;

	ooo_cpu[i].PTW.PSCL5.cache_type = IS_PSCL5;
	ooo_cpu[i].PTW.PSCL4.cache_type = IS_PSCL4;
	ooo_cpu[i].PTW.PSCL3.cache_type = IS_PSCL3;
	ooo_cpu[i].PTW.PSCL2.cache_type = IS_PSCL2;
	for(int j=0;j<NUM_CPUS;j++){
		ooo_cpu[ACCELERATOR_START].PTW.PSCL2A[j].cache_type = IS_PSCL2;
		ooo_cpu[ACCELERATOR_START].PTW.PSCL2A[j].NUM_WAY = PSCL2_WAY / NUM_CPUS;
	}
	ooo_cpu[i].PTW.PSCL2_VB.cache_type = IS_PSCL2_VB;
	ooo_cpu[i].PTW.PSCL2_PB.cache_type = IS_PSCL2_PB;
	ooo_cpu[i].PTW.PSCL2.PSCL2_initialize_replacement();
	ooo_cpu[i].PTW.PSCL2.PSCL2_prefetcher_initialize();

	if(i >= ACCELERATOR_START){
		ooo_cpu[i].SCRATCHPAD.cpu = i;
		ooo_cpu[i].SCRATCHPAD.cache_type = IS_SCRATCHPAD;
		ooo_cpu[i].SCRATCHPAD.MAX_READ = (FETCH_WIDTH > MAX_READ_PER_CYCLE) ? MAX_READ_PER_CYCLE : FETCH_WIDTH;
		ooo_cpu[i].SCRATCHPAD.fill_level = FILL_L1;
		ooo_cpu[i].SCRATCHPAD.lower_level = &uncore.DRAM; 
		uncore.DRAM.upper_level_icache[i] = &ooo_cpu[i].SCRATCHPAD;
		uncore.DRAM.upper_level_dcache[i] = &ooo_cpu[i].SCRATCHPAD;
	}
        // PRIVATE CACHE
	if(i < ACCELERATOR_START){
		ooo_cpu[i].L1I.cpu = i;
		ooo_cpu[i].L1I.cache_type = IS_L1I;
		ooo_cpu[i].L1I.MAX_READ = (FETCH_WIDTH > MAX_READ_PER_CYCLE) ? MAX_READ_PER_CYCLE : FETCH_WIDTH;
		ooo_cpu[i].L1I.fill_level = FILL_L1;
		ooo_cpu[i].L1I.lower_level = &ooo_cpu[i].L2C; 

		ooo_cpu[i].L1D.cpu = i;
		ooo_cpu[i].L1D.cache_type = IS_L1D;
		ooo_cpu[i].L1D.MAX_READ = (2 > MAX_READ_PER_CYCLE) ? MAX_READ_PER_CYCLE : 2;
		ooo_cpu[i].L1D.fill_level = FILL_L1;
		ooo_cpu[i].L1D.lower_level = &ooo_cpu[i].L2C; 
		ooo_cpu[i].L1D.l1d_prefetcher_initialize();

		ooo_cpu[i].L2C.cpu = i;
		ooo_cpu[i].L2C.cache_type = IS_L2C;
		ooo_cpu[i].L2C.fill_level = FILL_L2;
		ooo_cpu[i].L2C.upper_level_icache[i] = &ooo_cpu[i].L1I;
		ooo_cpu[i].L2C.upper_level_dcache[i] = &ooo_cpu[i].L1D;
		ooo_cpu[i].L2C.lower_level = &uncore.LLC;
		ooo_cpu[i].L2C.extra_interface = &ooo_cpu[i].PTW;
		ooo_cpu[i].L2C.l2c_prefetcher_initialize();

		// SHARED CACHE
		uncore.LLC.cache_type = IS_LLC;
		uncore.LLC.fill_level = FILL_LLC;
		uncore.LLC.MAX_READ = NUM_CPUS;
		uncore.LLC.upper_level_icache[i] = &ooo_cpu[i].L2C;
		uncore.LLC.upper_level_dcache[i] = &ooo_cpu[i].L2C;
		uncore.LLC.lower_level = &uncore.DRAM;
		uncore.DRAM.upper_level_icache[i] = &uncore.LLC;
		uncore.DRAM.upper_level_dcache[i] = &uncore.LLC;
	}

        // OFF-CHIP DRAM
        uncore.DRAM.fill_level = FILL_DRAM;
        for (uint32_t i=0; i<DRAM_CHANNELS; i++) {
            uncore.DRAM.RQ[i].is_RQ = 1;
            uncore.DRAM.WQ[i].is_WQ = 1;
        }

	if(i >= ACCELERATOR_START)
		uncore.DRAM.extra_interface = &ooo_cpu[ACCELERATOR_START].PTW;

        warmup_complete[i] = 0;
        //all_warmup_complete = NUM_CPUS;
        simulation_complete[i] = 0;
        current_core_cycle[i] = 0;
        stall_cycle[i] = 0;
        
        previous_ppage = 0;
        num_adjacent_page = 0;
        num_cl[i] = 0;
        allocated_pages = 0;
        num_page[i] = 0;
        minor_fault[i] = 0;
        major_fault[i] = 0;
    }

    uncore.LLC.llc_initialize_replacement();
    uncore.LLC.llc_prefetcher_initialize();

    // simulation entry point
    start_time = time(NULL);
    uint8_t run_simulation = 1;
    int cs_index = 0;
    
    bool prefetch_check[NUM_CPUS];
    for(int i=0;i<NUM_CPUS;i++)
	    prefetch_check[i] = true;
    while (run_simulation) {

        uint64_t elapsed_second = (uint64_t)(time(NULL) - start_time),
                 elapsed_minute = elapsed_second / 60,
                 elapsed_hour = elapsed_minute / 60;
        elapsed_minute -= elapsed_hour*60;
        elapsed_second -= (elapsed_hour*3600 + elapsed_minute*60);
	
	
	if(cs_index >= index)
		cs_index = -1;
	
        for (int i=0; i<NUM_CPUS; i++) {
            // proceed one cycle
            current_core_cycle[i]++;
/*	             cout << " ======================================================================================================================= "<< endl;
                cout << "Trying to process instr_id: " << ooo_cpu[i].instr_unique_id << " fetch_stall: " << +ooo_cpu[i].fetch_stall;
                cout << " stall_cycle: " << stall_cycle[i] << " current: " << current_core_cycle[i] <<" rob occupancy: " << ooo_cpu[i].ROB.occupancy << " cpu " << i <<  endl;*/

	   /* context-switch code
	    //cs_index - points the details of next context switch
	    //operating index - If the core is involved in context-switch, it stores the index of file which has details about the same 
	    */
		PPRINT(cout << " 1";)
	    if( cs_index!=-1 && ( current_core_cycle[i]==cs_file[cs_index].cycle) && (i==cs_file[cs_index].swap_cpu[0] || i==cs_file[cs_index].swap_cpu[1]))
	    {
		PPRINT(cout << " 2";)
		    //ooo_cpu[i].operating_index = cs_file[cs_index].index;
		    //ooo_cpu[i].context_switch = 1;
		    ooo_cpu[cs_file[cs_index].swap_cpu[0]].operating_index = cs_file[cs_index].index;
		    ooo_cpu[cs_file[cs_index].swap_cpu[1]].operating_index = cs_file[cs_index].index;
		    ooo_cpu[cs_file[cs_index].swap_cpu[0]].context_switch = 1;
		    ooo_cpu[cs_file[cs_index].swap_cpu[1]].context_switch = 1;
		    cout << "context_switch at cycle" << cs_file[cs_index].cycle << " ,i="<< i << cs_file[cs_index].swap_cpu[0] << " " << cs_file[cs_index].swap_cpu[1] << endl;
		    //read_context_switch_file = 1;	//@v - ready to read next cycle for context-switching
	    	    cs_index++;
	    	    if(cs_index >= index)
			cs_index = -1;
	
	    } 
	    
		PPRINT(cout << " 3";)

            // core might be stalled due to page fault or branch misprediction
            if (stall_cycle[i] <= current_core_cycle[i]) {
		
                // fetch unit
                if (ooo_cpu[i].ROB.occupancy < ooo_cpu[i].ROB.SIZE) {
                    // handle branch
                    if (ooo_cpu[i].fetch_stall == 0) 
                    {
                        ooo_cpu[i].handle_branch();
			/*@Vasudha - STOP simulation once trace file ends*/
			/*if(TRACE_ENDS_STOP == 1)
			{
				run_simulation = 0;
				simulation_complete[i] = 1;
			}*/
                    }
		 
                }

		PPRINT(cout << " 4";)
                // fetch
		PPRINT(if(i >= ACCELERATOR_START)cout << "fetch_instruction" << endl;)
                ooo_cpu[i].fetch_instruction();


                // schedule (including decode latency)
		PPRINT(if(i >= ACCELERATOR_START)cout << "schedule_instruction" << endl;)
                uint32_t schedule_index = ooo_cpu[i].ROB.next_schedule;
                if ((ooo_cpu[i].ROB.entry[schedule_index].scheduled == 0) && (ooo_cpu[i].ROB.entry[schedule_index].event_cycle <= current_core_cycle[i]))
                    ooo_cpu[i].schedule_instruction();

                // execute
		PPRINT(if(i >= ACCELERATOR_START)cout << "execute_instruction" << endl;)
                ooo_cpu[i].execute_instruction();

                // memory operation
		PPRINT(if(i >= ACCELERATOR_START)cout << "schedule_memory_instruction" << endl;)
                ooo_cpu[i].schedule_memory_instruction();
		PRINT(if(i >= ACCELERATOR_START)cout << "execute_memory_instruction" << endl;)
                ooo_cpu[i].execute_memory_instruction();

                // complete 
		PPRINT(cout << "update_rob" << endl;)
                ooo_cpu[i].update_rob();

		
                // retire
                if ((ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].executed == COMPLETED) && (ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].event_cycle <= current_core_cycle[i]))
                    ooo_cpu[i].retire_rob();
	/*	
	 	Context-switch code
		//If a core is involved in context-switch, check if ROB occupancy of this core as well as another core is 0. Also, if they have reached same current_core_cycle, swap the contexts of those two cores
	 	*/
		
		if(ooo_cpu[i].operating_index!=-1)
		{
			cout <<ooo_cpu[cs_file[ooo_cpu[i].operating_index].swap_cpu[0]].ROB.occupancy << " " <<  ooo_cpu[cs_file[ooo_cpu[i].operating_index].swap_cpu[1]].ROB.occupancy << " ";
			if(ooo_cpu[cs_file[ooo_cpu[i].operating_index].swap_cpu[0]].ROB.occupancy==0 && ooo_cpu[cs_file[ooo_cpu[i].operating_index].swap_cpu[1]].ROB.occupancy==0 && current_core_cycle[cs_file[ooo_cpu[i].operating_index].swap_cpu[0]] == current_core_cycle[cs_file[ooo_cpu[i].operating_index].swap_cpu[1]])
			{
				cout<<"entered with index "<< ooo_cpu[i].operating_index<< " with cpu = "<< i << endl;
				swap_context(cs_file[ooo_cpu[i].operating_index].swap_cpu[0], cs_file[ooo_cpu[i].operating_index].swap_cpu[1]);
				cout <<"IMPLEMENT CONTEXT SWITCH on cpu " << i <<" cycle of cpu0" << current_core_cycle[cs_file[ooo_cpu[i].operating_index].swap_cpu[0]] << " CPU1 " << current_core_cycle[cs_file[ooo_cpu[i].operating_index].swap_cpu[1]] << endl;
				ooo_cpu[cs_file[ooo_cpu[i].operating_index].swap_cpu[0]].context_switch=0;
				ooo_cpu[cs_file[ooo_cpu[i].operating_index].swap_cpu[1]].context_switch=0;
				cout << "Done! "<< cs_file[ooo_cpu[i].operating_index].swap_cpu[0] << cs_file[ooo_cpu[i].operating_index].swap_cpu[1] << endl;
				int index = ooo_cpu[i].operating_index;	//operating index will be modified
				ooo_cpu[cs_file[index].swap_cpu[0]].operating_index = -1;
				ooo_cpu[cs_file[index].swap_cpu[1]].operating_index = -1;

				cout << "operatingINDEX"<<ooo_cpu[cs_file[index].swap_cpu[0]].operating_index << " "<< ooo_cpu[cs_file[index].swap_cpu[1]].operating_index <<  endl;
			}
		}
            }

            // heartbeat information
	    
            if (show_heartbeat && (ooo_cpu[i].num_retired >= ooo_cpu[i].next_print_instruction)) {
                float cumulative_ipc;
                if (warmup_complete[i])
                    cumulative_ipc = (1.0*(ooo_cpu[i].num_retired - ooo_cpu[i].begin_sim_instr)) / (current_core_cycle[i] - ooo_cpu[i].begin_sim_cycle);
                else
                    cumulative_ipc = (1.0*ooo_cpu[i].num_retired) / current_core_cycle[i];
                float heartbeat_ipc = (1.0*ooo_cpu[i].num_retired - ooo_cpu[i].last_sim_instr) / (current_core_cycle[i] - ooo_cpu[i].last_sim_cycle);

                cout << "Heartbeat CPU " << i << " instructions: " << ooo_cpu[i].num_retired << " cycles: " << current_core_cycle[i];
                cout << " heartbeat IPC: " << heartbeat_ipc << " cumulative IPC: " << cumulative_ipc; 
                cout << " (Simulation time: " << elapsed_hour << " hr " << elapsed_minute << " min " << elapsed_second << " sec) " << endl;
                ooo_cpu[i].next_print_instruction += STAT_PRINTING_PERIOD;

                ooo_cpu[i].last_sim_instr = ooo_cpu[i].num_retired;
                ooo_cpu[i].last_sim_cycle = current_core_cycle[i];
            }

            // check for deadlock
            if (ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].ip && (ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].event_cycle + DEADLOCK_CYCLE) <= current_core_cycle[i])
                print_deadlock(i);

            // check for warmup
            // warmup complete
            if ((warmup_complete[i] == 0) && (ooo_cpu[i].num_retired > warmup_instructions)) {
                warmup_complete[i] = 1;
                all_warmup_complete++;
            }
            if (all_warmup_complete == NUM_CPUS) { // this part is called only once when all cores are warmed up
                all_warmup_complete++;
                finish_warmup();
            }

            /*
            if (all_warmup_complete == 0) { 
                all_warmup_complete = 1;
                finish_warmup();
            }
            if (ooo_cpu[1].num_retired > 0)
                warmup_complete[1] = 1;
            */
            
            // simulation complete
	 /*   if(prefetch_check[i]  && (ooo_cpu[i].num_retired >=  warmup_instructions + PREFETCH_CHECK_INSTR)){
		    prefetch_check[i] = false;
		    check_prefetch_status_ptl2();	
	    }*/
          
	    if(((all_warmup_complete > NUM_CPUS) && (simulation_complete[i] == 0) && (ooo_cpu[i].num_retired >= (ooo_cpu[i].begin_sim_instr + ooo_cpu[i].simulation_instructions)))) {
         //    if(simulation_complete[i] == 1){	/*@Vasudha STOP simulation once trace file ends*/
	    	simulation_complete[i] = 1;
		if(all_warmup_complete > NUM_CPUS)
                	ooo_cpu[i].finish_sim_instr = ooo_cpu[i].num_retired - ooo_cpu[i].begin_sim_instr;
		else	//In case trace ends before simulation starts
			ooo_cpu[i].finish_sim_instr = ooo_cpu[i].num_retired;
		ooo_cpu[i].finish_sim_cycle = current_core_cycle[i] - ooo_cpu[i].begin_sim_cycle;

              /*  cout << "Finished CPU " << i << " instructions: " << ooo_cpu[i].finish_sim_instr << " cycles: " << ooo_cpu[i].finish_sim_cycle;
                cout << " cumulative IPC: " << ((float) ooo_cpu[i].finish_sim_instr / ooo_cpu[i].finish_sim_cycle);
                cout << " (Simulation time: " << elapsed_hour << " hr " << elapsed_minute << " min " << elapsed_second << " sec) " << endl;
*/
		//@Vishal: record cpu stats
		ooo_cpu[i].roi_RAW_hits = ooo_cpu[i].sim_RAW_hits;
		ooo_cpu[i].roi_load_gen = ooo_cpu[i].sim_load_gen;
		ooo_cpu[i].roi_load_sent = ooo_cpu[i].sim_load_sent;
		ooo_cpu[i].roi_store_gen = ooo_cpu[i].sim_store_gen;
                ooo_cpu[i].roi_store_sent = ooo_cpu[i].sim_store_sent;

                record_roi_stats(i, &ooo_cpu[i].ITLB);
                record_roi_stats(i, &ooo_cpu[i].DTLB);
                record_roi_stats(i, &ooo_cpu[i].STLB);
		record_roi_stats(i, &ooo_cpu[i].L1D);
                record_roi_stats(i, &ooo_cpu[i].L1I);
                record_roi_stats(i, &ooo_cpu[i].L2C);
		record_roi_stats(i, &ooo_cpu[i].SCRATCHPAD);
                record_roi_stats(i, &uncore.LLC);
		#ifdef PUSH_DTLB_PB
		record_roi_stats(i, &ooo_cpu[i].DTLB_PB);
		#endif

		//MMU Caches
        	record_roi_stats(i, &ooo_cpu[i].PTW.PSCL5);
		record_roi_stats(i, &ooo_cpu[i].PTW.PSCL4);
		record_roi_stats(i, &ooo_cpu[i].PTW.PSCL3);
		record_roi_stats(i, &ooo_cpu[ACCELERATOR_START].PTW.PSCL2);
		record_roi_stats(i, &ooo_cpu[ACCELERATOR_START].PTW.PSCL2_PB);

                all_simulation_complete++;
            }

            if (all_simulation_complete == NUM_CPUS)
                run_simulation = 0;
       
       	}

        // TODO: should it be backward?
        uncore.LLC.operate();
        uncore.DRAM.operate();
    }

#ifndef CRC2_COMPILE
   // print_branch_stats();
#endif
    uint64_t elapsed_second = (uint64_t)(time(NULL) - start_time),
             elapsed_minute = elapsed_second / 60,
             elapsed_hour = elapsed_minute / 60;
    elapsed_minute -= elapsed_hour*60;
    elapsed_second -= (elapsed_hour*3600 + elapsed_minute*60);
 //   cout << "Average reuse distance " <<setw(10) << ooo_cpu[0].PTW.total_reuse_distance * 1.0 / ooo_cpu[0].DTLB.number_of_reuse_distance  <<setw(10) << ":"<< setw(10) << ooo_cpu[0].DTLB.total_reuse_distance <<setw(10) << " number of reuse distance " << setw(10) << ooo_cpu[0].DTLB.number_of_reuse_distance << endl;
	cout << endl;
/*    for (auto j = ooo_cpu[0].DTLB.dist_count.begin(); j !=  ooo_cpu[0].DTLB.dist_count.end(); j++)
            cout << j->first << "      " << j->second << endl;
*/
    cout << endl << "ChampSim completed all CPUs" << endl;
    cout << "Prefetcher status for accelerators " << endl;
    for(int i=0;i<NUM_CPUS;i++){
    	cout << "Accelerator " << i << "  status " << ooo_cpu[ACCELERATOR_START].PTW.PSCL2.is_prefetch[i]  << endl;
    }
    for(int i=0;i<NUM_CPUS;i++){
	    cout << "Eviction " << i ;
	    for(int j=0;j<NUM_CPUS;j++){
		cout << " " << ooo_cpu[ACCELERATOR_START].PTW.PSCL2_PB.eviction_matrix[i][j];
	    }
	    cout << endl;
    }
    cout << "\n Translation numbers ";
    for(int i=0;i<7;i++){
    	cout << ooo_cpu[ACCELERATOR_START].PTW.translation_number[i] << " ";
    }
    cout << endl;
    cout << endl;
/*    for(int i=0;i<NUM_CPUS;i++){
	    print_sim_stats(ACCELERATOR_START, &ooo_cpu[ACCELERATOR_START].PTW.PSCL2A[i]);
    }*/
    if (NUM_CPUS >= 1) {
        cout << endl << "Total Simulation Statistics (not including warmup)" << endl;
	cout << ooo_cpu[ACCELERATOR_START].PTW.rq_entries << " " << ooo_cpu[ACCELERATOR_START].PTW.rq_count_cycle << " ad " << ooo_cpu[ACCELERATOR_START].PTW.mshr_entries << " " << ooo_cpu[ACCELERATOR_START].PTW.mshr_count_cycle << " PREF MSHR " <<  ooo_cpu[ACCELERATOR_START].PTW.prefetch_mshr_entries << endl;
	    cout << " PTW average entries RQ : " << setw(10) << ooo_cpu[ACCELERATOR_START].PTW.rq_entries * 1.0 / ooo_cpu[ACCELERATOR_START].PTW.rq_count_cycle  << setw(10) << "  MSHR : " << setw(10) << ooo_cpu[ACCELERATOR_START].PTW.mshr_entries * 1.0 /  ooo_cpu[ACCELERATOR_START].PTW.mshr_count_cycle << setw(10) << " PREF MSHR: " << setw(10) << ooo_cpu[ACCELERATOR_START].PTW.prefetch_mshr_entries*1.0  / ooo_cpu[ACCELERATOR_START].PTW.prefetch_mshr_count_cycle << setw(10) << " PQ : " << setw(10) << ooo_cpu[ACCELERATOR_START].PTW.pq_entries * 1.0 / ooo_cpu[ACCELERATOR_START].PTW.pq_count_cycle << endl;
        for (uint32_t i=0; i<NUM_CPUS; i++) {
            cout << endl << "CPU " << i << " cumulative IPC: " << (float) (ooo_cpu[i].num_retired - ooo_cpu[i].begin_sim_instr) / (current_core_cycle[i] - ooo_cpu[i].begin_sim_cycle); 
            cout << " instructions: " << ooo_cpu[i].num_retired - ooo_cpu[i].begin_sim_instr << " cycles: " << current_core_cycle[i] - ooo_cpu[i].begin_sim_cycle << endl;
#ifndef CRC2_COMPILE
	    
	    print_sim_stats(i, &ooo_cpu[i].SCRATCHPAD);
	    print_sim_stats(i, &ooo_cpu[i].ITLB);
            print_sim_stats(i, &ooo_cpu[i].DTLB);
#ifdef PUSH_DTLB_PB
		    print_sim_stats(i, &ooo_cpu[i].DTLB_PB);
#endif
	    if(i ==  ACCELERATOR_START){
	    	cout << "\n\nSTLB merged: " << ooo_cpu[ACCELERATOR_START].STLB.stlb_merged ;
	    }
	    print_sim_stats(i, &ooo_cpu[i].STLB);
	    if(i< ACCELERATOR_START){
		    print_sim_stats(i, &ooo_cpu[i].L1D);
		    print_sim_stats(i, &ooo_cpu[i].L1I);
		    print_sim_stats(i, &ooo_cpu[i].L2C);
#ifdef PUSH_DTLB_PB
		    print_sim_stats(i, &ooo_cpu[i].DTLB_PB);
#endif
	    }
	    //MMU Caches
	    if(i<= ACCELERATOR_START){
		    cout << "\n\nPTW stats: " << endl << "PTW latency: "<< setw(10) <<ooo_cpu[i].PTW.PTW_latency << " PTW_rq:"<< setw(10) << ooo_cpu[i].PTW.PTW_rq << " AVG_latency: " << setw(10)<< (ooo_cpu[i].PTW.PTW_latency * 1.0 / ooo_cpu[i].PTW.PTW_rq) << endl;
		    print_sim_stats(i, &ooo_cpu[i].PTW.PSCL5);
		    print_sim_stats(i, &ooo_cpu[i].PTW.PSCL4);
		    print_sim_stats(i, &ooo_cpu[i].PTW.PSCL3);
	    }
	    print_sim_stats(i, &ooo_cpu[ACCELERATOR_START].PTW.PSCL2);
	    print_sim_stats(i, &ooo_cpu[ACCELERATOR_START].PTW.PSCL2_PB);

	    if(i< ACCELERATOR_START){
		    ooo_cpu[i].L1D.l1d_prefetcher_final_stats();
		    ooo_cpu[i].L2C.l2c_prefetcher_final_stats();
	    }
#endif
	    if(i < ACCELERATOR_START)
		    print_sim_stats(i, &uncore.LLC);
	    
	    //@Vishal: print stats
	    cout<<endl;
	    cout<<"RAW hits: "<<ooo_cpu[i].sim_RAW_hits<<endl;
	    cout<<"Loads Generated: "<<ooo_cpu[i].sim_load_gen<<endl;
	    cout<<"Loads sent to L1D: "<<ooo_cpu[i].sim_load_sent<<endl;
	    cout<<"Stores Generated: "<<ooo_cpu[i].sim_store_gen<<endl;
            cout<<"Stores sent to L1D: "<<ooo_cpu[i].sim_store_sent<<endl;
        }
        uncore.LLC.llc_prefetcher_final_stats();
    }

    cout << endl << "Region of Interest Statistics" << endl;
    for (uint32_t i=0; i<NUM_CPUS; i++) {
        cout << endl << "CPU " << i << " cumulative IPC: " << ((float) ooo_cpu[i].finish_sim_instr / ooo_cpu[i].finish_sim_cycle); 
        cout << " instructions: " << ooo_cpu[i].finish_sim_instr << " cycles: " << ooo_cpu[i].finish_sim_cycle << endl;
#ifndef CRC2_COMPILE
	if(i >= ACCELERATOR_START)
		print_roi_stats(i, &ooo_cpu[i].SCRATCHPAD);
	print_roi_stats(i, &ooo_cpu[i].ITLB);
        print_roi_stats(i, &ooo_cpu[i].DTLB);
	if(i <= ACCELERATOR_START)
		print_roi_stats(i, &ooo_cpu[i].STLB);
	if(i < ACCELERATOR_START){
		print_roi_stats(i, &ooo_cpu[i].L1D);
		print_roi_stats(i, &ooo_cpu[i].L1I);
		print_roi_stats(i, &ooo_cpu[i].L2C);
#ifdef PUSH_DTLB_PB
		print_roi_stats(i, &ooo_cpu[i].DTLB_PB);
#endif
	}
	//MMU Caches
	if(i <= ACCELERATOR_START){
		print_roi_stats(i, &ooo_cpu[i].PTW.PSCL5);
		print_roi_stats(i, &ooo_cpu[i].PTW.PSCL4);
		print_roi_stats(i, &ooo_cpu[i].PTW.PSCL3);
	}
	print_roi_stats(i, &ooo_cpu[ACCELERATOR_START].PTW.PSCL2);
	print_roi_stats(i, &ooo_cpu[ACCELERATOR_START].PTW.PSCL2_PB);
#endif
#ifdef PUSH_DTLB_PB
		    print_sim_stats(i, &ooo_cpu[i].DTLB_PB);
#endif
	if(i< ACCELERATOR_START)
		print_roi_stats(i, &uncore.LLC);
        
	//@Vishal: print stats
        cout<<endl;
	cout<<"RAW hits: "<<ooo_cpu[i].roi_RAW_hits<<endl;
	cout<<"Loads Generated: "<<ooo_cpu[i].roi_load_gen<<endl;
	cout<<"Loads sent to L1D: "<<ooo_cpu[i].roi_load_sent<<endl;
	cout<<"Stores Generated: "<<ooo_cpu[i].roi_store_gen<<endl;
        cout<<"Stores sent to L1D: "<<ooo_cpu[i].roi_store_sent<<endl;
	
	cout << "Major fault: " << major_fault[i] << " Minor fault: " << minor_fault[i] << endl;
    }
    

    for (uint32_t i=0; i<NUM_CPUS; i++) {
        ooo_cpu[i].L1D.l1d_prefetcher_final_stats();
        ooo_cpu[i].L2C.l2c_prefetcher_final_stats();
    }

    uncore.LLC.llc_prefetcher_final_stats();

#ifndef CRC2_COMPILE
    uncore.LLC.llc_replacement_final_stats();
    print_dram_stats();
#endif

    cout<<"DRAM PAGES: "<<DRAM_PAGES<<endl;
    cout<<"Allocated PAGES: "<<allocated_pages<<endl;

    return 0;
}

