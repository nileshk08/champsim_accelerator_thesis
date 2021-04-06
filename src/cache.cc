#include "cache.h"
#include "set.h"
#include "ooo_cpu.h"
#include "dram_controller.h"

#include<vector>

uint64_t l2pf_access = 0;

#define PREF_CLASS_MASK 0xF00 //0x1E000	//Neelu: IPCP pref class
#define NUM_OF_STRIDE_BITS 8 //13	//Neelu: IPCP stride

//Neelu: For Ideal Spatial Prefetcher
#define IDEAL_SPATIAL_REGIONS 16
vector <uint64_t> regions_accessed, total_regions;                             //Neelu: regions accessed for ideal spatial prefetcher
#define LOG2_SPATIAL_REGION_SIZE 11                             //Neelu: For 2 KB region size
bool STLB_FLAG = false;
bool ITLB_FLAG = false;
bool printFlag = false;
void CACHE::update_fill_cycle_scratchpad(uint8_t mshr_type)
{
        // update next_fill_cycle
        uint64_t min_cycle = UINT64_MAX;

	PACKET_QUEUE *temp_mshr = mshr_type ? &MSHR_TRANSLATION : &MSHR_DATA;
        uint32_t min_index = MSHR.SIZE;
        for (uint32_t i=0; i<MSHR.SIZE; i++) {
		if(mshr_type){
			if ((!temp_mshr->entry[i].is_translation) && (temp_mshr->entry[i].event_cycle < min_cycle)) {
				min_cycle = temp_mshr->entry[i].event_cycle;
				min_index = i;
			}
		
		}
		else{
			if ((temp_mshr->entry[i].returned == COMPLETED) && (temp_mshr->entry[i].event_cycle < min_cycle)) {
				min_cycle = temp_mshr->entry[i].event_cycle;
				min_index = i;
			}
		}

        temp_mshr->next_fill_cycle = min_cycle;
        temp_mshr->next_fill_index = min_index;
	}
	SCRATCHPADPRINT(cout << "inside update fill cycle scratchpad "  << min_index  << " is translation " << (int) mshr_type  << " min cycle " << min_cycle << " min index  " << min_index << endl;
			if(mshr_type){
			 cout << "cache type "<< NAME << " MSHR_TRANSLATION occupancy " << MSHR_TRANSLATION.occupancy << endl;
                	MSHR_TRANSLATION.queuePrint();
			}
			else{
			 cout << "cache type "<< NAME << " MAHR_Data of update fill  occupancy " << MSHR_DATA.occupancy << endl;
                	MSHR_DATA.queuePrint();
			}
			if(min_index != MSHR_SIZE){
				cout << "instr_id " << temp_mshr->entry[min_index].instr_id << endl;
			})
}
void CACHE::handle_scratchpad_mshr_translation(uint8_t mshr_type)
{
	// handle fill
	PACKET_QUEUE *temp_mshr = mshr_type ? &MSHR_TRANSLATION : &MSHR_DATA;
	uint32_t fill_cpu = (temp_mshr->next_fill_index == MSHR_SIZE) ? NUM_CPUS : temp_mshr->entry[temp_mshr->next_fill_index].cpu;
	SCRATCHPADPRINT(cout << "inside handle mshr for scratchpad mshrty type " << (int)mshr_type << " fill cpu " << fill_cpu  << " current core cycle " << current_core_cycle[cpu] << " nextfill cycle " << temp_mshr->next_fill_cycle  << " nextfill index " << temp_mshr->next_fill_index << endl;)
	if (fill_cpu == NUM_CPUS)
		return;

	if (temp_mshr->next_fill_cycle <= current_core_cycle[fill_cpu]) {

#ifdef SANITY_CHECK
		if (temp_mshr->next_fill_index >= temp_mshr->SIZE)
			assert(0);
#endif

		uint32_t mshr_index = temp_mshr->next_fill_index;

		if(mshr_type){
			if ((MSHR_DATA.occupancy < MSHR_SIZE)) { // this is a new miss

				if(MSHR_TRANSLATION.entry[mshr_index].scratchpad_mshr_merged){// handling merged translations
					while(!MSHR_TRANSLATION.entry[mshr_index].scratchpad_mshr_depends_on_me.empty()){
						MSHR_TRANSLATION.entry[mshr_index].scratchpad_mshr_depends_on_me.front().address = MSHR_TRANSLATION.entry[mshr_index].scratchpad_mshr_depends_on_me.front().full_virtual_address >> LOG2_BLOCK_SIZE;
						MSHR_TRANSLATION.entry[mshr_index].scratchpad_mshr_depends_on_me.front().type = LOAD;
						MSHR_TRANSLATION.entry[mshr_index].scratchpad_mshr_depends_on_me.front().is_translation = 0;
						int index = add_mshr_data_queue_scratchpad(&MSHR_DATA, &MSHR_TRANSLATION.entry[mshr_index].scratchpad_mshr_depends_on_me.front());
						if(index == -2)
							return;
				//		lower_level->add_rq(&MSHR_TRANSLATION.entry[mshr_index].scratchpad_mshr_depends_on_me.front());
						MSHR_TRANSLATION.entry[mshr_index].scratchpad_mshr_depends_on_me.pop();
					}		
					MSHR_TRANSLATION.entry[mshr_index].scratchpad_mshr_merged = false;
				}
				// add it to mshr (read miss)
				MSHR_TRANSLATION.entry[mshr_index].address = MSHR_TRANSLATION.entry[mshr_index].full_virtual_address >> LOG2_BLOCK_SIZE;
				MSHR_TRANSLATION.entry[mshr_index].type= LOAD; 
				int index = add_mshr_data_queue_scratchpad(&MSHR_DATA, &MSHR_TRANSLATION.entry[mshr_index]);
				if(index == -2)
					return;

/*				if(cpu == 1 && MSHR_TRANSLATION.entry[mshr_index].instr_id == 106096){
				cout << "cache type "<< NAME << " MSHR_TRANSLATION occupancy  after removal" << MSHR_TRANSLATION.occupancy << endl;
				MSHR_TRANSLATION.queuePrint();
				MSHR_DATA.queuePrint();
				}
*/
				temp_mshr->remove_queue(&temp_mshr->entry[mshr_index]);
				temp_mshr->num_returned--;

				update_fill_cycle_scratchpad(mshr_type);


			}		
		}

		else{
			// find victim
			uint32_t set = get_set(temp_mshr->entry[mshr_index].address), way;
			way = find_victim(fill_cpu, temp_mshr->entry[mshr_index].instr_id, set, block[set], temp_mshr->entry[mshr_index].ip, temp_mshr->entry[mshr_index].full_addr, temp_mshr->entry[mshr_index].type);


			uint8_t  do_fill = 1;

			// is this dirty?
			if (block[set][way].dirty) {

				// check if the lower level WQ has enough room to keep this writeback request
				if (lower_level) {
					if (lower_level->get_occupancy(2, block[set][way].address) == lower_level->get_size(2, block[set][way].address)) {

						// lower level WQ is full, cannot replace this victim
						do_fill = 0;
						lower_level->increment_WQ_FULL(block[set][way].address);
						STALL[temp_mshr->entry[mshr_index].type]++;

						DP ( if (warmup_complete[fill_cpu] ) {
								cout << "[" << NAME << "] " << __func__ << "do_fill: " << +do_fill;
								cout << " lower level wq is full!" << " fill_addr: " << hex << temp_mshr->entry[mshr_index].address;
								cout << " victim_addr: " << block[set][way].tag << dec << endl; });
					}
					else {
						PACKET writeback_packet;

						writeback_packet.fill_level = fill_level << 1;
						writeback_packet.cpu = fill_cpu;
						writeback_packet.address = block[set][way].address;
						writeback_packet.full_addr = block[set][way].full_addr;
						writeback_packet.data = block[set][way].data;
						writeback_packet.instr_id = temp_mshr->entry[mshr_index].instr_id;
						writeback_packet.ip = 0; // writeback does not have ip
						writeback_packet.type = WRITEBACK;
						writeback_packet.event_cycle = current_core_cycle[fill_cpu];

						cout << " inside cache addwq  cpu " << fill_cpu << " address " << writeback_packet.address <<" instr_id " << writeback_packet.instr_id << " dirty " << (int)block[set][way].dirty << endl; 

						lower_level->add_wq(&writeback_packet);
					}
				}
			}
			if (do_fill){
				// update prefetcher

				// update replacement policy
				update_replacement_state(fill_cpu, set, way, temp_mshr->entry[mshr_index].full_addr, temp_mshr->entry[mshr_index].ip, block[set][way].full_addr, temp_mshr->entry[mshr_index].type, 0);

				// COLLECT STATS
				sim_miss[fill_cpu][temp_mshr->entry[mshr_index].type]++;
				sim_access[fill_cpu][temp_mshr->entry[mshr_index].type]++;

				//Neelu: IPCP stats collection


				fill_cache(set, way, &temp_mshr->entry[mshr_index]);



				// RFO marks cache line dirty
				if (temp_mshr->entry[mshr_index].type == RFO)
					block[set][way].dirty = 1;


				// update processed packets
				if (PROCESSED.occupancy < PROCESSED.SIZE)
					PROCESSED.add_queue(&temp_mshr->entry[mshr_index]);
				else
					cout << "PROCESSED NOT Handled for MSHR_DATA";
				temp_mshr->remove_queue(&temp_mshr->entry[mshr_index]);
				temp_mshr->num_returned--;

				update_fill_cycle_scratchpad(mshr_type);
			}

		}
	}
}

void CACHE::handle_fill()
{
	// handle fill
//	cout << "Inside handle Fill " << NAME << " type " << (int)cache_type << endl;
	PRINT77(
	 if(cpu >= ACCELERATOR_START){
		 cout << "inside handle_fill " << endl;
                cout << "cache type "<< NAME << " RQ occupancy " << RQ.occupancy << endl;
                RQ.queuePrint();
                cout << "cache type "<< NAME << " MSHR occupancy " << MSHR.occupancy << endl;
                MSHR.queuePrint();
        }
	)
	uint32_t fill_cpu = (MSHR.next_fill_index == MSHR_SIZE) ? NUM_CPUS : MSHR.entry[MSHR.next_fill_index].cpu;
/*	if(cache_type == IS_PSCL2){
 	cout << "handle fill cache " << NAME << " type " << (int)cache_type << endl;
	cout << "MSHR next fill cycle " << MSHR.next_fill_cycle  << " fill cpu " << fill_cpu << " current core cycle " << current_core_cycle[fill_cpu]<< endl;
	}
*/	if(ITLB_FLAG)
		cout << "MSHR next fill cycle " << MSHR.next_fill_cycle  << " fill cpu " << fill_cpu << endl;
	if (fill_cpu == NUM_CPUS)
		return;

	if (MSHR.next_fill_cycle <= current_core_cycle[fill_cpu]) {

#ifdef SANITY_CHECK
		if (MSHR.next_fill_index >= MSHR.SIZE)
			assert(0);
#endif

		uint32_t mshr_index = MSHR.next_fill_index;

		// find victim
		uint32_t set = get_set(MSHR.entry[mshr_index].address), way;
		if (cache_type == IS_LLC) {
			way = llc_find_victim(fill_cpu, MSHR.entry[mshr_index].instr_id, set, block[set], MSHR.entry[mshr_index].ip, MSHR.entry[mshr_index].full_addr, MSHR.entry[mshr_index].type);
		}
		else if(cache_type == IS_DTLB){
			way = dtlb_find_victim(fill_cpu, MSHR.entry[mshr_index].instr_id, set, block[set], MSHR.entry[mshr_index].ip, MSHR.entry[mshr_index].full_addr, MSHR.entry[mshr_index].type);
		}
		else if(cache_type == IS_STLB){
			way = stlb_find_victim(fill_cpu, MSHR.entry[mshr_index].instr_id, set, block[set], MSHR.entry[mshr_index].ip, MSHR.entry[mshr_index].full_addr, MSHR.entry[mshr_index].type);
		}
		else if(cache_type == IS_PSCL2){
			way = PSCL2_find_victim(fill_cpu, MSHR.entry[mshr_index].instr_id, set, block[set], MSHR.entry[mshr_index].ip, MSHR.entry[mshr_index].full_addr, MSHR.entry[mshr_index].type);
		}
		else
			way = find_victim(fill_cpu, MSHR.entry[mshr_index].instr_id, set, block[set], MSHR.entry[mshr_index].ip, MSHR.entry[mshr_index].full_addr, MSHR.entry[mshr_index].type);
		if(cache_type == IS_PSCL2_PB)
			if(block[set][way].valid){
				eviction_matrix[MSHR.entry[mshr_index].cpu][block[set][way].cpu]++;
			}
#ifdef LLC_BYPASS
		if ((cache_type == IS_LLC) && (way == LLC_WAY)) { // this is a bypass that does not fill the LLC

			// update replacement policy
			if (cache_type == IS_LLC) {
				llc_update_replacement_state(fill_cpu, set, way, MSHR.entry[mshr_index].full_addr, MSHR.entry[mshr_index].ip, 0, MSHR.entry[mshr_index].type, 0);

			}
			else if (cache_type == IS_DTLB) {
				dtlb_update_replacement_state(fill_cpu, set, way, MSHR.entry[mshr_index].full_addr, MSHR.entry[mshr_index].ip, 0, MSHR.entry[mshr_index].type, 0);

			}
			else if (cache_type == IS_STLB) {
				stlb_update_replacement_state(fill_cpu, set, way, MSHR.entry[mshr_index].full_addr, MSHR.entry[mshr_index].ip, 0, MSHR.entry[mshr_index].type, 0);

			}

			else if (cache_type == IS_PSCL2) {
				PSCL2_update_replacement_state(fill_cpu, set, way, MSHR.entry[mshr_index].full_addr, MSHR.entry[mshr_index].ip, 0, MSHR.entry[mshr_index].type, 0);

			}
			else
				update_replacement_state(fill_cpu, set, way, MSHR.entry[mshr_index].full_addr, MSHR.entry[mshr_index].ip, 0, MSHR.entry[mshr_index].type, 0);

			// COLLECT STATS
			if(cache_type == IS_STLB && fill_cpu >= ACCELERATOR_START){
				sim_miss[ACCELERATOR_START][MSHR.entry[mshr_index].type]++;
				sim_access[ACCELERATOR_START][MSHR.entry[mshr_index].type]++;
			}
			else{
				sim_miss[fill_cpu][MSHR.entry[mshr_index].type]++;
				sim_access[fill_cpu][MSHR.entry[mshr_index].type]++;
			}


			// check fill level
			if (MSHR.entry[mshr_index].fill_level < fill_level) {
				STLBPRINT(if(STLB_FLAG) { cout << "adding to upper from stlb 1 packet cpu " << MSHR.entry[mshr_index].cpu << endl;})

				if (MSHR.entry[mshr_index].instruction) 
					upper_level_icache[fill_cpu]->return_data(&MSHR.entry[mshr_index]);
				else // data
					upper_level_dcache[fill_cpu]->return_data(&MSHR.entry[mshr_index]);
			}


			if(warmup_complete[fill_cpu])
			{
				uint64_t current_miss_latency = (current_core_cycle[fill_cpu] - MSHR.entry[mshr_index].cycle_enqueued);	
				total_miss_latency += current_miss_latency;
			}

			MSHR.remove_queue(&MSHR.entry[mshr_index]);
			MSHR.num_returned--;

			update_fill_cycle();

			return; // return here, no need to process further in this function
		}
#endif

		uint8_t  do_fill = 1;

		// is this dirty?
		if (block[set][way].dirty) {

			// check if the lower level WQ has enough room to keep this writeback request
			if (lower_level) {
				if (lower_level->get_occupancy(2, block[set][way].address) == lower_level->get_size(2, block[set][way].address)) {

					// lower level WQ is full, cannot replace this victim
					do_fill = 0;
					lower_level->increment_WQ_FULL(block[set][way].address);
					STALL[MSHR.entry[mshr_index].type]++;

					DP ( if (warmup_complete[fill_cpu] ) {
							cout << "[" << NAME << "] " << __func__ << "do_fill: " << +do_fill;
							cout << " lower level wq is full!" << " fill_addr: " << hex << MSHR.entry[mshr_index].address;
							cout << " victim_addr: " << block[set][way].tag << dec << endl; });
				}
				else {
					PACKET writeback_packet;

					writeback_packet.fill_level = fill_level << 1;
					writeback_packet.cpu = fill_cpu;
					writeback_packet.address = block[set][way].address;
					writeback_packet.full_addr = block[set][way].full_addr;
					writeback_packet.data = block[set][way].data;
					writeback_packet.instr_id = MSHR.entry[mshr_index].instr_id;
					writeback_packet.ip = 0; // writeback does not have ip
					writeback_packet.type = WRITEBACK;
					writeback_packet.event_cycle = current_core_cycle[fill_cpu];
					cout << "1 inside cache addwq  cpu " << writeback_packet.cpu << " address " << writeback_packet.address <<" instr_id " << writeback_packet.instr_id << " dirty " << (int)block[set][way].dirty << " cache type " << (int) cache_type  << endl; 

					lower_level->add_wq(&writeback_packet);
				}
			}
#ifdef SANITY_CHECK
			else {
				// sanity check
				if (cache_type != IS_STLB)
					assert(0);
			}
#endif
		}
		if (do_fill){
			// update prefetcher
			if (cache_type == IS_L1D)
				l1d_prefetcher_cache_fill(MSHR.entry[mshr_index].full_addr, set, way, (MSHR.entry[mshr_index].type == PREFETCH) ? 1 : 0, block[set][way].address<<LOG2_BLOCK_SIZE,
						MSHR.entry[mshr_index].pf_metadata);
			if  (cache_type == IS_L2C && MSHR.entry[mshr_index].type != TRANSLATION)
				MSHR.entry[mshr_index].pf_metadata = l2c_prefetcher_cache_fill(MSHR.entry[mshr_index].address<<LOG2_BLOCK_SIZE, set, way, (MSHR.entry[mshr_index].type == PREFETCH) ? 1 : 0,
						block[set][way].address<<LOG2_BLOCK_SIZE, MSHR.entry[mshr_index].pf_metadata);
			if (cache_type == IS_LLC)
			{
				cpu = fill_cpu;
				MSHR.entry[mshr_index].pf_metadata = llc_prefetcher_cache_fill(MSHR.entry[mshr_index].address<<LOG2_BLOCK_SIZE, set, way, (MSHR.entry[mshr_index].type == PREFETCH) ? 1 : 0,
						block[set][way].address<<LOG2_BLOCK_SIZE, MSHR.entry[mshr_index].pf_metadata);
				cpu = 0;
			}
			if (cache_type == IS_PSCL2 && is_prefetch[MSHR.entry[mshr_index].cpu]){
				PSCL2_prefetcher_operate(MSHR.entry[mshr_index].address << LOG2_PAGE_SIZE, MSHR.entry[mshr_index].ip, 1, MSHR.entry[mshr_index].type, MSHR.entry[mshr_index].instr_id, MSHR.entry[mshr_index].instruction, MSHR.entry[mshr_index].asid[0], MSHR.entry[mshr_index].cpu);
			}
//						dtlb_prefetcher_operate(RQ.entry[index].address<<LOG2_PAGE_SIZE, RQ.entry[index].ip, 1, RQ.entry[index].type, RQ.entry[index].instr_id, RQ.entry[index].instruction,RQ.entry[index].asid[0] );

			// update replacement policy
			if (cache_type == IS_LLC) {
				llc_update_replacement_state(fill_cpu, set, way, MSHR.entry[mshr_index].full_addr, MSHR.entry[mshr_index].ip, block[set][way].full_addr, MSHR.entry[mshr_index].type, 0);
			}
			else if (cache_type == IS_DTLB) {
				dtlb_update_replacement_state(fill_cpu, set, way, MSHR.entry[mshr_index].full_addr, MSHR.entry[mshr_index].ip, 0, MSHR.entry[mshr_index].type, 0);
			
			}
			else if (cache_type == IS_STLB) {
				stlb_update_replacement_state(fill_cpu, set, way, MSHR.entry[mshr_index].full_addr, MSHR.entry[mshr_index].ip, 0, MSHR.entry[mshr_index].type, 0);
			
			}
			else if (cache_type == IS_PSCL2) {
			/*	if(block[set][way].valid){
					ooo_cpu[ACCELERATOR_START].PTW.PSCL2_VB.MSHR.entry[0].fill_level = 0;
					ooo_cpu[ACCELERATOR_START].PTW.PSCL2_VB.MSHR.entry[0].cpu = block[set][way].cpu;
					ooo_cpu[ACCELERATOR_START].PTW.PSCL2_VB.MSHR.entry[0].address =  block[set][way].address;
					ooo_cpu[ACCELERATOR_START].PTW.PSCL2_VB.MSHR.entry[0].full_addr = block[set][way].full_addr;
					ooo_cpu[ACCELERATOR_START].PTW.PSCL2_VB.MSHR.entry[0].data = block[set][way].data;
					ooo_cpu[ACCELERATOR_START].PTW.PSCL2_VB.MSHR.entry[0].instr_id = block[set][way].instr_id;
					ooo_cpu[ACCELERATOR_START].PTW.PSCL2_VB.MSHR.entry[0].ip = 0;
					ooo_cpu[ACCELERATOR_START].PTW.PSCL2_VB.MSHR.entry[0].type = TRANSLATION;
					ooo_cpu[ACCELERATOR_START].PTW.PSCL2_VB.MSHR.entry[0].event_cycle = current_core_cycle[cpu];
					ooo_cpu[ACCELERATOR_START].PTW.PSCL2_VB.MSHR.next_fill_index = 0;
					ooo_cpu[ACCELERATOR_START].PTW.PSCL2_VB.MSHR.next_fill_cycle = current_core_cycle[cpu];
					ooo_cpu[ACCELERATOR_START].PTW.PSCL2_VB.MSHR.occupancy = 1;
					ooo_cpu[ACCELERATOR_START].PTW.PSCL2_VB.handle_fill();

				}*/
				PSCL2_update_replacement_state(fill_cpu, set, way, MSHR.entry[mshr_index].full_addr, MSHR.entry[mshr_index].ip, 0, MSHR.entry[mshr_index].type, 0);

			}
			else
				update_replacement_state(fill_cpu, set, way, MSHR.entry[mshr_index].full_addr, MSHR.entry[mshr_index].ip, block[set][way].full_addr, MSHR.entry[mshr_index].type, 0);

			// COLLECT STATS
			if(cache_type == IS_STLB && fill_cpu >= ACCELERATOR_START){
				sim_miss[ACCELERATOR_START][MSHR.entry[mshr_index].type]++;
				sim_access[ACCELERATOR_START][MSHR.entry[mshr_index].type]++;
			}
			else{
				sim_miss[fill_cpu][MSHR.entry[mshr_index].type]++;
				sim_access[fill_cpu][MSHR.entry[mshr_index].type]++;
			}

			//Neelu: IPCP stats collection
			if(cache_type == IS_L1D)
			{
				if(MSHR.entry[mshr_index].late_pref == 1)
				{
					int temp_pf_class = (MSHR.entry[mshr_index].pf_metadata & PREF_CLASS_MASK) >> NUM_OF_STRIDE_BITS;
					if(temp_pf_class < 5)
					{
						pref_late[cpu][((MSHR.entry[mshr_index].pf_metadata & PREF_CLASS_MASK) >> NUM_OF_STRIDE_BITS)]++;
					}
				}
			}



			fill_cache(set, way, &MSHR.entry[mshr_index]);


#ifdef PUSH_DTLB_PB
			//@Vasudha: If STLB MSHR should fill prefetched translation to both STLB as well as DTLB Prefetch buffer
			if( cache_type == IS_STLB && MSHR.entry[mshr_index].type == PREFETCH && MSHR.entry[mshr_index].instruction == 0)
			{
				//Find in which way to fill the translation
				uint32_t victim_way;
				victim_way = ooo_cpu[fill_cpu].DTLB_PB.find_victim( fill_cpu, MSHR.entry[mshr_index].instr_id, 0, ooo_cpu[fill_cpu].DTLB_PB.block[0] , MSHR.entry[mshr_index].ip, MSHR.entry[mshr_index].full_addr, MSHR.entry[mshr_index].type);
				ooo_cpu[fill_cpu].DTLB_PB.update_replacement_state(fill_cpu, 0, way, MSHR.entry[mshr_index].full_addr, MSHR.entry[mshr_index].ip, ooo_cpu[fill_cpu].DTLB_PB.block[0][victim_way].full_addr, MSHR.entry[mshr_index].type, 0);
				ooo_cpu[fill_cpu].DTLB_PB.fill_cache( 0, victim_way, &MSHR.entry[mshr_index] );
			}
#endif

			// RFO marks cache line dirty
			if (cache_type == IS_L1D || cache_type == IS_SCRATCHPAD) {
				if (MSHR.entry[mshr_index].type == RFO)
					block[set][way].dirty = 1;
			}

			if(cache_type == IS_STLB && MSHR.entry[mshr_index].l1_pq_index != -1) //@Vishal: Prefetech request from L1D prefetcher
			{
				PACKET temp = MSHR.entry[mshr_index];
				temp.data_pa = block[set][way].data;
				assert(temp.l1_rq_index == -1 && temp.l1_wq_index == -1);
				temp.read_translation_merged = 0; //@Vishal: Remove this before adding to PQ
				temp.write_translation_merged = 0;
				if (PROCESSED.occupancy < PROCESSED.SIZE)
					PROCESSED.add_queue(&temp);
				else
					assert(0);			
			}
			else if(cache_type == IS_STLB && MSHR.entry[mshr_index].prefetch_translation_merged) //@Vishal: Prefetech request from L1D prefetcher
			{
				PACKET temp = MSHR.entry[mshr_index];
				temp.data_pa = block[set][way].data;
				temp.read_translation_merged = 0; //@Vishal: Remove this before adding to PQ
				temp.write_translation_merged = 0;
				if (PROCESSED.occupancy < PROCESSED.SIZE)
					PROCESSED.add_queue(&temp);
				else
					assert(0);
			}



			// check fill level
			if (MSHR.entry[mshr_index].fill_level < fill_level) {

				if(cache_type == IS_STLB)
				{
					MSHR.entry[mshr_index].prefetch_translation_merged = 0;
					if(MSHR.entry[mshr_index].stlb_merged)
						handle_stlb_merged_translations(&MSHR.entry[mshr_index],NULL, false);

					STLBPRINT(if(STLB_FLAG) { cout << "two  adding to   upper from stlb 1 packet cpu  " << MSHR.entry[mshr_index].cpu << endl;})
					if(MSHR.entry[mshr_index].send_both_tlb)
					{
						if(fill_cpu >= ACCELERATOR_START){
							upper_level_icache[MSHR.entry[mshr_index].cpu]->return_data(&MSHR.entry[mshr_index]);
							upper_level_dcache[MSHR.entry[mshr_index].cpu]->return_data(&MSHR.entry[mshr_index]);
						}
						else{
							upper_level_icache[fill_cpu]->return_data(&MSHR.entry[mshr_index]);
							upper_level_dcache[fill_cpu]->return_data(&MSHR.entry[mshr_index]);
						}

					}
					else if (MSHR.entry[mshr_index].instruction)
						upper_level_icache[MSHR.entry[mshr_index].cpu]->return_data(&MSHR.entry[mshr_index]);
					else // data
						upper_level_dcache[MSHR.entry[mshr_index].cpu]->return_data(&MSHR.entry[mshr_index]);
				}
				else if(cache_type == IS_L2C && MSHR.entry[mshr_index].type == TRANSLATION)
				{
					extra_interface->return_data(&MSHR.entry[mshr_index]);
				}
				else if(cache_type == IS_L2C)
				{
					if(MSHR.entry[mshr_index].send_both_cache)
					{
						// if(knob_cloudsuite)
						//	MSHR.entry[mshr_index].address = (( MSHR.entry[mshr_index].ip >> LOG2_PAGE_SIZE) << 9) | ( 256 + MSHR.entry[mshr_index].asid[0]);
						upper_level_icache[fill_cpu]->return_data(&MSHR.entry[mshr_index]);
						//if(knob_cloudsuite)
						//	 MSHR.entry[mshr_index].address = (( MSHR.entry[mshr_index].full_virtual_address >> LOG2_PAGE_SIZE) << 9) |  MSHR.entry[mshr_index].asid[1];
						upper_level_dcache[fill_cpu]->return_data(&MSHR.entry[mshr_index]);
					}
					else if (MSHR.entry[mshr_index].instruction)
						upper_level_icache[fill_cpu]->return_data(&MSHR.entry[mshr_index]);
					else	//data
						upper_level_dcache[fill_cpu]->return_data(&MSHR.entry[mshr_index]);
				}	
				else
				{
					if (MSHR.entry[mshr_index].instruction) 
						upper_level_icache[fill_cpu]->return_data(&MSHR.entry[mshr_index]);
					else // data
						upper_level_dcache[fill_cpu]->return_data(&MSHR.entry[mshr_index]);
				}
			}
			//@v if send_both_tlb == 1 in STLB, response should return to both ITLB and DTLB


			// update processed packets
			if(ITLB_FLAG)
				cout << "type " << (int)MSHR.entry[mshr_index].type << endl;
			if ((cache_type == IS_ITLB) && (MSHR.entry[mshr_index].type != PREFETCH)) { //@v Responses to prefetch requests should not go to PROCESSED queue 
				auto difference = current_core_cycle[cpu] - MSHR.entry[mshr_index].tlb_start_cycle;
				if (warmup_complete[cpu] && MSHR.entry[mshr_index].tlb_start_cycle > 0 &&  difference > 0){
				QPRINT(	if( current_core_cycle[cpu] - MSHR.entry[mshr_index].tlb_start_cycle <= 0)
						cout <<  current_core_cycle[cpu] << " current start " <<  MSHR.entry[mshr_index].tlb_start_cycle << endl;)
					total_lat_req[cpu]++;
					sim_latency[cpu] +=   current_core_cycle[cpu] - MSHR.entry[mshr_index].tlb_start_cycle ;
				}
				MSHR.entry[mshr_index].instruction_pa = block[set][way].data;
				MSHR.entry[mshr_index].is_translation = 1;
				if (cpu < ACCELERATOR_START && PROCESSED.occupancy < PROCESSED.SIZE)
					PROCESSED.add_queue(&MSHR.entry[mshr_index]);
				else if(cpu >= ACCELERATOR_START){
					SCRATCHPADPRINT(cout << " 1 return from ITLB addr " << MSHR.entry[mshr_index].address << endl;)
					extra_interface->return_data(&MSHR.entry[mshr_index]);
				
				}
			}
			else if ((cache_type == IS_DTLB) && (MSHR.entry[mshr_index].type != PREFETCH)) {
				auto difference = current_core_cycle[cpu] - MSHR.entry[mshr_index].tlb_start_cycle;
				if (warmup_complete[cpu] && MSHR.entry[mshr_index].tlb_start_cycle > 0 &&  difference > 0){
				QPRINT(	if( current_core_cycle[cpu] - MSHR.entry[mshr_index].tlb_start_cycle <= 0)
						cout <<  current_core_cycle[cpu] << " current start " <<  MSHR.entry[mshr_index].tlb_start_cycle << endl;)
					total_lat_req[cpu]++;
					sim_latency[cpu] +=   current_core_cycle[cpu] - MSHR.entry[mshr_index].tlb_start_cycle ;
				}

				MSHR.entry[mshr_index].data_pa = block[set][way].data;
				if (cpu < ACCELERATOR_START && PROCESSED.occupancy < PROCESSED.SIZE)
					PROCESSED.add_queue(&MSHR.entry[mshr_index]);
				else if(cpu >= ACCELERATOR_START)
					extra_interface->return_data(&MSHR.entry[mshr_index]);
			}
			else if (cache_type == IS_L1I) {
				if (PROCESSED.occupancy < PROCESSED.SIZE)
					PROCESSED.add_queue(&MSHR.entry[mshr_index]);
			}
			else if ((cache_type == IS_L1D) && (MSHR.entry[mshr_index].type != PREFETCH)) {
				if (PROCESSED.occupancy < PROCESSED.SIZE)
					PROCESSED.add_queue(&MSHR.entry[mshr_index]);
			}

			if(warmup_complete[fill_cpu])
			{
				uint64_t current_miss_latency = (current_core_cycle[fill_cpu] - MSHR.entry[mshr_index].cycle_enqueued);
				total_miss_latency += current_miss_latency;
			}

			MSHR.remove_queue(&MSHR.entry[mshr_index]);
			MSHR.num_returned--;

			update_fill_cycle();
			//update_fill_cycle_scratchpad(1);
		}
	}
}

