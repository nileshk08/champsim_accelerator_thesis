#include "ooo_cpu.h"
#include "uncore.h"

void PAGE_TABLE_WALKER::handle_fill(uint8_t mshr_type){

        PACKET_QUEUE *temp_mshr = mshr_type ? &MSHR : &PREFETCH_MSHR;
        uint8_t mshr_size = mshr_type ? PTW_MSHR_SIZE : PTW_PREFETCH_MSHR_SIZE;
//	if(MSHR.occupancy > 0) //Handle pending request, only one request is serviced at a time.
	for(int index = 0; index < mshr_size; index++){
		if((temp_mshr->entry[index].ip != 0) && (temp_mshr->entry[index].returned == COMPLETED) && (temp_mshr->entry[index].event_cycle <= current_core_cycle[cpu])) //Check if current level translation complete
		{
			//int index = temp_mshr->head;

			PRINT77(cout << "inside mshr complete  translation level " <<(int) temp_mshr->entry[index].translation_level << endl;) 
				assert(CR3_addr != UINT64_MAX);
			PAGE_TABLE_PAGE* curr_page = L5; //Start wth the L5 page
			uint64_t next_level_base_addr = UINT64_MAX;
			bool page_fault = false;

			for (int i = 5; i > temp_mshr->entry[index].translation_level; i--)
			{
				uint64_t offset = get_offset(temp_mshr->entry[index].full_virtual_address, i); //Get offset according to page table level
				assert(curr_page != NULL);
				next_level_base_addr = curr_page->next_level_base_addr[offset];
				if(next_level_base_addr == UINT64_MAX)
				{
					handle_page_fault(curr_page, &temp_mshr->entry[index], i); //i means next level does not exist.
					page_fault = true;
					temp_mshr->entry[index].translation_level = 0; //In page fault, All levels are translated.
					break;
				}
				curr_page = curr_page->entry[offset];
			}

			if(temp_mshr->entry[index].translation_level == 0) //If translation complete
			{
				curr_page = L5;
				next_level_base_addr = UINT64_MAX;
				for (int i = 5; i > 1; i--) //Walk the page table and fill MMU caches
				{
					uint64_t offset = get_offset(temp_mshr->entry[index].full_virtual_address, i);
					assert(curr_page != NULL);
					next_level_base_addr = curr_page->next_level_base_addr[offset];
					assert(next_level_base_addr != UINT64_MAX);
					curr_page = curr_page->entry[offset];

					if(temp_mshr->entry[index].init_translation_level - i >= 0) //Check which translation levels needs to filled
					{
						switch(i)
						{
							case 5: fill_mmu_cache(PSCL5, next_level_base_addr, &temp_mshr->entry[index], IS_PSCL5);
								break;
							case 4: fill_mmu_cache(PSCL4, next_level_base_addr, &temp_mshr->entry[index], IS_PSCL4);
								break;
							case 3: fill_mmu_cache(PSCL3, next_level_base_addr, &temp_mshr->entry[index], IS_PSCL3);
								break;
							case 2: fill_mmu_cache(PSCL2, next_level_base_addr, &temp_mshr->entry[index], IS_PSCL2);
								break;
						}
					}
				}

				uint64_t offset = get_offset(temp_mshr->entry[index].full_virtual_address, IS_PTL1);
				next_level_base_addr = curr_page->next_level_base_addr[offset];

				if(page_fault)
					temp_mshr->entry[index].event_cycle = stall_cycle[cpu]; //It is updated according to whether is required page swap or not.
				else
					temp_mshr->entry[index].event_cycle = current_core_cycle[cpu];	


				temp_mshr->entry[index].data = next_level_base_addr << LOG2_PAGE_SIZE | (temp_mshr->entry[index].full_virtual_address && ((1<<LOG2_PAGE_SIZE) - 1)); //Return the translated physical address to STLB

				if (knob_cloudsuite && cpu < ACCELERATOR_START )
				{
					if(temp_mshr->entry[index].instruction)
					{
						temp_mshr->entry[index].address = (( temp_mshr->entry[index].ip >> LOG2_PAGE_SIZE) << 9) | ( 256 + temp_mshr->entry[index].asid[0]);	
					}
					else
					{
						temp_mshr->entry[index].address = (( temp_mshr->entry[index].full_virtual_address >> LOG2_PAGE_SIZE) << 9) |  temp_mshr->entry[index].asid[1];
					}
				}
				else
				{
					temp_mshr->entry[index].address = temp_mshr->entry[index].full_virtual_address >> LOG2_PAGE_SIZE;
				}

				temp_mshr->entry[index].full_addr = temp_mshr->entry[index].full_virtual_address;
		//		SCRATCHPADPRINT(cout << "address in PTW MSHR " << temp_mshr->entry[index].address << " full_virt " << temp_mshr->entry[index].full_virtual_address << " instr id " << temp_mshr->entry[index].instr_id << " cpu " << temp_mshr->entry[index].cpu << endl;)

				PRINT77(cout << "return data from PTW 1"<< endl;)
					if (warmup_complete[cpu] && temp_mshr->entry[index].ptw_start_cycle > 0 && current_core_cycle[cpu] - temp_mshr->entry[index].ptw_start_cycle >= LATENCY){
						PTW_rq++;
						PTW_latency += current_core_cycle[cpu] - temp_mshr->entry[index].ptw_start_cycle ;
					}
				if (temp_mshr->entry[index].instruction)
					upper_level_icache[cpu]->return_data(&temp_mshr->entry[index]);
				else // data
					upper_level_dcache[cpu]->return_data(&temp_mshr->entry[index]);

				if(warmup_complete[cpu])
				{
					uint64_t current_miss_latency = (current_core_cycle[cpu] - temp_mshr->entry[index].cycle_enqueued);	
					total_miss_latency += current_miss_latency;
				}

				temp_mshr->remove_queue(&temp_mshr->entry[index]);
			}
			else
			{
				assert(!page_fault); //If page fault was there, then all levels of translation should have be done.


				PRINT77(cout << "inside else " <<   endl;) 
					bool flag = false;
				if(cpu < ACCELERATOR_START)
					flag = ooo_cpu[cpu].L2C.RQ.occupancy < ooo_cpu[cpu].L2C.RQ.SIZE ? true : false;
				else
					flag = uncore.DRAM.get_occupancy(1, temp_mshr->entry[index].address) < uncore.DRAM.get_size(1, temp_mshr->entry[index].address) ? true : false;

				if(flag){
					temp_mshr->entry[index].event_cycle = current_core_cycle[cpu];
					temp_mshr->entry[index].full_addr = next_level_base_addr << LOG2_PAGE_SIZE | (get_offset(temp_mshr->entry[index].full_virtual_address, temp_mshr->entry[index].translation_level) << 3);
					temp_mshr->entry[index].address = temp_mshr->entry[index].full_addr >> LOG2_BLOCK_SIZE;
					temp_mshr->entry[index].returned = INFLIGHT;

					int rq_index;
					if(cpu < ACCELERATOR_START)
						rq_index = ooo_cpu[cpu].L2C.add_rq(&temp_mshr->entry[index]);
					else
						rq_index = uncore.DRAM.add_rq(&temp_mshr->entry[index]);
					if(rq_index != -1)
					{
						cout <<"rqindex " << rq_index << "cpu " << cpu << " packet.cpu " << temp_mshr->entry[index].cpu << " instr id " << temp_mshr->entry[index].instr_id << " addr " <<temp_mshr->entry[index].address<< endl;
						assert(0);
					}
					//assert(rq_index == -1);
				}
				else
					rq_full++;
			}
		}
	}
}

