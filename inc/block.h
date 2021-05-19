#ifndef BLOCK_H
#define BLOCK_H

#include "champsim.h"
#include "instruction.h"
#include "set.h"

// CACHE BLOCK
class BLOCK {
  public:
    uint8_t valid,
            prefetch,
            dirty,
            used;

    int delta,
        depth,
        signature,
        confidence,
	pref_class;

    uint64_t address,
             full_addr,
             tag,
             data,
             cpu,
             instr_id;

    // replacement state
    uint32_t lru;

    BLOCK() {
        valid = 0;
        prefetch = 0;
        dirty = 0;
        used = 0;

        delta = 0;
        depth = 0;
        signature = 0;
        confidence = 0;
	pref_class = 0;

        address = 0;
        full_addr = 0;
        tag = 0;
        data = 0;
        cpu = 0;
        instr_id = 0;

        lru = 0;
    };
};

// DRAM CACHE BLOCK
class DRAM_ARRAY {
  public:
    BLOCK **block;

    DRAM_ARRAY() {
        block = NULL;
    };
};

// message packet
class PACKET {
  public:
	//Nilesh : for tracking merge request
    bool stlb_merged,
	 scratchpad_mshr_merged,
	 is_translation;
    uint8_t instruction, 
            tlb_access,
            scheduled,
            translated,
            fetched,
            prefetched,
            drc_tag_read;

    int fill_level, 
        pf_origin_level,
        rob_signal, 
        rob_index, 
        producer,
        delta,
        depth,
        signature,
        confidence,
	ptw_mshr_index,
	late_pref;

    uint32_t pf_metadata;

    uint64_t tlb_start_cycle, stlb_start_cycle,ptw_start_cycle;
    uint8_t  is_producer, 
             //rob_index_depend_on_me[ROB_SIZE], 
             //lq_index_depend_on_me[ROB_SIZE], 
             //sq_index_depend_on_me[ROB_SIZE], 
             instr_merged,
             load_merged, 
             store_merged,
             returned,
             asid[2],
             type;

    fastset
             rob_index_depend_on_me, 
             lq_index_depend_on_me, 
             sq_index_depend_on_me;
    

    //@Vishal: VIPT added for handling transalation merging at L1D
    fastset l1_rq_index_depend_on_me,
	    l1_wq_index_depend_on_me,
        l1_pq_index_depend_on_me;
    uint8_t read_translation_merged,
	    write_translation_merged,
        prefetch_translation_merged;
    int l1_rq_index, l1_wq_index, l1_pq_index;
    uint64_t full_physical_address;
    bool send_both_tlb; // For STLB (if true, STLB should return translation to both DTLB and ITLB)
    bool send_both_cache;	// For L2C (if true, L2C should return data to both L1D and L1C)

    //@Vishal: PTW
    uint64_t full_virtual_address;
    uint8_t translation_level,
            init_translation_level;


    uint32_t cpu, data_index, lq_index, sq_index;

    uint64_t address, 
             full_addr, 
             instruction_pa,
             data_pa,
             data,
             instr_id,
             prefetch_id,//@v
             ip, 
             event_cycle,
             cycle_enqueued;
	//Nilesh: added for tracking merge packets
    queue<PACKET> stlb_depends_on_me, scratchpad_mshr_depends_on_me;

    PACKET() {
	//Nilesh: initializing merge status
	stlb_merged = false;
	scratchpad_mshr_merged = false;
	is_translation = false;
        instruction = 0;
        tlb_access = 0;
        scheduled = 0;
        translated = 0;
        fetched = 0;
        prefetched = 0;
        drc_tag_read = 0;
        send_both_tlb = 0;
	send_both_cache = 0;
	ptw_mshr_index = -1;
	late_pref = 0;

        returned = 0;
        asid[0] = UINT8_MAX;
        asid[1] = UINT8_MAX;
        type = 0;

        fill_level = -1; 
        rob_signal = -1;
        rob_index = -1;
        producer = -1;
        delta = 0;
        depth = 0;
        signature = 0;
        confidence = 0;

	tlb_start_cycle = 0;
	stlb_start_cycle = 0;
	ptw_start_cycle = 0;
#if 0
        for (uint32_t i=0; i<ROB_SIZE; i++) {
            rob_index_depend_on_me[i] = 0;
            lq_index_depend_on_me[i] = 0;
            sq_index_depend_on_me[i] = 0;
        }
#endif
        is_producer = 0;
        instr_merged = 0;
        load_merged = 0;
        store_merged = 0;

        cpu = NUM_CPUS;
        data_index = 0;
        lq_index = 0;
        sq_index = 0;

        address = 0;
        full_addr = 0;
        instruction_pa = 0;
        data = 0;
        instr_id = 0;
        prefetch_id = 0,//@v
        ip = 0;
        event_cycle = UINT64_MAX;
	cycle_enqueued = 0;


	read_translation_merged = 0;
	write_translation_merged = 0;
    prefetch_translation_merged = 0;
    	l1_rq_index = -1;
    	l1_wq_index = -1;
        l1_pq_index = -1;
    	full_physical_address = 0;
	send_both_tlb = false;
    };
};

// packet queue
class PACKET_QUEUE {
  public:
    string NAME;
    uint32_t SIZE;

    uint8_t  is_RQ, 
             is_WQ,
             write_mode;