ostream& operator<<(ostream& os, const PACKET &packet)
{
	return os << " cpu: " << packet.cpu << " instr_id: " << packet.instr_id << " Translated: " << +packet.translated << " address: " << hex << packet.address << " full_addr: " << packet.full_addr << dec << " event_cycle: " << packet.event_cycle <<  " current_core_cycle: " <<  current_core_cycle[packet.cpu] << endl;
};



void CACHE::handle_writeback()
{

	if(WQ.occupancy == 0)
		return;

	multimap<uint64_t, uint32_t> writes_ready; //{cycle_time,index}

assert(cache_type != IS_L1I || cache_type != IS_STLB); //@Vishal: TLB should not generate write packets

if(cache_type == IS_L1D) //Get completed index in WQ, as it is non-fifo
{
	for (uint32_t wq_index=0; wq_index < WQ.SIZE; wq_index++)
		if(WQ.entry[wq_index].translated == COMPLETED && (WQ.entry[wq_index].event_cycle <= current_core_cycle[cpu])) 
			writes_ready.insert({WQ.entry[wq_index].event_cycle, wq_index});
}
auto writes_ready_it = writes_ready.begin();

if(cache_type == IS_L1D && writes_ready.size() == 0)
	return;

if(cache_type == IS_L1D)
	WQ.head = writes_ready_it->second; //@Vishal: L1 WQ is non fifo, so head variable is set to next index to be read	

	// handle write
	uint32_t writeback_cpu = WQ.entry[WQ.head].cpu;
if (writeback_cpu == NUM_CPUS)
	return;


	// handle the oldest entry
	if ((WQ.entry[WQ.head].event_cycle <= current_core_cycle[writeback_cpu]) && (WQ.occupancy > 0)) {
		int index = WQ.head;

		// access cache
		uint32_t set = get_set(WQ.entry[index].address);
		int way = check_hit(&WQ.entry[index]);


		//Addition by Neelu: For Ideal Spatial Prefetcher
		/*	if(cache_type == IS_L1D)
			{
			int found_region = 0;
			assert(WQ.entry[index].address != 0);
			for(int temp_index = 0; temp_index < regions_accessed.size(); temp_index++)
			{
			if(regions_accessed[temp_index] == (WQ.entry[index].address >> LOG2_SPATIAL_REGION_SIZE))
			{
			found_region = 1;
			way = 0;
			break; 
			}
			}
			if(found_region == 0)
			{       
			total_regions.push_back(WQ.entry[index].address >> LOG2_SPATIAL_REGION_SIZE);
			unique_region_count = total_regions.size();
			regions_accessed.push_back(WQ.entry[index].address >> LOG2_SPATIAL_REGION_SIZE);
			if(regions_accessed.size() > IDEAL_SPATIAL_REGIONS)
			{
			regions_accessed.erase(regions_accessed.begin());       
			region_conflicts++;
			}

		//assert(way < 0);			

		}
		}*/

		//        if(cache_type==IS_L1D)
		//		way = 0 ;	//Perfect L1D

		if (way >= 0) { // writeback hit (or RFO hit for L1D)

			if (cache_type == IS_LLC) {
				llc_update_replacement_state(writeback_cpu, set, way, block[set][way].full_addr, WQ.entry[index].ip, 0, WQ.entry[index].type, 1);
			}
			else if (cache_type == IS_DTLB) {
				dtlb_update_replacement_state(writeback_cpu, set, way, block[set][way].full_addr, WQ.entry[index].ip, 0, WQ.entry[index].type, 1);

			}
			else if (cache_type == IS_STLB) {
				stlb_update_replacement_state(writeback_cpu, set, way, block[set][way].full_addr, WQ.entry[index].ip, 0, WQ.entry[index].type, 1);

			}
			else if (cache_type == IS_PSCL2) {
				PSCL2_update_replacement_state(writeback_cpu, set, way, block[set][way].full_addr, WQ.entry[index].ip, 0, WQ.entry[index].type, 1);

			}
			else
				update_replacement_state(writeback_cpu, set, way, block[set][way].full_addr, WQ.entry[index].ip, 0, WQ.entry[index].type, 1);

			// COLLECT STATS
			if(cache_type == IS_STLB && cpu >= ACCELERATOR_START){
				sim_hit[ACCELERATOR_START][WQ.entry[index].type]++;
				sim_access[ACCELERATOR_START][WQ.entry[index].type]++;
			}
			else{
				sim_hit[writeback_cpu][WQ.entry[index].type]++;
				sim_access[writeback_cpu][WQ.entry[index].type]++;
			}

			// mark dirty
			block[set][way].dirty = 1;

			if (cache_type == IS_ITLB)
				WQ.entry[index].instruction_pa = block[set][way].data;
			else if (cache_type == IS_DTLB)
				WQ.entry[index].data_pa = block[set][way].data;

			WQ.entry[index].data = block[set][way].data;

			// check fill level
			if (WQ.entry[index].fill_level < fill_level) {
				if (WQ.entry[index].instruction) 
					upper_level_icache[writeback_cpu]->return_data(&WQ.entry[index]);
				else // data
					upper_level_dcache[writeback_cpu]->return_data(&WQ.entry[index]);
			}

			HIT[WQ.entry[index].type]++;
			ACCESS[WQ.entry[index].type]++;

			// remove this entry from WQ
			WQ.remove_queue(&WQ.entry[index]);
		}
		else { // writeback miss (or RFO miss for L1D)

			DP ( if (warmup_complete[writeback_cpu]) {
					cout << "[" << NAME << "] " << __func__ << " type: " << +WQ.entry[index].type << " miss";
					cout << " instr_id: " << WQ.entry[index].instr_id << " address: " << hex << WQ.entry[index].address;
					cout << " full_addr: " << WQ.entry[index].full_addr << dec;
					cout << " cycle: " << WQ.entry[index].event_cycle << endl; });

			if (cache_type == IS_L1D) { // RFO miss

				// check mshr
				uint8_t miss_handled = 1;
				int mshr_index = check_nonfifo_queue(&MSHR, &WQ.entry[index],false); //@Vishal: Updated from check_mshr

				if ((mshr_index == -1) && (MSHR.occupancy < MSHR_SIZE)) { // this is a new miss

					assert(WQ.entry[index].full_physical_address != 0);
					PACKET new_packet = WQ.entry[index];
					//@Vishal: Send physical address to lower level and track physical address in MSHR  
					new_packet.address = WQ.entry[index].full_physical_address >> LOG2_BLOCK_SIZE;
					new_packet.full_addr = WQ.entry[index].full_physical_address; 


					// add it to mshr (RFO miss)
					add_nonfifo_queue(&MSHR, &new_packet); //@Vishal: Updated from add_mshr

					// add it to the next level's read queue
					//if (lower_level) // L1D always has a lower level cache
					lower_level->add_rq(&new_packet);
				}
				else {
					if ((mshr_index == -1) && (MSHR.occupancy == MSHR_SIZE)) { // not enough MSHR resource

						// cannot handle miss request until one of MSHRs is available
						miss_handled = 0;
						STALL[WQ.entry[index].type]++;
					}
					else if (mshr_index != -1) { // already in-flight miss

						// update fill_level
						if (WQ.entry[index].fill_level < MSHR.entry[mshr_index].fill_level)
							MSHR.entry[mshr_index].fill_level = WQ.entry[index].fill_level;

						// update request
						if (MSHR.entry[mshr_index].type == PREFETCH) {
							uint8_t  prior_returned = MSHR.entry[mshr_index].returned;
							uint64_t prior_event_cycle = MSHR.entry[mshr_index].event_cycle;

							uint64_t prior_address = MSHR.entry[mshr_index].address;
							uint64_t prior_full_addr = MSHR.entry[mshr_index].full_addr;
							uint64_t prior_full_physical_address = MSHR.entry[mshr_index].full_physical_address;


							MSHR.entry[mshr_index] = WQ.entry[index];

							assert(cache_type != IS_L1I);//@Vishal: L1I cache does not generate prefetch packet.

							//@Vishal: L1 RQ has virtual address, but miss needs to track physical address, so prior addresses are kept
							if(cache_type == IS_L1D)
							{
								MSHR.entry[mshr_index].address = prior_address;
								MSHR.entry[mshr_index].full_addr = prior_full_addr;
								MSHR.entry[mshr_index].full_physical_address = prior_full_physical_address;
							}

							// in case request is already returned, we should keep event_cycle and retunred variables
							MSHR.entry[mshr_index].returned = prior_returned;
							MSHR.entry[mshr_index].event_cycle = prior_event_cycle;
						}

						MSHR_MERGED[WQ.entry[index].type]++;

						DP ( if (warmup_complete[writeback_cpu]) {
								cout << "[" << NAME << "] " << __func__ << " mshr merged";
								cout << " instr_id: " << WQ.entry[index].instr_id << " prior_id: " << MSHR.entry[mshr_index].instr_id; 
								cout << " address: " << hex << WQ.entry[index].address;
								cout << " full_addr: " << WQ.entry[index].full_addr << dec;
								cout << " cycle: " << WQ.entry[index].event_cycle << endl; });
					}
					else { // WE SHOULD NOT REACH HERE
						cerr << "[" << NAME << "] MSHR errors" << endl;
						assert(0);
					}
				}

				if (miss_handled) {

					MISS[WQ.entry[index].type]++;
					ACCESS[WQ.entry[index].type]++;

					// remove this entry from WQ
					WQ.remove_queue(&WQ.entry[index]);
				}

			}
			else {
				// find victim
				uint32_t set = get_set(WQ.entry[index].address), way;
				if (cache_type == IS_LLC) {
					way = llc_find_victim(writeback_cpu, WQ.entry[index].instr_id, set, block[set], WQ.entry[index].ip, WQ.entry[index].full_addr, WQ.entry[index].type);
				}
				else if (cache_type == IS_DTLB) {
					way = dtlb_find_victim(writeback_cpu, WQ.entry[index].instr_id, set, block[set], WQ.entry[index].ip, WQ.entry[index].full_addr, WQ.entry[index].type);
				}
				else if (cache_type == IS_STLB) {
					way = stlb_find_victim(writeback_cpu, WQ.entry[index].instr_id, set, block[set], WQ.entry[index].ip, WQ.entry[index].full_addr, WQ.entry[index].type);
				}
				else
					way = find_victim(writeback_cpu, WQ.entry[index].instr_id, set, block[set], WQ.entry[index].ip, WQ.entry[index].full_addr, WQ.entry[index].type);

#ifdef LLC_BYPASS
				if ((cache_type == IS_LLC) && (way == LLC_WAY)) {
					cerr << "LLC bypassing for writebacks is not allowed!" << endl;
					assert(0);
				}
#endif

				uint8_t  do_fill = 1;

				// is this dirty?
				if (block[set][way].dirty) {

					// check if the lower level WQ has enough room to keep this writeback request
					if (lower_level) { 
						if (lower_level->get_occupancy(2, block[set][way].address) == lower_level->get_size(2, block[set][way].address)) {

							// lower level WQ is full, cannot replace this victim
							do_fill = 0;
							lower_level->increment_WQ_FULL(block[set][way].address);
							STALL[WQ.entry[index].type]++;

							DP ( if (warmup_complete[writeback_cpu] ) {
									cout << "[" << NAME << "] " << __func__ << "do_fill: " << +do_fill;
									cout << " lower level wq is full!" << " fill_addr: " << hex << WQ.entry[index].address;
									cout << " victim_addr: " << block[set][way].tag << dec << endl; });
						}
						else { 
							PACKET writeback_packet;

							writeback_packet.fill_level = fill_level << 1;
							writeback_packet.cpu = writeback_cpu;
							writeback_packet.address = block[set][way].address;
							writeback_packet.full_addr = block[set][way].full_addr;
							writeback_packet.data = block[set][way].data;
							writeback_packet.instr_id = WQ.entry[index].instr_id;
							writeback_packet.ip = 0;
							writeback_packet.type = WRITEBACK;
							writeback_packet.event_cycle = current_core_cycle[writeback_cpu];

					cout << "2 inside cache addwq  cpu " << writeback_packet.cpu << " address " << writeback_packet.address <<" instr_id " << writeback_packet.instr_id << " dirty " << (int)block[set][way].dirty << " cache type " << (int) cache_type  << endl; 
						lower_level->add_wq(&writeback_packet);
						}
					}
#ifdef SANITY_CHECK
					else {
						// sanity check
						if (cache_type != IS_STLB)
							assert(0);
					}
#endif
				}

				if (do_fill) {
					// update prefetcher
					if (cache_type == IS_L1D)
						l1d_prefetcher_cache_fill(WQ.entry[index].full_addr, set, way, 0, block[set][way].address<<LOG2_BLOCK_SIZE, WQ.entry[index].pf_metadata);
					else if (cache_type == IS_L2C && WQ.entry[index].type != TRANSLATION)
						WQ.entry[index].pf_metadata = l2c_prefetcher_cache_fill(WQ.entry[index].address<<LOG2_BLOCK_SIZE, set, way, 0,
								block[set][way].address<<LOG2_BLOCK_SIZE, WQ.entry[index].pf_metadata);
					if (cache_type == IS_LLC)
					{
						cpu = writeback_cpu;
						WQ.entry[index].pf_metadata =llc_prefetcher_cache_fill(WQ.entry[index].address<<LOG2_BLOCK_SIZE, set, way, 0,
								block[set][way].address<<LOG2_BLOCK_SIZE, WQ.entry[index].pf_metadata);
						cpu = 0;
					}

					// update replacement policy
					if (cache_type == IS_LLC) {
						llc_update_replacement_state(writeback_cpu, set, way, WQ.entry[index].full_addr, WQ.entry[index].ip, block[set][way].full_addr, WQ.entry[index].type, 0);
					}
					else if (cache_type == IS_DTLB) {
						dtlb_update_replacement_state(writeback_cpu, set, way, WQ.entry[index].full_addr, WQ.entry[index].ip, block[set][way].full_addr, WQ.entry[index].type, 0);

					}
					else if (cache_type == IS_STLB) {
						stlb_update_replacement_state(writeback_cpu, set, way, WQ.entry[index].full_addr, WQ.entry[index].ip, block[set][way].full_addr, WQ.entry[index].type, 0);

					}
					else if (cache_type == IS_PSCL2) {
						PSCL2_update_replacement_state(writeback_cpu, set, way, WQ.entry[index].full_addr, WQ.entry[index].ip, block[set][way].full_addr, WQ.entry[index].type, 0);

					}
					else
						update_replacement_state(writeback_cpu, set, way, WQ.entry[index].full_addr, WQ.entry[index].ip, block[set][way].full_addr, WQ.entry[index].type, 0);

					// COLLECT STATS
					sim_miss[writeback_cpu][WQ.entry[index].type]++;
					sim_access[writeback_cpu][WQ.entry[index].type]++;

					fill_cache(set, way, &WQ.entry[index]);

					// mark dirty
					block[set][way].dirty = 1; 

					// check fill level
					if (WQ.entry[index].fill_level < fill_level) {
						if (WQ.entry[index].instruction) 
							upper_level_icache[writeback_cpu]->return_data(&WQ.entry[index]);
						else // data
							upper_level_dcache[writeback_cpu]->return_data(&WQ.entry[index]);
					}

					MISS[WQ.entry[index].type]++;
					ACCESS[WQ.entry[index].type]++;

					// remove this entry from WQ
					WQ.remove_queue(&WQ.entry[index]);
				}
			}
		}
	}
}