void PAGE_TABLE_WALKER::handle_RQ(){

	if(RQ.occupancy > 0) //If there is no pending request which is undergoing translation, then process new request.
	{
		PRINT77(
				if(cpu >= ACCELERATOR_START){
				cout << "inside  opearte PTW  after MSHR handling" << endl;
				cout << "cache type "<< NAME << " RQ occupancy " << RQ.occupancy << endl;
				RQ.queuePrint();
				cout << "cache type "<< NAME << " MSHR occupancy " << MSHR.occupancy << endl;
				MSHR.queuePrint();
				}
		       )
			bool flag = false;
		if(cpu < ACCELERATOR_START){
			if((RQ.entry[RQ.head].event_cycle <= current_core_cycle[cpu]) && (ooo_cpu[cpu].L2C.RQ.occupancy < ooo_cpu[cpu].L2C.RQ.SIZE))
				flag = true;
		}
		else{
			if((RQ.entry[RQ.head].event_cycle <= current_core_cycle[cpu]) && (uncore.DRAM.get_occupancy(1, 0) < uncore.DRAM.get_size(1, 0)))
				flag = true;
		}
		if(flag){
			int index = RQ.head;

			//cout << hex << RQ.entry[index].full_addr << dec << endl;
			assert((RQ.entry[index].full_addr >> 32) != 0xf000000f); //Page table is stored at this address
			assert(RQ.entry[index].full_virtual_address != 0);

			uint64_t address_pscl5 = check_hit(PSCL5,get_index(RQ.entry[index].full_addr,IS_PSCL5));
			uint64_t address_pscl4 = check_hit(PSCL4,get_index(RQ.entry[index].full_addr,IS_PSCL4));
			uint64_t address_pscl3 = check_hit(PSCL3,get_index(RQ.entry[index].full_addr,IS_PSCL3));
			uint64_t address_pscl2 = check_hit(PSCL2,get_index(RQ.entry[index].full_addr,IS_PSCL2));


			PACKET packet = RQ.entry[index];

			packet.fill_level = FILL_L1; //@Vishal: This packet will be sent from L2 to PTW, TODO: check if this is done or not
			packet.cpu = RQ.entry[index].cpu;//@Nilesh: Added for accelerator cpu
			packet.instr_id = RQ.entry[index].instr_id;
			packet.ip = RQ.entry[index].ip;	//@Vasudha: IP required to calculate address of cloudsuite benchmarks
			packet.type = TRANSLATION;
			packet.event_cycle = current_core_cycle[cpu];
			packet.full_virtual_address = RQ.entry[index].full_addr;

			uint64_t next_address = UINT64_MAX;

			if(address_pscl2 != UINT64_MAX)
			{
				next_address = address_pscl2 << LOG2_PAGE_SIZE | (get_offset(RQ.entry[index].full_addr,IS_PTL1) << 3);				
				packet.translation_level = 1;
			}
			else if(address_pscl3 != UINT64_MAX)
			{
				next_address = address_pscl3 << LOG2_PAGE_SIZE | (get_offset(RQ.entry[index].full_addr,IS_PTL2) << 3);				
				packet.translation_level = 2;
			}
			else if(address_pscl4 != UINT64_MAX)
			{
				next_address = address_pscl4 << LOG2_PAGE_SIZE | (get_offset(RQ.entry[index].full_addr,IS_PTL3) << 3);				
				packet.translation_level = 3;
			}
			else if(address_pscl5 != UINT64_MAX)
			{
				next_address = address_pscl5 << LOG2_PAGE_SIZE | (get_offset(RQ.entry[index].full_addr,IS_PTL4) << 3);				
				packet.translation_level = 4;
			}
			else
			{
				if(CR3_addr == UINT64_MAX)
				{
					assert(!CR3_set); //This should be called only once when the process is starting
					handle_page_fault(L5, &RQ.entry[index], 6); //6 means first level is also not there
					CR3_set = true;

					PAGE_TABLE_PAGE* curr_page = L5;
					uint64_t next_level_base_addr = UINT64_MAX;
					for (int i = 5; i > 1; i--) //Fill MMU caches
					{
						uint64_t offset = get_offset(RQ.entry[index].full_virtual_address, i);
						assert(curr_page != NULL);
						next_level_base_addr = curr_page->next_level_base_addr[offset];
						assert(next_level_base_addr != UINT64_MAX); //Page fault serviced, all levels should be there.
						curr_page = curr_page->entry[offset];

						switch(i)
						{
							case 5: fill_mmu_cache(PSCL5, next_level_base_addr, &RQ.entry[index], IS_PSCL5);
								break;
							case 4: fill_mmu_cache(PSCL4, next_level_base_addr, &RQ.entry[index], IS_PSCL4);
								break;
							case 3: fill_mmu_cache(PSCL3, next_level_base_addr, &RQ.entry[index], IS_PSCL3);
								break;
							case 2: fill_mmu_cache(PSCL2, next_level_base_addr, &RQ.entry[index], IS_PSCL2);
								break;
						}
					}

					uint64_t offset = get_offset(RQ.entry[index].full_virtual_address, IS_PTL1);
					next_level_base_addr = curr_page->next_level_base_addr[offset];

					RQ.entry[index].event_cycle = current_core_cycle[cpu] + PAGE_TABLE_LATENCY;
					RQ.entry[index].data = next_level_base_addr << LOG2_PAGE_SIZE | (RQ.entry[index].full_virtual_address && ((1<<LOG2_PAGE_SIZE) - 1));

					if (warmup_complete[cpu] && MSHR.entry[index].ptw_start_cycle > 0 && current_core_cycle[cpu] - MSHR.entry[index].ptw_start_cycle >= LATENCY){
						PTW_rq++;
						PTW_latency += current_core_cycle[cpu] - MSHR.entry[index].ptw_start_cycle ;
					}
					PRINT77(cout << "return data from PTW 2"<< endl;)
						if (RQ.entry[index].instruction)
							upper_level_icache[cpu]->return_data(&RQ.entry[index]);
						else // data
							upper_level_dcache[cpu]->return_data(&RQ.entry[index]);

					RQ.remove_queue(&RQ.entry[index]);

					return;

				}
				next_address = CR3_addr << LOG2_PAGE_SIZE | (get_offset(RQ.entry[index].full_addr,IS_PTL5) << 3);				
				packet.translation_level = 5;
			}

			packet.init_translation_level = packet.translation_level;
			packet.address = next_address >> LOG2_BLOCK_SIZE;
			packet.full_addr = next_address;
			

			add_mshr(&packet);
			/*if(mshr_index != -1){
				MSHR.entry[mshr_index].ptw_merged_number++;
				RQ.remove_queue(&RQ.entry[index]); // already added to the mshr;
				return;
			}*/
			int rq_index;
			if(cpu < ACCELERATOR_START){
				rq_index = ooo_cpu[cpu].L2C.add_rq(&packet);//Packet should not merge as one translation is sent at a time.
				assert(rq_index == -1);
			}
			else{
				if(all_warmup_complete < NUM_CPUS){
					uncore.DRAM.add_rq(&packet);
				}
				else{
					rq_index = uncore.DRAM.add_rq(&packet);
					if(rq_index != -1){

						cout << "rq_index " << rq_index << " cpu " << cpu << " packet->cpu " << packet.cpu  <<" instrid " << packet.instr_id <<  endl;
					}
					assert(rq_index != -2);
				}
			}
			//cout << rq_index<< endl; //@Vishal: Remove this
			/*if(rq_index < -1)
			  {
			  assert(0);
			  }*/
			//  assert(rq_index == -1); //Packet should not merge as one translation is sent at a time.


			RQ.remove_queue(&RQ.entry[index]);
		}
	}
}
void PAGE_TABLE_WALKER::handle_PQ(){

	if(PQ.occupancy > 0) 
	{
			 PRINT(cout << "index in handle_PQ " << PQ.head << " occupance " << PQ.occupancy<< endl;
				cout << "entry = " << PQ.entry[PQ.head].event_cycle << endl;	 
					 ) 
		bool flag = false;
		if(cpu < ACCELERATOR_START){
			if((PQ.entry[PQ.head].event_cycle <= current_core_cycle[cpu]) && (ooo_cpu[cpu].L2C.PQ.occupancy < ooo_cpu[cpu].L2C.PQ.SIZE))
				flag = true;
		}
		else{
			if((PQ.entry[PQ.head].event_cycle <= current_core_cycle[cpu]) && (uncore.DRAM.get_occupancy(1, 0) < uncore.DRAM.get_size(1, 0)))
				flag = true;
		}
		if(flag){
                         int index = PQ.head;
			 PRINT(cout << "index in handle_PQ " << index << endl;) 
                          //cout << hex << PQ.entry[index].full_addr << dec << endl;
                         assert((PQ.entry[index].full_addr >> 32) != 0xf000000f); //Page table is stored at this address
                         assert(PQ.entry[index].full_virtual_address != 0);
 
                         uint64_t address_pscl5 = check_hit(PSCL5,get_index(PQ.entry[index].full_addr,IS_PSCL5));
                         uint64_t address_pscl4 = check_hit(PSCL4,get_index(PQ.entry[index].full_addr,IS_PSCL4));
                         uint64_t address_pscl3 = check_hit(PSCL3,get_index(PQ.entry[index].full_addr,IS_PSCL3));
			 uint64_t address_pscl2 = check_hit(PSCL2,get_index(PQ.entry[index].full_addr,IS_PSCL2));

                         PACKET packet = PQ.entry[index];
             		 packet.fill_level = FILL_L1; //@Vishal: This packet will be sent from L2 to PTW, TODO: check if this is done or not
	     		 packet.cpu = PQ.entry[index].cpu;
             		 packet.prefetch_id = PQ.entry[index].prefetch_id;
             		 packet.ip = PQ.entry[index].ip;	//@Vasudha: IP required to calculate address of cloudsuite benchmarks
             		 packet.type = PREFETCH;
             		 packet.event_cycle = current_core_cycle[cpu];
             		 packet.full_virtual_address = PQ.entry[index].full_addr;
		
			 uint64_t next_address = UINT64_MAX;
	
	                 if(address_pscl2 != UINT64_MAX)
	                 {
        	                  next_address = address_pscl2 << LOG2_PAGE_SIZE | (get_offset(PQ.entry[index].full_addr,IS_PTL1) << 3);
                		  packet.translation_level = 1;
                         }
                         else if(address_pscl3 != UINT64_MAX)
                         {
                                 next_address = address_pscl3 << LOG2_PAGE_SIZE | (get_offset(PQ.entry[index].full_addr,IS_PTL2) << 3);
                 		 packet.translation_level = 2;
             		}
                         else if(address_pscl4 != UINT64_MAX)
                         {
				 next_address = address_pscl4 << LOG2_PAGE_SIZE | (get_offset(PQ.entry[index].full_addr,IS_PTL3) << 3);
                		 packet.translation_level = 3;
             		 }
                         else if(address_pscl5 != UINT64_MAX)
                         {
                                 next_address = address_pscl5 << LOG2_PAGE_SIZE | (get_offset(PQ.entry[index].full_addr,IS_PTL4) << 3);
                 		 packet.translation_level = 4;
             		 }
	       else {
		     if(CR3_addr == UINT64_MAX)
		     {
			     assert(!CR3_set); //This should be called only once when the process is starting
                        handle_page_fault(L5, &PQ.entry[index], 6); //6 means first level is also not there
                        CR3_set = true;

                        PAGE_TABLE_PAGE* curr_page = L5;
                        uint64_t next_level_base_addr = UINT64_MAX;
                        for (int i = 5; i > 1; i--) //Fill MMU caches
                        {
                                  uint64_t offset = get_offset(PQ.entry[index].full_virtual_address, i);
                                  assert(curr_page != NULL);
                                  next_level_base_addr = curr_page->next_level_base_addr[offset];
                                  assert(next_level_base_addr != UINT64_MAX); //Page fault serviced, all levels should be there.
                                  curr_page = curr_page->entry[offset];

                                  switch(i)
                                  {
                                                        case 5: fill_mmu_cache(PSCL5, next_level_base_addr, &PQ.entry[index], IS_PSCL5);
                                                                        break;
                                                        case 4: fill_mmu_cache(PSCL4, next_level_base_addr, &PQ.entry[index], IS_PSCL4);
                                                                        break;
                                                        case 3: fill_mmu_cache(PSCL3, next_level_base_addr, &PQ.entry[index], IS_PSCL3);
                                                                        break;
                                                        case 2: fill_mmu_cache(PSCL2, next_level_base_addr, &PQ.entry[index], IS_PSCL2);
                                                                        break;
                                   }
                         }

                         uint64_t offset = get_offset(PQ.entry[index].full_virtual_address, IS_PTL1);
                         next_level_base_addr = curr_page->next_level_base_addr[offset];
                         PQ.entry[index].event_cycle = current_core_cycle[cpu] + PAGE_TABLE_LATENCY;
                         PQ.entry[index].data = next_level_base_addr << LOG2_PAGE_SIZE | (PQ.entry[index].full_virtual_address && ((1<<LOG2_PAGE_SIZE) - 1));

			 if (warmup_complete[cpu] && PREFETCH_MSHR.entry[index].ptw_start_cycle > 0 && current_core_cycle[cpu] - PREFETCH_MSHR.entry[index].ptw_start_cycle >= LATENCY){
				 PTW_rq++;
				 PTW_latency += current_core_cycle[cpu] - PREFETCH_MSHR.entry[index].ptw_start_cycle ;
			 }
			PRINT77(cout << "return data from PTW 3"<< endl;)
                         if (PQ.entry[index].instruction)
                                  upper_level_icache[cpu]->return_data(&PQ.entry[index]);
                          else // data
                                  upper_level_dcache[cpu]->return_data(&PQ.entry[index]);

                          PQ.remove_queue(&PQ.entry[index]);

                          return;
		     }
		     next_address = CR3_addr << LOG2_PAGE_SIZE | (get_offset(PQ.entry[index].full_addr,IS_PTL5) << 3);
                     packet.translation_level = 5;
	     }
	      packet.init_translation_level = packet.translation_level;
                        packet.address = next_address >> LOG2_BLOCK_SIZE;
            packet.full_addr = next_address;

		    add_prefetch_mshr(&packet);
                    //    int rq_index = ooo_cpu[cpu].L2C.add_rq(&packet);
			int rq_index;
			if(cpu < ACCELERATOR_START){
				rq_index = ooo_cpu[cpu].L2C.add_rq(&packet);//Packet should not merge as one translation is sent at a time.
				assert(rq_index == -1);
			}
			else{
				if(all_warmup_complete < NUM_CPUS){
					uncore.DRAM.add_rq(&packet);
				}
				else{
					rq_index = uncore.DRAM.add_rq(&packet);
					if(rq_index != -1){

						cout << "rq_index " << rq_index << " cpu " << cpu << " packet->cpu " << packet.cpu  <<" instrid " << packet.instr_id <<  endl;
					}
					assert(rq_index != -2);
				}
			}
                    //cout << rq_index<< endl; //@Vishal: Remove this
                    /*if(rq_index < -1)
                    {
                            assert(0);
                    }*/
                 //   assert(rq_index == -1); //Packet should not merge as one translation is sent at a time.

                    PQ.remove_queue(&PQ.entry[index]);
		}
	}	
}

