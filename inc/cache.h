#ifndef CACHE_H
#define CACHE_H

#include "memory_class.h"
#include<unordered_set>
#include<unordered_map>

// PAGE
extern uint32_t PAGE_TABLE_LATENCY, SWAP_LATENCY;

// CACHE TYPE
#define IS_ITLB 0
#define IS_DTLB 1
#define IS_STLB 2
#define IS_L1I  3
#define IS_L1D  4
#define IS_L2C  5
#define IS_LLC  6
#define IS_PTW  7
#define IS_SCRATCHPAD 15
#ifdef PUSH_DTLB_PB
#define IS_DTLB_PB 9
#endif

// MMU CACHE TYPE
#define IS_PSCL5  8
#define IS_PSCL4  9
#define IS_PSCL3  10
#define IS_PSCL2  11
#define IS_PSCL2_VB  12 //victim buffer
#define IS_PSCL2_PB  13

// QUEUE TYPE
#define IS_RQ 0
#define IS_WQ 1
#define IS_PQ 2

// INSTRUCTION TLB
#define ITLB_SET 8
#define ITLB_WAY 4
#define ITLB_RQ_SIZE 16
#define ITLB_WQ_SIZE 16
#define ITLB_PQ_SIZE 8
#define ITLB_MSHR_SIZE 8
#define ITLB_LATENCY 1

// DATA TLB
#define DTLB_SET 8
#define DTLB_WAY 4
#define DTLB_RQ_SIZE 16
#define DTLB_WQ_SIZE 16
#define DTLB_PQ_SIZE 8
#define DTLB_MSHR_SIZE 8
#define DTLB_LATENCY 1

//@Vasudha: Coding DTLB prefetch buffer
#ifdef PUSH_DTLB_PB
#define DTLB_PB_SET 1
#define DTLB_PB_WAY 32
#define DTLB_PB_RQ_SIZE 0
#define DTLB_PB_WQ_SIZE 0
#define DTLB_PB_PQ_SIZE 0
#define DTLB_PB_MSHR_SIZE 0
#endif

// SECOND LEVEL TLB
#define STLB_SET 64
#define STLB_WAY 16
#define STLB_RQ_SIZE 64
#define STLB_WQ_SIZE 64
#define STLB_PQ_SIZE 64
#define STLB_MSHR_SIZE 32
#define STLB_LATENCY 10

// SCRATCHPAD CACHE
#define SCRATCHPAD_SET 64
#define SCRATCHPAD_WAY 16 
#define SCRATCHPAD_RQ_SIZE 64
#define SCRATCHPAD_WQ_SIZE 64 
#define SCRATCHPAD_PQ_SIZE 16	
#define SCRATCHPAD_MSHR_SIZE 16 
#define SCRATCHPAD_LATENCY 6 

// L1 INSTRUCTION CACHE
#define L1I_SET 64
#define L1I_WAY 8
#define L1I_RQ_SIZE 64
#define L1I_WQ_SIZE 64 
#define L1I_PQ_SIZE 8
#define L1I_MSHR_SIZE 8
#define L1I_LATENCY 1


// L1 DATA CACHE
#define L1D_SET 64
#define L1D_WAY 12
#define L1D_RQ_SIZE 64
#define L1D_WQ_SIZE 64 
#define L1D_PQ_SIZE 16	//Neelu: Changed from 8 to 16.
#define L1D_MSHR_SIZE 16
#define L1D_LATENCY 5 //@Vishal: PIPT:5 VIPT:3 //@Nilesh: scratchpad 6

// L2 CACHE
#define L2C_SET 1024
#define L2C_WAY 8
#define L2C_RQ_SIZE 32
#define L2C_WQ_SIZE 32
#define L2C_PQ_SIZE 16
#define L2C_MSHR_SIZE 32
#define L2C_LATENCY 10  // 5 (L1I or L1D) + 10 = 15 cycles

// LAST LEVEL CACHE
#define LLC_SET NUM_CPUS*2048
#define LLC_WAY 16
#define LLC_RQ_SIZE NUM_CPUS*L2C_MSHR_SIZE //48
#define LLC_WQ_SIZE NUM_CPUS*L2C_MSHR_SIZE //48
#define LLC_PQ_SIZE NUM_CPUS*32
#define LLC_MSHR_SIZE NUM_CPUS*64
#define LLC_LATENCY 20  // 5 (L1I or L1D) + 10 + 20 = 35 cycles

class CACHE : public MEMORY {
	public:
    uint32_t cpu;
    bool is_prefetch[NUM_CPUS];
    const string NAME;
    const uint32_t NUM_SET,  NUM_LINE, WQ_SIZE, RQ_SIZE, PQ_SIZE, MSHR_SIZE;
    uint32_t LATENCY, NUM_WAY;
    unordered_set<uint64_t> coldMiss_DTLB, coldMiss_STLB;
    BLOCK **block;
    int fill_level;
    uint32_t MAX_READ, MAX_FILL;
    uint32_t reads_available_this_cycle, cold_miss_warmup;
    uint8_t cache_type;