//@Vishal: Translation coming from TLB to L1 cache
void CACHE::handle_processed()
{
	if(cpu >= ACCELERATOR_START)	//Nilesh: Method will not be used if its an accelerator
		assert(0);
	assert(cache_type == IS_L1I || cache_type == IS_L1D);

	CACHE &tlb = cache_type == IS_L1I ? ooo_cpu[cpu].ITLB : ooo_cpu[cpu].DTLB;

	//@Vishal: one translation is processed per cycle
	if(tlb.PROCESSED.occupancy != 0)
	{
		if((tlb.PROCESSED.entry[tlb.PROCESSED.head].event_cycle <= current_core_cycle[cpu]))
		{
			int index = tlb.PROCESSED.head;

			if(tlb.PROCESSED.entry[index].l1_rq_index != -1)
			{
				assert(tlb.PROCESSED.entry[index].l1_wq_index == -1 && tlb.PROCESSED.entry[index].l1_pq_index == -1); //Entry can't have write and prefetch index

				int rq_index = tlb.PROCESSED.entry[index].l1_rq_index;

				DP ( if (warmup_complete[cpu] ) {	
						cout << "["<<NAME << "_handle_processed] instr_id: " << RQ.entry[rq_index].instr_id << endl; 
						});

				RQ.entry[rq_index].translated = COMPLETED;

				if(tlb.cache_type == IS_ITLB)
					RQ.entry[rq_index].full_physical_address = (tlb.PROCESSED.entry[index].instruction_pa << LOG2_PAGE_SIZE) | (RQ.entry[rq_index].full_addr & ((1 << LOG2_PAGE_SIZE) - 1));
				else
					RQ.entry[rq_index].full_physical_address = (tlb.PROCESSED.entry[index].data_pa << LOG2_PAGE_SIZE) | (RQ.entry[rq_index].full_addr & ((1 << LOG2_PAGE_SIZE) - 1));

				RQ.entry[rq_index].event_cycle = current_core_cycle[cpu];
			}
			else if(tlb.PROCESSED.entry[index].l1_wq_index != -1)
			{
				assert(tlb.PROCESSED.entry[index].l1_rq_index == -1 && tlb.PROCESSED.entry[index].l1_pq_index == -1); //Entry can't have read and prefetch index

				int wq_index = tlb.PROCESSED.entry[index].l1_wq_index;

				WQ.entry[wq_index].translated = COMPLETED;

				if(tlb.cache_type == IS_ITLB)
					WQ.entry[wq_index].full_physical_address = (tlb.PROCESSED.entry[index].instruction_pa << LOG2_PAGE_SIZE) | (WQ.entry[wq_index].full_addr & ((1 << LOG2_PAGE_SIZE) - 1));
				else
					WQ.entry[wq_index].full_physical_address = (tlb.PROCESSED.entry[index].data_pa << LOG2_PAGE_SIZE) | (WQ.entry[wq_index].full_addr & ((1 << LOG2_PAGE_SIZE) - 1));


				WQ.entry[wq_index].event_cycle = current_core_cycle[cpu];

				DP ( if (warmup_complete[cpu] ) {
						cout << "["<<NAME << "_handle_processed] instr_id: " << WQ.entry[wq_index].instr_id;
						});

			}
			else
			{
				assert(0); //Either read queue or write queue index should be present
			}


			assert(tlb.PROCESSED.entry[index].prefetch_translation_merged == false);

			if(tlb.PROCESSED.entry[index].read_translation_merged)
			{
				ITERATE_SET(other_rq_index,tlb.PROCESSED.entry[index].l1_rq_index_depend_on_me, RQ.SIZE)
				{
					if(RQ.entry[other_rq_index].translated == INFLIGHT)
					{
						RQ.entry[other_rq_index].translated = COMPLETED;

						if(tlb.cache_type == IS_ITLB)
							RQ.entry[other_rq_index].full_physical_address = (tlb.PROCESSED.entry[index].instruction_pa << LOG2_PAGE_SIZE) | (RQ.entry[other_rq_index].full_addr & ((1 << LOG2_PAGE_SIZE) - 1));
						else
							RQ.entry[other_rq_index].full_physical_address = (tlb.PROCESSED.entry[index].data_pa << LOG2_PAGE_SIZE) | (RQ.entry[other_rq_index].full_addr & ((1 << LOG2_PAGE_SIZE) - 1));

						RQ.entry[other_rq_index].event_cycle = current_core_cycle[cpu];

						DP ( if (warmup_complete[cpu] ) {
								cout << "["<<NAME << "_handle_processed] instr_id: " << RQ.entry[other_rq_index].instr_id << endl;
								});
					}
				}
			}


			if(tlb.PROCESSED.entry[index].write_translation_merged)
			{
				ITERATE_SET(other_wq_index,tlb.PROCESSED.entry[index].l1_wq_index_depend_on_me, WQ.SIZE) 
				{
					WQ.entry[other_wq_index].translated = COMPLETED;

					if(tlb.cache_type == IS_ITLB)
						WQ.entry[other_wq_index].full_physical_address = (tlb.PROCESSED.entry[index].instruction_pa << LOG2_PAGE_SIZE) | (WQ.entry[other_wq_index].full_addr & ((1 << LOG2_PAGE_SIZE) - 1));
					else
						WQ.entry[other_wq_index].full_physical_address = (tlb.PROCESSED.entry[index].data_pa << LOG2_PAGE_SIZE) | (WQ.entry[other_wq_index].full_addr & ((1 << LOG2_PAGE_SIZE) - 1));

					WQ.entry[other_wq_index].event_cycle = current_core_cycle[cpu];

					DP ( if (warmup_complete[cpu] ) {
							cout << "["<<NAME << "_handle_processed] instr_id: " << WQ.entry[other_wq_index].instr_id << endl;
							});
				}
			}

			tlb.PROCESSED.remove_queue(&tlb.PROCESSED.entry[index]);
		}
	}

	//@Vishal: Check for Prefetch translations from STLB processed queue
	if(cache_type == IS_L1D)
	{
		CACHE &tlb = ooo_cpu[cpu].STLB;

		//@Vishal: one translation is processed per cycle
		if(tlb.PROCESSED.occupancy != 0)
		{
			if((tlb.PROCESSED.entry[tlb.PROCESSED.head].event_cycle <= current_core_cycle[cpu]))
			{
				int index = tlb.PROCESSED.head;

				if(tlb.PROCESSED.entry[index].l1_pq_index != -1)
				{
					int pq_index = tlb.PROCESSED.entry[index].l1_pq_index;

					PQ.entry[pq_index].translated = COMPLETED;

					//@Vishal: L1D prefetch is sending request, so translation should be in data_pa.
					assert(tlb.PROCESSED.entry[index].data_pa != 0 && tlb.PROCESSED.entry[index].instruction_pa == 0);

					PQ.entry[pq_index].full_physical_address = (tlb.PROCESSED.entry[index].data_pa << LOG2_PAGE_SIZE) | (PQ.entry[pq_index].full_addr & ((1 << LOG2_PAGE_SIZE) - 1));

					PQ.entry[pq_index].event_cycle = current_core_cycle[cpu];

					assert((tlb.PROCESSED.entry[index].read_translation_merged == false) && (tlb.PROCESSED.entry[index].write_translation_merged == false));
				}

				if(tlb.PROCESSED.entry[index].prefetch_translation_merged)
				{
					ITERATE_SET(other_pq_index, tlb.PROCESSED.entry[index].l1_pq_index_depend_on_me, PQ.SIZE) 
					{
						if(PQ.entry[other_pq_index].translated == INFLIGHT)
						{
							PQ.entry[other_pq_index].translated = COMPLETED;
							PQ.entry[other_pq_index].full_physical_address = (tlb.PROCESSED.entry[index].data_pa << LOG2_PAGE_SIZE) | (PQ.entry[other_pq_index].full_addr & ((1 << LOG2_PAGE_SIZE) - 1));
							PQ.entry[other_pq_index].event_cycle = current_core_cycle[cpu];
						}
					}
				}
				else
					assert(0);
				tlb.PROCESSED.remove_queue(&tlb.PROCESSED.entry[index]);
			}
			else
				return;
		}
	}
}