void PAGE_TABLE_WALKER::operate()
{

#ifndef INS_PAGE_TABLE_WALKER
	assert(0);
#endif

/*	if(cpu == 0){
		cout << "*************************** beforePTW cpu " << cpu << endl;
                cout << "occupancy mshr " << MSHR.occupancy << " rq " << RQ.occupancy << " PQ " << PQ.occupancy << endl;
                cout << "cache type "<< NAME << " RQ occupancy " << RQ.occupancy << endl;
                RQ.queuePrint();
                cout << "cache type "<< NAME << " MSHR occupancy " << MSHR.occupancy << endl;
                MSHR.queuePrint();
                cout << "cache type "<< NAME << " PQ occupancy " << PQ.occupancy << endl;
		PQ.queuePrint();
	}*/
	
	if(cpu >= ACCELERATOR_START){
/*		cout << "*************************** cpu " << cpu << endl;
                cout << "occupancy mshr " << MSHR.occupancy << " rq " << RQ.occupancy << " PQ " << PQ.occupancy << endl;
                cout << "cache type "<< NAME << " RQ occupancy " << RQ.occupancy << endl;
                RQ.queuePrint();
                cout << "cache type "<< NAME << " MSHR occupancy " << MSHR.occupancy << endl;
                MSHR.queuePrint();
*/
		int handle_width = IOMMU_PTW_HANDLE - MSHR.occupancy;
//		cout << "handle width " << handle_width << " mshr occ " << MSHR.occupancy << " rq " << RQ.occupancy << " pq " << PQ.occupancy << endl;    
		handle_fill(1);
		while(RQ.occupancy &&   handle_width){
			handle_width--;
			handle_RQ();
		}
		handle_width = PTW_PREFETCH_MSHR_SIZE - PREFETCH_MSHR.occupancy;
		handle_fill(0);
                while(PQ.occupancy && handle_width){
                        handle_width--;
                        handle_PQ();
                }

	/*	if(cpu == 1){
			cout << "*************************** after PTW cpu " << cpu << endl;
			cout << "occupancy mshr " << MSHR.occupancy << " rq " << RQ.occupancy << " PQ " << PQ.occupancy << endl;
			cout << "cache type "<< NAME << " RQ occupancy " << RQ.occupancy << endl;
			RQ.queuePrint();
			cout << "cache type "<< NAME << " MSHR occupancy " << MSHR.occupancy << endl;
			MSHR.queuePrint();
			/* cout << "cache type "<< NAME << " PQ occupancy " << PQ.occupancy << endl;
			   PQ.queuePrint();
		}*/
	}
	else{
                /* cout << "*************************** " << endl;
		cout << "occupancy mshr " << MSHR.occupancy << " rq " << RQ.occupancy << " PQ " << PQ.occupancy << endl;
                cout << "cache type "<< NAME << " RQ occupancy " << RQ.occupancy << endl;
                RQ.queuePrint();
                cout << "cache type "<< NAME << " MSHR occupancy " << MSHR.occupancy << endl;
                MSHR.queuePrint();
                cout << "cache type "<< NAME << " PQ occupancy " << PQ.occupancy << endl;
		PQ.queuePrint();*/
		if(MSHR.occupancy){
			handle_fill(1);
		}
		else if(RQ.occupancy)
			handle_RQ();
		else 
			handle_PQ();
	}
	rq_entries += RQ.occupancy;
	if(RQ.occupancy)
		rq_count_cycle++;
	if(MSHR.occupancy)
		mshr_count_cycle++;
	if(PREFETCH_MSHR.occupancy)
		prefetch_mshr_count_cycle++;
	if(PQ.occupancy)
		pq_count_cycle++;
	pq_entries += PQ.occupancy;
	mshr_entries += MSHR.occupancy;
	prefetch_mshr_entries += PREFETCH_MSHR.occupancy;
}