    uint64_t eviction_matrix[NUM_CPUS][NUM_CPUS];

    // prefetch stats
    uint64_t pf_requested,
             pf_issued,
             pf_useful,
             pf_useless,
             pf_fill,
             pf_late,	// Number of On-demand translation requests hit in TLB MSHR with packet.type = PREFETCH.
             pf_lower_level, // Count prefetch request that enters MSHR as new miss(not merged)-used for TLBs as many prefetches issued gets merged  
	     //Addition by Neelu begin
//	     pq_full,
//           mshr_full,
             late_prefetch,
             prefetch_count,
	     unique_region_count,
	     region_conflicts;

    	     uint64_t pref_useful[NUM_CPUS][6],
             pref_filled[NUM_CPUS][6],
             pref_late[NUM_CPUS][6];
	     //Addition by Neelu end


    // queues
    PACKET_QUEUE WQ{NAME + "_WQ", WQ_SIZE}, // write queue
                 RQ{NAME + "_RQ", RQ_SIZE}, // read queue
                 PQ{NAME + "_PQ", PQ_SIZE}, // prefetch queue
                 MSHR{NAME + "_MSHR", MSHR_SIZE}, // MSHR
		 MSHR_TRANSLATION{NAME + "_MSHR_TRANSLATION", MSHR_SIZE}, //MSHR for scratchpad
		 MSHR_DATA{NAME + "_MSHR_DATA", MSHR_SIZE}, //MSHR for scratchpad
                 PROCESSED{NAME + "_PROCESSED", ROB_SIZE}; // processed queue

    uint64_t sim_access[NUM_CPUS][NUM_TYPES],
             sim_hit[NUM_CPUS][NUM_TYPES],
             sim_miss[NUM_CPUS][NUM_TYPES],
             roi_access[NUM_CPUS][NUM_TYPES],
             roi_hit[NUM_CPUS][NUM_TYPES],
             roi_miss[NUM_CPUS][NUM_TYPES],
	     stlb_merged;

    uint64_t total_miss_latency, sim_latency[NUM_CPUS], total_lat_req[NUM_CPUS], check_stlb_counter[NUM_CPUS];

    unordered_map<uint64_t,uint64_t> add_loc;
    unordered_set<uint64_t> coldMiss,total_address;
    map<uint64_t,uint64_t> loc_add, dist_count;
    
    // constructor
    CACHE(string v1, uint32_t v2, int v3, uint32_t v4, uint32_t v5, uint32_t v6, uint32_t v7, uint32_t v8) 
        : NAME(v1), NUM_SET(v2), NUM_WAY(v3), NUM_LINE(v4), WQ_SIZE(v5), RQ_SIZE(v6), PQ_SIZE(v7), MSHR_SIZE(v8) {

        LATENCY = 0;
	cold_miss_warmup = 0;

        // cache block
        block = new BLOCK* [NUM_SET];
        for (uint32_t i=0; i<NUM_SET; i++) {
            block[i] = new BLOCK[NUM_WAY]; 

            for (uint32_t j=0; j<NUM_WAY; j++) {
                block[i][j].lru = j;
            }
        }

        for (uint32_t i=0; i<NUM_CPUS; i++) {
            upper_level_icache[i] = NULL;
            upper_level_dcache[i] = NULL;
	    sim_latency[i] = 0;
	    total_lat_req[i] = 0;
	    check_stlb_counter[i] = 0;
	    is_prefetch[i] = false;

	    for (uint32_t j=0; j<NUM_CPUS; j++) 
		    eviction_matrix[i][j]=0;
            for (uint32_t j=0; j<NUM_TYPES; j++) {
                sim_access[i][j] = 0;
                sim_hit[i][j] = 0;
                sim_miss[i][j] = 0;
                roi_access[i][j] = 0;
                roi_hit[i][j] = 0;
                roi_miss[i][j] = 0;
            }
        }
//	is_prefetch[0] = true;

	total_miss_latency = 0;

        lower_level = NULL;
        extra_interface = NULL;
        fill_level = -1;
        MAX_READ = 1;
        MAX_FILL = 1;
	stlb_merged = 0;

        pf_requested = 0;
        pf_issued = 0;
        pf_useful = 0;
        pf_useless = 0;
        pf_fill = 0;
        pf_late = 0;
        pf_lower_level = 0;

	//Addition by Neelu begin
	
//	pq_full = 0;
//	mshr_full = 0;
	late_prefetch = 0;
	prefetch_count = 0;
	unique_region_count = 0;
	region_conflicts = 0;

	for(int i = 0; i < NUM_CPUS; i++)
	{
		for(int j = 0; j < 6; j++)
		{
			pref_useful[i][j] = 0;
			pref_filled[i][j] = 0;
			pref_late[i][j] = 0;
		}
	}

	//Addition by Neelu end
    };

    // destructor //TODO: @Vishal: double free error coming because of PTW
    /*~CACHE() {
        for (uint32_t i=0; i<NUM_SET; i++)
            delete[] block[i];
        delete[] block;
    };*/