void CACHE::handle_read_scratchpad()
{

	if(RQ.occupancy == 0)
		return;


	// handle read
	static int counter;
	for (uint32_t i=0; i<MAX_READ; i++) {

		uint32_t read_cpu = RQ.entry[RQ.head].cpu;
		if (read_cpu == NUM_CPUS)
			return;


		// handle the oldest entry
		if ((RQ.entry[RQ.head].event_cycle <= current_core_cycle[read_cpu]) && (RQ.occupancy > 0)) {
			int index = RQ.head;

			// access cache
			uint32_t set = get_set(RQ.entry[index].address);
			int way = check_hit(&RQ.entry[index]);

			if (way >= 0) { // read hit

				if (PROCESSED.occupancy < PROCESSED.SIZE)
					PROCESSED.add_queue(&RQ.entry[index]);
				else
					assert(0);

				// update replacement policy
				update_replacement_state(read_cpu, set, way, block[set][way].full_addr, RQ.entry[index].ip, 0, RQ.entry[index].type, 1);

				// COLLECT STATS
				sim_hit[read_cpu][RQ.entry[index].type]++;
				sim_access[read_cpu][RQ.entry[index].type]++;



				block[set][way].used = 1;

				HIT[RQ.entry[index].type]++;
				ACCESS[RQ.entry[index].type]++;

				// remove this entry from RQ
				RQ.remove_queue(&RQ.entry[index]);
				reads_available_this_cycle--;
			}
			else { // read miss


				// check mshr
				uint8_t miss_handled = 1;
				int mshr_index = check_nonfifo_queue(&MSHR_DATA, &RQ.entry[index],false); 
				PACKET_QUEUE *temp_mshr =    &MSHR_DATA;
				if(mshr_index != -1){

					if (RQ.entry[index].type == RFO) {

						if (RQ.entry[index].tlb_access) {
							uint32_t sq_index = RQ.entry[index].sq_index;
							temp_mshr->entry[mshr_index].store_merged = 1;
							temp_mshr->entry[mshr_index].sq_index_depend_on_me.insert (sq_index);
							temp_mshr->entry[mshr_index].sq_index_depend_on_me.join (RQ.entry[index].sq_index_depend_on_me, SQ_SIZE);
						}

						if (RQ.entry[index].load_merged) {
							//uint32_t lq_index = RQ.entry[index].lq_index; 
							temp_mshr->entry[mshr_index].load_merged = 1;
							//MSHR.entry[mshr_index].lq_index_depend_on_me[lq_index] = 1;
							temp_mshr->entry[mshr_index].lq_index_depend_on_me.join (RQ.entry[index].lq_index_depend_on_me, LQ_SIZE);
						}

					}
					else {
						if (RQ.entry[index].instruction) {
							uint32_t rob_index = RQ.entry[index].rob_index;
							DP (if (warmup_complete[MSHR.entry[mshr_index].cpu] ) {
									//if(cache_type==IS_ITLB || cache_type==IS_DTLB || cache_type==IS_STLB)
									cout << "read request merged with MSHR entry -"<< MSHR.entry[mshr_index].type << endl; });
							temp_mshr->entry[mshr_index].instr_merged = 1;
							temp_mshr->entry[mshr_index].rob_index_depend_on_me.insert (rob_index);

							DP (if (warmup_complete[MSHR.entry[mshr_index].cpu] ) {
									cout << "[INSTR_MERGED] " << __func__ << " cpu: " << MSHR.entry[mshr_index].cpu << " instr_id: " << MSHR.entry[mshr_index].instr_id;
									cout << " merged rob_index: " << rob_index << " instr_id: " << RQ.entry[index].instr_id << endl; });

							if (RQ.entry[index].instr_merged) {
								temp_mshr->entry[mshr_index].rob_index_depend_on_me.join (RQ.entry[index].rob_index_depend_on_me, ROB_SIZE);
								DP (if (warmup_complete[MSHR.entry[mshr_index].cpu] ) {
										cout << "[INSTR_MERGED] " << __func__ << " cpu: " << MSHR.entry[mshr_index].cpu << " instr_id: " << MSHR.entry[mshr_index].instr_id;
										cout << " merged rob_index: " << i << " instr_id: N/A" << endl; });
							}
						}
						else 
						{
							uint32_t lq_index = RQ.entry[index].lq_index;
							temp_mshr->entry[mshr_index].load_merged = 1;
							temp_mshr->entry[mshr_index].lq_index_depend_on_me.insert (lq_index);

							DP (if (warmup_complete[read_cpu] ) {
									cout << "[DATA_MERGED] " << __func__ << " cpu: " << read_cpu << " instr_id: " << RQ.entry[index].instr_id;
									cout << " merged rob_index: " << RQ.entry[index].rob_index << " instr_id: " << RQ.entry[index].instr_id << " lq_index: " << RQ.entry[index].lq_index << endl; });
							temp_mshr->entry[mshr_index].lq_index_depend_on_me.join (RQ.entry[index].lq_index_depend_on_me, LQ_SIZE);
							if (RQ.entry[index].store_merged) {
								temp_mshr->entry[mshr_index].store_merged = 1;
								temp_mshr->entry[mshr_index].sq_index_depend_on_me.join (RQ.entry[index].sq_index_depend_on_me, SQ_SIZE);
							}
						}
					}

					// update fill_level

					ACCESS[RQ.entry[index].type]++;
					MSHR_MERGED[RQ.entry[index].type]++;


				}

				else{ // MISS in MSHR_DATA
					RQ.entry[index].address >>= 6; // left shift for translation
					RQ.entry[index].type = TRANSLATION;
					int mshr_index = check_nonfifo_queue(&MSHR_TRANSLATION, &RQ.entry[index],false); 

					PACKET_QUEUE *temp_mshr = &MSHR_TRANSLATION ;
					if ((mshr_index == -1)  && (MSHR_TRANSLATION.occupancy < MSHR_SIZE)) { // this is a new miss

						// add it to mshr (read miss)
						add_nonfifo_queue(&MSHR_TRANSLATION, &RQ.entry[index]); //@Vishal: Updated from add_mshr

						if(RQ.entry[index].instruction)
							ooo_cpu[cpu].ITLB.add_rq(&RQ.entry[index]);
						else
							ooo_cpu[cpu].DTLB.add_rq(&RQ.entry[index]);


					}
					else {
						if ((mshr_index == -1) && (MSHR_TRANSLATION.occupancy == MSHR_SIZE)) { // not enough MSHR resource


							RQ.entry[index].address = RQ.entry[index].full_virtual_address>> LOG2_BLOCK_SIZE; // if not handled address should be of of LOG2_BLOCK_SIZE

							miss_handled = 0;
							STALL[RQ.entry[index].type]++;

						}
						else if (mshr_index != -1) { // already in-flight miss

							MSHR_TRANSLATION.entry[mshr_index].scratchpad_mshr_merged = true;
							MSHR_TRANSLATION.entry[mshr_index].scratchpad_mshr_depends_on_me.push(RQ.entry[index]);
							if(RQ.entry[index].scratchpad_mshr_merged){
								while(!RQ.entry[index].scratchpad_mshr_depends_on_me.empty()){
									MSHR_TRANSLATION.entry[mshr_index].scratchpad_mshr_depends_on_me.push(RQ.entry[index].scratchpad_mshr_depends_on_me.front());
									RQ.entry[index].scratchpad_mshr_depends_on_me.pop();
								}		
								RQ.entry[index].scratchpad_mshr_merged  =  false;
							}

						}
					}
				}

				if(miss_handled){
					MISS[RQ.entry[index].type]++;
					ACCESS[RQ.entry[index].type]++;

					// remove this entry from RQ
					RQ.remove_queue(&RQ.entry[index]);
					reads_available_this_cycle--;
				}
			}

		}
		else
		{
			return;
		}

		if(reads_available_this_cycle == 0)
		{
			return;
		}
	}
}
void CACHE::handle_read()
{

	PRINT77(
	if(cpu >= ACCELERATOR_START){
		cout <<"inside handle_read " << endl;
                cout << "cache type "<< NAME << " RQ occupancy " << RQ.occupancy << endl;
                RQ.queuePrint();
                cout << "cache type "<< NAME << " MSHR occupancy " << MSHR.occupancy << endl;
                MSHR.queuePrint();
        }
	)
	if(RQ.occupancy == 0)
		return;


	multimap<uint64_t, uint32_t> reads_ready; //{cycle_time,index}

	if(cache_type == IS_L1I || cache_type == IS_L1D) //Get completed index in RQ, as it is non-fifo
	{
		for (uint32_t rq_index=0; rq_index < RQ.SIZE; rq_index++)
			if(RQ.entry[rq_index].translated == COMPLETED && (RQ.entry[rq_index].event_cycle <= current_core_cycle[cpu])) 
				reads_ready.insert({RQ.entry[rq_index].event_cycle, rq_index});
	}
	auto reads_ready_it = reads_ready.begin();

	if((cache_type == IS_L1I || cache_type == IS_L1D) && reads_ready.size() == 0)
		return;

	// handle read
	static int counter;
	for (uint32_t i=0; i<MAX_READ; i++) {


		if(cache_type == IS_L1I || cache_type == IS_L1D)
		{
			if(reads_ready_it == reads_ready.end())
				return;
			RQ.head = reads_ready_it->second; //@Vishal: L1 RQ is non fifo, so head variable is set to next index to be read
			reads_ready_it++;
		}

		uint32_t read_cpu = RQ.entry[RQ.head].cpu;
		if (read_cpu == NUM_CPUS)
			return;


		// handle the oldest entry
		if ((RQ.entry[RQ.head].event_cycle <= current_core_cycle[read_cpu]) && (RQ.occupancy > 0)) {
			int index = RQ.head;
			if(STLB_FLAG){
				cout << " handle read processing rob id " << RQ.entry[index].rob_index << " packet cpu " << RQ.entry[index].cpu << endl;
			}

/*			if(cpu >= ACCELERATOR_START && ( cache_type == IS_DTLB)){
                          //      coldMiss_DTLB.insert(RQ.entry[index].address);
                                extra_interface->return_data(&RQ.entry[index]);
                                RQ.remove_queue(&RQ.entry[index]);
				ACCESS[RQ.entry[index].type]++;
                                reads_available_this_cycle--;
                                continue;
                        }*/
			// access cache
			uint32_t set = get_set(RQ.entry[index].address);
			int way = check_hit(&RQ.entry[index]);

#ifdef PUSH_DTLB_PB
			//If DTLB misses, check DTLB Prefetch Buffer
			if(cache_type == IS_DTLB && way < 0)
			{
				int  way_pb;
				way_pb = ooo_cpu[read_cpu].DTLB_PB.check_hit( &RQ.entry[index] );
				if(way_pb >= 0)
				{
		//			cout << "found in pb dtlb rq cpu " << cpu << " address " << RQ.entry[index].address << endl;
					ooo_cpu[read_cpu].DTLB_PB.block[0][way_pb].used = 1;
					RQ.entry[index].data_pa = ooo_cpu[read_cpu].DTLB_PB.block[0][way_pb].data;
					ooo_cpu[read_cpu].DTLB_PB.sim_hit[read_cpu][RQ.entry[index].type]++;
					ooo_cpu[read_cpu].DTLB_PB.sim_access[read_cpu][RQ.entry[index].type]++;
					ooo_cpu[read_cpu].DTLB_PB.HIT[RQ.entry[index].type]++;
					ooo_cpu[read_cpu].DTLB_PB.ACCESS[RQ.entry[index].type]++;
					ooo_cpu[read_cpu].DTLB_PB.update_replacement_state(read_cpu, 0, way_pb, ooo_cpu[read_cpu].DTLB_PB.block[0][way_pb].full_addr, RQ.entry[index].ip, 0, RQ.entry[index].type, 1);
					RQ.entry[index].data = ooo_cpu[read_cpu].DTLB_PB.block[0][way_pb].data;
					ooo_cpu[read_cpu].DTLB_PB.pf_useful++;
					// If DTLB prefetch buffer gets hit, fill DTLB and then proceed
					way = dtlb_find_victim(read_cpu, RQ.entry[index].instr_id, set, block[set], RQ.entry[index].ip, RQ.entry[index].full_addr, RQ.entry[index].type);
					//cout << "DTLB_PB hit "<< RQ.entry[index].instr_id << " " << ooo_cpu[read_cpu].DTLB_PB.pf_useful << endl;
					
					if (cpu < ACCELERATOR_START && PROCESSED.occupancy < PROCESSED.SIZE)
						 PROCESSED.add_queue(&RQ.entry[index]);
					else if(cpu >= ACCELERATOR_START){
						SCRATCHPADPRINT(cout << " 1 return from ITLB addr " << MSHR.entry[mshr_index].address << endl;)
							extra_interface->return_data(&RQ.entry[index]);

					}
					if(RQ.entry[index].type == LOAD)
						dtlb_prefetcher_operate(RQ.entry[index].address<<LOG2_PAGE_SIZE, RQ.entry[index].ip, 1, RQ.entry[index].type, RQ.entry[index].instr_id, RQ.entry[index].instruction,RQ.entry[index].asid[0] );
					fill_cache(set, way, &RQ.entry[index]);
					RQ.remove_queue(&RQ.entry[index]);
					reads_available_this_cycle--;
					continue;
				}
				else
				{
					//DTLB_PB MISS
					ooo_cpu[read_cpu].DTLB_PB.MISS[RQ.entry[index].type]++;
					ooo_cpu[read_cpu].DTLB_PB.ACCESS[RQ.entry[index].type]++;
					ooo_cpu[read_cpu].DTLB_PB.sim_miss[read_cpu][RQ.entry[index].type]++;
					ooo_cpu[read_cpu].DTLB_PB.sim_access[read_cpu][RQ.entry[index].type]++;
				}
			}
#endif

			if (way >= 0) { // read hit


				if (cache_type == IS_ITLB) {

					auto difference = current_core_cycle[cpu] - RQ.entry[index].tlb_start_cycle;
					if (warmup_complete[cpu] && RQ.entry[index].tlb_start_cycle > 0 &&  difference > 0){
						QPRINT(	if( current_core_cycle[cpu] - RQ.entry[index].tlb_start_cycle <= 0)
						cout <<  current_core_cycle[cpu] << " current start " <<  RQ.entry[index].tlb_start_cycle << endl;)
						total_lat_req[cpu]++;
						sim_latency[cpu] +=  current_core_cycle[cpu] -  RQ.entry[index].tlb_start_cycle;
					}
		//RQ.entry[index].instruction_pa = (va_to_pa(read_cpu, RQ.entry[index].instr_id, RQ.entry[index].full_addr, RQ.entry[index].address))>>LOG2_PAGE_SIZE; //block[set][way].data;
					RQ.entry[index].instruction_pa = block[set][way].data;
					//RQ.entry[index].event_cycle = current_core_cycle[read_cpu];
					if (cpu < ACCELERATOR_START && PROCESSED.occupancy < PROCESSED.SIZE)
						PROCESSED.add_queue(&RQ.entry[index]);
					else if(cpu >= ACCELERATOR_START ){
						extra_interface->return_data(&RQ.entry[index]);
					}
				}
				else if (cache_type == IS_DTLB) {
					//RQ.entry[index].data_pa = (va_to_pa(read_cpu, RQ.entry[index].instr_id, RQ.entry[index].full_addr, RQ.entry[index].address))>>LOG2_PAGE_SIZE;  //block[set][way].data;
					if (warmup_complete[cpu] && RQ.entry[index].tlb_start_cycle > 0 && current_core_cycle[cpu] -  RQ.entry[index].tlb_start_cycle >0 ){
						total_lat_req[cpu]++;
						QPRINT(	if( current_core_cycle[cpu] - RQ.entry[index].tlb_start_cycle <= 0)
						cout <<  current_core_cycle[cpu] << " current start " <<  RQ.entry[index].tlb_start_cycle << endl;)
						sim_latency[cpu] +=  current_core_cycle[cpu] -  RQ.entry[index].tlb_start_cycle;
					}

					RQ.entry[index].data_pa = block[set][way].data;
					//RQ.entry[index].event_cycle = current_core_cycle[read_cpu];
					if (cpu < ACCELERATOR_START && PROCESSED.occupancy < PROCESSED.SIZE)
						PROCESSED.add_queue(&RQ.entry[index]);
					else if(cpu >= ACCELERATOR_START )
						extra_interface->return_data(&RQ.entry[index]);
				}
				else if (cache_type == IS_STLB  && cpu < ACCELERATOR_START) 
				{

					PACKET temp = RQ.entry[index];

					if (temp.l1_pq_index != -1) //@Vishal: Check if the current request is sent from L1D prefetcher
					{
						assert(RQ.entry[index].l1_rq_index == -1 && RQ.entry[index].l1_wq_index == -1);//@Vishal: these should not be set

						temp.data_pa = block[set][way].data;
						temp.read_translation_merged = false;
						temp.write_translation_merged = false;
						if (warmup_complete[cpu] && RQ.entry[index].stlb_start_cycle > 0){
							if(cpu >= ACCELERATOR_START){
								if(current_core_cycle[ACCELERATOR_START] -  RQ.entry[index].stlb_start_cycle >0){
								total_lat_req[ACCELERATOR_START]++;
								STLBPRINT(cout << "1cpu " << cpu << " simLat " << sim_latency[ACCELERATOR_START] << " start cycle " << RQ.entry[index].stlb_start_cycle << " current core cycle " << current_core_cycle[ACCELERATOR_START] << endl;)
								sim_latency[ACCELERATOR_START] +=  current_core_cycle[ACCELERATOR_START] -  RQ.entry[index].stlb_start_cycle;
								}
							}
							else{
								if(current_core_cycle[cpu] -  RQ.entry[index].stlb_start_cycle > 0){
								total_lat_req[cpu]++;
								sim_latency[cpu] +=  current_core_cycle[cpu] -  RQ.entry[index].stlb_start_cycle;
								}
							}
						}

						if (PROCESSED.occupancy < PROCESSED.SIZE)
							PROCESSED.add_queue(&temp);
						else
							assert(0);
					}
					if(RQ.entry[index].prefetch_translation_merged) //@Vishal: Prefetech request from L1D prefetcher
					{
						PACKET temp = RQ.entry[index];
						temp.data_pa = block[set][way].data;
						temp.read_translation_merged = 0; //@Vishal: Remove this before adding to PQ
						temp.write_translation_merged = 0;
						if (warmup_complete[cpu] && RQ.entry[index].stlb_start_cycle > 0){
							if(cpu >= ACCELERATOR_START){
								if(current_core_cycle[ACCELERATOR_START] -  RQ.entry[index].stlb_start_cycle > 0){
								total_lat_req[ACCELERATOR_START]++;
								STLBPRINT(cout << "2cpu " << cpu << " simLat " << sim_latency[ACCELERATOR_START] << " start cycle " << RQ.entry[index].stlb_start_cycle << " current core cycle " << current_core_cycle[ACCELERATOR_START] << endl;)
								sim_latency[ACCELERATOR_START] +=  current_core_cycle[ACCELERATOR_START] -  RQ.entry[index].stlb_start_cycle;
								}
							}
							else{
								if(current_core_cycle[cpu] -  RQ.entry[index].stlb_start_cycle > 0){
								total_lat_req[cpu]++;
								sim_latency[cpu] +=  current_core_cycle[cpu] -  RQ.entry[index].stlb_start_cycle;
								}
							}
						}

						if (PROCESSED.occupancy < PROCESSED.SIZE)
							PROCESSED.add_queue(&temp);
						else
							assert(0);
					}

				}
				else if (cache_type == IS_L1I) {
					if (PROCESSED.occupancy < PROCESSED.SIZE)
						PROCESSED.add_queue(&RQ.entry[index]);
				}
				else if ((cache_type == IS_L1D) && (RQ.entry[index].type != PREFETCH)) {
					if (PROCESSED.occupancy < PROCESSED.SIZE)
						PROCESSED.add_queue(&RQ.entry[index]);
				}



				// update prefetcher on load instruction
				if(cpu >= ACCELERATOR_START && cache_type == IS_DTLB){
					dtlb_prefetcher_operate(RQ.entry[index].address<<LOG2_PAGE_SIZE, RQ.entry[index].ip, 1, RQ.entry[index].type, RQ.entry[index].instr_id, RQ.entry[index].instruction, RQ.entry[index].asid[0]);
				}
				if (RQ.entry[index].type == LOAD) {
					if (cache_type == IS_L1D) 
						l1d_prefetcher_operate(RQ.entry[index].full_addr, RQ.entry[index].ip, 1, RQ.entry[index].type);	// RQ.entry[index].instr_id);
					else if (cache_type == IS_L2C && RQ.entry[index].type !=  TRANSLATION ) {
						l2c_prefetcher_operate(block[set][way].address<<LOG2_BLOCK_SIZE, RQ.entry[index].ip, 1, RQ.entry[index].type, 0);	// RQ.entry[index].instr_id);
					}else if (cache_type == IS_LLC)
					{
						cpu = read_cpu;
						llc_prefetcher_operate(block[set][way].address<<LOG2_BLOCK_SIZE, RQ.entry[index].ip, 1, RQ.entry[index].type, 0);	//	RQ.entry[index].instr_id);
						cpu = 0;
					}
					else if (cache_type == IS_ITLB)
					{
						itlb_prefetcher_operate(RQ.entry[index].address<<LOG2_PAGE_SIZE, RQ.entry[index].ip, 1, RQ.entry[index].type, RQ.entry[index].instr_id, RQ.entry[index].instruction);

					}
					else if (cache_type == IS_DTLB)
					{
#ifdef SANITY_CHECK
						if(RQ.entry[index].instruction)
						{
							cout << "DTLB prefetch packet should not prefetch address translation of instruction"<< endl;
							assert(0);
						}
#endif

						dtlb_prefetcher_operate(RQ.entry[index].address<<LOG2_PAGE_SIZE, RQ.entry[index].ip, 1, RQ.entry[index].type, RQ.entry[index].instr_id, RQ.entry[index].instruction, RQ.entry[index].asid[0]);


					}
					else if (cache_type == IS_STLB)
					{
						stlb_prefetcher_operate(RQ.entry[index].address<<LOG2_PAGE_SIZE, RQ.entry[index].ip, 1, RQ.entry[index].type, RQ.entry[index].instr_id, RQ.entry[index].instruction);

					}
				}

				// update replacement policy
				if (cache_type == IS_LLC) {
					llc_update_replacement_state(read_cpu, set, way, block[set][way].full_addr, RQ.entry[index].ip, 0, RQ.entry[index].type, 1);

				}
				else if (cache_type == IS_DTLB) {
					dtlb_update_replacement_state(read_cpu, set, way, block[set][way].full_addr, RQ.entry[index].ip, 0, RQ.entry[index].type, 1);
				}
				else if (cache_type == IS_STLB) {
					stlb_update_replacement_state(read_cpu, set, way, block[set][way].full_addr, RQ.entry[index].ip, 0, RQ.entry[index].type, 1);
				}
				else
					update_replacement_state(read_cpu, set, way, block[set][way].full_addr, RQ.entry[index].ip, 0, RQ.entry[index].type, 1);

				// COLLECT STATS
				if(cache_type == IS_STLB && cpu >= ACCELERATOR_START){
					sim_hit[ACCELERATOR_START][RQ.entry[index].type]++;
					sim_access[ACCELERATOR_START][RQ.entry[index].type]++;
				}
				else{
					sim_hit[read_cpu][RQ.entry[index].type]++;
					sim_access[read_cpu][RQ.entry[index].type]++;
				}


				// check fill level
				// data should be updated (for TLBs) in case of hit
				if (RQ.entry[index].fill_level < fill_level) {


					if(cache_type == IS_STLB)
					{
						RQ.entry[index].prefetch_translation_merged = false;
						if(STLB_FLAG){	 
							cout << RQ.entry[index].cpu << " upper level " << endl;
							cout << "three added to upper level cache packet cpu " << RQ.entry[index].cpu << endl;}
						if(RQ.entry[index].stlb_merged)
							handle_stlb_merged_translations(&RQ.entry[index],block[set][way].data,true);
						if(RQ.entry[index].send_both_tlb)
						{
							RQ.entry[index].data = block[set][way].data;
							RQ.entry[index].send_both_tlb = 1; /* Inside block, send_both_tlb may be zero */ 
				/*			assert(RQ.entry[index].stlb_start_cycle >= 0);
							total_lat_req[cpu]++;
							sim_latency[cpu] += current_core_cycle[cpu] - RQ.entry[index].stlb_start_cycle ;
*/
							upper_level_icache[RQ.entry[index].cpu]->return_data(&RQ.entry[index]);
							upper_level_dcache[RQ.entry[index].cpu]->return_data(&RQ.entry[index]);
						}
						else if (RQ.entry[index].instruction)
						{
							RQ.entry[index].data = block[set][way].data;
				/*			assert(RQ.entry[index].stlb_start_cycle >= 0);
							total_lat_req[cpu]++;
							sim_latency[cpu] += current_core_cycle[cpu] - RQ.entry[index].stlb_start_cycle ;
				*/			upper_level_icache[RQ.entry[index].cpu]->return_data(&RQ.entry[index]);
						}
						else // data
						{	
							RQ.entry[index].data = block[set][way].data;
		/*					assert(RQ.entry[index].stlb_start_cycle >= 0);
							total_lat_req[cpu]++;
							sim_latency[cpu] += current_core_cycle[cpu] - RQ.entry[index].stlb_start_cycle ;
		*/					upper_level_dcache[RQ.entry[index].cpu]->return_data(&RQ.entry[index]);
						}
#ifdef SANITY_CHECK
						if(RQ.entry[index].data == 0)
							assert(0);
#endif
					}
					else if(cache_type == IS_L2C && RQ.entry[index].type == TRANSLATION)
					{
						extra_interface->return_data(&RQ.entry[index]);
					}
					else if(cache_type == IS_L2C)
					{
						if(RQ.entry[index].send_both_cache)
						{
							//if(knob_cloudsuite)
							//	RQ.entry[index].address = (( RQ.entry[index].ip >> LOG2_PAGE_SIZE) << 9) | ( 256 + RQ.entry[index].asid[0]);                                        
							upper_level_icache[read_cpu]->return_data(&RQ.entry[index]);
							//if(knob_cloudsuite)
							// 	RQ.entry[index].address = (( RQ.entry[index].full_virtual_address >> LOG2_PAGE_SIZE) << 9) |  RQ.entry[index].asid[1];
							upper_level_dcache[read_cpu]->return_data(&RQ.entry[index]);
						}
						else if (RQ.entry[index].instruction)
							upper_level_icache[read_cpu]->return_data(&RQ.entry[index]);
						else	//data
							upper_level_dcache[read_cpu]->return_data(&RQ.entry[index]);
					}	
					else	
					{
						if (RQ.entry[index].instruction) 
						{
							RQ.entry[index].data = block[set][way].data;
							upper_level_icache[read_cpu]->return_data(&RQ.entry[index]);
						}
						else // data
						{
							RQ.entry[index].data = block[set][way].data;
							upper_level_dcache[read_cpu]->return_data(&RQ.entry[index]);
						}
#ifdef SANITY_CHECK
						if(cache_type == IS_ITLB || cache_type == IS_DTLB)
							if(RQ.entry[index].data == 0)
								assert(0);
#endif
					}
				}

				// update prefetch stats and reset prefetch bit
				if (block[set][way].prefetch) {
					pf_useful++;
					block[set][way].prefetch = 0;

					//Neelu: IPCP prefetch stats
					if(block[set][way].pref_class < 5)
						pref_useful[cpu][block[set][way].pref_class]++;

				}
				block[set][way].used = 1;

				HIT[RQ.entry[index].type]++;
				ACCESS[RQ.entry[index].type]++;

				// remove this entry from RQ
				STLBPRINT( if(cpu >= ACCELERATOR_START) cout << " removing from RQ from fill" << NAME << " cpu " <<  RQ.entry[index].cpu << " rob " << RQ.entry[index].rob_index << endl;)
				RQ.remove_queue(&RQ.entry[index]);
				reads_available_this_cycle--;
			}
			else { // read miss
				if(STLB_FLAG)
					cout << "read miss" << endl;

				DP ( if (warmup_complete[read_cpu] ) {
						cout << "[" << NAME << "] " << __func__ << " read miss";
						cout << " instr_id: " << RQ.entry[index].instr_id << " address: " << hex << RQ.entry[index].address;
						cout << " full_addr: " << RQ.entry[index].full_addr << dec;
						cout << " cycle: " << RQ.entry[index].event_cycle << endl; });

	/*			if(cache_type == IS_DTLB){
					if(add_loc.count(RQ.entry[index].address-1)){
						int count = 0;
						uint64_t  location = add_loc[RQ.entry[index].address-1];
						for(auto itr= loc_add.find(location); itr != loc_add.end(); itr++){
							count++;
						}
						dist_count[count]++;
						//     total_reuse_distance += total_address.size();
						// number_of_reuse_distance++;
						// total_address.clear();

					}
					else
						dist_count[UINT64_MAX]++;
					add_loc[RQ.entry[index].address] = current_core_cycle[cpu];
					loc_add[current_core_cycle[cpu]] = RQ.entry[index].address;

				}
*/

				// check mshr
				uint8_t miss_handled = 1;
				int mshr_index = check_nonfifo_queue(&MSHR, &RQ.entry[index],false); //@Vishal: Updated from check_mshr

				if(STLB_FLAG)
					cout << "mshr_index " << mshr_index << endl;

				if ((mshr_index == -1) && (MSHR.occupancy < MSHR_SIZE)) { // this is a new miss

					if(cache_type == IS_LLC)
					{
						// check to make sure the DRAM RQ has room for this LLC read miss
						if (lower_level->get_occupancy(1, RQ.entry[index].address) == lower_level->get_size(1, RQ.entry[index].address))
						{
							miss_handled = 0;
						}
						else
						{

							add_nonfifo_queue(&MSHR, &RQ.entry[index]); //@Vishal: Updated from add_mshr
							if(lower_level)
							{
								lower_level->add_rq(&RQ.entry[index]);
							}
						}
					}
					else
					{

						if(cache_type == IS_L1I || cache_type == IS_L1D) //@Vishal: VIPT
						{
							assert(RQ.entry[index].full_physical_address != 0);
							PACKET new_packet = RQ.entry[index];
							//@Vishal: Send physical address to lower level and track physical address in MSHR  
							new_packet.address = RQ.entry[index].full_physical_address >> LOG2_BLOCK_SIZE;
							new_packet.full_addr = RQ.entry[index].full_physical_address; 

							add_nonfifo_queue(&MSHR, &new_packet); //@Vishal: Updated from add_mshr
							lower_level->add_rq(&new_packet);
						}
						else
						{
							// add it to mshr (read miss)
							add_nonfifo_queue(&MSHR, &RQ.entry[index]); //@Vishal: Updated from add_mshr

							// add it to the next level's read queue
							if (lower_level)
							{
								if(lower_level->RQ.occupancy < lower_level->RQ.SIZE){
									lower_level->add_rq(&RQ.entry[index]);
								}
								else{
									MSHR.remove_queue(&RQ.entry[index]);
									miss_handled = 0;
								}
							}
							else { // this is the last level
#ifdef INS_PAGE_TABLE_WALKER
								PRINT(cout << "cache type = " << (int)cache_type << " cpu " << cpu << endl;)
								assert(0);
#else
								if (cache_type == IS_STLB) {
									// TODO: need to differentiate page table walk and actual swap

									// emulate page table walk
									uint64_t pa = va_to_pa(read_cpu, RQ.entry[index].instr_id, RQ.entry[index].full_addr, RQ.entry[index].address);
									RQ.entry[index].data = pa >> LOG2_PAGE_SIZE; 
									RQ.entry[index].event_cycle = current_core_cycle[read_cpu];

									if (RQ.entry[index].l1_pq_index != -1) //@Vishal: Check if the current request is sent from L1D prefetcher
									{
										assert(RQ.entry[index].l1_rq_index == -1 && RQ.entry[index].l1_wq_index == -1);//@Vishal: these should not be set

										if (warmup_complete[cpu] && RQ.entry[index].stlb_start_cycle > 0){
											if(cpu >= ACCELERATOR_START){
												if(current_core_cycle[ACCELERATOR_START] -  RQ.entry[index].stlb_start_cycle > 0){
												total_lat_req[ACCELERATOR_START]++;
								STLBPRINT(cout << "3cpu " << cpu << " simLat " << sim_latency[ACCELERATOR_START] << " start cycle " << RQ.entry[index].stlb_start_cycle << " current core cycle " << current_core_cycle[ACCELERATOR_START] << endl;)
												sim_latency[ACCELERATOR_START] +=  current_core_cycle[ACCELERATOR_START] -  RQ.entry[index].stlb_start_cycle;
												}
											}
											else{
												if(current_core_cycle[cpu] -  RQ.entry[index].stlb_start_cycle > 0){
												total_lat_req[cpu]++;
												sim_latency[cpu] +=  current_core_cycle[cpu] -  RQ.entry[index].stlb_start_cycle;
												}
											}
										}
										RQ.entry[index].data_pa = pa >> LOG2_PAGE_SIZE;
										if (PROCESSED.occupancy < PROCESSED.SIZE)
											PROCESSED.add_queue(&RQ.entry[index]);
										else
											assert(0);
									}

								if(STLB_FLAG) cout << "four eturn data cache packet cpu " << RQ.entry[index].cpu << endl;
									return_data(&RQ.entry[index]);
								}
#endif
							}
						}
					}
				}
				else {
					if ((mshr_index == -1) && (MSHR.occupancy == MSHR_SIZE)) { // not enough MSHR resource

						// cannot handle miss request until one of MSHRs is available
						miss_handled = 0;
						STALL[RQ.entry[index].type]++;
					}
					else if (mshr_index != -1) { // already in-flight miss


						// mark merged consumer
						if (RQ.entry[index].type == RFO) {

							if (RQ.entry[index].tlb_access) {
								uint32_t sq_index = RQ.entry[index].sq_index;
								MSHR.entry[mshr_index].store_merged = 1;
								MSHR.entry[mshr_index].sq_index_depend_on_me.insert (sq_index);
								MSHR.entry[mshr_index].sq_index_depend_on_me.join (RQ.entry[index].sq_index_depend_on_me, SQ_SIZE);
							}

							if (RQ.entry[index].load_merged) {
								//uint32_t lq_index = RQ.entry[index].lq_index; 
								MSHR.entry[mshr_index].load_merged = 1;
								//MSHR.entry[mshr_index].lq_index_depend_on_me[lq_index] = 1;
								MSHR.entry[mshr_index].lq_index_depend_on_me.join (RQ.entry[index].lq_index_depend_on_me, LQ_SIZE);
							}

						}
						else {
							if (RQ.entry[index].instruction) {
								uint32_t rob_index = RQ.entry[index].rob_index;
								DP (if (warmup_complete[MSHR.entry[mshr_index].cpu] ) {
										//if(cache_type==IS_ITLB || cache_type==IS_DTLB || cache_type==IS_STLB)
										cout << "read request merged with MSHR entry -"<< MSHR.entry[mshr_index].type << endl; });
								MSHR.entry[mshr_index].instr_merged = 1;
								MSHR.entry[mshr_index].rob_index_depend_on_me.insert (rob_index);

								DP (if (warmup_complete[MSHR.entry[mshr_index].cpu] ) {
										cout << "[INSTR_MERGED] " << __func__ << " cpu: " << MSHR.entry[mshr_index].cpu << " instr_id: " << MSHR.entry[mshr_index].instr_id;
										cout << " merged rob_index: " << rob_index << " instr_id: " << RQ.entry[index].instr_id << endl; });

								if (RQ.entry[index].instr_merged) {
									MSHR.entry[mshr_index].rob_index_depend_on_me.join (RQ.entry[index].rob_index_depend_on_me, ROB_SIZE);
									DP (if (warmup_complete[MSHR.entry[mshr_index].cpu] ) {
											cout << "[INSTR_MERGED] " << __func__ << " cpu: " << MSHR.entry[mshr_index].cpu << " instr_id: " << MSHR.entry[mshr_index].instr_id;
											cout << " merged rob_index: " << i << " instr_id: N/A" << endl; });
								}
							}
							else 
							{
								uint32_t lq_index = RQ.entry[index].lq_index;
								MSHR.entry[mshr_index].load_merged = 1;
								MSHR.entry[mshr_index].lq_index_depend_on_me.insert (lq_index);

								DP (if (warmup_complete[read_cpu] ) {
										cout << "[DATA_MERGED] " << __func__ << " cpu: " << read_cpu << " instr_id: " << RQ.entry[index].instr_id;
										cout << " merged rob_index: " << RQ.entry[index].rob_index << " instr_id: " << RQ.entry[index].instr_id << " lq_index: " << RQ.entry[index].lq_index << endl; });
								MSHR.entry[mshr_index].lq_index_depend_on_me.join (RQ.entry[index].lq_index_depend_on_me, LQ_SIZE);
								if (RQ.entry[index].store_merged) {
									MSHR.entry[mshr_index].store_merged = 1;
									MSHR.entry[mshr_index].sq_index_depend_on_me.join (RQ.entry[index].sq_index_depend_on_me, SQ_SIZE);
								}
							}
						}

						// update fill_level
						if (RQ.entry[index].fill_level < MSHR.entry[mshr_index].fill_level)
							MSHR.entry[mshr_index].fill_level = RQ.entry[index].fill_level;


						bool merging_already_done = false;



						// update request
						if (MSHR.entry[mshr_index].type == PREFETCH && RQ.entry[index].type != PREFETCH) {

							merging_already_done = true;
							uint8_t  prior_returned = MSHR.entry[mshr_index].returned;
							uint64_t prior_event_cycle = MSHR.entry[mshr_index].event_cycle;
							uint64_t prior_data;

							uint64_t prior_address = MSHR.entry[mshr_index].address;
							uint64_t prior_full_addr = MSHR.entry[mshr_index].full_addr;
							uint64_t prior_full_physical_address = MSHR.entry[mshr_index].full_physical_address; 
							if(cache_type==IS_ITLB || cache_type==IS_DTLB || cache_type==IS_STLB)
							{
								/* data(translation) should be copied in case of TLB if MSHR entry is completed and is not filled in cache yet */
								if(MSHR.entry[mshr_index].returned == COMPLETED)
								{
									prior_data = MSHR.entry[mshr_index].data;
								}

								//@Vishal: Copy previous data from MSHR


								if(MSHR.entry[mshr_index].read_translation_merged)
								{
									RQ.entry[index].read_translation_merged = 1;
									RQ.entry[index].l1_rq_index_depend_on_me.join(MSHR.entry[mshr_index].l1_rq_index_depend_on_me, RQ_SIZE);
								}

								if(MSHR.entry[mshr_index].write_translation_merged)
								{
									RQ.entry[index].write_translation_merged = 1;
									RQ.entry[index].l1_wq_index_depend_on_me.join(MSHR.entry[mshr_index].l1_wq_index_depend_on_me, WQ_SIZE);
								}

								if(MSHR.entry[mshr_index].prefetch_translation_merged)
								{
									RQ.entry[index].prefetch_translation_merged = 1;
									RQ.entry[index].l1_pq_index_depend_on_me.join(MSHR.entry[mshr_index].l1_pq_index_depend_on_me, PQ_SIZE);
								}

								if(MSHR.entry[mshr_index].l1_rq_index != -1)
								{
									assert((MSHR.entry[mshr_index].l1_wq_index == -1) && (MSHR.entry[mshr_index].l1_pq_index == -1));
									RQ.entry[index].read_translation_merged = 1;
									RQ.entry[index].l1_rq_index_depend_on_me.insert(MSHR.entry[mshr_index].l1_rq_index);
								}

								if(MSHR.entry[mshr_index].l1_wq_index != -1)
								{
									assert((MSHR.entry[mshr_index].l1_rq_index == -1) && (MSHR.entry[mshr_index].l1_pq_index == -1));
									RQ.entry[index].write_translation_merged = 1;
									RQ.entry[index].l1_wq_index_depend_on_me.insert(MSHR.entry[mshr_index].l1_wq_index);
								}

								if(MSHR.entry[mshr_index].l1_pq_index != -1)
								{
									assert((MSHR.entry[mshr_index].l1_wq_index == -1) && (MSHR.entry[mshr_index].l1_rq_index == -1));
									RQ.entry[index].prefetch_translation_merged = 1;
									RQ.entry[index].l1_pq_index_depend_on_me.insert(MSHR.entry[mshr_index].l1_pq_index);


									DP ( if (warmup_complete[read_cpu]){
											cout << "[" << NAME << "] " << __func__ << " this should be printed";
											cout << " instr_id: " << RQ.entry[index].instr_id << " prior_id: " << MSHR.entry[mshr_index].instr_id;
											cout << " address: " << hex << RQ.entry[index].address;
											cout << " full_addr: " << RQ.entry[index].full_addr << dec;
											if(RQ.entry[index].read_translation_merged)
											cout << " read_translation_merged ";
											if(RQ.entry[index].write_translation_merged)
											cout << " write_translation_merged ";
											if(RQ.entry[index].prefetch_translation_merged)
											cout << " prefetch_translation_merged ";

											cout << " cycle: " << RQ.entry[index].event_cycle << endl; });





								}








							}


							MSHR.entry[mshr_index] = RQ.entry[index];


							DP ( if (warmup_complete[read_cpu]){ 
									cout << "[" << NAME << "] " << __func__ << " this should be printed";
									cout << " instr_id: " << RQ.entry[index].instr_id << " prior_id: " << MSHR.entry[mshr_index].instr_id;
									cout << " address: " << hex << RQ.entry[index].address;
									cout << " full_addr: " << RQ.entry[index].full_addr << dec;
									if(MSHR.entry[mshr_index].read_translation_merged)
									cout << " read_translation_merged ";
									if(MSHR.entry[mshr_index].write_translation_merged)
									cout << " write_translation_merged ";
									if(MSHR.entry[mshr_index].prefetch_translation_merged)
									cout << " prefetch_translation_merged ";

									cout << " cycle: " << RQ.entry[index].event_cycle << endl; });



							assert(cache_type != IS_L1I);//@Vishal: L1I cache does not generate prefetch packet.

							//@Vishal: L1 RQ has virtual address, but miss needs to track physical address, so prior addresses are kept
							if(cache_type == IS_L1D)
							{
								MSHR.entry[mshr_index].address = prior_address;
								MSHR.entry[mshr_index].full_addr = prior_full_addr;
								MSHR.entry[mshr_index].full_physical_address = prior_full_physical_address;
							}

							// in case request is already returned, we should keep event_cycle and retunred variables
							MSHR.entry[mshr_index].returned = prior_returned;
							MSHR.entry[mshr_index].event_cycle = prior_event_cycle;
							MSHR.entry[mshr_index].data = prior_data;
							++pf_late;//@v Late prefetch-> on-demand requests hit in MSHR


							//Neelu: set the late bit
							if(cache_type == IS_L1D)
							{
								//cout<<"Neelu: MSHR entry late_pref INC"<<endl;
								MSHR.entry[mshr_index].late_pref = 1;
								late_prefetch++;
							} 
						}


						//@Vishal: Check if any translation is dependent on this read request
						if(!merging_already_done && (cache_type == IS_ITLB || cache_type ==  IS_DTLB || cache_type == IS_STLB))
						{
							if(RQ.entry[index].read_translation_merged)
							{
								MSHR.entry[mshr_index].read_translation_merged = 1;
								MSHR.entry[mshr_index].l1_rq_index_depend_on_me.join(RQ.entry[index].l1_rq_index_depend_on_me, RQ_SIZE);
							}

							if(RQ.entry[index].write_translation_merged)
							{
								MSHR.entry[mshr_index].write_translation_merged = 1;
								MSHR.entry[mshr_index].l1_wq_index_depend_on_me.join(RQ.entry[index].l1_wq_index_depend_on_me, WQ_SIZE);
							}

							if(RQ.entry[index].prefetch_translation_merged)
							{
								MSHR.entry[mshr_index].prefetch_translation_merged = 1;
								MSHR.entry[mshr_index].l1_pq_index_depend_on_me.join(RQ.entry[index].l1_pq_index_depend_on_me, PQ_SIZE);
							}

							if(RQ.entry[index].l1_rq_index != -1)
							{
								assert((RQ.entry[index].l1_wq_index == -1) && (RQ.entry[index].l1_pq_index == -1));
								MSHR.entry[mshr_index].read_translation_merged = 1;
								MSHR.entry[mshr_index].l1_rq_index_depend_on_me.insert(RQ.entry[index].l1_rq_index);
							}

							if(RQ.entry[index].l1_wq_index != -1)
							{
								assert((RQ.entry[index].l1_rq_index == -1) && (RQ.entry[index].l1_pq_index == -1));
								MSHR.entry[mshr_index].write_translation_merged = 1;
								MSHR.entry[mshr_index].l1_wq_index_depend_on_me.insert(RQ.entry[index].l1_wq_index);
							}

							if(RQ.entry[index].l1_pq_index != -1)
							{
								assert((RQ.entry[index].l1_wq_index == -1) && (RQ.entry[index].l1_rq_index == -1));
								MSHR.entry[mshr_index].prefetch_translation_merged = 1;
								MSHR.entry[mshr_index].l1_pq_index_depend_on_me.insert(RQ.entry[index].l1_pq_index);
							}
						}



						MSHR_MERGED[RQ.entry[index].type]++;

						DP ( if (warmup_complete[read_cpu]){
								cout << "[" << NAME << "] " << __func__ << " mshr merged";
								cout << " instr_id: " << RQ.entry[index].instr_id << " prior_id: " << MSHR.entry[mshr_index].instr_id; 
								cout << " address: " << hex << RQ.entry[index].address;
								cout << " full_addr: " << RQ.entry[index].full_addr << dec;
								if(MSHR.entry[mshr_index].read_translation_merged)
								cout << " read_translation_merged ";
								if(MSHR.entry[mshr_index].write_translation_merged)
								cout << " write_translation_merged ";
								if(MSHR.entry[mshr_index].prefetch_translation_merged)
								cout << " prefetch_translation_merged ";

								cout << " cycle: " << RQ.entry[index].event_cycle << endl; });
					}
					else { // WE SHOULD NOT REACH HERE
						cerr << "[" << NAME << "] MSHR errors" << endl;
						assert(0);
					}
				}

				if (miss_handled) {
					// update prefetcher on load instruction
					if(cpu >= ACCELERATOR_START && cache_type == IS_DTLB){
					dtlb_prefetcher_operate(RQ.entry[index].address<<LOG2_PAGE_SIZE, RQ.entry[index].ip, 1, RQ.entry[index].type, RQ.entry[index].instr_id, RQ.entry[index].instruction, RQ.entry[index].asid[0]);
				}
					if (RQ.entry[index].type == LOAD) {
						if (cache_type == IS_L1D) 
							l1d_prefetcher_operate(RQ.entry[index].full_addr, RQ.entry[index].ip, 0, RQ.entry[index].type);	//RQ.entry[index].instr_id);
						else if (cache_type == IS_L2C && RQ.entry[index].type != TRANSLATION )
							l2c_prefetcher_operate(RQ.entry[index].address<<LOG2_BLOCK_SIZE, RQ.entry[index].ip, 0, RQ.entry[index].type, 0);	// RQ.entry[index].instr_id);
						else if (cache_type == IS_LLC)
						{
							cpu = read_cpu;
							llc_prefetcher_operate(RQ.entry[index].address<<LOG2_BLOCK_SIZE, RQ.entry[index].ip, 0, RQ.entry[index].type, 0);	// RQ.entry[index].instr_id);
							cpu = 0;
						}
						else if (cache_type == IS_ITLB)
						{
							itlb_prefetcher_operate(RQ.entry[index].address<<LOG2_PAGE_SIZE, RQ.entry[index].ip, 0, RQ.entry[index].type, RQ.entry[index].instr_id, RQ.entry[index].instruction);

						}
						else if (cache_type == IS_DTLB)
						{
#ifdef SANITY_CHECK
							if(RQ.entry[index].instruction)
							{
								cout << "DTLB prefetch packet should not prefetch address translation of instruction " << endl;
								assert(0);
							}
#endif
							dtlb_prefetcher_operate(RQ.entry[index].address<<LOG2_PAGE_SIZE, RQ.entry[index].ip, 0, RQ.entry[index].type, RQ.entry[index].instr_id, RQ.entry[index].instruction, RQ.entry[index].asid[0]);

						}
						else if (cache_type == IS_STLB)
						{
							stlb_prefetcher_operate(RQ.entry[index].address<<LOG2_PAGE_SIZE, RQ.entry[index].ip, 0, RQ.entry[index].type, RQ.entry[index].instr_id, RQ.entry[index].instruction);

						}
					}

					MISS[RQ.entry[index].type]++;
					ACCESS[RQ.entry[index].type]++;
		/*			if( cache_type == IS_DTLB){
						DTLBPRINT(if( warmup_complete[cpu] ){
						cout << RQ.entry[index].instr_id<< " "   <<setw(10) << (RQ.entry[index].address)  << endl;
						})
					}*/

					// remove this entry from RQ
					STLBPRINT( if(cpu >= ACCELERATOR_START) cout << " removing from RQ from second fill " << NAME << " cpu " <<  RQ.entry[index].cpu << " rob " << RQ.entry[index].rob_index << endl;)
					if(STLB_FLAG)
						cout << "removed from RQ "  << endl;
				/*	if(cache_type == IS_DTLB && warmup_complete){
						cout << RQ.entry[index].instr_id << " " << RQ.entry[index].address << endl;
					}*/
					RQ.remove_queue(&RQ.entry[index]);
					reads_available_this_cycle--;
				}
			}
		}
		else
		{
			return;
		}

		if(reads_available_this_cycle == 0)
		{
			return;
		}
	}
}