void PAGE_TABLE_WALKER::handle_page_fault(PAGE_TABLE_PAGE* page, PACKET *packet, uint8_t pt_level)
{
	bool page_swap = false;

	if(pt_level == 6)
	{
		assert(page == NULL && CR3_addr == UINT64_MAX);
		L5 = new PAGE_TABLE_PAGE();
		CR3_addr = map_translation_page(&page_swap);
		pt_level--;
		write_translation_page(CR3_addr, packet, pt_level);
		page = L5;
	}

	while(pt_level > 1)
	{
		uint64_t offset = get_offset(packet->full_virtual_address, pt_level);
		
		assert(page != NULL && page->entry[offset] == NULL);
		
		page->entry[offset] =  new PAGE_TABLE_PAGE();
		page->next_level_base_addr[offset] = map_translation_page(&page_swap);
		write_translation_page(page->next_level_base_addr[offset], packet, pt_level);
		page = page->entry[offset];
		pt_level--;
	}

	uint64_t offset = get_offset(packet->full_virtual_address, pt_level);
		
	assert(page != NULL && page->next_level_base_addr[offset] == UINT64_MAX);

	page->next_level_base_addr[offset] = map_data_page(packet->instr_id, packet->full_virtual_address, &page_swap);

	//This is done so that latency is added once, not five times
	if (page_swap)
        	stall_cycle[cpu] = current_core_cycle[cpu] + SWAP_LATENCY;
    	else
        	stall_cycle[cpu] = current_core_cycle[cpu] + PAGE_TABLE_LATENCY; 

}