    uint32_t cpu, 
             head, 
             tail, 
             occupancy, 
             num_returned, 
             next_fill_index, 
             next_schedule_index, 
             next_process_index;

    uint64_t next_fill_cycle, 
             next_schedule_cycle, 
             next_process_cycle,
             ACCESS,
             FORWARD,
             MERGED,
             TO_CACHE,
             ROW_BUFFER_HIT,
             ROW_BUFFER_MISS,
             FULL;

    PACKET *entry, processed_packet[2*MAX_READ_PER_CYCLE];

    // constructor
    PACKET_QUEUE(string v1, uint32_t v2) : NAME(v1), SIZE(v2) {
        is_RQ = 0;
        is_WQ = 0;
        write_mode = 0;

        cpu = 0; 
        head = 0;
        tail = 0;
        occupancy = 0;
        num_returned = 0;
        next_fill_index = 0;
        next_schedule_index = 0;
        next_process_index = 0;

        next_fill_cycle = UINT64_MAX;
        next_schedule_cycle = UINT64_MAX;
        next_process_cycle = UINT64_MAX;

        ACCESS = 0;
        FORWARD = 0;
        MERGED = 0;
        TO_CACHE = 0;
        ROW_BUFFER_HIT = 0;
        ROW_BUFFER_MISS = 0;
        FULL = 0;

        entry = new PACKET[SIZE]; 
    };

    PACKET_QUEUE() {
        is_RQ = 0;
        is_WQ = 0;

        cpu = 0; 
        head = 0;
        tail = 0;
        occupancy = 0;
        num_returned = 0;
        next_fill_index = 0;
        next_schedule_index = 0;
        next_process_index = 0;

        next_fill_cycle = UINT64_MAX;
        next_schedule_cycle = UINT64_MAX;
        next_process_cycle = UINT64_MAX;

        ACCESS = 0;
        FORWARD = 0;
        MERGED = 0;
        TO_CACHE = 0;
        ROW_BUFFER_HIT = 0;
        ROW_BUFFER_MISS = 0;
        FULL = 0;

        //entry = new PACKET[SIZE]; 
    };

/*    // destructor
    ~PACKET_QUEUE() {
        delete[] entry;
    };
*/
    // functions
    int check_queue(PACKET* packet);
    void add_queue(PACKET* packet),
	 queuePrint(),
         remove_queue(PACKET* packet);
};

// reorder buffer
class CORE_BUFFER {
  public:
    const string NAME;
    const uint32_t SIZE;
    uint32_t cpu, 
             head, 
             tail,
             occupancy,
             last_read, last_fetch, last_scheduled, 
             inorder_fetch[2],
             next_fetch[2],
             next_schedule;
    uint64_t event_cycle,
             fetch_event_cycle,
             schedule_event_cycle,
             execute_event_cycle,
             lsq_event_cycle,
             retire_event_cycle;

    ooo_model_instr *entry;

    // constructor
    CORE_BUFFER(string v1, uint32_t v2) : NAME(v1), SIZE(v2) {
        head = 0;
        tail = 0;
        occupancy = 0;

        last_read = SIZE-1;
        last_fetch = SIZE-1;
        last_scheduled = 0;

        inorder_fetch[0] = 0;
        inorder_fetch[1] = 0;
        next_fetch[0] = 0;
        next_fetch[1] = 0;
        next_schedule = 0;

        event_cycle = 0;
        fetch_event_cycle = UINT64_MAX;
        schedule_event_cycle = UINT64_MAX;
        execute_event_cycle = UINT64_MAX;
        lsq_event_cycle = UINT64_MAX;
        retire_event_cycle = UINT64_MAX;

        entry = new ooo_model_instr[SIZE];
    };

    // destructor
    ~CORE_BUFFER() {
        delete[] entry;
    };
};

// load/store queue 
class LSQ_ENTRY {
  public:
    uint64_t instr_id,
             producer_id,
             virtual_address,
             physical_address,
             ip,
             event_cycle;

    uint32_t rob_index, data_index, sq_index;

    uint8_t translated,
            fetched,
            asid[2];
// forwarding_depend_on_me[ROB_SIZE];
    fastset
		forwarding_depend_on_me;

    // constructor
    LSQ_ENTRY() {
        instr_id = 0;
        producer_id = UINT64_MAX;
        virtual_address = 0;
        physical_address = 0;
        ip = 0;
        event_cycle = 0;

        rob_index = 0;
        data_index = 0;
        sq_index = UINT32_MAX;

        translated = 0;
        fetched = 0;
        asid[0] = UINT8_MAX;
        asid[1] = UINT8_MAX;

#if 0
        for (uint32_t i=0; i<ROB_SIZE; i++)
            forwarding_depend_on_me[i] = 0;
#endif
    };
};

class LOAD_STORE_QUEUE {
  public:
    const string NAME;
    const uint32_t SIZE;
    uint32_t occupancy, head, tail;

    LSQ_ENTRY *entry;

    // constructor
    LOAD_STORE_QUEUE(string v1, uint32_t v2) : NAME(v1), SIZE(v2) {
        occupancy = 0;
        head = 0;
        tail = 0;

        entry = new LSQ_ENTRY[SIZE];
    };

    // destructor
    ~LOAD_STORE_QUEUE() {
        delete[] entry;
    };
};
#endif