void CACHE::handle_prefetch()
{
	// handle prefetch

	for (uint32_t i=0; i<MAX_READ; i++) {

		uint32_t prefetch_cpu = PQ.entry[PQ.head].cpu;
		if (prefetch_cpu == NUM_CPUS)
			return;

		// handle the oldest entry
		if ((PQ.entry[PQ.head].event_cycle <= current_core_cycle[prefetch_cpu]) && (PQ.occupancy > 0))
		{

			if(cache_type == IS_L1D && (PQ.entry[PQ.head].translated != COMPLETED)) //@Vishal: Check if the translation is done for that prefetch request or not.
			{	
				return;
			}

			int index = PQ.head;

			// access cache
			uint32_t set = get_set(PQ.entry[index].address);
			int way = check_hit(&PQ.entry[index]);

			if (way >= 0) { // prefetch hit

				// update replacement policy
				if (cache_type == IS_LLC) {
					llc_update_replacement_state(prefetch_cpu, set, way, block[set][way].full_addr, PQ.entry[index].ip, 0, PQ.entry[index].type, 1);

				}
				else if (cache_type == IS_DTLB) {
					dtlb_update_replacement_state(prefetch_cpu, set, way, block[set][way].full_addr, PQ.entry[index].ip, 0, PQ.entry[index].type, 1);
				}
				else if (cache_type == IS_STLB) {
					stlb_update_replacement_state(prefetch_cpu, set, way, block[set][way].full_addr, PQ.entry[index].ip, 0, PQ.entry[index].type, 1);
				}
				else
					update_replacement_state(prefetch_cpu, set, way, block[set][way].full_addr, PQ.entry[index].ip, 0, PQ.entry[index].type, 1);

				// COLLECT STATS
				sim_hit[prefetch_cpu][PQ.entry[index].type]++;
				sim_access[prefetch_cpu][PQ.entry[index].type]++;

				// run prefetcher on prefetches from higher caches
				if(PQ.entry[index].pf_origin_level < fill_level)
				{
					if (cache_type == IS_L1D)
					{
						//@Vishal: This should never be executed as fill_level of L1 is 1 and minimum pf_origin_level is 1
						assert(0);
						l1d_prefetcher_operate(PQ.entry[index].full_addr, PQ.entry[index].ip, 1, PREFETCH);	//, PQ.entry[index].prefetch_id);
					}
					else if (cache_type == IS_L2C && PQ.entry[index].type != TRANSLATION )
						PQ.entry[index].pf_metadata = l2c_prefetcher_operate(block[set][way].address<<LOG2_BLOCK_SIZE, PQ.entry[index].ip, 1, PREFETCH, PQ.entry[index].pf_metadata);	//PQ.entry[index].prefetch_id);
					else if (cache_type == IS_LLC)
					{
						cpu = prefetch_cpu;
						PQ.entry[index].pf_metadata = llc_prefetcher_operate(block[set][way].address<<LOG2_BLOCK_SIZE, PQ.entry[index].ip, 1, PREFETCH, PQ.entry[index].pf_metadata);	// PQ.entry[index].prefetch_id);
						cpu = 0;
					}
					else if (cache_type == IS_ITLB)
					{
						itlb_prefetcher_operate(PQ.entry[index].address<<LOG2_PAGE_SIZE, PQ.entry[index].ip, 0, PQ.entry[index].type, PQ.entry[index].prefetch_id, PQ.entry[index].instruction);
						DP ( if (warmup_complete[PQ.entry[index].cpu] ) {
								cout << "[" << NAME << "_PQ] " <<  __func__ << " prefetch_id: " << PQ.entry[index].prefetch_id << "from handle prefetch on prefetch hit" << "instruction : " << PQ.entry[index].instruction << endl; });
					}
					else if (cache_type == IS_DTLB)
					{
#ifdef SANITY_CHECK
						if(PQ.entry[index].instruction)
						{
							cout << "DTLB prefetch packet should not prefetch address translation of instruction " << endl;
							assert(0);
						}
#endif
						dtlb_prefetcher_operate(PQ.entry[index].address<<LOG2_PAGE_SIZE, PQ.entry[index].ip, 0, PQ.entry[index].type, PQ.entry[index].prefetch_id, PQ.entry[index].instruction,PQ.entry[index].asid[0]);
						DP ( if (warmup_complete[PQ.entry[index].cpu] ) {
								cout << "[" << NAME << "_PQ] " <<  __func__ << " prefetch_id: " << PQ.entry[index].prefetch_id << "from handle prefetch on prefetch hit" << "instruction : "<< PQ.entry[index].instruction << endl;});
					}
					else if (cache_type == IS_STLB)
					{
						stlb_prefetcher_operate(PQ.entry[index].address<<LOG2_PAGE_SIZE, PQ.entry[index].ip, 0, PQ.entry[index].type, PQ.entry[index].prefetch_id, PQ.entry[index].instruction);
						DP ( if (warmup_complete[PQ.entry[index].cpu] ) {
								cout << "[" << NAME << "_PQ] " <<  __func__ << " prefetch_id: " << PQ.entry[index].prefetch_id << "from handle prefetch on prefetch hit" << "instruction " << PQ.entry[index].instruction << "  address: "<< PQ.entry[index].address << endl;});
					}
				}

				// check fill level
				// data should be updated (for TLBs) in case of hit
				if (PQ.entry[index].fill_level < fill_level) {

					if(cache_type == IS_STLB)
					{
						if(PQ.entry[index].stlb_merged)
                                                        handle_stlb_merged_translations(&PQ.entry[index],block[set][way].data,true);
						if(PQ.entry[index].send_both_tlb)
						{
							PQ.entry[index].data = block[set][way].data;
							upper_level_icache[PQ.entry[index].cpu]->return_data(&PQ.entry[index]);
							upper_level_dcache[PQ.entry[index].cpu]->return_data(&PQ.entry[index]);
						}
						else if (PQ.entry[index].instruction)
						{
							PQ.entry[index].data = block[set][way].data;
							upper_level_icache[PQ.entry[index].cpu]->return_data(&PQ.entry[index]);
						}
						else // data
						{
							PQ.entry[index].data = block[set][way].data;
							upper_level_dcache[PQ.entry[index].cpu]->return_data(&PQ.entry[index]);
						}

#ifdef SANITY_CHECK
						if(PQ.entry[index].data == 0)
							assert(0);
#endif
					}
					else
					{
						if (PQ.entry[index].instruction)
						{
							PQ.entry[index].data = block[set][way].data; 
							upper_level_icache[prefetch_cpu]->return_data(&PQ.entry[index]);
						}
						else // data
						{
							PQ.entry[index].data = block[set][way].data;
							upper_level_dcache[prefetch_cpu]->return_data(&PQ.entry[index]);
						}

#ifdef SANITY_CHECK
						if(cache_type == IS_ITLB || cache_type == IS_DTLB)
							if(PQ.entry[index].data == 0)
								assert(0);
#endif
					}
				}

#ifdef PUSH_DTLB_PB
				//@Vasudha: In case of STLB prefetch hit, just fill DTLB Prefetch Buffer
				if( cache_type == IS_STLB && PQ.entry[index].fill_level == fill_level && PQ.entry[index].instruction == 0)
				{
					PQ.entry[index].data = block[set][way].data;
					//Find in which way to fill the translation
					uint32_t victim_way;
					victim_way = ooo_cpu[prefetch_cpu].DTLB_PB.find_victim( prefetch_cpu, PQ.entry[index].instr_id, 0, ooo_cpu[prefetch_cpu].DTLB_PB.block[0] , PQ.entry[index].ip, PQ.entry[index].full_addr, PQ.entry[index].type);
					ooo_cpu[prefetch_cpu].DTLB_PB.update_replacement_state(prefetch_cpu, 0, victim_way, PQ.entry[index].full_addr, PQ.entry[index].ip, ooo_cpu[prefetch_cpu].DTLB_PB.block[0][victim_way].full_addr, PQ.entry[index].type, 0);
					ooo_cpu[PQ.entry[index].cpu].DTLB_PB.fill_cache( 0, victim_way, &PQ.entry[index] );
				}
#endif

				HIT[PQ.entry[index].type]++;
				ACCESS[PQ.entry[index].type]++;

				// remove this entry from PQ
				PQ.remove_queue(&PQ.entry[index]);
				reads_available_this_cycle--;
			}
			else { // prefetch miss

				DP ( if (warmup_complete[prefetch_cpu] ) {
						cout << "[" << NAME << "] " << __func__ << " prefetch miss";
						cout << " instr_id: " << PQ.entry[index].prefetch_id << " address: " << hex << PQ.entry[index].address;
						cout << " full_addr: " << PQ.entry[index].full_addr << dec << " fill_level: " << PQ.entry[index].fill_level;
						cout << " cycle: " << PQ.entry[index].event_cycle << endl; });

				// check mshr
				uint8_t miss_handled = 1;
				int mshr_index = check_nonfifo_queue(&MSHR, &PQ.entry[index],false); //@Vishal: Updated from check_mshr

				if ((mshr_index == -1) && (MSHR.occupancy < MSHR_SIZE)) { // this is a new miss

					//Neelu: Calculting number of prefetches issued from L1D to L2C i.e. the next level
					if(cache_type == IS_L1D)
						prefetch_count++;

					++pf_lower_level;	//@v Increment for new prefetch miss

					DP ( if (warmup_complete[PQ.entry[index].cpu] ) {
							cout << "[" << NAME << "_PQ] " <<  __func__ << " want to add prefetch_id: " << PQ.entry[index].prefetch_id << " address: " << hex << PQ.entry[index].address;
							cout << " full_addr: " << PQ.entry[index].full_addr << dec;
							if(lower_level)
							cout << " occupancy: " << lower_level->get_occupancy(3, PQ.entry[index].address) << " SIZE: " << lower_level->get_size(3, PQ.entry[index].address) << endl; });

					// first check if the lower level PQ is full or not
					// this is possible since multiple prefetchers can exist at each level of caches
					if (lower_level) {
						if (cache_type == IS_LLC) {
							if (lower_level->get_occupancy(1, PQ.entry[index].address) == lower_level->get_size(1, PQ.entry[index].address))
								miss_handled = 0;
							else {

								// run prefetcher on prefetches from higher caches
								if(PQ.entry[index].pf_origin_level < fill_level)
								{
									if (cache_type == IS_LLC)
									{
										cpu = prefetch_cpu;
										PQ.entry[index].pf_metadata = llc_prefetcher_operate(PQ.entry[index].address<<LOG2_BLOCK_SIZE, PQ.entry[index].ip, 0, PREFETCH, PQ.entry[index].pf_metadata);	// PQ.entry[index].prefetch_id);
										cpu = 0;
									}
								}

								// add it to MSHRs if this prefetch miss will be filled to this cache level
								if (PQ.entry[index].fill_level <= fill_level)
									add_nonfifo_queue(&MSHR, &PQ.entry[index]); //@Vishal: Updated from add_mshr

								lower_level->add_rq(&PQ.entry[index]); // add it to the DRAM RQ
							}
						}
						else {
							if (lower_level->get_occupancy(3, PQ.entry[index].address) == lower_level->get_size(3, PQ.entry[index].address))
								miss_handled = 0;
							else {

								// run prefetcher on prefetches from higher caches
								if(PQ.entry[index].pf_origin_level < fill_level)
								{
									if (cache_type == IS_L1D)
										l1d_prefetcher_operate(PQ.entry[index].full_addr, PQ.entry[index].ip, 0, PREFETCH);	// PQ.entry[index].prefetch_id);
									else if (cache_type == IS_L2C && PQ.entry[index].type != TRANSLATION )
										PQ.entry[index].pf_metadata = l2c_prefetcher_operate(PQ.entry[index].address<<LOG2_BLOCK_SIZE, PQ.entry[index].ip, 0, PREFETCH, PQ.entry[index].pf_metadata);	// PQ.entry[index].prefetch_id);
									else if (cache_type == IS_ITLB)
									{
										itlb_prefetcher_operate(PQ.entry[index].address<<LOG2_PAGE_SIZE, PQ.entry[index].ip, 0, PQ.entry[index].type, PQ.entry[index].prefetch_id, PQ.entry[index].instruction);
										DP ( if (warmup_complete[PQ.entry[index].cpu] ) {
												cout << "[" << NAME << "_PQ] " <<  __func__ << " prefetch_id: " << PQ.entry[index].prefetch_id << "from handle prefetch" << "instruction: " << PQ.entry[index].instruction << endl; });
									}
									else if (cache_type == IS_DTLB)
									{
										cout << "yes I am called"<<endl;
#ifdef SANITY_CHECK
										if(PQ.entry[index].instruction)
										{
											cout << "DTLB prefetch packet should not prefetch address translation of instruction" << endl;
											assert(0);
										}
#endif
										dtlb_prefetcher_operate(PQ.entry[index].address<<LOG2_PAGE_SIZE, PQ.entry[index].ip, 0, PQ.entry[index].type, PQ.entry[index].prefetch_id, PQ.entry[index].instruction, PQ.entry[index].asid[0]);
										DP ( if (warmup_complete[PQ.entry[index].cpu] ) {
												cout << "[" << NAME << "_PQ] " <<  __func__ << " prefetch_id: " << PQ.entry[index].prefetch_id << "from handle prefetch" << "instruction: " << PQ.entry[index].instruction << endl; });
									}
									else if (cache_type == IS_STLB)
									{
										stlb_prefetcher_operate(PQ.entry[index].address<<LOG2_PAGE_SIZE, PQ.entry[index].ip, 0, PQ.entry[index].type, PQ.entry[index].prefetch_id, PQ.entry[index].instruction);
										DP ( if (warmup_complete[PQ.entry[index].cpu] ) {
												cout << "[" << NAME << "_PQ] " <<  __func__ << " prefetch_id: " << PQ.entry[index].prefetch_id << "from handle prefetch" <<  " instruction : "<< PQ.entry[index].instruction << endl; });
									}
								}

								if(cache_type == IS_L1D)
								{
									assert(PQ.entry[index].full_physical_address != 0);
									PACKET new_packet = PQ.entry[index];
									//@Vishal: Send physical address to lower level and track physical address in MSHR  
									new_packet.address = PQ.entry[index].full_physical_address >> LOG2_BLOCK_SIZE;
									new_packet.full_addr = PQ.entry[index].full_physical_address; 

									if (PQ.entry[index].fill_level <= fill_level)
										add_nonfifo_queue(&MSHR, &new_packet); //@Vishal: Updated from add_mshr
									lower_level->add_pq(&new_packet);
								}
								else
								{

									// add it to MSHRs if this prefetch miss will be filled to this cache level
									if (PQ.entry[index].fill_level <= fill_level)
										add_nonfifo_queue(&MSHR, &PQ.entry[index]); //@Vishal: Updated from add_mshr

									lower_level->add_pq(&PQ.entry[index]); // add it to the DRAM RQ

								}
							}
						}
					}
					else {
#ifdef INS_PAGE_TABLE_WALKER
						assert(0);
#else

						if(PQ.entry[index].fill_level <= fill_level)
							add_nonfifo_queue(&MSHR, &PQ.entry[index]);
						if(cache_type == IS_STLB) {
							//emulate page table walk
							uint64_t pa = va_to_pa(PQ.entry[index].cpu, PQ.entry[index].instr_id, PQ.entry[index].full_addr, PQ.entry[index].address);
							PQ.entry[index].data = pa >> LOG2_PAGE_SIZE;
							PQ.entry[index].event_cycle = current_core_cycle[cpu];
							if(PQ.entry[index].l1_pq_index != -1)
							{
								assert(PQ.entry[index].l1_pq_index == -1 && PQ.entry[index].l1_wq_index == -1);
								PQ.entry[index].data_pa = pa >> LOG2_PAGE_SIZE;
								if(PROCESSED.occupancy < PROCESSED.SIZE)
									PROCESSED.add_queue(&PQ.entry[index]);
								else
									assert(0);
							}
							return_data(&PQ.entry[index]);
						}
#endif
					}
				}
				else {
					if ((mshr_index == -1) && (MSHR.occupancy == MSHR_SIZE)) { // not enough MSHR resource

						// TODO: should we allow prefetching with lower fill level at this case?

						// cannot handle miss request until one of MSHRs is available
						miss_handled = 0;
						STALL[PQ.entry[index].type]++;
					}
					else if (mshr_index != -1) { // already in-flight miss

						// no need to update request except fill_level
						// update fill_level
						if (PQ.entry[index].fill_level < MSHR.entry[mshr_index].fill_level)
							MSHR.entry[mshr_index].fill_level = PQ.entry[index].fill_level;


						MSHR_MERGED[PQ.entry[index].type]++;

						DP ( if (warmup_complete[prefetch_cpu] ) {
								cout << "[" << NAME << "] " << __func__ << " mshr merged";
								cout << " instr_id: " << PQ.entry[index].instr_id << " prior_id: " << MSHR.entry[mshr_index].instr_id; 
								cout << " address: " << hex << PQ.entry[index].address;
								cout << " full_addr: " << PQ.entry[index].full_addr << dec << " fill_level: " << MSHR.entry[mshr_index].fill_level;
								cout << " cycle: " << MSHR.entry[mshr_index].event_cycle << endl; });
					}
					else { // WE SHOULD NOT REACH HERE
						cerr << "[" << NAME << "] MSHR errors" << endl;
						assert(0);
					}
				}

				if (miss_handled) {

					DP ( if (warmup_complete[prefetch_cpu] ) {
							cout << "[" << NAME << "] " << __func__ << " prefetch miss handled";
							cout << " instr_id: " << PQ.entry[index].instr_id << " address: " << hex << PQ.entry[index].address;
							cout << " full_addr: " << PQ.entry[index].full_addr << dec << " fill_level: " << PQ.entry[index].fill_level;
							cout << " cycle: " << PQ.entry[index].event_cycle << endl; });

					MISS[PQ.entry[index].type]++;
					ACCESS[PQ.entry[index].type]++;

					// remove this entry from PQ
					PQ.remove_queue(&PQ.entry[index]);
					reads_available_this_cycle--;
				}
			}
		}
		else
		{
			return;
		}

		if(reads_available_this_cycle == 0)
		{
			return;
		}
	}
}
void CACHE::operate()
{
/*	if(cpu >= ACCELERATOR_START && (cache_type == IS_DTLB || cache_type == IS_STLB)){
		cout <<"cpu " << cpu <<  " cache type "<< NAME << " RQ occupancy " << RQ.occupancy << endl;
		RQ.queuePrint();
		cout << "cache type "<< NAME << " MSHR occupancy " << MSHR.occupancy << endl;
		MSHR.queuePrint();
		cout << "cache type "<< NAME << " pQ occupancy " << MSHR.occupancy << endl;
		PQ.queuePrint();
	}*/
/*		if(cpu >= ACCELERATOR_START )
		STLB_FLAG = true;
	if(cpu >= ACCELERATOR_START && cache_type == IS_ITLB)
		ITLB_FLAG = true;
	PRINT77(if(cpu >= ACCELERATOR_START ){
		cout << "****************************************************handle_fill " << NAME<< endl;
			});
*//*	if(cpu == 4){
                cout << "cache type "<< NAME << " RQ occupancy " << RQ.occupancy << endl;
                RQ.queuePrint();
                cout << "cache type "<< NAME << " MSHR occupancy " << MSHR.occupancy << endl;
                MSHR.queuePrint();
        }*/
/*	if(cpu == 1 && cache_type == IS_SCRATCHPAD) //&& (cache_type == IS_ITLB || && cache_type == IS_SCRATCHPAD)
*/	if(printFlag  &&  ((cpu == 0 && cache_type == IS_STLB) || (cpu == 5)))
		      {
		cout << "before start ======================================================================================== \n" ;
			cout <<"cpu " << cpu <<  " cache type "<< NAME << " RQ occupancy " << RQ.occupancy << endl;
		RQ.queuePrint();
		if(cache_type ==  IS_SCRATCHPAD){

			cout << "cache type "<< NAME << " MSHR_TRNAS occupancy " << MSHR_TRANSLATION.occupancy << endl;
			MSHR_TRANSLATION.queuePrint();
			cout << "cache type "<< NAME << " MSHR_DATA occupancy " << MSHR_DATA.occupancy << endl;
			MSHR_DATA.queuePrint();
		}
		else{
			cout << "cache type "<< NAME << " MSHR occupancy " << MSHR.occupancy << endl;
			MSHR.queuePrint();
		}
	}
	if(cpu >= ACCELERATOR_START && cache_type == IS_SCRATCHPAD){
		update_fill_cycle_scratchpad(1);
		update_fill_cycle_scratchpad(0);
		handle_scratchpad_mshr_translation(0); //handle MSHR_DATA
		handle_scratchpad_mshr_translation(1); //handle MSHR_TRANSLATION
	}
	else
		handle_fill();

/*	PRINT77(if(cpu >= ACCELERATOR_START && cache_type == IS_STLB){
			cout << "****************************************************handle_writeback"<< endl;
			})
*/	handle_writeback();
	reads_available_this_cycle = MAX_READ;

	//@Vishal: VIPT
	if(cache_type == IS_L1I || cache_type == IS_L1D)
		handle_processed();
	if(cpu >= ACCELERATOR_START && cache_type == IS_SCRATCHPAD)
		handle_read_scratchpad();
	else
		handle_read();

	if (PQ.occupancy && (reads_available_this_cycle > 0)){
		STLBPRINT(if(cpu >= ACCELERATOR_START && cache_type == IS_STLB){cout << "****************************************************handle_prefetch " << endl;})
			handle_prefetch();
	}

	if(PQ.occupancy && ((current_core_cycle[cpu] - PQ.entry[PQ.head].cycle_enqueued) > DEADLOCK_CYCLE))
	{
		cout << "cpu " << cpu << endl;
		cout << "cache type "<< NAME << " RQ occupancy " << RQ.occupancy << endl;
		RQ.queuePrint();
		cout << "cache type "<< NAME << " MSHR occupancy " << MSHR.occupancy << endl;
		MSHR.queuePrint();
		cout << "cache type "<< NAME << " PQ occupancy " << PQ.occupancy << endl;
		PQ.queuePrint();
		cout << "DEADLOCK, PQ entry is not serviced for " << DEADLOCK_CYCLE << " cycles" << endl;
		cout << PQ.entry[PQ.head];	
		assert(0);
	}
	STLB_FLAG = false;
	ITLB_FLAG = false;
/*	if(cpu == 1 && cache_type == IS_SCRATCHPAD) //&& (cache_type == IS_ITLB ||&& cache_type == IS_SCRATCHPAD
*//*	if(printFlag && ((cpu == 0 && cache_type == IS_STLB) || (cpu == 0)))
	{
		cout << "after  ======================================================================================== \n" ;
			cout <<"cpu " << cpu <<  " cache type "<< NAME << " RQ occupancy " << RQ.occupancy << endl;
		RQ.queuePrint();
		if(cache_type ==  IS_SCRATCHPAD){

			cout << "cache type "<< NAME << " MSHR_TRNAS occupancy " << MSHR_TRANSLATION.occupancy << endl;
			MSHR_TRANSLATION.queuePrint();
			cout << "cache type "<< NAME << " MSHR_DATA occupancy " << MSHR_DATA.occupancy << endl;
			MSHR_DATA.queuePrint();
		}
		else{
			cout << "cache type "<< NAME << " MSHR occupancy " << MSHR.occupancy << endl;
			MSHR.queuePrint();
		}
	}*/
/*	if(cpu >=  ACCELERATOR_START && (cache_type == IS_ITLB || cache_type == IS_DTLB)){
		cout << "cache type "<< NAME << " PRocess occupancy " << RQ.occupancy << endl;
		PROCESSED.queuePrint();
	}*/
}