uint64_t PAGE_TABLE_WALKER::va_to_pa_ptw(uint8_t cpu, uint64_t instr_id, bool translation_page, uint64_t va, uint64_t unique_vpage, bool *page_swap)
{

#ifdef SANITY_CHECK
    if (va == 0) 
        assert(0);
#endif

    uint64_t unique_va = va,
    		 vpage = unique_vpage ,
             voffset = unique_va & ((1<<LOG2_PAGE_SIZE) - 1);

    // smart random number generator
    uint64_t random_ppage;

    auto pr = page_table.find(vpage);
    auto ppage_check = inverse_table.begin();

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
            *page_swap = true;
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

        if (*page_swap)
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

    /*if (swap)
        stall_cycle[cpu] = current_core_cycle[cpu] + SWAP_LATENCY;
    else
        stall_cycle[cpu] = current_core_cycle[cpu] + PAGE_TABLE_LATENCY;*/

    //cout << "cpu: " << cpu << " allocated unique_vpage: " << hex << unique_vpage << " to ppage: " << ppage << dec << endl;

    return pa;
}

uint64_t PAGE_TABLE_WALKER::map_translation_page(bool *page_swap)
{
	uint64_t physical_address = va_to_pa_ptw(cpu, 0, true , next_translation_virtual_address, next_translation_virtual_address >> LOG2_PAGE_SIZE, page_swap);
	next_translation_virtual_address = ( (next_translation_virtual_address >> LOG2_PAGE_SIZE) + 1 ) << LOG2_PAGE_SIZE;
	
	return physical_address >> LOG2_PAGE_SIZE;
		
	
}