    // functions
    int  add_rq(PACKET *packet),
         add_wq(PACKET *packet),
         add_pq(PACKET *packet);

    void return_data(PACKET *packet),
         operate(),
         increment_WQ_FULL(uint64_t address);

    uint32_t get_occupancy(uint8_t queue_type, uint64_t address),
             get_size(uint8_t queue_type, uint64_t address);

    int  check_hit(PACKET *packet),
         invalidate_entry(uint64_t inval_addr),
         check_mshr(PACKET *packet),
         prefetch_line(uint64_t ip, uint64_t base_addr, uint64_t pf_addr, int prefetch_fill_level, uint32_t prefetch_metadata)/* Neelu: commenting, uint64_t prefetch_id)*/,
         kpc_prefetch_line(uint64_t base_addr, uint64_t pf_addr, int prefetch_fill_level, int delta, int depth, int signature, int confidence, uint32_t prefetch_metadata),
         prefetch_translation(uint64_t ip, uint64_t pf_addr, int pf_fill_level, uint32_t prefetch_metadata, uint64_t prefetch_id, uint8_t instruction),
         prefetch_translation(uint64_t ip, uint64_t pf_addr, int pf_fill_level, uint32_t prefetch_metadata, uint64_t prefetch_id, uint8_t instruction,uint8_t packet_cpu),
         check_nonfifo_queue(PACKET_QUEUE *queue, PACKET *packet, bool packet_direction); //@Vishal: Updated from check_mshr

    void handle_fill(),
	 handle_scratchpad_mshr_translation(uint8_t mshr_type), //@Nilesh: for scratchpad
         handle_writeback(),
         handle_read(),
	 handle_read_scratchpad(), //@Nilesh: for scratchpad
         handle_prefetch(),
	 flush_TLB(),
         handle_processed();

    void add_nonfifo_queue(PACKET_QUEUE *queue, PACKET *packet), //@Vishal: Updated from add_mshr
	 handle_stlb_merged_translations(PACKET *packet,uint64_t data, bool flag), 	//@Nilesh: to handle stlb merged translations
         update_fill_cycle(),
	 update_fill_cycle_scratchpad(uint8_t mshr_type), //@Nilesh: for scratchpad
         llc_initialize_replacement(),
         dtlb_initialize_replacement(),
         stlb_initialize_replacement(),
         PSCL2_initialize_replacement(),
         update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit),
         llc_update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit),
         dtlb_update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit),
         stlb_update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit),
         PSCL2_update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit),
         lru_update(uint32_t set, uint32_t way),
         fill_cache(uint32_t set, uint32_t way, PACKET *packet),
         replacement_final_stats(),
         llc_replacement_final_stats(),
         dtlb_replacement_final_stats(),
         stlb_replacement_final_stats(),
         PSCL2_replacement_final_stats(),
         //prefetcher_initialize(),
         l1d_prefetcher_initialize(),
         l2c_prefetcher_initialize(),
         llc_prefetcher_initialize(),
         itlb_prefetcher_initialize(),
         dtlb_prefetcher_initialize(), 
         PSCL2_prefetcher_initialize(), 
         stlb_prefetcher_initialize(),
         prefetcher_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type),
         l1d_prefetcher_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type), //, uint64_t prefetch_id),
         itlb_prefetcher_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type, uint64_t prefetch_id, uint8_t instruction),
         dtlb_prefetcher_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type, uint64_t prefetch_id, uint8_t instruction,uint8_t core_asid),
         PSCL2_prefetcher_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type, uint64_t prefetch_id, uint8_t instruction,uint8_t core_asid, uint8_t cpu),
         stlb_prefetcher_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type, uint64_t prefetch_id, uint8_t instruction),
         prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr),
         l1d_prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in),
         itlb_prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in),
         dtlb_prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in),
         PSCL2_prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in),
         stlb_prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in),
         //prefetcher_final_stats(),
         l1d_prefetcher_final_stats(),
         l2c_prefetcher_final_stats(),
         llc_prefetcher_final_stats(),
         itlb_prefetcher_final_stats(),
         dtlb_prefetcher_final_stats(),
         PSCL2_prefetcher_final_stats(),
         stlb_prefetcher_final_stats();

	int add_mshr_data_queue_scratchpad(PACKET_QUEUE *queue, PACKET *packet);

    uint32_t l2c_prefetcher_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type, uint32_t metadata_in),	// uint64_t prefetch_id),
         llc_prefetcher_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type, uint32_t metadata_in),	//, uint64_t prefetch_id),
         l2c_prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in),
         llc_prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in);
    
    uint32_t get_set(uint64_t address),
             get_way(uint64_t address, uint32_t set),
             find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t ip, uint64_t full_addr, uint32_t type),
             llc_find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t ip, uint64_t full_addr, uint32_t type),
             dtlb_find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t ip, uint64_t full_addr, uint32_t type),
             stlb_find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t ip, uint64_t full_addr, uint32_t type),
             PSCL2_find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t ip, uint64_t full_addr, uint32_t type),
             lru_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t ip, uint64_t full_addr, uint32_t type);
};

#endif