uint32_t CACHE::get_set(uint64_t address)
{
#ifdef PUSH_DTLB_PB
	if(cache_type == IS_DTLB_PB)
		return 0;

	else
#endif
	if (knob_cloudsuite  && (cache_type == IS_DTLB || cache_type == IS_ITLB || cache_type == IS_STLB))
                return (uint32_t) ((address >> 9) & ((1 << lg2(NUM_SET)) - 1));
	else
		return (uint32_t) (address & ((1 << lg2(NUM_SET)) - 1)); 
}

uint32_t CACHE::get_way(uint64_t address, uint32_t set)
{
	for (uint32_t way=0; way<NUM_WAY; way++) {
		if (block[set][way].valid && (block[set][way].tag == address)) 
			return way;
	}

	return NUM_WAY;
}

void CACHE::fill_cache(uint32_t set, uint32_t way, PACKET *packet)
{
/*	if(cache_type == IS_PSCL2){
		cout << "filling cache for " << NAME << " way  set " << way << " " << set << endl;
	}*/
#ifdef SANITY_CHECK

#ifdef PUSH_DTLB_PB
	if(cache_type == IS_DTLB_PB) {
		if(packet->data == 0)
		{
			cout << "Inside DTLB_PB, current = " << current_core_cycle[cpu] << " instr_id = " << packet->instr_id << endl;
			assert(0);
		}
	}
#endif
	if (cache_type == IS_ITLB) {
		if (packet->data == 0)
		{
			cout << "current = " << current_core_cycle[cpu] << " instr_id = "<< packet->instr_id << endl;
			assert(0);
		}
	}

	if (cache_type == IS_DTLB) {
		if (packet->data == 0)
		{
			cout << "current = " << current_core_cycle[cpu] << " instr_id = "<< packet->instr_id << endl;
			assert(0);
		}
	}

	if (cache_type == IS_STLB) {
		if (packet->data == 0)
			assert(0);
	}

	if (cache_type == IS_PSCL5) {
		if (packet->data == 0){
			cout << "Inside DTLB_PB,cpu " << cpu << "  current = " << current_core_cycle[cpu] << " instr_id = " << packet->instr_id << endl;
			assert(0);
		}
	}

	if (cache_type == IS_PSCL4) {
		if (packet->data == 0)
			assert(0);
	}

	if (cache_type == IS_PSCL3) {
		if (packet->data == 0)
			assert(0);
	}

	if (cache_type == IS_PSCL2) {
		if (packet->data == 0)
			assert(0);
	}
#endif
	if (block[set][way].prefetch && (block[set][way].used == 0))
		pf_useless++;

	if (block[set][way].valid == 0)
		block[set][way].valid = 1;
	block[set][way].dirty = 0;
	block[set][way].prefetch = (packet->type == PREFETCH) ? 1 : 0;
	block[set][way].used = 0;

	//Neelu: setting IPCP prefetch class
	block[set][way].pref_class = ((packet->pf_metadata & PREF_CLASS_MASK) >> NUM_OF_STRIDE_BITS);

	if (block[set][way].prefetch) 
	{
		pf_fill++;

		//Neelu: IPCP prefetch stats
		if(cache_type == IS_L1D)
		{
			if(block[set][way].pref_class < 5)						                     
			{
				pref_filled[cpu][block[set][way].pref_class]++;
			}
		}
	}

	block[set][way].delta = packet->delta;
	block[set][way].depth = packet->depth;
	block[set][way].signature = packet->signature;
	block[set][way].confidence = packet->confidence;

	block[set][way].tag = packet->address; //@Vishal: packet->address will be physical address for L1I, as it is only filled on a miss.
	block[set][way].address = packet->address;
	block[set][way].full_addr = packet->full_addr;
	block[set][way].data = packet->data;
	block[set][way].cpu = packet->cpu;
	block[set][way].instr_id = packet->instr_id;
//	cout << "address for block " << block[set][way].address << " instr id " << block[set][way].instr_id << endl;

	DP ( if (warmup_complete[packet->cpu] ) {
			cout << "[" << NAME << "] " << __func__ << " set: " << set << " way: " << way;
			cout << " lru: " << block[set][way].lru << " tag: " << hex << block[set][way].tag << " full_addr: " << block[set][way].full_addr;
			cout << " data: " << block[set][way].data << dec << endl; });
}

int CACHE::check_hit(PACKET *packet)
{
	uint32_t set = get_set(packet->address);
	int match_way = -1;

	if (NUM_SET < set) {
		cerr << "[" << NAME << "_ERROR] " << __func__ << " invalid set index: " << set << " NUM_SET: " << NUM_SET;
		cerr << " address: " << hex << packet->address << " full_addr: " << packet->full_addr << dec;
		cerr << " event: " << packet->event_cycle << endl;
		assert(0);
	}

	uint64_t packet_tag;
	if(cache_type == IS_L1I || cache_type == IS_L1D) //@Vishal: VIPT
	{
		assert(packet->full_physical_address != 0);
		packet_tag = packet->full_physical_address >> LOG2_BLOCK_SIZE;
	}
	else
		packet_tag = packet->address;

	// hit
	for (uint32_t way=0; way<NUM_WAY; way++) {
		if (block[set][way].valid && (block[set][way].tag == packet_tag)) {

			match_way = way;

			DP ( if (warmup_complete[packet->cpu] ) {
					cout << "[" << NAME << "] " << __func__ << " instr_id: " << packet->instr_id << " type: " << +packet->type << hex << " addr: " << packet->address;
					cout << " full_addr: " << packet->full_addr << " tag: " << block[set][way].tag << " data: " << block[set][way].data << dec;
					cout << " set: " << set << " way: " << way << " lru: " << block[set][way].lru;
					cout << " event: " << packet->event_cycle << " cycle: " << current_core_cycle[cpu] << endl; });

			break;
		}
	}

#ifdef PRINT_QUEUE_TRACE
	if(packet->instr_id == QTRACE_INSTR_ID)
	{
		cout << "[" << NAME << "] " << __func__ << " instr_id: " << packet->instr_id << " type: " << +packet->type << hex << " addr: " << packet->address;
		cout << " full_addr: " << packet->full_addr<<dec;
		cout << " set: " << set << " way: " << match_way;
		cout << " event: " << packet->event_cycle << " cycle: " << current_core_cycle[cpu]<<" cpu: "<<cpu<< endl;
	}
#endif



	return match_way;
}

int CACHE::invalidate_entry(uint64_t inval_addr)
{
	uint32_t set = get_set(inval_addr);
	int match_way = -1;

	if (NUM_SET < set) {
		cerr << "[" << NAME << "_ERROR] " << __func__ << " invalid set index: " << set << " NUM_SET: " << NUM_SET;
		cerr << " inval_addr: " << hex << inval_addr << dec << endl;
		assert(0);
	}

	// invalidate
	for (uint32_t way=0; way<NUM_WAY; way++) {
		if (block[set][way].valid && (block[set][way].tag == inval_addr)) {

			block[set][way].valid = 0;

			match_way = way;

			DP ( if (warmup_complete[cpu] ) {
					cout << "[" << NAME << "] " << __func__ << " inval_addr: " << hex << inval_addr;  
					cout << " tag: " << block[set][way].tag << " data: " << block[set][way].data << dec;
					cout << " set: " << set << " way: " << way << " lru: " << block[set][way].lru << " cycle: " << current_core_cycle[cpu] << endl; });

			break;
		}
	}

	return match_way;
}

void CACHE::flush_TLB()
{
	for(uint32_t set=0; set<NUM_SET; set++)
	{
		for(uint32_t way=0; way<NUM_WAY; way++)
		{
			block[set][way].valid = 0;
		}
	}
}