uint64_t PAGE_TABLE_WALKER::map_data_page(uint64_t instr_id, uint64_t full_virtual_address, bool *page_swap)
{
	uint64_t physical_address = va_to_pa_ptw(cpu, instr_id, false , full_virtual_address, full_virtual_address >> LOG2_PAGE_SIZE, page_swap);

        return physical_address >> LOG2_PAGE_SIZE;
}

void PAGE_TABLE_WALKER::write_translation_page(uint64_t next_level_base_addr, PACKET *packet, uint8_t pt_level)
{
	//@Vishal: Need to complete it, Problem: If lower level WQ is full, then what to do?
}

int PAGE_TABLE_WALKER::add_mshr(PACKET *packet)
{
	uint32_t index = 0;
	packet->ptw_mshr_index = index;
	packet->cycle_enqueued = current_core_cycle[packet->cpu];	
	if(cpu < ACCELERATOR_START){
		MSHR.entry[0] = *packet;
		MSHR.entry[0].returned = INFLIGHT;
		MSHR.occupancy++;
		return -1;
	}

   /* for(int i=0;i < MSHR.SIZE; i++){
        if(MSHR.entry[i].address == packet->address && MSHR.entry[i].asid[0] == packet->asid[0])
                return i;
    }*/

    	  //  cout << "cpu " << cpu << " packet cpu " << packet->cpu << " instr id " << packet->instr_id << endl;
    
   // assert(index == -1); //@Vishal: Duplicate request should not be sent.
    
    index = -1;
    for(int i=0;i < MSHR.SIZE; i++){
    	if(MSHR.entry[i].ip == 0)
		index = i;
    }
/*	if(index == -1){
		cout << "cache type "<< NAME << " RQ occupancy " << RQ.occupancy << endl;
                RQ.queuePrint();
                cout << "cache type "<< NAME << " MSHR occupancy " << MSHR.occupancy << endl;
                MSHR.queuePrint();
                cout << "cache type "<< NAME << " PQ occupancy " << PQ.occupancy << endl;
                PQ.queuePrint();
	
	}*/

    assert(index != -1);
/*    if(cpu == ACCELERATOR_START)
	    cout <<"addmshr index " << index << " cpu " << packet->cpu << " insrtid " << packet->instr_id << endl;
*/
#ifdef SANITY_CHECK
    if (MSHR.entry[index].address != 0) {
        cerr << "[" << NAME << "_ERROR] " << __func__ << " is not empty index: " << index;
        cerr << " address: " << hex << MSHR.entry[index].address;
        cerr << " full_addr: " << MSHR.entry[index].full_addr << dec << endl;
        assert(0);
    }
#endif

    packet->ptw_mshr_index = index;
    MSHR.entry[index] = *packet;
    MSHR.entry[index].returned = INFLIGHT;
    MSHR.occupancy++;


    if (packet->address == 0)
        assert(0);

    return -1;
}
int PAGE_TABLE_WALKER::add_prefetch_mshr(PACKET *packet)
{
        uint32_t index =-1;
        packet->ptw_mshr_index = index;
        packet->cycle_enqueued = current_core_cycle[packet->cpu];
    for(int i=0;i < PREFETCH_MSHR.SIZE; i++){
        if(PREFETCH_MSHR.entry[i].ip == 0)
                index = i;
    }

    assert(index != -1);
/*    if(cpu == ACCELERATOR_START)
            cout <<"addmshr index " << index << " cpu " << packet->cpu << " insrtid " << packet->instr_id << endl;
*/
#ifdef SANITY_CHECK
    if (PREFETCH_MSHR.entry[index].address != 0) {
        cerr << "[" << NAME << "_ERROR] " << __func__ << " is not empty index: " << index;
        cerr << " address: " << hex << PREFETCH_MSHR.entry[index].address;
        cerr << " full_addr: " << PREFETCH_MSHR.entry[index].full_addr << dec << endl;
        assert(0);
    }
#endif

    packet->ptw_mshr_index = index;
    PREFETCH_MSHR.entry[index] = *packet;
    PREFETCH_MSHR.entry[index].returned = INFLIGHT;
    PREFETCH_MSHR.occupancy++;


    if (packet->address == 0)
        assert(0);

    return -1;
}

void PAGE_TABLE_WALKER::fill_mmu_cache(CACHE &cache, uint64_t next_level_base_addr, PACKET *packet, uint8_t cache_type)
{
	cache.MSHR.entry[0].cpu = packet->cpu;
	cache.MSHR.entry[0].address = get_index(packet->full_virtual_address,cache_type);
	cache.MSHR.entry[0].full_addr = get_index(packet->full_virtual_address,cache_type);
	cache.MSHR.entry[0].data = next_level_base_addr;
	cache.MSHR.entry[0].instr_id = packet->instr_id;

	cache.MSHR.entry[0].ip = 0;
	cache.MSHR.entry[0].fill_level = 0;
	cache.MSHR.entry[0].type = TRANSLATION;
	cache.MSHR.entry[0].event_cycle = current_core_cycle[cpu];

	cache.MSHR.next_fill_index = 0;
	cache.MSHR.next_fill_cycle = current_core_cycle[cpu];

	cache.MSHR.occupancy = 1;
	cache.handle_fill();

	//cout << "["<< cache.NAME << "]" << "Miss: "<< cache.sim_miss[cpu][TRANSLATION] << endl;
}

uint64_t PAGE_TABLE_WALKER::get_index(uint64_t address, uint8_t cache_type)
{

	address = address & ( (1L<<57) -1); //Extract Last 57 bits

	int shift = 12;

	switch(cache_type)
	{
		case IS_PSCL5: shift+= 9+9+9+9;
					   break;
		case IS_PSCL4: shift+= 9+9+9;
					   break;
		case IS_PSCL3: shift+= 9+9;
					   break;
		case IS_PSCL2: shift+= 9; //Most siginificant 36 bits will be used to index PSCL2 
					   break;
	}

	return (address >> shift); 
}

uint64_t PAGE_TABLE_WALKER::check_hit(CACHE &cache, uint64_t address)
{

	uint32_t set = cache.get_set(address);

    if (cache.NUM_SET < set) {
        cerr << "[" << NAME << "_ERROR] " << __func__ << " invalid set index: " << set << " NUM_SET: " << cache.NUM_SET;
        assert(0);
    }

	 /*   if(add_loc.count(address)){
		    uint64_t  location = add_loc[address];
		    for(auto itr= loc_add.find(location); itr != loc_add.end(); itr++){
			    total_address.insert(itr->second);
		    }
		    dist_count[total_address.size()]++;
		    //      total_reuse_distance += total_address.size();
		    number_of_reuse_distance++;
		    total_address.clear();

	    }
	    add_loc[address] = current_core_cycle[cpu];
	    loc_add[current_core_cycle[cpu]] = address;*/
    
    for (uint32_t way=0; way<cache.NUM_WAY; way++) {
        if (cache.block[set][way].valid && (cache.block[set][way].tag == address)) {
	    
	    // COLLECT STATS
            cache.sim_hit[cpu][TRANSLATION]++;
            cache.sim_access[cpu][TRANSLATION]++;
	    if(cache.cache_type == IS_PSCL2){
		    DTLBPRINT(if( warmup_complete[cpu] ){
				    cout << "Hit " <<current_core_cycle[cpu]<< " "   <<setw(10) << (address)  << endl;
				    });
	    }
	    if(cache.cache_type == IS_PSCL2){
		  //  cout << "calling for hit \n"; 
		    cache.PSCL2_update_replacement_state(cpu, set, way, cache.block[set][way].full_addr,0 , 0, TRANSLATION, 1);
	    }
	    return cache.block[set][way].data;
        }
    }

    if(cache.cache_type == IS_PSCL2){
	    //check for victim buffer at PSCL2
	    for (uint32_t way=0; way<PSCL2_VB.NUM_WAY; way++) {
		    if (PSCL2_VB.block[set][way].valid && (PSCL2_VB.block[set][way].tag == address)) {
			    

			    // COLLECT STATS
			    PSCL2_VB.sim_hit[cpu][TRANSLATION]++;
			    PSCL2_VB.sim_access[cpu][TRANSLATION]++;
			    PSCL2_VB.block[set][way].valid = 0;

			    PSCL2.MSHR.entry[0].fill_level = 0;
			    PSCL2.MSHR.entry[0].cpu = PSCL2_VB.block[set][way].cpu;
			    PSCL2.MSHR.entry[0].address =  PSCL2_VB.block[set][way].address;
			    PSCL2.MSHR.entry[0].full_addr = PSCL2_VB.block[set][way].full_addr;
			    PSCL2.MSHR.entry[0].data = PSCL2_VB.block[set][way].data;
			    PSCL2.MSHR.entry[0].instr_id = PSCL2_VB.block[set][way].instr_id;
			    PSCL2.MSHR.entry[0].ip = 0;
			    PSCL2.MSHR.entry[0].type = TRANSLATION;
			    PSCL2.MSHR.entry[0].event_cycle = current_core_cycle[cpu];
			    PSCL2.MSHR.next_fill_index = 0;
			    PSCL2.MSHR.next_fill_cycle = current_core_cycle[cpu];
			    PSCL2.MSHR.occupancy = 1;
			    PSCL2.handle_fill();

			    return PSCL2_VB.block[set][way].data;
		    }
	    }
	    PSCL2_VB.sim_miss[cpu][TRANSLATION]++;
	    PSCL2_VB.sim_access[cpu][TRANSLATION]++;

    }
    return UINT64_MAX;
}
    
uint64_t PAGE_TABLE_WALKER::get_offset(uint64_t full_virtual_addr, uint8_t pt_level)
{
	full_virtual_addr = full_virtual_addr & ( (1L<<57) -1); //Extract Last 57 bits

	int shift = 12;

	switch(pt_level)
	{
		case IS_PTL5: shift+= 9+9+9+9;
					   break;
		case IS_PTL4: shift+= 9+9+9;
					   break;
		case IS_PTL3: shift+= 9+9;
					   break;
	   	case IS_PTL2: shift+= 9;
	   				   break;
	}

	uint64_t offset = (full_virtual_addr >> shift) & 0x1ff; //Extract the offset to generate next physical address

	return offset; 
}

int  PAGE_TABLE_WALKER::add_rq(PACKET *packet)
{
	// check for duplicates in the read queue
   int index = -1;
   if(warmup_complete[cpu])
	   packet->ptw_start_cycle = current_core_cycle[cpu];
   // int index = RQ.check_queue(packet);
   // assert(index == -1); //@Vishal: Duplicate request should not be sent.
    
    // check occupancy
    if (RQ.occupancy == PTW_RQ_SIZE) {
        RQ.FULL++;

        return -2; // cannot handle this request
    }

    // if there is no duplicate, add it to RQ
    index = RQ.tail;

#ifdef SANITY_CHECK
    if (RQ.entry[index].address != 0) {
        cerr << "[" << NAME << "_ERROR] " << __func__ << " is not empty index: " << index;
        cerr << " address: " << hex << RQ.entry[index].address;
        cerr << " full_addr: " << RQ.entry[index].full_addr << dec << endl;
        assert(0);
    }
#endif

    RQ.entry[index] = *packet;

    // ADD LATENCY
    if (RQ.entry[index].event_cycle < current_core_cycle[packet->cpu])
        RQ.entry[index].event_cycle = current_core_cycle[packet->cpu] + LATENCY;
    else
        RQ.entry[index].event_cycle += LATENCY;

    RQ.occupancy++;
    RQ.tail++;
    if (RQ.tail >= RQ.SIZE)
        RQ.tail = 0;

    DP ( if (warmup_complete[RQ.entry[index].cpu]) {
    cout << "[" << NAME << "_RQ] " <<  __func__ << " instr_id: " << RQ.entry[index].instr_id << " address: " << hex << RQ.entry[index].address;
    cout << " full_addr: " << RQ.entry[index].full_addr << dec;
    cout << " type: " << +RQ.entry[index].type << " head: " << RQ.head << " tail: " << RQ.tail << " occupancy: " << RQ.occupancy;
    cout << " event: " << RQ.entry[index].event_cycle << " current: " << current_core_cycle[RQ.entry[index].cpu] << endl; });


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

    return -1;
}