int CACHE::add_rq(PACKET *packet)
{
/*	if( cache_type == IS_DTLB){
		DTLBPRINT(if( warmup_complete[cpu] ){
				cout << packet->instr_id<< " "   <<setw(10) << (packet->address)  << endl;
				})
	}*/
//	if(cpu == 5 && packet->instr_id ==  50234 )
//		printFlag = true;
	// check for the latest wirtebacks in the write queue 
	// @Vishal: WQ is non-fifo for L1 cache
		bool flag = false;
/*	if(packet->instr_id == 106096 && cpu == 1){
		cout << "inside add_rq cpu " << cpu << " address " << packet->address  << " instrid " << packet->instr_id << " instruction " <<(int) packet->instruction  << endl;
		flag = true;
	}
*/	if(cache_type == IS_STLB){
		if (warmup_complete[cpu]){
			if(cpu >= ACCELERATOR_START){
				check_stlb_counter[ACCELERATOR_START]++;
			    packet->stlb_start_cycle = current_core_cycle[ACCELERATOR_START];
			    STLBPRINT(cout << "5cpu " << cpu << " simLat " << sim_latency[ACCELERATOR_START] << " start cycle " << packet->stlb_start_cycle << " current core cycle " << current_core_cycle[ACCELERATOR_START] << endl;)

			}
			else{
				check_stlb_counter[cpu]++;
				    packet->stlb_start_cycle = current_core_cycle[cpu];
			    STLBPRINT(cout << "6cpu " << cpu << " simLat " << sim_latency[cpu] << " start cycle " << packet->stlb_start_cycle << " current core cycle " << current_core_cycle[cpu] << endl;)
			
			}
		}
	    }
	int wq_index;
	if(cache_type == IS_L1D || cache_type == IS_L1I)
		wq_index = check_nonfifo_queue(&WQ,packet,false);
	else
		wq_index = WQ.check_queue(packet);

	if(flag)
		cout << "inside add_rq wq_index " << wq_index << endl;
	if (wq_index != -1) {

		// check fill level
		if (packet->fill_level < fill_level) {

			packet->data = WQ.entry[wq_index].data;
			if (packet->instruction) 
				upper_level_icache[packet->cpu]->return_data(packet);
			else // data
				upper_level_dcache[packet->cpu]->return_data(packet);
		}

#ifdef SANITY_CHECK
		if (cache_type == IS_ITLB)
			assert(0);
		else if (cache_type == IS_DTLB)
			assert(0);
		else if (cache_type ==  IS_STLB)
			assert(0);
		else if (cache_type == IS_L1I)
			assert(0);
#endif
		// update processed packets
		if (((cache_type == IS_L1D) || (cache_type == IS_SCRATCHPAD))  && (packet->type != PREFETCH)) {
			if (PROCESSED.occupancy < PROCESSED.SIZE)
				PROCESSED.add_queue(packet);

			DP ( if (warmup_complete[packet->cpu]) {
					cout << "[" << NAME << "_RQ] " << __func__ << " instr_id: " << packet->instr_id << " found recent writebacks";
					cout << hex << " read: " << packet->address << " writeback: " << WQ.entry[wq_index].address << dec;
					cout << " index: " << MAX_READ << " rob_signal: " << packet->rob_signal << endl; });
		}

		HIT[packet->type]++;
		ACCESS[packet->type]++;

		WQ.FORWARD++;
		RQ.ACCESS++;

		return -1;
	}

	// check for duplicates in the read queue
	// @Vishal: RQ is non-fifo for L1 cache

	int index;
	if(cache_type == IS_L1D || cache_type == IS_L1I)
		index = check_nonfifo_queue(&RQ,packet,false);
	/*  else if(cache_type == STLB && cpu > ACCELERATOR_START )
	    index = ooo_cpu[ACCELERATOR_START].STLB.RQ.check_queue(packet);*/
	else
		index = RQ.check_queue(packet);

	if(flag)
		cout << "inside add_rq rq_index " << index << endl;
	if (index != -1) {


		//@v send_both_tlb should be updated in STLB PQ if the entry needs to be serviced to both ITLB and DTLB
		if(cache_type == IS_STLB)
		{
			/* Fill level of incoming request and prefetch packet should be same else STLB prefetch request(with instruction=1) might get 			merged with DTLB/ITLB, making send_both_tlb=1 due to a msimatch in instruction variable. If this happens, data will be returned to 			both ITLB and DTLB, incurring MSHR miss*/

			if(RQ.entry[index].fill_level == 1 && packet->fill_level == 1)
			{
				if((RQ.entry[index].instruction != packet-> instruction) && RQ.entry[index].send_both_tlb == 0)
				{
					RQ.entry[index].send_both_tlb = 1;
				}
			}
		} 

		if(cache_type == IS_L2C)
		{
			if(RQ.entry[index].fill_level == 1 && packet->fill_level == 1)
			{
				if((RQ.entry[index].instruction != packet->instruction) && RQ.entry[index].send_both_cache == 0)
					RQ.entry[index].send_both_cache = 1;		
			}
		}

		if(packet->fill_level < RQ.entry[index].fill_level)
		{
			RQ.entry[index].fill_level = packet->fill_level;
		}



		if (packet->instruction) {
			uint32_t rob_index = packet->rob_index;
			RQ.entry[index].rob_index_depend_on_me.insert (rob_index);
			RQ.entry[index].instr_merged = 1;

			//@Vishal: ITLB read miss request is getting merged with pending DTLB read miss request. when completed send to both caches
			//@Vasudha: Done below
			//if(cache_type == IS_STLB)
			//	    RQ.entry[index].send_both_tlb = true;

			DP (if (warmup_complete[packet->cpu]) {
					cout << "[" << NAME << "_INSTR_MERGED] " << __func__ << " cpu: " << packet->cpu << " instr_id: " << RQ.entry[index].instr_id;
					cout << " merged rob_index: " << rob_index << " instr_id: " << packet->instr_id << endl; });

#ifdef PRINT_QUEUE_TRACE
			if(packet->instr_id == QTRACE_INSTR_ID)
			{
				cout << "["<<NAME<<"_INSTR_MERGED] " << __func__ << " cpu: " << packet->cpu << " instr_id: " << RQ.entry[index].instr_id;
				cout << " merged rob_index: " << rob_index << " instr_id: " << packet->instr_id << endl;
			}
#endif

			if(cpu < ACCELERATOR_START)
				if(cache_type == IS_ITLB || cache_type == IS_DTLB || cache_type == IS_STLB)
				{
					RQ.entry[index].read_translation_merged = true;
					assert(packet->l1_rq_index != -1);
					RQ.entry[index].l1_rq_index_depend_on_me.insert(packet->l1_rq_index);
				}
		}
		else 
		{
			if(packet->fill_level < fill_level)
			{
				RQ.entry[index].fill_level = packet->fill_level;
			}


			// mark merged consumer
			if((cache_type == IS_ITLB || cache_type == IS_DTLB || cache_type == IS_STLB) && (cpu < ACCELERATOR_START))
			{
				if (packet->l1_wq_index != -1) {
					RQ.entry[index].write_translation_merged = true;
					RQ.entry[index].l1_wq_index_depend_on_me.insert(packet->l1_wq_index);
				}
				else if(packet->l1_rq_index != -1){
					RQ.entry[index].read_translation_merged = true;
					RQ.entry[index].l1_rq_index_depend_on_me.insert(packet->l1_rq_index);

				}
				else if(packet->l1_pq_index != -1){
					assert(cache_type == IS_STLB);
					RQ.entry[index].prefetch_translation_merged = true;
					RQ.entry[index].l1_pq_index_depend_on_me.insert(packet->l1_pq_index);

				}
			}
			else
			{
				if(packet->type == RFO)
				{	
					uint32_t sq_index = packet->sq_index;
					RQ.entry[index].sq_index_depend_on_me.insert (sq_index);
					RQ.entry[index].store_merged = 1;
				}
				else {
					uint32_t lq_index = packet->lq_index; 
					RQ.entry[index].lq_index_depend_on_me.insert (lq_index);
					RQ.entry[index].load_merged = 1;
				}
			}



			//@Vishal: DTLB read miss request is getting merged with pending ITLB read miss request. when completed send to both caches
			//@Vasudha: Done below
			//if(cache_type == IS_STLB)
			//       RQ.entry[index].send_both_tlb = true;


			DP (if (warmup_complete[packet->cpu] ) {
					cout << "["<<NAME<<"_DATA_MERGED] " << __func__ << " cpu: " << packet->cpu << " instr_id: " << RQ.entry[index].instr_id;
					cout << " Fill level: " << RQ.entry[index].fill_level;
					if(RQ.entry[index].read_translation_merged)
					cout << " read_translation_merged ";
					if(RQ.entry[index].write_translation_merged)
					cout << " write_translation_merged ";
					if(RQ.entry[index].prefetch_translation_merged)
					cout << " prefetch_translation_merged ";

					cout << " merged rob_index: " << packet->rob_index << " instr_id: " << packet->instr_id << " lq_index: " << packet->lq_index << endl; });

#ifdef PRINT_QUEUE_TRACE
			if(packet->instr_id == QTRACE_INSTR_ID)
			{
				cout << "["<<NAME<<"_DATA_MERGED] " << __func__ << " cpu: " << packet->cpu << " instr_id: " << RQ.entry[index].instr_id;
				cout << " merged rob_index: " << packet->rob_index << " instr_id: " << packet->instr_id << " lq_index: " << packet->lq_index << endl;
			}
#endif
		}
		RQ.MERGED++;
		RQ.ACCESS++;

		return index; // merged index
	}
	//if(RQ.entry[index].instruction)
	//cout << NAME << " NEW READ packet inserted with instructions, address =  \n\n" << hex << packet->address << endl ; 
	// check occupancy
	if (RQ.occupancy == RQ_SIZE) {
		RQ.FULL++;

		return -2; // cannot handle this request
	}

	if(flag)
		cout << "inside add_rq adding to rq " << endl;
	bool translation_sent = false;
	int get_translation_index = -1;
	int get_translation_queue = IS_RQ;
	// if there is no duplicate, add it to RQ
	index = RQ.tail;

	//@Vishal: Since L1 RQ is non fifo, find empty index
	if(cache_type == IS_L1I || cache_type == IS_L1D)
	{
		for (uint i = 0; i < RQ.SIZE; i++)
			if(RQ.entry[i].address == 0)
			{
				index = i;
				break;
			}
	}

	if(flag)
		cout << "inside add_rq adding to rq_tail " << index<< endl;
	//@Vishal: Check if pending translation sent to TLB
	if(cache_type == IS_L1I || cache_type == IS_L1D)
	{

		if(cache_type == IS_L1I) // TODO: Check if extra interface can be used here?
		{
			if(ooo_cpu[packet->cpu].ITLB.RQ.occupancy == ooo_cpu[packet->cpu].ITLB.RQ.SIZE)
			{
				ooo_cpu[packet->cpu].ITLB.RQ.FULL++;
				return -2; // cannot handle this request because translation cannot be sent to TLB
			}

			PACKET translation_packet = *packet;
			translation_packet.instruction = 1;
			translation_packet.fill_level = FILL_L1;
			translation_packet.l1_rq_index = index;
			if(warmup_complete[cpu])
				translation_packet.tlb_start_cycle = current_core_cycle[cpu];

			if (knob_cloudsuite)
				translation_packet.address = ((packet->ip >> LOG2_PAGE_SIZE) << 9) | ( 256 + packet->asid[0]);
			else
				translation_packet.address = packet->ip >> LOG2_PAGE_SIZE;

			ooo_cpu[packet->cpu].ITLB.add_rq(&translation_packet);
		}
		else 
		{
			if(ooo_cpu[packet->cpu].DTLB.RQ.occupancy == ooo_cpu[packet->cpu].DTLB.RQ.SIZE)
			{
				ooo_cpu[packet->cpu].DTLB.RQ.FULL++;
				return -2; // cannot handle this request because translation cannot be sent to TLB
			}

			PACKET translation_packet = *packet;
			translation_packet.instruction = 0;
			translation_packet.fill_level = FILL_L1;
			translation_packet.l1_rq_index = index;
			if(warmup_complete[cpu])
				translation_packet.tlb_start_cycle = current_core_cycle[cpu];
			if (knob_cloudsuite)
				translation_packet.address = ((packet->full_addr >> LOG2_PAGE_SIZE) << 9) | packet->asid[1];
			else
				translation_packet.address = packet->full_addr >> LOG2_PAGE_SIZE;

			ooo_cpu[packet->cpu].DTLB.add_rq(&translation_packet);
		}
	}


#ifdef SANITY_CHECK
	if (RQ.entry[index].address != 0) {
		cerr << "[" << NAME << "_ERROR] " << __func__ << " is not empty index: " << index;
		cerr << " address: " << hex << RQ.entry[index].address;
		cerr << " full_addr: " << RQ.entry[index].full_addr << dec << endl;
		assert(0);
	}
#endif

	if(cache_type == IS_SCRATCHPAD)
		packet->is_translation = 1;
	RQ.entry[index] = *packet;

	if(flag){
		cout << "inside add_rq added to rq " << endl;
		RQ.queuePrint();
	}
	// ADD LATENCY
	if (RQ.entry[index].event_cycle < current_core_cycle[packet->cpu])
		RQ.entry[index].event_cycle = current_core_cycle[packet->cpu] + LATENCY;
	else
		RQ.entry[index].event_cycle += LATENCY;

	if(cache_type == IS_L1I || cache_type == IS_L1D)
	{
		RQ.entry[index].translated = INFLIGHT;
	}

	RQ.occupancy++;
	RQ.tail++;
	if (RQ.tail >= RQ.SIZE)
		RQ.tail = 0;

	DP ( if (warmup_complete[RQ.entry[index].cpu] ) {
			cout << "[" << NAME << "_RQ] " <<  __func__ << " instr_id: " << RQ.entry[index].instr_id << " address: " << hex << RQ.entry[index].address;
			cout << " full_addr: " << RQ.entry[index].full_addr << dec;
			cout << " type: " << +RQ.entry[index].type << " head: " << RQ.head << " tail: " << RQ.tail << " occupancy: " << RQ.occupancy;
			cout << " event: " << RQ.entry[index].event_cycle << " current: " << current_core_cycle[RQ.entry[index].cpu] << endl;});


#ifdef PRINT_QUEUE_TRACE
	if(packet->instr_id == QTRACE_INSTR_ID)
	{
		cout << "[" << NAME << "_RQ] " <<  __func__ << " instr_id: " << RQ.entry[index].instr_id << " address: " << hex << RQ.entry[index].address;
		cout << " full_addr: " << RQ.entry[index].full_addr << dec;
		cout << " type: " << +RQ.entry[index].type << " head: " << RQ.head << " tail: " << RQ.tail << " occupancy: " << RQ.occupancy;
		cout << " event: " << RQ.entry[index].event_cycle << " current: " << current_core_cycle[RQ.entry[index].cpu] << " cpu: "<<cpu<<endl;
	}
#endif


	if (packet->address == 0)
		assert(0);

	RQ.TO_CACHE++;
	RQ.ACCESS++;

	flag = false;
	return -1;
}

int CACHE::add_wq(PACKET *packet)
{

	assert(cache_type != IS_L1I || cache_type != IS_ITLB || cache_type != IS_DTLB || cache_type != IS_STLB); //@Vishal: L1I cache does not have write packets

	// check for duplicates in the write queue
	int index;
	if(cache_type == IS_L1D)
		index = check_nonfifo_queue(&WQ,packet,false);
	else 
		index = WQ.check_queue(packet);

	if (index != -1) {

		WQ.MERGED++;
		WQ.ACCESS++;

		return index; // merged index
	}

	// sanity check
	if (WQ.occupancy >= WQ.SIZE)
		assert(0);

	bool translation_sent = false;
	int get_translation_index = -1;
	int get_translation_queue = IS_RQ;

	// if there is no duplicate, add it to the write queue
	index = WQ.tail;

	//@Vishal: Since L1 WQ is non fifo, find empty index
	if(cache_type == IS_L1D)
	{
		for (uint i = 0; i < WQ.SIZE; i++)
			if(WQ.entry[i].address == 0)
			{
				index = i;
				break;
			}
	}

	//@Vishal: Check if pending translation sent to TLB
	if(cache_type == IS_L1D)
	{

		if(ooo_cpu[packet->cpu].DTLB.RQ.occupancy == ooo_cpu[packet->cpu].DTLB.RQ.SIZE)
		{
			ooo_cpu[packet->cpu].DTLB.RQ.FULL++;
			return -2; // cannot handle this request because translation cannotbe sent to TLB
		}
		PACKET translation_packet = *packet;
		translation_packet.instruction = 0;
		translation_packet.l1_wq_index = index;
		translation_packet.fill_level = FILL_L1;
		if(warmup_complete[cpu])
			translation_packet.tlb_start_cycle = current_core_cycle[cpu];
		if (knob_cloudsuite)
			translation_packet.address = ((packet->full_addr >> LOG2_PAGE_SIZE) << 9) | packet->asid[1];
		else
			translation_packet.address = packet->full_addr >> LOG2_PAGE_SIZE;

		ooo_cpu[packet->cpu].DTLB.add_rq(&translation_packet);
	}


	if (WQ.entry[index].address != 0) {
		cerr << "[" << NAME << "_ERROR] " << __func__ << " is not empty index: " << index;
		cerr << " address: " << hex << WQ.entry[index].address;
		cerr << " full_addr: " << WQ.entry[index].full_addr << dec << endl;
		assert(0);
	}

	WQ.entry[index] = *packet;

	// ADD LATENCY
	if (WQ.entry[index].event_cycle < current_core_cycle[packet->cpu])
		WQ.entry[index].event_cycle = current_core_cycle[packet->cpu] + LATENCY;
	else
		WQ.entry[index].event_cycle += LATENCY;

	if(cache_type == IS_L1D)
		WQ.entry[index].translated = INFLIGHT;

	WQ.occupancy++;
	WQ.tail++;
	if (WQ.tail >= WQ.SIZE)
		WQ.tail = 0;

	DP (if (warmup_complete[WQ.entry[index].cpu]) {
			cout << "[" << NAME << "_WQ] " <<  __func__ << " instr_id: " << WQ.entry[index].instr_id << " address: " << hex << WQ.entry[index].address;
			cout << " full_addr: " << WQ.entry[index].full_addr << dec;
			cout << " head: " << WQ.head << " tail: " << WQ.tail << " occupancy: " << WQ.occupancy;
			cout << " data: " << hex << WQ.entry[index].data << dec;
			cout << " event: " << WQ.entry[index].event_cycle << " current: " << current_core_cycle[WQ.entry[index].cpu] << endl;});


#ifdef PRINT_QUEUE_TRACE
	if(packet->instr_id == QTRACE_INSTR_ID)
	{
		cout << "[" << NAME << "_WQ] " <<  __func__ << " instr_id: " << WQ.entry[index].instr_id << " address: " << hex << WQ.entry[index].address;
		cout << " full_addr: " << WQ.entry[index].full_addr << dec;
		cout << " head: " << WQ.head << " tail: " << WQ.tail << " occupancy: " << WQ.occupancy;
		cout << " data: " << hex << WQ.entry[index].data << dec;
		cout << " event: " << WQ.entry[index].event_cycle << " current: " << current_core_cycle[WQ.entry[index].cpu] << " cpu: "<<cpu<<endl;
	}
#endif

	WQ.TO_CACHE++;
	WQ.ACCESS++;

	return -1;
}

int CACHE::prefetch_line(uint64_t ip, uint64_t base_addr, uint64_t pf_addr, int pf_fill_level, uint32_t prefetch_metadata) /*, uint64_t prefetch_id)*/		//Neelu: commented. 
{
	pf_requested++;
	DP ( if (warmup_complete[cpu]) {cout << "entered prefetch_line, occupancy = " << PQ.occupancy << "SIZE=" << PQ.SIZE << endl; });
	if (PQ.occupancy < PQ.SIZE) {
		DP ( if (warmup_complete[cpu]) {cout << "packet entered in PQ" << endl; });
		PACKET pf_packet;
		pf_packet.fill_level = pf_fill_level;
		pf_packet.pf_origin_level = fill_level;
		pf_packet.pf_metadata = prefetch_metadata;
		pf_packet.cpu = cpu;
		//pf_packet.data_index = LQ.entry[lq_index].data_index;
		//pf_packet.lq_index = lq_index;
		pf_packet.address = pf_addr >> LOG2_BLOCK_SIZE;
		pf_packet.full_addr = pf_addr;
		pf_packet.full_virtual_address = pf_addr;
		//pf_packet.instr_id = LQ.entry[lq_index].instr_id;
		//pf_packet.rob_index = LQ.entry[lq_index].rob_index;
		pf_packet.ip = ip;
		//pf_packet.prefetch_id = prefetch_id;		Neelu: commented, Vasudha was using for debugging. Assigning to zero for now.
		pf_packet.prefetch_id = 0; 
		pf_packet.type = PREFETCH;
		pf_packet.event_cycle = current_core_cycle[cpu];

		// give a dummy 0 as the IP of a prefetch
		add_pq(&pf_packet);
		DP ( if (warmup_complete[pf_packet.cpu]) {cout << "returned from add_pq" << endl; });
		pf_issued++;

		return 1;

	}

	return 0;
}

int CACHE::prefetch_translation(uint64_t ip, uint64_t pf_addr, int pf_fill_level, uint32_t prefetch_metadata, uint64_t prefetch_id, uint8_t instruction,uint8_t packet_cpu)
{
	pf_requested++;
	DP ( if (warmup_complete[cpu]) {cout << "entered prefetch_translation, occupancy = " << PQ.occupancy << "SIZE=" << PQ.SIZE << endl; });
	if (ooo_cpu[ACCELERATOR_START].PTW.PQ.occupancy < ooo_cpu[ACCELERATOR_START].PTW.PQ.SIZE) 
	{
		DP ( if (warmup_complete[cpu]) {cout << "packet entered in PQ" << endl; });
		PACKET pf_packet;
		pf_packet.fill_level = pf_fill_level;
		pf_packet.pf_origin_level = fill_level;
		pf_packet.pf_metadata = prefetch_metadata;
		pf_packet.cpu = packet_cpu;
		pf_packet.instruction = instruction;
		//pf_packet.data_index = LQ.entry[lq_index].data_index;
		//pf_packet.lq_index = lq_index;
		pf_packet.address = pf_addr >> LOG2_PAGE_SIZE;
		pf_packet.full_addr = pf_addr;
		pf_packet.full_virtual_address = pf_addr;
		//pf_packet.instr_id = LQ.entry[lq_index].instr_id;
		//pf_packet.rob_index = LQ.entry[lq_index].rob_index;
		pf_packet.ip = ip;
		pf_packet.prefetch_id = prefetch_id;
		pf_packet.type = PREFETCH;
		pf_packet.event_cycle = current_core_cycle[cpu];

		// give a dummy 0 as the IP of a prefetch
		if(cache_type == IS_PSCL2){
//			pf_packet.translation_level = 1; 
			ooo_cpu[ACCELERATOR_START].PTW.add_pq(&pf_packet);
		}
		else
			add_pq(&pf_packet);
		DP ( if (warmup_complete[pf_packet.cpu]) {cout << "returned from add_pq" << endl; });
		pf_issued++;

		return 1;
	}

	return 0;
}
int CACHE::prefetch_translation(uint64_t ip, uint64_t pf_addr, int pf_fill_level, uint32_t prefetch_metadata, uint64_t prefetch_id, uint8_t instruction)
{
	pf_requested++;
	DP ( if (warmup_complete[cpu]) {cout << "entered prefetch_translation, occupancy = " << PQ.occupancy << "SIZE=" << PQ.SIZE << endl; });
	if (ooo_cpu[ACCELERATOR_START].PTW.PQ.occupancy < ooo_cpu[ACCELERATOR_START].PTW.PQ.SIZE) 
	{
		DP ( if (warmup_complete[cpu]) {cout << "packet entered in PQ" << endl; });
		PACKET pf_packet;
		pf_packet.fill_level = pf_fill_level;
		pf_packet.pf_origin_level = fill_level;
		pf_packet.pf_metadata = prefetch_metadata;
		pf_packet.cpu = cpu;
		pf_packet.instruction = instruction;
		//pf_packet.data_index = LQ.entry[lq_index].data_index;
		//pf_packet.lq_index = lq_index;
		pf_packet.address = pf_addr >> LOG2_PAGE_SIZE;
		pf_packet.full_addr = pf_addr;
		pf_packet.full_virtual_address = pf_addr;
		//pf_packet.instr_id = LQ.entry[lq_index].instr_id;
		//pf_packet.rob_index = LQ.entry[lq_index].rob_index;
		pf_packet.ip = ip;
		pf_packet.prefetch_id = prefetch_id;
		pf_packet.type = PREFETCH;
		pf_packet.event_cycle = current_core_cycle[cpu];

		// give a dummy 0 as the IP of a prefetch
		if(cache_type == IS_PSCL2){
//			pf_packet.translation_level = 1; 
			ooo_cpu[ACCELERATOR_START].PTW.add_pq(&pf_packet);
		}
		else
			add_pq(&pf_packet);
		DP ( if (warmup_complete[pf_packet.cpu]) {cout << "returned from add_pq" << endl; });
		pf_issued++;

		return 1;
	}

	return 0;
}

int CACHE::kpc_prefetch_line(uint64_t base_addr, uint64_t pf_addr, int pf_fill_level, int delta, int depth, int signature, int confidence, uint32_t prefetch_metadata)
{

	assert(0); //@Vishal: This should not be called


	if (PQ.occupancy < PQ.SIZE) {

		PACKET pf_packet;
		pf_packet.fill_level = pf_fill_level;
		pf_packet.pf_origin_level = fill_level;
		pf_packet.pf_metadata = prefetch_metadata;
		pf_packet.cpu = cpu;
		//pf_packet.data_index = LQ.entry[lq_index].data_index;
		//pf_packet.lq_index = lq_index;
		pf_packet.address = pf_addr >> LOG2_BLOCK_SIZE;
		pf_packet.full_addr = pf_addr;
		//pf_packet.instr_id = LQ.entry[lq_index].instr_id;
		//pf_packet.rob_index = LQ.entry[lq_index].rob_index;
		pf_packet.ip = 0;
		pf_packet.type = PREFETCH;
		pf_packet.delta = delta;
		pf_packet.depth = depth;
		pf_packet.signature = signature;
		pf_packet.confidence = confidence;
		pf_packet.event_cycle = current_core_cycle[cpu];

		if ((base_addr>>LOG2_PAGE_SIZE) == (pf_addr>>LOG2_PAGE_SIZE))
		{ 
			pf_packet.full_physical_address = pf_addr;
			pf_packet.translated = COMPLETED;
		}
		else
			pf_packet.full_physical_address = 0;

		// give a dummy 0 as the IP of a prefetch
		int return_val = add_pq(&pf_packet);

		if(return_val > -2) //@Vishal: In some cases, even if the PQ is empty, request cannot be serviced.
			pf_issued++;

		return 1;
	}

	return 0;
}