int PAGE_TABLE_WALKER::add_wq(PACKET *packet)
{
	assert(0); //@Vishal: No request is added to WQ
}

int PAGE_TABLE_WALKER::add_pq(PACKET *packet)
{
	//check for duplicates in prefetch queue 
	int index = PQ.check_queue(packet);
	assert(index == -1);

	PRINT(cout << "inside add_pq index " << index << " occupancy " << PQ.occupancy << " packet event=" << packet->event_cycle 	<< endl;)
	//check occupancy
	if(PQ.occupancy == PTW_PQ_SIZE) {
		PQ.FULL++;
		return -2;	//cannot handle this request
	}

	//if there is no duplicate, add it to PQ
	index = PQ.tail;

#ifdef SANITY_CHECK
	if(PQ.entry[index].address != 0) {
		cerr << "[" << NAME << "_ERROR] " << __func__ << " is not empty index: " << index;
		cerr << " address: " << hex << PQ.entry[index].address;
		cerr << " full_addr: " << hex << PQ.entry[index].full_addr << endl;
		assert(0);
	}
#endif

	PQ.entry[index] = *packet;

	//ADD LATENCY
	if(PQ.entry[index].event_cycle < current_core_cycle[packet->cpu])
		PQ.entry[index].event_cycle = current_core_cycle[packet->cpu] + LATENCY;
	else
		PQ.entry[index].event_cycle += LATENCY;

	PQ.occupancy++;
	PQ.tail++;
	if(PQ.tail >= PQ.SIZE)
		PQ.tail = 0;

	DP( if(warmup_complete[PQ.entry[index].cpu]) {
		cout << "[" << NAME << "_RQ]" << __func__ << "instr_id: " << PQ.entry[index].instr_id << " address: "<< hex << PQ.entry[index].address;
		cout << " full_addr: " << PQ.entry[index].full_addr << dec;
		cout << " type: " << +PQ.entry[index].type << " head: " << PQ.head << " tail: " << PQ.tail << " occupancy: " << PQ.occupancy;
	        cout << " event: " << PQ.entry[index].event_cycle << " current: " << current_core_cycle[PQ.entry[index].cpu] << endl; });


#ifdef PRINT_QUEUE_TRACE
        if(packet->instr_id == QTRACE_INSTR_ID)
        {
	         cout << "[" << NAME << "_PQ] " <<  __func__ << " instr_id: " << PQ.entry[index].instr_id << " address: " << hex << PQ.entry[index].address;		       
		 cout << " full_addr: " << PQ.entry[index].full_addr << dec;
		 cout << " type: " << +PQ.entry[index].type << " head: " << PQ.head << " tail: " << PQ.tail << " occupancy: " << PQ.occupancy;
		 cout << " event: " << PQ.entry[index].event_cycle << " current: " << current_core_cycle[PQ.entry[index].cpu] << " cpu: "<<cpu<<endl;
	}
#endif

     if (packet->address == 0)
         assert(0);

     PRINT(cout << "before to cache " << endl; )
     PQ.TO_CACHE++;
     PRINT(cout << "after to cache " << endl; )
	PRINT(cout << "1 inside add_pq index " << index << " occupancy " << PQ.occupancy	<< endl;)
	
    return index;

}

void PAGE_TABLE_WALKER::return_data(PACKET *packet)
{

	int mshr_index = packet->ptw_mshr_index;
        PACKET_QUEUE *temp_mshr = packet->type == PREFETCH ? &PREFETCH_MSHR : &MSHR;	
//	if(packet->cpu == 4)
//	    cout <<"mshr index " << mshr_index << "packet cpu " <<packet->cpu  << " cpu " << cpu << " instrid" << packet->instr_id << " address " << packet->address <<endl;

    // sanity check
    if (mshr_index == -1) {
	    cout << "packet cpu " <<packet->cpu  << " cpu " << cpu << " instrid" << packet->instr_id << " address " << packet->address <<endl;
        cerr << "[" << NAME << "_MSHR] " << __func__ << " instr_id: " << packet->instr_id << " cannot find a matching entry!";
        cerr << " full_addr: " << hex << packet->full_addr;
        cerr << " address: " << packet->address << dec;
        cerr << " event: " << packet->event_cycle << " current: " << current_core_cycle[packet->cpu] << endl;
        assert(0);
    }

    // MSHR holds the most updated information about this request
    // no need to do memcpy
    temp_mshr->num_returned++;
    temp_mshr->entry[mshr_index].returned = COMPLETED;

    assert(temp_mshr->entry[mshr_index].translation_level > 0);
    temp_mshr->entry[mshr_index].translation_level--;
}

void PAGE_TABLE_WALKER::increment_WQ_FULL(uint64_t address)
{
	WQ.FULL++;
}

uint32_t PAGE_TABLE_WALKER::get_occupancy(uint8_t queue_type, uint64_t address)
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
        
uint32_t PAGE_TABLE_WALKER::get_size(uint8_t queue_type, uint64_t address)
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