int CACHE::add_pq(PACKET *packet)
{

	// @Vishal: L1I cache does not send prefetch request
	assert(cache_type != IS_L1I);

	// check for the latest wirtebacks in the write queue 
	// @Vishal: WQ is non-fifo for L1 cache

	int wq_index;
	if(cache_type == IS_L1D || cache_type == IS_L1I)
		wq_index = check_nonfifo_queue(&WQ,packet,false);
	else
		wq_index = WQ.check_queue(packet);

	if (wq_index != -1) {


#ifdef SANITY_CHECK

		if(cache_type == IS_L1I || cache_type == IS_ITLB || cache_type == IS_DTLB || cache_type == IS_STLB)
			assert(0);

#endif


		// check fill level
		if (packet->fill_level < fill_level) {

			packet->data = WQ.entry[wq_index].data;
			if (packet->instruction) 
				upper_level_icache[packet->cpu]->return_data(packet);
			else // data
				upper_level_dcache[packet->cpu]->return_data(packet);
		}

		HIT[packet->type]++;
		ACCESS[packet->type]++;

		WQ.FORWARD++;
		PQ.ACCESS++;

		return -1;
	}

	// check for duplicates in the PQ
	int index = PQ.check_queue(packet);
	if (index != -1) {
		//@v send_both_tlb should be updated in STLB PQ if the entry needs to be serviced to both ITLB and DTLB
		if(cache_type == IS_STLB)
		{
			/* Fill level of incoming request and prefetch packet should be same else STLB prefetch request(with instruction=1) might get 			merged with DTLB/ITLB, making send_both_tlb=1 due to a msimatch in instruction variable. If this happens, data will be returned to 			both ITLB and DTLB, incurring MSHR miss*/

			if(PQ.entry[index].fill_level==1 && packet -> fill_level == 1)
			{
				if((PQ.entry[index].instruction != packet-> instruction) && PQ.entry[index].send_both_tlb == 0)
				{        PQ.entry[index].send_both_tlb = 1;
				}
			}
		}

		if (packet->fill_level < PQ.entry[index].fill_level)
		{
			PQ.entry[index].fill_level = packet->fill_level;
			PQ.entry[index].instruction = packet->instruction; 
		}

		PQ.MERGED++;
		PQ.ACCESS++;

		return index; // merged index
	}
	//if(PQ.entry[index].instruction)
	//cout << NAME << " NEW PREFETCH packet inserted with instructions, address =  \n\n" << hex << packet->address << endl ; 
	// check occupancy
	if (PQ.occupancy == PQ_SIZE) {
		PQ.FULL++;

		DP ( if (warmup_complete[packet->cpu]) {
				cout << "[" << NAME << "] cannot process add_pq since it is full" << endl; });
		return -2; // cannot handle this request
	}

	// if there is no duplicate, add it to PQ
	index = PQ.tail;

#ifdef SANITY_CHECK
	if (PQ.entry[index].address != 0) {
		cerr << "[" << NAME << "_ERROR] " << __func__ << " is not empty index: " << index;
		cerr << " address: " << hex << PQ.entry[index].address;
		cerr << " full_addr: " << PQ.entry[index].full_addr << dec << endl;
		assert(0);
	}
#endif


	bool translation_sent = false;
	int get_translation_index = -1;
	int get_translation_queue = IS_RQ;

	//@Vishal: Check if pending translation sent to TLB if its need to be translated
	if(cache_type == IS_L1D && packet->full_physical_address == 0)
	{
		if(ooo_cpu[packet->cpu].STLB.RQ.occupancy == ooo_cpu[packet->cpu].STLB.RQ.SIZE)
		{
			ooo_cpu[packet->cpu].STLB.RQ.FULL++;
			return -2; // cannot handle this request because translation cannot be sent to TLB
		}

		PACKET translation_packet = *packet;
		translation_packet.l1_pq_index = index;
		translation_packet.fill_level = FILL_L2;

		if (knob_cloudsuite)
			translation_packet.address = ((packet->full_addr >> LOG2_PAGE_SIZE) << 9) | packet -> asid[1]; //@Vishal: TODO Check this address, will be wrong when L1I prefetcher is used
		else
			translation_packet.address = packet->full_addr >> LOG2_PAGE_SIZE;

		//@Vasudha: To separate translation requests coming from L1D prefetcher and STLB prefetcher, change type to LOAD
		translation_packet.type = 0;
		//@Vishal: Add translation packet from PQ to L2 cache.
		ooo_cpu[packet->cpu].STLB.add_rq(&translation_packet);
	}


	PQ.entry[index] = *packet;
	PQ.entry[index].cycle_enqueued = current_core_cycle[cpu];

	//@Vasudha - if any TLB calls add_pq
	if(knob_cloudsuite  && (cache_type==IS_ITLB || cache_type==IS_DTLB || cache_type==IS_STLB))
	{
		if(PQ.entry[index].instruction == 1)
		{
			PQ.entry[index].address = ((packet->ip >> LOG2_PAGE_SIZE) << 9) | ( 256 + packet->asid[0]);
		}
		else
			PQ.entry[index].address = ((packet->full_addr >> LOG2_PAGE_SIZE) << 9) | packet -> asid[1];
	}

	// ADD LATENCY
	if (PQ.entry[index].event_cycle < current_core_cycle[packet->cpu])
		PQ.entry[index].event_cycle = current_core_cycle[packet->cpu] + LATENCY + 10;
	else
		PQ.entry[index].event_cycle += LATENCY ;


	if(cache_type == IS_L1D)
	{
		PQ.entry[index].translated = INFLIGHT;
	}


	PQ.occupancy++;
	PQ.tail++;
	if (PQ.tail >= PQ.SIZE)
		PQ.tail = 0;

	DP ( if (warmup_complete[PQ.entry[index].cpu] ) {
			cout << "[" << NAME << "_PQ] " <<  __func__ << " prefetch_id: " << PQ.entry[index].prefetch_id << " address: " << hex << PQ.entry[index].address;
			cout << " full_addr: " << PQ.entry[index].full_addr << dec;
			cout << " type: " << +PQ.entry[index].type << " head: " << PQ.head << " tail: " << PQ.tail << " occupancy: " << PQ.occupancy;
			cout << " event: " << PQ.entry[index].event_cycle << " current: " << current_core_cycle[PQ.entry[index].cpu] << endl; });

#ifdef PRINT_QUEUE_TRACE
	if(packet->instr_id == QTRACE_INSTR_ID)
	{
		cout << "[" << NAME << "_PQ] " <<  __func__ << " instr_id: " << PQ.entry[index].instr_id << " address: " << hex << PQ.entry[index].address;
		cout << " full_addr: " << PQ.entry[index].full_addr << dec;
		cout << " type: " << +PQ.entry[index].type << " head: " << PQ.head << " tail: " << PQ.tail << " occupancy: " << PQ.occupancy;
		cout << " event: " << PQ.entry[index].event_cycle << " current: " << current_core_cycle[PQ.entry[index].cpu] << endl;
	}
#endif

	if (packet->address == 0)
		assert(0);

	PQ.TO_CACHE++;
	PQ.ACCESS++;

	return -1;
}

void CACHE::return_data(PACKET *packet)
{
	// check MSHR information
	if(cache_type == IS_SCRATCHPAD){
		SCRATCHPADPRINT(cout << "inside return data for scratchpad  instrid " << packet->instr_id << " is translation " << packet->is_translation << " address " << packet->address << endl;)
		PACKET_QUEUE *temp_mshr = (packet->type == TRANSLATION) ? &MSHR_TRANSLATION : &MSHR_DATA;
		int mshr_index = check_nonfifo_queue(temp_mshr, packet, false); //@Vishal: Updated from check_mshr
		if (mshr_index == -1) {
			cout << "inside return data for scratchpad  instrid " << packet->instr_id << " is translation " << packet->is_translation << " address " << packet->address << " packet_type " << (int)packet->type << endl;
			temp_mshr->queuePrint();
			cerr << "[" << NAME << "_MSHR] " << cpu << "_cpu"  << __func__ << " prefetch_id: " << packet->prefetch_id << " cannot find a matching entry!";
			cerr << " full_addr: " << hex << packet->full_addr;
			cerr << " address: " << packet->address << dec;
			cerr << " event: " << packet->event_cycle << " current: " << current_core_cycle[packet->cpu] << endl;
			assert(0);
		}
		temp_mshr->num_returned++;
		temp_mshr->entry[mshr_index].data = packet->data;
		if(packet->type == TRANSLATION)
			temp_mshr->entry[mshr_index].is_translation = 0;
		if(packet->type != TRANSLATION)
			temp_mshr->entry[mshr_index].returned = COMPLETED; 

		// ADD LATENCY
		if (temp_mshr->entry[mshr_index].event_cycle < current_core_cycle[packet->cpu])
			temp_mshr->entry[mshr_index].event_cycle = current_core_cycle[packet->cpu] + LATENCY;
		else
			temp_mshr->entry[mshr_index].event_cycle += LATENCY;

		update_fill_cycle_scratchpad(packet->is_translation);

		return;

	}
	int mshr_index = check_nonfifo_queue(&MSHR, packet, true); //@Vishal: Updated from check_mshr

	SPRINT(	cout << "cpu " << cpu << " packet cpu " << packet-> cpu << " robid " << packet->rob_index<< endl;)
	// sanity check
	if (mshr_index == -1) {
		cout << "cpu " << cpu << " packet cpu " << packet-> cpu << " robid " << packet->rob_index <<" address "	<<packet->address << endl;
		cerr << "[" << NAME << "_MSHR] " << __func__ << " prefetch_id: " << packet->prefetch_id << " cannot find a matching entry!";
		cerr << " full_addr: " << hex << packet->full_addr;
		cerr << " address: " << packet->address << dec;
		cerr << " event: " << packet->event_cycle << " current: " << current_core_cycle[packet->cpu] << endl;
		assert(0);
	}

	// MSHR holds the most updated information about this request
	// no need to do memcpy
	MSHR.num_returned++;
	MSHR.entry[mshr_index].returned = COMPLETED;

	if(cache_type == IS_STLB)
	{
		packet->data >>= LOG2_PAGE_SIZE; //@Vishal: Remove last 12 bits from the data coming from PTW
	}

	MSHR.entry[mshr_index].data = packet->data;
	if(cache_type==IS_ITLB||cache_type==IS_DTLB||cache_type==IS_STLB)
	{
		if(MSHR.entry[mshr_index].data == 0)
		{
			cout << "return_data writes 0 in TLB.data\n";
			assert(0);
		}
	}
	if(cache_type == IS_ITLB || cache_type == IS_DTLB){
		auto fill_cpu = packet->cpu;
		if(fill_cpu > ACCELERATOR_START)
			fill_cpu = ACCELERATOR_START;
		if (warmup_complete[cpu] && packet->stlb_start_cycle > 0){
		/*	if(cpu >= ACCELERATOR_START){
				ooo_cpu[ACCELERATOR_START].STLB.total_lat_req[ACCELERATOR_START]++;
				ooo_cpu[ACCELERATOR_START].STLB.sim_latency[ACCELERATOR_START] += current_core_cycle[ACCELERATOR_START] - packet->stlb_start_cycle;
			}
			else{*/
			if(current_core_cycle[fill_cpu] - packet->stlb_start_cycle>0){
				ooo_cpu[fill_cpu].STLB.total_lat_req[fill_cpu]++;
				ooo_cpu[fill_cpu].STLB.sim_latency[fill_cpu] += current_core_cycle[fill_cpu] - packet->stlb_start_cycle;
			}
		//	}
		}
	}

	MSHR.entry[mshr_index].pf_metadata = packet->pf_metadata;

	// ADD LATENCY
	if (MSHR.entry[mshr_index].event_cycle < current_core_cycle[packet->cpu])
		MSHR.entry[mshr_index].event_cycle = current_core_cycle[packet->cpu] + LATENCY;
	else
		MSHR.entry[mshr_index].event_cycle += LATENCY;

	update_fill_cycle();

	DP (if (warmup_complete[packet->cpu] ) {
			cout << "[" << NAME << "_MSHR] " <<  __func__ << " instr_id: " << MSHR.entry[mshr_index].instr_id;
			cout << " address: " << hex << MSHR.entry[mshr_index].address << " full_addr: " << MSHR.entry[mshr_index].full_addr;
			cout << " data: " << MSHR.entry[mshr_index].data << dec << " num_returned: " << MSHR.num_returned;
			cout << " index: " << mshr_index << " occupancy: " << MSHR.occupancy;
			if(MSHR.entry[mshr_index].read_translation_merged)
			cout << " read_translation_merged ";
			else if(MSHR.entry[mshr_index].write_translation_merged)
			cout << " write_translation_merged ";
			else if(MSHR.entry[mshr_index].prefetch_translation_merged)
			cout << " prefetch_translation_merged ";

			cout << " event: " << MSHR.entry[mshr_index].event_cycle << " current: " << current_core_cycle[packet->cpu] << " next: " << MSHR.next_fill_cycle << endl; });

#ifdef PRINT_QUEUE_TRACE
	if(packet->instr_id == QTRACE_INSTR_ID)
	{
		cout << "[" << NAME << "_MSHR] " <<  __func__ << " instr_id: " << MSHR.entry[mshr_index].instr_id;
		cout << " address: " << hex << MSHR.entry[mshr_index].address << " full_addr: " << MSHR.entry[mshr_index].full_addr;
		cout << " data: " << MSHR.entry[mshr_index].data << dec << " num_returned: " << MSHR.num_returned;
		cout << " index: " << mshr_index << " occupancy: " << MSHR.occupancy;
		cout << " event: " << MSHR.entry[mshr_index].event_cycle << " current: " << current_core_cycle[packet->cpu] << " next: " << MSHR.next_fill_cycle << endl;

	}
#endif

}

void CACHE::update_fill_cycle()
{
	// update next_fill_cycle
	uint64_t min_cycle = UINT64_MAX;
	uint32_t min_index = MSHR.SIZE;
	for (uint32_t i=0; i<MSHR.SIZE; i++) {
		if ((MSHR.entry[i].returned == COMPLETED) && (MSHR.entry[i].event_cycle < min_cycle)) {
			min_cycle = MSHR.entry[i].event_cycle;
			min_index = i;
		}
		DP (if (warmup_complete[MSHR.entry[i].cpu] ) {
				cout << "[" << NAME << "_MSHR] " <<  __func__ << " checking instr_id: " << MSHR.entry[i].instr_id;
				cout << " address: " << hex << MSHR.entry[i].address << " full_addr: " << MSHR.entry[i].full_addr;
				cout << " data: " << MSHR.entry[i].data << dec << " returned: " << +MSHR.entry[i].returned << " fill_level: " << MSHR.entry[i].fill_level;
				cout << " index: " << i << " occupancy: " << MSHR.occupancy;
				cout << " event: " << MSHR.entry[i].event_cycle << " current: " << current_core_cycle[MSHR.entry[i].cpu] << " next: " << MSHR.next_fill_cycle << endl; });
	}

	MSHR.next_fill_cycle = min_cycle;
	MSHR.next_fill_index = min_index;
	if (min_index < MSHR.SIZE) {

		DP (if (warmup_complete[MSHR.entry[min_index].cpu] ) {
				cout << "[" << NAME << "_MSHR] " <<  __func__ << " instr_id: " << MSHR.entry[min_index].instr_id;
				cout << " address: " << hex << MSHR.entry[min_index].address << " full_addr: " << MSHR.entry[min_index].full_addr;
				cout << " data: " << MSHR.entry[min_index].data << dec << " num_returned: " << MSHR.num_returned;
				cout << " event: " << MSHR.entry[min_index].event_cycle << " current: " << current_core_cycle[MSHR.entry[min_index].cpu] << " next: " << MSHR.next_fill_cycle << endl;});
	}
}

//@Vishal: Made check_mshr generic; packet_direction (Required only for MSHR) =>true, going to lower level else coming from lower level
int CACHE::check_nonfifo_queue(PACKET_QUEUE *queue, PACKET *packet, bool packet_direction)
{
	uint64_t check_address = packet->address;
	//@Vishal: packet_direction will be true only for return_data function. We don't need to check address translation for that.
	if(!packet_direction && (cache_type == IS_L1I || cache_type == IS_L1D) && queue->NAME.compare(NAME+"_MSHR") == 0)
	{
		if(packet->full_physical_address == 0)
		{
			assert(packet->full_physical_address != 0); //@Vishal: If MSHR is checked, then address translation should be present 
		}

		if(packet->address != (packet->full_physical_address >> LOG2_BLOCK_SIZE))
			check_address = packet->full_physical_address >> LOG2_BLOCK_SIZE; //@Vishal: L1 MSHR has physical address
	}

	if(cache_type == IS_L1D && queue->NAME.compare(NAME+"_WQ") == 0)
	{
		// search queue
		for (uint32_t index=0; index < queue->SIZE; index++) {
			if (queue->entry[index].full_addr == packet->full_addr) {

				DP ( if (warmup_complete[packet->cpu]) {
						cout << "[" << NAME << "_" << queue->NAME << "] " << __func__ << " same entry instr_id: " << packet->instr_id << " prior_id: " << queue->entry[index].instr_id;
						cout << " address: " << hex << packet->address;
						cout << " full_addr: " << packet->full_addr << dec << endl; });

				return index;
			}
		}

	}
	else
	{
		// search queue
		for (uint32_t index=0; index < queue->SIZE; index++) {
			if (queue->entry[index].address == check_address  && (queue->entry[index].asid[0] == packet->asid[0])) {	//@Nilesh: added for accelerators address space 

				DP ( if (warmup_complete[packet->cpu]) {
						cout << "[" << NAME << "_" << queue->NAME << "] " << __func__ << " same entry instr_id: " << packet->instr_id << " prior_id: " << queue->entry[index].instr_id;
						cout << " address: " << hex << packet->address;
						cout << " full_addr: " << packet->full_addr << dec << endl; });
				if(cache_type == IS_STLB && (queue->entry[index].cpu != packet->cpu)){
					stlb_merged++;
					queue->entry[index].stlb_merged = true;		//@Nilesh: address space is shared among accelerators
					queue->entry[index].stlb_depends_on_me.push(*packet);
					if(packet->stlb_merged){
						while(!packet->stlb_depends_on_me.empty()){
							queue->entry[index].stlb_depends_on_me.push(packet->stlb_depends_on_me.front());
							packet->stlb_depends_on_me.pop();
						}		
					}
	/*				cout << "instruction merged " << queue->NAME << " with packet cpu " << queue->entry[index].cpu << " of packet cpu "<<  packet->cpu << " address " << packet->address <<endl;*/
					SPRINT(cout << "instruction merged " << queue->NAME << " with packet cpu " << queue->entry[index].cpu << " of packet cpu "<<  packet->cpu << endl;)
				}
				//@Vasudha: If packet from L1D and L1I gets merged in L2C MSHR
				if(cache_type == IS_L2C)
					if(queue->entry[index].fill_level == 1 && packet->fill_level == 1 && queue->entry[index].send_both_cache == 0)
						if(queue->entry[index].instruction != packet->instruction)
							queue->entry[index].send_both_cache = 1;
				return index;
			}
		}
	}

	DP ( if (warmup_complete[packet->cpu]) {
			cout << "[" << NAME << "_" << queue->NAME << "] " << __func__ << " new address: " << hex << packet->address;
			cout << " full_addr: " << packet->full_addr << dec << endl; });

	DP ( if (warmup_complete[packet->cpu] && (queue->occupancy == queue->SIZE)) { 
			cout << "[" << NAME << "_" << queue->NAME << "] " << __func__ << " mshr is full";
			cout << " instr_id: " << packet->instr_id << " occupancy: " << queue->occupancy;
			cout << " address: " << hex << packet->address;
			cout << " full_addr: " << packet->full_addr << dec;
			cout << " cycle: " << current_core_cycle[packet->cpu] << endl;});

	return -1;
}

int CACHE::add_mshr_data_queue_scratchpad(PACKET_QUEUE *queue, PACKET *packet){
	uint32_t mshr_index = -1;
	mshr_index = check_nonfifo_queue(queue,packet,false);
/*	if(cpu == 1 && packet->instr_id == 106096){
		cout << "inside add_mshr scratchpad  mshr index " << mshr_index << endl;
	}
*/	if(mshr_index != -1){
		PACKET_QUEUE * temp_mshr = queue;
		if (packet->type == RFO) {

			if (packet->tlb_access) {
				uint32_t sq_index = packet->sq_index;
				temp_mshr->entry[mshr_index].store_merged = 1;
				temp_mshr->entry[mshr_index].sq_index_depend_on_me.insert (sq_index);
				temp_mshr->entry[mshr_index].sq_index_depend_on_me.join (packet->sq_index_depend_on_me, SQ_SIZE);
			}

			if (packet->load_merged) {
				//uint32_t lq_index = RQ.entry[index].lq_index; 
				temp_mshr->entry[mshr_index].load_merged = 1;
				//MSHR.entry[mshr_index].lq_index_depend_on_me[lq_index] = 1;
				temp_mshr->entry[mshr_index].lq_index_depend_on_me.join (packet->lq_index_depend_on_me, LQ_SIZE);
			}

		}
		else {
			if (packet->instruction) {
				uint32_t rob_index = packet->rob_index;
				DP (if (warmup_complete[MSHR.entry[mshr_index].cpu] ) {
						//if(cache_type==IS_ITLB || cache_type==IS_DTLB || cache_type==IS_STLB)
						cout << "read request merged with MSHR entry -"<< MSHR.entry[mshr_index].type << endl; });
				temp_mshr->entry[mshr_index].instr_merged = 1;
				temp_mshr->entry[mshr_index].rob_index_depend_on_me.insert (rob_index);


				if (packet->instr_merged) {
					temp_mshr->entry[mshr_index].rob_index_depend_on_me.join (packet->rob_index_depend_on_me, ROB_SIZE);
				}
			}
			else 
			{
				uint32_t lq_index = packet->lq_index;
				temp_mshr->entry[mshr_index].load_merged = 1;
				temp_mshr->entry[mshr_index].lq_index_depend_on_me.insert (lq_index);

				temp_mshr->entry[mshr_index].lq_index_depend_on_me.join (packet->lq_index_depend_on_me, LQ_SIZE);
				if (packet->store_merged) {
					temp_mshr->entry[mshr_index].store_merged = 1;
					temp_mshr->entry[mshr_index].sq_index_depend_on_me.join (packet->sq_index_depend_on_me, SQ_SIZE);
				}
			}
		}

		// update fill_level

		MSHR_MERGED[packet->type]++;
		return -1;

	}
	if(MSHR_DATA.occupancy < MSHR_SIZE ){
		add_nonfifo_queue(queue,packet);
/*		if(cpu == 1 && packet->instr_id == 106096){
			cout << "inside add_mshr scratchpad  adding to data mshr "  << endl;
		}
*/		int dram_index = lower_level->add_rq(packet);
		if(dram_index == -2){
			queue->remove_queue(packet);
			return -2;
		}
		return -1;
	}
	else
		return -2;

}
//@Vishal: Made add_mshr generic
void CACHE::add_nonfifo_queue(PACKET_QUEUE *queue, PACKET *packet)
{
	uint32_t index = 0;

	packet->cycle_enqueued = current_core_cycle[packet->cpu];

	// search queue
	for (index=0; index < queue->SIZE; index++) {
		if (queue->entry[index].address == 0) {

			queue->entry[index] = *packet;
			queue->entry[index].returned = INFLIGHT;
			queue->occupancy++;

			DP ( if (warmup_complete[packet->cpu]) {
					cout << "[" << NAME << "_" << queue->NAME << "] " << __func__ << " instr_id: " << packet->instr_id;
					cout << " address: " << hex << packet->address << " full_addr: " << packet->full_addr << dec;
					if(packet->read_translation_merged)
					cout << " read_translation_merged ";
					else if(packet->write_translation_merged)
					cout << " write_translation_merged ";
					else if(packet->prefetch_translation_merged)
					cout << " prefetch_translation_merged ";
					cout << " fill_level: " << queue->entry[index].fill_level;
					cout << " index: " << index << " occupancy: " << queue->occupancy << endl; });


#ifdef PRINT_QUEUE_TRACE
			if(packet->instr_id == QTRACE_INSTR_ID)
			{
				cout << "[" << NAME << "_MSHR] " << __func__ << " instr_id: " << packet->instr_id;
				cout << " address: " << hex << packet->address << " full_addr: " << packet->full_addr << dec<<endl;
				cout << " index: " << index << " occupancy: " << MSHR.occupancy << " cpu: "<<cpu<<endl;
			}
#endif

			break;
		}
	}
}

uint32_t CACHE::get_occupancy(uint8_t queue_type, uint64_t address)
{
	if (queue_type == 0)
		return MSHR.occupancy;
	else if (queue_type == 1)
		return RQ.occupancy;
	else if (queue_type == 2)
		return WQ.occupancy;
	else if (queue_type == 3)
		return PQ.occupancy;

	return 0;
}

uint32_t CACHE::get_size(uint8_t queue_type, uint64_t address)
{
	if (queue_type == 0)
		return MSHR.SIZE;
	else if (queue_type == 1)
		return RQ.SIZE;
	else if (queue_type == 2)
		return WQ.SIZE;
	else if (queue_type == 3)
		return PQ.SIZE;

	return 0;
}

void CACHE::increment_WQ_FULL(uint64_t address)
{
	WQ.FULL++;
}

void CACHE::handle_stlb_merged_translations(PACKET *packet,uint64_t data, bool flag){
	packet->stlb_merged = false;
	queue<PACKET> dependent_packets = packet->stlb_depends_on_me;
	SPRINT(cout << "inside handle_stlb_merged_translations "<< packet->cpu << endl;)
	while( !dependent_packets.empty()){
		PACKET dependent_packet = dependent_packets.front();
		dependent_packets.pop();
		if(flag){
			dependent_packet.data = data;
		}
		else
			dependent_packet.data = packet -> data;
		SPRINT(cout << "packet cpu " << packet-> cpu  << " id " << packet->instr_id << " dependent_packet cpu " << dependent_packet.cpu << " id " << dependent_packet.instr_id << endl;)
		if (dependent_packet.instruction)
			upper_level_icache[dependent_packet.cpu]->return_data(&dependent_packet);
		else // data
			upper_level_dcache[dependent_packet.cpu]->return_data(&dependent_packet);
	}
}
