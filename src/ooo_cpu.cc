#include "uncore.h"
#include "ooo_cpu.h"
#include "set.h"

//#define ADDRESS_RANGE 2e10
#define ADDRESS_RANGE 10
// out-of-order core
O3_CPU ooo_cpu[NUM_CPUS]; 
uint64_t current_core_cycle[NUM_CPUS], stall_cycle[NUM_CPUS];
uint32_t SCHEDULING_LATENCY = 0, EXEC_LATENCY = 0;
uint8_t TRACE_ENDS_STOP = 0;
uint8_t UNIQUE_ASID[5];
int asid_index=0;
bool print_flag = false;

void O3_CPU::print_LSQ(LOAD_STORE_QUEUE *queue){
        cout<< "\n%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n";
	cout << " queue name = " <<queue->NAME  <<  " head " << queue->head << "  tail "<<  queue->tail<< endl;
	if(queue->occupancy == 0)
		return;
	if(queue->head < queue->tail){
		for(auto i=queue->head; i< queue->tail; i++){
			if(queue->entry[i].event_cycle  == 0 )
				continue;
			cout << " i " << i << endl;
			cout << "rob index= " << queue->entry[i].rob_index ;
			cout << "data index = " << queue->entry[i].data_index ;
			cout <<"event cycle= " << queue->entry[i].event_cycle ;
			cout << "addres " << queue->entry[i].virtual_address;
			cout << "inst id= " << queue->entry[i].instr_id << endl;
			cout << "translated= " << (int)queue->entry[i].translated ;
			cout << "fetched= " <<(int) queue-> entry[i].fetched ;
		}
	}
	else{
		for(auto i=queue->head; i< queue->SIZE; i++){
			if(queue->entry[i].event_cycle  == 0 )
				continue;
			cout << " i " << i << endl;
                        cout << "rob index= " << queue->entry[i].rob_index << endl;
                        cout << "data index = " << queue->entry[i].data_index << endl;
                        cout <<"event cycle= " << queue->entry[i].event_cycle << endl;
                        cout << "inst id= " << queue->entry[i].instr_id << endl;
                        cout << "translated= " <<(int) queue->entry[i].translated << endl;
                        cout << "fetched= " <<(int) queue-> entry[i].fetched << endl;
                }
		for(auto i=0; i< queue->tail; i++){
			if(queue->entry[i].event_cycle  == 0 )
				continue;
			cout << " i " << i << endl;
                        cout << "rob index= " << queue->entry[i].rob_index << endl;
                        cout << "data index = " << queue->entry[i].data_index << endl;
                        cout <<"event cycle= " << queue->entry[i].event_cycle << endl;
                        cout << "inst id= " << queue->entry[i].instr_id << endl;
                        cout << "translated= " <<(int) queue->entry[i].translated << endl;
                        cout << "fetched= " << (int)queue-> entry[i].fetched << endl;
                }
	}
}
void O3_CPU::print_ROB(uint32_t index){

        cout<< "\n%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n";
	cout << "cpu = " << cpu << endl;
	cout << "rob_index = " <<index << endl;
	cout << "event_cycle = " <<(int) ROB.entry[index].event_cycle << endl;
	cout << "translated = " << (int)ROB.entry[index].translated << endl;
	cout << "fetched = " << (int) ROB.entry[index].fetched << endl;
	cout << "sscheduled " << (int) ROB.entry[index].scheduled << endl;
	cout << "executed = " << (int) ROB.entry[index].executed << endl;
	cout << "is memory= " <<(int) ROB.entry[index].is_memory << endl;
	cout << "num of mem op = " << (int) ROB.entry[index].num_mem_ops << endl;
	cout << "isProducer = " << (int) ROB.entry[index].producer_id << endl;
	cout << "virtual_address = " << ROB.entry[index].virtual_address << endl;
	cout << "destination memory " << endl;
        for (uint32_t i=0; i<MAX_INSTR_DESTINATIONS; i++) {
                    cout << ROB.entry[index].destination_memory[i] << " " << ROB.entry[index].destination_virtual_address[i] << endl;
	}
	cout << endl;
	cout << "sourcememory " << endl;
        for (uint32_t i=0; i<NUM_INSTR_SOURCES; i++) {
                    cout << ROB.entry[index].source_memory[i] << " " << ROB.entry[index].source_virtual_address[i] << endl;
	}
	cout << endl;
}

void O3_CPU::initialize_core()
{

}

void O3_CPU::handle_branch()
{
    // actual processors do not work like this but for easier implementation,
    // we read instruction traces and virtually add them in the ROB
    // note that these traces are not yet translated and fetched 

    uint8_t continue_reading = 1;
    uint32_t num_reads = 0;
    instrs_to_read_this_cycle = FETCH_WIDTH;
	
    // first, read PIN trace
    while (continue_reading) {

        size_t instr_size = knob_cloudsuite ? sizeof(cloudsuite_instr) : sizeof(input_instr);
	
        if (knob_cloudsuite) {
        
            if (!fread(&current_cloudsuite_instr, instr_size, 1, trace_file)) {
                // reached end of file for this trace
                //cout << "*** Reached end of trace for Core: " << cpu << " Repeating trace: " << trace_string << endl; 
		//TRACE_ENDS_STOP = 1; /*@Vasudha - STOP simulation once trace file ends*/
                // close the trace file and re-open it
        //        pclose(trace_file);
		//return; /*@Vasudha */
                trace_file = popen(gunzip_command, "r");
                if (trace_file == NULL) {
                    cerr << endl << "*** CANNOT REOPEN TRACE FILE: " << trace_string << " ***" << endl;
                    assert(0);
                }
            } else { // successfully read the trace

                // copy the instruction into the performance model's instruction format
                ooo_model_instr arch_instr;
                int num_reg_ops = 0, num_mem_ops = 0;

                arch_instr.instr_id = instr_unique_id;
                arch_instr.ip = current_cloudsuite_instr.ip;
//		cout << "ip for trace " << arch_instr.ip << " cloudsuite " << current_cloudsuite_instr.ip << endl;
                arch_instr.is_branch = current_cloudsuite_instr.is_branch;
                arch_instr.branch_taken = current_cloudsuite_instr.branch_taken;

              //  arch_instr.asid[0] = current_cloudsuite_instr.asid[0];
              //  arch_instr.asid[1] = current_cloudsuite_instr.asid[1];
		//Nilesh: set ASID for accelerators
                arch_instr.asid[0] = core_asid;
                arch_instr.asid[1] = core_asid;


		/*@Vasudha - find unique asid
		int flag=0;
		for (int i=0; i<asid_index; i++)
		{
			if(UNIQUE_ASID[i] == arch_instr.asid[0])
			{
				flag = 1;
				break;
			}
		}
		if(flag==0)
		{
			UNIQUE_ASID[asid_index] = arch_instr.asid[0];
			cout <<"UNIQUE_ASID[" << asid_index << "] = " << UNIQUE_ASID[asid_index] << endl; 
			asid_index++;
		}*/
//		cout << "asid for cpu " << cpu << " is  " << (int)arch_instr.asid[0] << endl;
                for (uint32_t i=0; i<MAX_INSTR_DESTINATIONS; i++) {
                    arch_instr.destination_registers[i] = current_cloudsuite_instr.destination_registers[i];
                    arch_instr.destination_memory[i] = current_cloudsuite_instr.destination_memory[i];
                    arch_instr.destination_virtual_address[i] = current_cloudsuite_instr.destination_memory[i];

                    if (arch_instr.destination_registers[i])
                        num_reg_ops++;
                    if (arch_instr.destination_memory[i]) {
                        num_mem_ops++;

                        // update STA, this structure is required to execute store instructios properly without deadlock
                        if (num_mem_ops > 0) {
#ifdef SANITY_CHECK
                            if (STA[STA_tail] < UINT64_MAX) {
                                if (STA_head != STA_tail)
                                    assert(0);
                            }
#endif
                            STA[STA_tail] = instr_unique_id;
                            STA_tail++;

                            if (STA_tail == STA_SIZE)
                                STA_tail = 0;
                        }
                    }
                }

                for (int i=0; i<NUM_INSTR_SOURCES; i++) {
                    arch_instr.source_registers[i] = current_cloudsuite_instr.source_registers[i];
                    arch_instr.source_memory[i] = current_cloudsuite_instr.source_memory[i];
                    arch_instr.source_virtual_address[i] = current_cloudsuite_instr.source_memory[i];

                    if (arch_instr.source_registers[i])
                        num_reg_ops++;
                    if (arch_instr.source_memory[i])
                        num_mem_ops++;
                }

                arch_instr.num_reg_ops = num_reg_ops;
                arch_instr.num_mem_ops = num_mem_ops;
                if (num_mem_ops > 0) 
                    arch_instr.is_memory = 1;

                // virtually add this instruction to the ROB
                if (ROB.occupancy < ROB.SIZE) {
                    uint32_t rob_index = add_to_rob(&arch_instr);
                    num_reads++;

                    // branch prediction
                    if (arch_instr.is_branch) {

                        //DP( if (warmup_complete[cpu]) {
                        //cout << "[BRANCH] instr_id: " << instr_unique_id << " ip: " << hex << arch_instr.ip << dec << " taken: " << +arch_instr.branch_taken << endl; });

                        num_branch++;

                        /*
                        uint8_t branch_prediction;
                        // for faster simulation, force perfect prediction during the warmup
                        // note that branch predictor is still learning with real branch results
                        if (all_warmup_complete == 0)
                            branch_prediction = arch_instr.branch_taken; 
                        else
                            branch_prediction = predict_branch(arch_instr.ip);
                        */
                        uint8_t branch_prediction = predict_branch(arch_instr.ip);
                        
                        if (arch_instr.branch_taken != branch_prediction) {
   			    //if(false) { // this simulates perfect branch prediction			  
                            branch_mispredictions++;

			    total_rob_occupancy_at_branch_mispredict +=	ROB.occupancy;

                            //DP( if (warmup_complete[cpu]) {
                            //cout << "[BRANCH] MISPREDICTED instr_id: " << instr_unique_id << " ip: " << hex << arch_instr.ip << dec;
                            //cout << " taken: " << +arch_instr.branch_taken << " predicted: " << +branch_prediction << endl; });

                            // halt any further fetch this cycle
                            instrs_to_read_this_cycle = 0;

                            // and stall any additional fetches until the branch is executed
                            fetch_stall = 1; 

                            ROB.entry[rob_index].branch_mispredicted = 1;
                        }
                        else {
                            if (branch_prediction == 1) {
                                // if we are accurately predicting a branch to be taken, then we can't possibly fetch down that path this cycle,
                                // so we have to wait until the next cycle to fetch those
                                instrs_to_read_this_cycle = 0;
                            }

                            //DP( if (warmup_complete[cpu]) {
                            //cout << "[BRANCH] PREDICTED    instr_id: " << instr_unique_id << " ip: " << hex << arch_instr.ip << dec;
                            //cout << " taken: " << +arch_instr.branch_taken << " predicted: " << +branch_prediction << endl; });
                        }

                        last_branch_result(arch_instr.ip, arch_instr.branch_taken);
                    }

                    //if ((num_reads == FETCH_WIDTH) || (ROB.occupancy == ROB.SIZE))
                    if ((num_reads >= instrs_to_read_this_cycle) || (ROB.occupancy == ROB.SIZE))
                        continue_reading = 0;
                }
                instr_unique_id++;
            }
        }
	else
	  {
	  	//DP( if (warmup_complete[cpu]) {
                            //cout << "handle branch " << endl; });
	   	/* context-switch code*/
		if(ooo_cpu[cpu].context_switch)
		{
			//cout << "context_switch = " << cpu <<" cycle="<<current_core_cycle[cpu]<<"ROB-occupancy = "<< ROB.occupancy <<  endl;
	               
			break;
		} 
            if (!fread(&current_instr, instr_size, 1, trace_file)) {
                // reached end of file for this trace
                //cout << "*** Reached end of trace for Core: " << cpu << " Repeating trace: " << trace_string << endl; 

                // close the trace file and re-open it
                pclose(trace_file);
                trace_file = popen(gunzip_command, "r");
                if (trace_file == NULL) {
                    cerr << endl << "*** CANNOT REOPEN TRACE FILE: " << trace_string << " ***" << endl;
                    assert(0);
                }
            } else { // successfully read the trace

                // copy the instruction into the performance model's instruction format
                ooo_model_instr arch_instr;
                int num_reg_ops = 0, num_mem_ops = 0;

                arch_instr.instr_id = instr_unique_id;
                arch_instr.ip = current_instr.ip;
                arch_instr.is_branch = current_instr.is_branch;
                arch_instr.branch_taken = current_instr.branch_taken;
		//Nilesh: set asid for Accelerator
                arch_instr.asid[0] = core_asid;
                arch_instr.asid[1] = core_asid;
		////cout << arch_instr.instr_id <<" "<<arch_instr.ip<<" "<< arch_instr.is_branch<<" "<<arch_instr.branch_taken<<" "<<cpu<<endl;
                for (uint32_t i=0; i<MAX_INSTR_DESTINATIONS; i++) {
                    arch_instr.destination_registers[i] = current_instr.destination_registers[i];
                    arch_instr.destination_memory[i] = current_instr.destination_memory[i];
                    arch_instr.destination_virtual_address[i] = current_instr.destination_memory[i];

                    if (arch_instr.destination_registers[i])
                        num_reg_ops++;
                    if (arch_instr.destination_memory[i]) {
                        num_mem_ops++;
                        // update STA, this structure is required to execute store instructios properly without deadlock
                        if (num_mem_ops > 0) {
#ifdef SANITY_CHECK
                            if (STA[STA_tail] < UINT64_MAX) {
                                if (STA_head != STA_tail)
                                    assert(0);
                            }
#endif
                            STA[STA_tail] = instr_unique_id;
                            STA_tail++;

                            if (STA_tail == STA_SIZE)
                                STA_tail = 0;
                        }
                    }
                }

                for (int i=0; i<NUM_INSTR_SOURCES; i++) {
                    arch_instr.source_registers[i] = current_instr.source_registers[i];
                    arch_instr.source_memory[i] = current_instr.source_memory[i];
                    arch_instr.source_virtual_address[i] = current_instr.source_memory[i];

                    if (arch_instr.source_registers[i])
                        num_reg_ops++;
                    if (arch_instr.source_memory[i])
                        num_mem_ops++;
                }

                arch_instr.num_reg_ops = num_reg_ops;
                arch_instr.num_mem_ops = num_mem_ops;
                if (num_mem_ops > 0) 
                    arch_instr.is_memory = 1;

                // virtually add this instruction to the ROB
                if (ROB.occupancy < ROB.SIZE) {
                    uint32_t rob_index = add_to_rob(&arch_instr);
                    num_reads++;
		   // //cout << "cycle="<<current_core_cycle[cpu] << " cpu="<<cpu<<" addto rob -" <<ROB.occupancy << " "; 
		    
                    // branch prediction
                    if (arch_instr.is_branch) {

                        //DP( if (warmup_complete[cpu]) {
                        //cout << "[BRANCH] instr_id: " << instr_unique_id << " ip: " << hex << arch_instr.ip << dec << " taken: " << +arch_instr.branch_taken << endl; });

                        num_branch++;

                        /*
                        uint8_t branch_prediction;
                        // for faster simulation, force perfect prediction during the warmup
                        // note that branch predictor is still learning with real branch results
                        if (all_warmup_complete == 0)
                            branch_prediction = arch_instr.branch_taken; 
                        else
                            branch_prediction = predict_branch(arch_instr.ip);
                        */
                        uint8_t branch_prediction = predict_branch(arch_instr.ip);
                        
                        if (arch_instr.branch_taken != branch_prediction) {
			    //if(false) { // this simulates perfect branch prediction
			    branch_mispredictions++;

			    total_rob_occupancy_at_branch_mispredict += ROB.occupancy;

                            //DP( if (warmup_complete[cpu]) {
                            //cout << "[BRANCH] MISPREDICTED instr_id: " << instr_unique_id << " ip: " << hex << arch_instr.ip << dec;
                            //cout << " taken: " << +arch_instr.branch_taken << " predicted: " << +branch_prediction << endl; });

                            // halt any further fetch this cycle
                            instrs_to_read_this_cycle = 0;

                            // and stall any additional fetches until the branch is executed
                            fetch_stall = 1; 

                            ROB.entry[rob_index].branch_mispredicted = 1;
                        }
                        else {
                            if (branch_prediction == 1) {
                                // if we are accurately predicting a branch to be taken, then we can't possibly fetch down that path this cycle,
                                // so we have to wait until the next cycle to fetch those
                                instrs_to_read_this_cycle = 0;
                            }

                            //DP( if (warmup_complete[cpu]) {
                            //cout << "[BRANCH] PREDICTED    instr_id: " << instr_unique_id << " ip: " << hex << arch_instr.ip << dec;
                            //cout << " taken: " << +arch_instr.branch_taken << " predicted: " << +branch_prediction << endl; });
                        }

                        last_branch_result(arch_instr.ip, arch_instr.branch_taken);
                    }

                    //if ((num_reads == FETCH_WIDTH) || (ROB.occupancy == ROB.SIZE))
                    if ((num_reads >= instrs_to_read_this_cycle) || (ROB.occupancy == ROB.SIZE))
                        continue_reading = 0;
                }
                instr_unique_id++;
            }
        }
    }
////cout<<endl;
    //instrs_to_fetch_this_cycle = num_reads;
}

uint32_t O3_CPU::add_to_rob(ooo_model_instr *arch_instr)
{
	
    uint32_t index = ROB.tail;

    // sanity check
    if (ROB.entry[index].instr_id != 0) {
        cerr << "[ROB_ERROR] " << __func__ << " is not empty index: " << index;
        cerr << " instr_id: " << ROB.entry[index].instr_id << endl;
        assert(0);
    }

    ROB.entry[index] = *arch_instr;
    ROB.entry[index].event_cycle = current_core_cycle[cpu];

    ROB.occupancy++;
    ROB.tail++;
    if (ROB.tail >= ROB.SIZE)
        ROB.tail = 0;

    //DP ( if (warmup_complete[cpu]) {
    //cout << "[ROB] " <<  __func__ << " instr_id: " << ROB.entry[index].instr_id;
    //cout << " ip: " << hex << ROB.entry[index].ip << dec;
    //cout << " head: " << ROB.head << " tail: " << ROB.tail << " occupancy: " << ROB.occupancy;
    //cout << " event: " << ROB.entry[index].event_cycle << " current: " << current_core_cycle[cpu] << endl; });

#ifdef SANITY_CHECK
    if (ROB.entry[index].ip == 0) {
        cerr << "[ROB_ERROR] " << __func__ << " ip is zero index: " << index;
        cerr << " instr_id: " << ROB.entry[index].instr_id << " ip: " << ROB.entry[index].ip << endl;
        assert(0);
    }
#endif

    return index;
}

uint32_t O3_CPU::check_rob(uint64_t instr_id)
{
    if ((ROB.head == ROB.tail) && ROB.occupancy == 0)
        return ROB.SIZE;

    if (ROB.head < ROB.tail) {
        for (uint32_t i=ROB.head; i<ROB.tail; i++) {
            if (ROB.entry[i].instr_id == instr_id) {
                //DP ( if (warmup_complete[ROB.cpu]) {
                //cout << "[ROB] " << __func__ << " same instr_id: " << ROB.entry[i].instr_id;
                //cout << " rob_index: " << i << endl; });
                return i;
            }
        }
    }
    else {
        for (uint32_t i=ROB.head; i<ROB.SIZE; i++) {
            if (ROB.entry[i].instr_id == instr_id) {
                //DP ( if (warmup_complete[cpu]) {
                //cout << "[ROB] " << __func__ << " same instr_id: " << ROB.entry[i].instr_id;
                //cout << " rob_index: " << i << endl; });
                return i;
            }
        }
        for (uint32_t i=0; i<ROB.tail; i++) {
            if (ROB.entry[i].instr_id == instr_id) {
                //DP ( if (warmup_complete[cpu]) {
                //cout << "[ROB] " << __func__ << " same instr_id: " << ROB.entry[i].instr_id;
                //cout << " rob_index: " << i << endl; });
                return i;
            }
        }
    }
    
    cerr << "[ROB_ERROR] " << __func__ << " does not have any matching index! " << " head="<<ROB.head<<" tail="<<ROB.tail<<endl;
    cerr << " instr_id: " << instr_id << endl;
    assert(0);

    return ROB.SIZE;
}

void O3_CPU::fetch_instruction()
{
    // TODO: can we model wrong path execusion?

	
  // if we had a branch mispredict, turn fetching back on after the branch mispredict penalty
  ////////cout <<"prev, fetch_stall="<<fetch_stall<<" "<<current_core_cycle[cpu]<<" "<<fetch_resume_cycle<<endl;
  if((fetch_stall == 1) && (current_core_cycle[cpu] >= fetch_resume_cycle) && (fetch_resume_cycle != 0))
    {
	 //  cout<<"fetch_resume at "<< fetch_resume_cycle<<" " ;
      fetch_stall = 0;
      fetch_resume_cycle = 0;
      ////cout <<"****noww, fetch_stall="<<fetch_stall<<" "<<current_core_cycle[cpu]<<" "<<fetch_resume_cycle<<endl;
    }
  

    uint32_t fetch_index = (ROB.last_fetch == (ROB.SIZE-1)) ? 0 : (ROB.last_fetch + 1);
    for (uint32_t i=0; i<FETCH_WIDTH; i++) {

        // fetch is in-order so it should be break
	// @Vishal: TODO see if we can make out of order fetch as schedule_instruction is in order.
        // @Vishal: Removed transalation condition
	//if ((ROB.entry[fetch_index].translated != COMPLETED) || (ROB.entry[fetch_index].event_cycle > current_core_cycle[cpu]))
//  SCRATCHPADPRINT(cout << "1" << endl;)
	 if(ROB.entry[fetch_index].event_cycle > current_core_cycle[cpu]) 
            break;
	
//  SCRATCHPADPRINT(cout << "2 fetch_index " << fetch_index  << " ip " << ROB.entry[fetch_index].ip << endl;)
 // SCRATCHPADPRINT(print_ROB(fetch_index);)
        //@Vishal: check if empty ROB slot
	if(ROB.entry[fetch_index].ip == 0)
		break;

	//Nilesh: acclerators can fetch only after tranlation completed
/*	if(cpu >= ACCELERATOR_START)
		if(ROB.entry[fetch_index].translated != COMPLETED)
			break;
*/        // sanity check
 // SCRATCHPADPRINT(cout << "3" << endl;)
        if (ROB.entry[fetch_index].fetched) {
		
            if (fetch_index == ROB.head)
                break;
            else {
                cout <<" fetched assert cpu "<< cpu << "fetch_index: " << fetch_index << " ROB.head: " << ROB.head << " ROB.tail: " << ROB.tail << endl;
		break;			//@Nilesh: added for accelerator
                assert(0);
            }
        }

//  SCRATCHPADPRINT(cout << "4" << endl;)
        // add it to L1I
        PACKET fetch_packet;
        fetch_packet.instruction = 1;
        fetch_packet.fill_level = FILL_L1;
        fetch_packet.cpu = cpu;
	
	//@Vishal: VIPT virtual address will be sent to L1I instead of physical address
        /*fetch_packet.address = ROB.entry[fetch_index].instruction_pa >> 6;
        fetch_packet.instruction_pa = ROB.entry[fetch_index].instruction_pa;
        fetch_packet.full_addr = ROB.entry[fetch_index].instruction_pa;*/

/*	if(cpu >= ACCELERATOR_START)
		fetch_packet.address = ROB.entry[fetch_index].ip >> LOG2_PAGE_SIZE;
	else
 */
/*	if(knob_cloudsuite){
		fetch_packet.address =(((ROB.entry[fetch_index].ip >>LOG2_PAGE_SIZE ) << 9 ) | (256 + ROB.entry[fetch_index].asid[0])) << 6; 
	}
	else
*/		fetch_packet.address = ROB.entry[fetch_index].ip >> 6;
        fetch_packet.full_addr = ROB.entry[fetch_index].ip;	
	fetch_packet.full_virtual_address =  ROB.entry[fetch_index].ip;

        fetch_packet.instr_id = ROB.entry[fetch_index].instr_id;
        fetch_packet.rob_index = fetch_index;
        fetch_packet.producer = 0;
        fetch_packet.ip = ROB.entry[fetch_index].ip;
        fetch_packet.type = LOAD; 
        fetch_packet.asid[0] = ROB.entry[fetch_index].asid[0];
        fetch_packet.asid[1] = ROB.entry[fetch_index].asid[1];
        fetch_packet.event_cycle = current_core_cycle[cpu];

/*	if(fetch_packet.instr_id == 1481215 && cpu ==  0){
		cout << "cpu " << cpu << " inside fetch instruction for 1481215 " << endl;
		cout << "address in fetch instruction " << fetch_packet.address << " full add " << fetch_packet.full_virtual_address << endl;
	}
*/	int rq_index;
	if(cpu >= ACCELERATOR_START){	//Nilesh: added to the scratchpad instead of L1U
//		SCRATCHPADPRINT(cout << "address in fetch instruction " << fetch_packet.address << " full add " << fetch_packet.full_virtual_address << endl;)
//		if(cpu == 0 && fetch_packet.instr_id == 1481215)
//			cout << "adding to scrathcpad" << endl;
		fetch_packet.fill_level = FILL_SCRATCHPAD;
		rq_index = SCRATCHPAD.add_rq(&fetch_packet);
	}
	else
        	rq_index = L1I.add_rq(&fetch_packet);

//	SCRATCHPADPRINT(cout << "added to the L1I rq" << endl;)
        if (rq_index == -2)
            break;
        else {
            /*
            if (rq_index >= 0) {
                uint32_t producer = L1I.RQ.entry[rq_index].rob_index;
                ROB.entry[fetch_index].fetch_producer = producer;
                ROB.entry[fetch_index].is_consumer = 1;

                ROB.entry[producer].memory_instrs_depend_on_me[fetch_index] = 1;
                ROB.entry[producer].is_producer = 1;
            }
            */

            ROB.entry[fetch_index].fetched = INFLIGHT;
            ROB.last_fetch = fetch_index;
            fetch_index++;
            if (fetch_index == ROB.SIZE)
                fetch_index = 0;
        }
    }
    ////DP ( if (warmup_complete[cpu]) {
      //          //cout << "Exit fetch_instruction() with read instr id:"<< ROB.entry[read_index].instr_id << "fetch instr_id:"<< ROB.entry[fetch_index].instr_id<< endl; });
}

// TODO: When should we update ROB.schedule_event_cycle?
// I. Instruction is fetched
// II. Instruction is completed
// III. Instruction is retired
void O3_CPU::schedule_instruction()
{
    if ((ROB.head == ROB.tail) && ROB.occupancy == 0)
        return;

    // execution is out-of-order but we have an in-order scheduling algorithm to detect all RAW dependencies
    uint32_t limit = ROB.next_fetch[1];
    ////DP ( if (warmup_complete[cpu]) {	//*******************************************************************************************
      //          //cout << "Entered schedule_instruction() with instr id:"<< ROB.entry[limit].instr_id << endl; });
    num_searched = 0;
    if (ROB.head < limit) {
        for (uint32_t i=ROB.head; i<limit; i++) { 
            if ((ROB.entry[i].fetched != COMPLETED) || (ROB.entry[i].event_cycle > current_core_cycle[cpu]) || (num_searched >= SCHEDULER_SIZE))
                return;

            if (ROB.entry[i].scheduled == 0)
                do_scheduling(i);

            num_searched++;
        }
    }
    else {
        for (uint32_t i=ROB.head; i<ROB.SIZE; i++) {
            if ((ROB.entry[i].fetched != COMPLETED) || (ROB.entry[i].event_cycle > current_core_cycle[cpu]) || (num_searched >= SCHEDULER_SIZE))
                return;

            if (ROB.entry[i].scheduled == 0)
                do_scheduling(i);

            num_searched++;
        }
        for (uint32_t i=0; i<limit; i++) { 
            if ((ROB.entry[i].fetched != COMPLETED) || (ROB.entry[i].event_cycle > current_core_cycle[cpu]) || (num_searched >= SCHEDULER_SIZE))
                return;

            if (ROB.entry[i].scheduled == 0)
                do_scheduling(i);

            num_searched++;
        }
    }
    ////DP ( if (warmup_complete[cpu]) {	//*******************************************************************************************
      //          //cout << "Exiting schedule_instruction"<< endl; });
}

void O3_CPU::do_scheduling(uint32_t rob_index)
{
    ROB.entry[rob_index].reg_ready = 1; // reg_ready will be reset to 0 if there is RAW dependency 

    reg_dependency(rob_index);
    ROB.next_schedule = (rob_index == (ROB.SIZE - 1)) ? 0 : (rob_index + 1);

    if (ROB.entry[rob_index].is_memory)
        ROB.entry[rob_index].scheduled = INFLIGHT;
    else {
        ROB.entry[rob_index].scheduled = COMPLETED;

        // ADD LATENCY
        if (ROB.entry[rob_index].event_cycle < current_core_cycle[cpu])
            ROB.entry[rob_index].event_cycle = current_core_cycle[cpu] + SCHEDULING_LATENCY;
        else
            ROB.entry[rob_index].event_cycle += SCHEDULING_LATENCY;

        if (ROB.entry[rob_index].reg_ready) {

#ifdef SANITY_CHECK
            if (RTE1[RTE1_tail] < ROB_SIZE)
                assert(0);
#endif
            // remember this rob_index in the Ready-To-Execute array 1
            RTE1[RTE1_tail] = rob_index;

            //DP (if (warmup_complete[cpu]) {
            //cout << "[RTE1] " << __func__ << " instr_id: " << ROB.entry[rob_index].instr_id << " rob_index: " << rob_index << " is added to RTE1";
            //cout << " head: " << RTE1_head << " tail: " << RTE1_tail << endl; }); 

            RTE1_tail++;
            if (RTE1_tail == ROB_SIZE)
                RTE1_tail = 0;
        }
    }
}

void O3_CPU::reg_dependency(uint32_t rob_index)
{
    // print out source/destination registers
    /*DP (if (warmup_complete[cpu]) {
    for (uint32_t i=0; i<NUM_INSTR_SOURCES; i++) {
        if (ROB.entry[rob_index].source_registers[i]) {
            //cout << "[ROB] " << __func__ << " instr_id: " << ROB.entry[rob_index].instr_id << " is_memory: " << +ROB.entry[rob_index].is_memory;
            //cout << " load  reg_index: " << +ROB.entry[rob_index].source_registers[i] << endl;
        }
    }
    for (uint32_t i=0; i<MAX_INSTR_DESTINATIONS; i++) {
        if (ROB.entry[rob_index].destination_registers[i]) {
            //cout << "[ROB] " << __func__ << " instr_id: " << ROB.entry[rob_index].instr_id << " is_memory: " << +ROB.entry[rob_index].is_memory;
            //cout << " store reg_index: " << +ROB.entry[rob_index].destination_registers[i] << endl;
        }
    } }); */

    // check RAW dependency
    int prior = rob_index - 1;
    if (prior < 0)
        prior = ROB.SIZE - 1;

    if (rob_index != ROB.head) {
        if ((int)ROB.head <= prior) {
            for (int i=prior; i>=(int)ROB.head; i--) if (ROB.entry[i].executed != COMPLETED) {
		for (uint32_t j=0; j<NUM_INSTR_SOURCES; j++) {
			if (ROB.entry[rob_index].source_registers[j] && (ROB.entry[rob_index].reg_RAW_checked[j] == 0))
				reg_RAW_dependency(i, rob_index, j);
		}
	    }
        } else {
            for (int i=prior; i>=0; i--) if (ROB.entry[i].executed != COMPLETED) {
		for (uint32_t j=0; j<NUM_INSTR_SOURCES; j++) {
			if (ROB.entry[rob_index].source_registers[j] && (ROB.entry[rob_index].reg_RAW_checked[j] == 0))
				reg_RAW_dependency(i, rob_index, j);
		}
	    }
            for (int i=ROB.SIZE-1; i>=(int)ROB.head; i--) if (ROB.entry[i].executed != COMPLETED) {
		for (uint32_t j=0; j<NUM_INSTR_SOURCES; j++) {
			if (ROB.entry[rob_index].source_registers[j] && (ROB.entry[rob_index].reg_RAW_checked[j] == 0))
				reg_RAW_dependency(i, rob_index, j);
		}
	    }
        }
    }
}

void O3_CPU::reg_RAW_dependency(uint32_t prior, uint32_t current, uint32_t source_index)
{
    for (uint32_t i=0; i<MAX_INSTR_DESTINATIONS; i++) {
        if (ROB.entry[prior].destination_registers[i] == 0)
            continue;

        if (ROB.entry[prior].destination_registers[i] == ROB.entry[current].source_registers[source_index]) {

            // we need to mark this dependency in the ROB since the producer might not be added in the store queue yet
            ROB.entry[prior].registers_instrs_depend_on_me.insert (current);   // this load cannot be executed until the prior store gets executed
            ROB.entry[prior].registers_index_depend_on_me[source_index].insert (current);   // this load cannot be executed until the prior store gets executed
            ROB.entry[prior].reg_RAW_producer = 1;

            ROB.entry[current].reg_ready = 0;
            ROB.entry[current].producer_id = ROB.entry[prior].instr_id; 
            ROB.entry[current].num_reg_dependent++;
            ROB.entry[current].reg_RAW_checked[source_index] = 1;

            //DP (if(warmup_complete[cpu]) {
            //cout << "[ROB] " << __func__ << " instr_id: " << ROB.entry[current].instr_id << " is_memory: " << +ROB.entry[current].is_memory;
            //cout << " RAW reg_index: " << +ROB.entry[current].source_registers[source_index];
            //cout << " producer_id: " << ROB.entry[prior].instr_id << endl; });

            return;
        }
    }
}

void O3_CPU::execute_instruction()
{
    if ((ROB.head == ROB.tail) && ROB.occupancy == 0)
        return;

    // out-of-order execution for non-memory instructions
    // memory instructions are handled by memory_instruction()
    uint32_t exec_issued = 0, num_iteration = 0;

    while (exec_issued < EXEC_WIDTH) {
        if (RTE0[RTE0_head] < ROB_SIZE) {
            uint32_t exec_index = RTE0[RTE0_head];
            ////DP ( if (warmup_complete[cpu]) {	//*******************************************************************************************
              //  //cout << "Entered execute_instruction() with  RTE0 with instr_id: "<< ROB.entry[exec_index].instr_id<< endl; });
            if (ROB.entry[exec_index].event_cycle <= current_core_cycle[cpu]) {
                do_execution(exec_index);

                RTE0[RTE0_head] = ROB_SIZE;
                RTE0_head++;
                if (RTE0_head == ROB_SIZE)
                    RTE0_head = 0;
                exec_issued++;
            }
        }
        else {
            ////DP (if (warmup_complete[cpu]) {
            ////cout << "[RTE0] is empty head: " << RTE0_head << " tail: " << RTE0_tail << endl; });
            break;
        }

        num_iteration++;
        if (num_iteration == (ROB_SIZE-1))
            break;
    }

    num_iteration = 0;
    while (exec_issued < EXEC_WIDTH) {
        if (RTE1[RTE1_head] < ROB_SIZE) {
            uint32_t exec_index = RTE1[RTE1_head];
           // //DP ( if (warmup_complete[cpu]) {	//*******************************************************************************************
             //   //cout << "Entered execute_instruction() with  RTE1 with instr_id: "<< ROB.entry[exec_index].instr_id<< endl; });
	    if(ROB.entry[exec_index].instr_id == 29024){
             cout << "Entered execute_instruction() with  RTE1 with instr_id: "<< ROB.entry[exec_index].instr_id<< endl;
	    } 
            if (ROB.entry[exec_index].event_cycle <= current_core_cycle[cpu]) {
                do_execution(exec_index);

                RTE1[RTE1_head] = ROB_SIZE;
                RTE1_head++;
                if (RTE1_head == ROB_SIZE)
                    RTE1_head = 0;
                exec_issued++;
            }
        }
        else {
            ////DP (if (warmup_complete[cpu]) {
            ////cout << "[RTE1] is empty head: " << RTE1_head << " tail: " << RTE1_tail << endl; });
            break;
        }

        num_iteration++;
        if (num_iteration == (ROB_SIZE-1))
            break;
    }
    ////DP ( if (warmup_complete[cpu]) {	//*******************************************************************************************
      //          //cout << "Exit execute_instruction() "<< endl; });
}

void O3_CPU::do_execution(uint32_t rob_index)
{
    //if (ROB.entry[rob_index].reg_ready && (ROB.entry[rob_index].scheduled == COMPLETED) && (ROB.entry[rob_index].event_cycle <= current_core_cycle[cpu])) {
	    if(ROB.entry[rob_index].instr_id == 29024){
             cout << "Entered execute_instruction() with  RTE1 with instr_id: "<< ROB.entry[rob_index].instr_id<< endl;
	    } 
        // ADD LATENCY
     	     ROB.entry[rob_index].executed = INFLIGHT;
        if (ROB.entry[rob_index].event_cycle < current_core_cycle[cpu])
            ROB.entry[rob_index].event_cycle = current_core_cycle[cpu] + EXEC_LATENCY;
        else
            ROB.entry[rob_index].event_cycle += EXEC_LATENCY;

        inflight_reg_executions++;

        //DP (if (warmup_complete[cpu]) {
        //cout << "[ROB] " << __func__ << " non-memory instr_id: " << ROB.entry[rob_index].instr_id; 
        //cout << " event_cycle: " << ROB.entry[rob_index].event_cycle << endl;});
    //}
}

void O3_CPU::schedule_memory_instruction()
{
    if ((ROB.head == ROB.tail) && ROB.occupancy == 0)
        return;

    // execution is out-of-order but we have an in-order scheduling algorithm to detect all RAW dependencies
    uint32_t limit = ROB.next_schedule;
   // //DP ( if (warmup_complete[cpu]) {	//*******************************************************************************************
     //           //cout << "Entered schedule_memory_instruction() with  next_schedule instr_id: "<< ROB.entry[limit].instr_id<< endl; });
    num_searched = 0;
    if (ROB.head < limit) {
        for (uint32_t i=ROB.head; i<limit; i++) {

            if (ROB.entry[i].is_memory == 0)
                continue;

            if ((ROB.entry[i].fetched != COMPLETED) || (ROB.entry[i].event_cycle > current_core_cycle[cpu]) || (num_searched >= SCHEDULER_SIZE))
                break;

            if (ROB.entry[i].is_memory && ROB.entry[i].reg_ready && (ROB.entry[i].scheduled == INFLIGHT))
                do_memory_scheduling(i);
        }
    }
    else {
        for (uint32_t i=ROB.head; i<ROB.SIZE; i++) {

            if (ROB.entry[i].is_memory == 0)
                continue;

            if ((ROB.entry[i].fetched != COMPLETED) || (ROB.entry[i].event_cycle > current_core_cycle[cpu]) || (num_searched >= SCHEDULER_SIZE))
                break;

            if (ROB.entry[i].is_memory && ROB.entry[i].reg_ready && (ROB.entry[i].scheduled == INFLIGHT))
                do_memory_scheduling(i);
        }
        for (uint32_t i=0; i<limit; i++) {

            if (ROB.entry[i].is_memory == 0)
                continue;

            if ((ROB.entry[i].fetched != COMPLETED) || (ROB.entry[i].event_cycle > current_core_cycle[cpu]) || (num_searched >= SCHEDULER_SIZE))
                break;

            if (ROB.entry[i].is_memory && ROB.entry[i].reg_ready && (ROB.entry[i].scheduled == INFLIGHT))
                do_memory_scheduling(i);
        }
    }
   // //DP ( if (warmup_complete[cpu]) {	//*******************************************************************************************
     //           //cout << "Exit execute_instruction() "<< endl; });
}

void O3_CPU::execute_memory_instruction()
{
	////DP ( if (warmup_complete[cpu]) {	//*******************************************************************************************
    operate_lsq();
    operate_cache();
    ////DP ( if (warmup_complete[cpu]) {	//*******************************************************************************************
      //          //cout << "Exit execute_memory_instruction() "<< endl; });
}

void O3_CPU::do_memory_scheduling(uint32_t rob_index)
{
    uint32_t not_available = check_and_add_lsq(rob_index);
    if (not_available == 0) {
        ROB.entry[rob_index].scheduled = COMPLETED;
        if (ROB.entry[rob_index].executed == 0) {// it could be already set to COMPLETED due to store-to-load forwarding
         
	/*	if(ROB.entry[rob_index].instr_id == 29024){
			cout << " inside do memory scheduling " << endl;
			print_flag= true;
		}
	  */   	ROB.entry[rob_index].executed  = INFLIGHT;
	}

        //DP (if (warmup_complete[cpu]) {
        //cout << "[ROB] " << __func__ << " instr_id: " << ROB.entry[rob_index].instr_id << " rob_index: " << rob_index;
        //cout << " scheduled all num_mem_ops: " << ROB.entry[rob_index].num_mem_ops << endl; });
    }

    num_searched++;
}

uint32_t O3_CPU::check_and_add_lsq(uint32_t rob_index) 
{
    uint32_t num_mem_ops = 0, num_added = 0;

    // load
    for (uint32_t i=0; i<NUM_INSTR_SOURCES; i++) {
        if (ROB.entry[rob_index].source_memory[i]) {
            num_mem_ops++;
            if (ROB.entry[rob_index].source_added[i])
                num_added++;
            else if (LQ.occupancy < LQ.SIZE) {
                add_load_queue(rob_index, i);
                num_added++;
            }
            else {
                //DP(if(warmup_complete[cpu]) {
                //cout << "[LQ] " << __func__ << " instr_id: " << ROB.entry[rob_index].instr_id;
                //cout << " cannot be added in the load queue occupancy: " << LQ.occupancy << " cycle: " << current_core_cycle[cpu] << endl; });
            }
        }
    }

    // store
    for (uint32_t i=0; i<MAX_INSTR_DESTINATIONS; i++) {
        if (ROB.entry[rob_index].destination_memory[i]) {
            num_mem_ops++;
            if (ROB.entry[rob_index].destination_added[i])
                num_added++;
            else if (SQ.occupancy < SQ.SIZE) {
                if (STA[STA_head] == ROB.entry[rob_index].instr_id) {
                    add_store_queue(rob_index, i);
                    num_added++;
                }
                //add_store_queue(rob_index, i);
                //num_added++;
            }
            else {
                //DP(if(warmup_complete[cpu]) {
                //cout << "[SQ] " << __func__ << " instr_id: " << ROB.entry[rob_index].instr_id;
                //cout << " cannot be added in the store queue occupancy: " << SQ.occupancy << " cycle: " << current_core_cycle[cpu] << endl; });
            }
        }
    }

    if (num_added == num_mem_ops)
        return 0;

    uint32_t not_available = num_mem_ops - num_added;
    if (not_available > num_mem_ops) {
        cerr << "instr_id: " << ROB.entry[rob_index].instr_id << endl;
        assert(0);
    }

    return not_available;
}

void O3_CPU::add_load_queue(uint32_t rob_index, uint32_t data_index)
{
    // search for an empty slot 
    uint32_t lq_index = LQ.SIZE;
    for (uint32_t i=0; i<LQ.SIZE; i++) {
        if (LQ.entry[i].virtual_address == 0) {
            lq_index = i;
            break;
        }
    }

    // sanity check
    if (lq_index == LQ.SIZE) {
        cerr << "instr_id: " << ROB.entry[rob_index].instr_id << " no empty slot in the load queue!!!" << endl;
        assert(0);
    }

    sim_load_gen++;

    // add it to the load queue
    ROB.entry[rob_index].lq_index[data_index] = lq_index;
    LQ.entry[lq_index].instr_id = ROB.entry[rob_index].instr_id;
    LQ.entry[lq_index].virtual_address = ROB.entry[rob_index].source_memory[data_index];
    LQ.entry[lq_index].ip = ROB.entry[rob_index].ip;
    LQ.entry[lq_index].data_index = data_index;
    LQ.entry[lq_index].rob_index = rob_index;
    LQ.entry[lq_index].asid[0] = ROB.entry[rob_index].asid[0];
    LQ.entry[lq_index].asid[1] = ROB.entry[rob_index].asid[1];
    LQ.entry[lq_index].event_cycle = current_core_cycle[cpu] + SCHEDULING_LATENCY;
    LQ.occupancy++;

    // check RAW dependency
    int prior = rob_index - 1;
    if (prior < 0)
        prior = ROB.SIZE - 1;

    if (rob_index != ROB.head) {
        if ((int)ROB.head <= prior) {
            for (int i=prior; i>=(int)ROB.head; i--) {
                if (LQ.entry[lq_index].producer_id != UINT64_MAX)
                    break;

                    mem_RAW_dependency(i, rob_index, data_index, lq_index);
            }
        }
        else {
            for (int i=prior; i>=0; i--) {
                if (LQ.entry[lq_index].producer_id != UINT64_MAX)
                    break;

                    mem_RAW_dependency(i, rob_index, data_index, lq_index);
            }
            for (int i=ROB.SIZE-1; i>=(int)ROB.head; i--) { 
                if (LQ.entry[lq_index].producer_id != UINT64_MAX)
                    break;

                    mem_RAW_dependency(i, rob_index, data_index, lq_index);
            }
        }
    }

    // check
    // 1) if store-to-load forwarding is possible
    // 2) if there is WAR that are not correctly executed
    uint32_t forwarding_index = SQ.SIZE;
    for (uint32_t i=0; i<SQ.SIZE; i++) {

        // skip empty slot
        if (SQ.entry[i].virtual_address == 0)
            continue;

        // forwarding should be done by the SQ entry that holds the same producer_id from RAW dependency check
        if (SQ.entry[i].virtual_address == LQ.entry[lq_index].virtual_address) { // store-to-load forwarding check

            // forwarding store is in the SQ
            if ((rob_index != ROB.head) && (LQ.entry[lq_index].producer_id == SQ.entry[i].instr_id)) { // RAW
                forwarding_index = i;
                break; // should be break
            }

            if ((LQ.entry[lq_index].producer_id == UINT64_MAX) && (LQ.entry[lq_index].instr_id <= SQ.entry[i].instr_id)) { // WAR 
                // a load is about to be added in the load queue and we found a store that is 
                // "logically later in the program order but already executed" => this is not correctly executed WAR
                // due to out-of-order execution, this case is possible, for example
                // 1) application is load intensive and load queue is full
                // 2) we have loads that can't be added in the load queue
                // 3) subsequent stores logically behind in the program order are added in the store queue first

                // thanks to the store buffer, data is not written back to the memory system until retirement
                // also due to in-order retirement, this "already executed store" cannot be retired until we finish the prior load instruction 
                // if we detect WAR when a load is added in the load queue, just let the load instruction to access the memory system
                // no need to mark any dependency because this is actually WAR not RAW

                // do not forward data from the store queue since this is WAR
                // just read correct data from data cache

                LQ.entry[lq_index].physical_address = 0;
                LQ.entry[lq_index].translated = 0;
                LQ.entry[lq_index].fetched = 0;
                
                //DP(if(warmup_complete[cpu]) {
                //cout << "[LQ] " << __func__ << " instr_id: " << LQ.entry[lq_index].instr_id << " reset fetched: " << +LQ.entry[lq_index].fetched;
                //cout << " to obey WAR store instr_id: " << SQ.entry[i].instr_id << " cycle: " << current_core_cycle[cpu] << endl; });
            }
        }
    }

    if (forwarding_index != SQ.SIZE) { // we have a store-to-load forwarding

        if ((SQ.entry[forwarding_index].fetched == COMPLETED) && (SQ.entry[forwarding_index].event_cycle <= current_core_cycle[cpu])) {
           
	    //@Vishal: count RAW forwarding
	    sim_RAW_hits++;

            //@Vishal: VIPT, translation is not required, Just mark the entry as fetched
            //LQ.entry[lq_index].physical_address = (SQ.entry[forwarding_index].physical_address & ~(uint64_t) ((1 << LOG2_BLOCK_SIZE) - 1)) | (LQ.entry[lq_index].virtual_address & ((1 << LOG2_BLOCK_SIZE) - 1));
            LQ.entry[lq_index].translated = COMPLETED;
            LQ.entry[lq_index].fetched = COMPLETED;

            uint32_t fwr_rob_index = LQ.entry[lq_index].rob_index;
            ROB.entry[fwr_rob_index].num_mem_ops--;
            ROB.entry[fwr_rob_index].event_cycle = current_core_cycle[cpu];
            if (ROB.entry[fwr_rob_index].num_mem_ops < 0) {
                cerr << "instr_id: " << ROB.entry[fwr_rob_index].instr_id << endl;
                assert(0);
            }
            if (ROB.entry[fwr_rob_index].num_mem_ops == 0)
                inflight_mem_executions++;

            //DP(if(warmup_complete[cpu]) {
            //cout << "[LQ] " << __func__ << " instr_id: " << LQ.entry[lq_index].instr_id << hex;
            //cout << " full_addr: " << LQ.entry[lq_index].physical_address << dec << " is forwarded by store instr_id: ";
            //cout << SQ.entry[forwarding_index].instr_id << " remain_num_ops: " << ROB.entry[fwr_rob_index].num_mem_ops << " cycle: " << current_core_cycle[cpu] << endl; });

            release_load_queue(lq_index);
        }
        else
            ; // store is not executed yet, forwarding will be handled by execute_store()
    }

    // succesfully added to the load queue
    ROB.entry[rob_index].source_added[data_index] = 1;

    if (LQ.entry[lq_index].virtual_address && (LQ.entry[lq_index].producer_id == UINT64_MAX)) { // not released and no forwarding
        RTL0[RTL0_tail] = lq_index;
        RTL0_tail++;
        if (RTL0_tail == LQ_SIZE)
            RTL0_tail = 0;

        //DP (if (warmup_complete[cpu]) {
        //cout << "[RTL0] " << __func__ << " instr_id: " << LQ.entry[lq_index].instr_id << " rob_index: " << LQ.entry[lq_index].rob_index << " is added to RTL0";
        //cout << " head: " << RTL0_head << " tail: " << RTL0_tail << endl; }); 
    }

    //DP(if(warmup_complete[cpu]) {
    //cout << "[LQ] " << __func__ << " instr_id: " << LQ.entry[lq_index].instr_id;
    //cout << " is added in the LQ address: " << hex << LQ.entry[lq_index].virtual_address << dec << " translated: " << +LQ.entry[lq_index].translated;
    //cout << " fetched: " << +LQ.entry[lq_index].fetched << " index: " << lq_index << " occupancy: " << LQ.occupancy << " cycle: " << current_core_cycle[cpu] << endl; });
}

void O3_CPU::mem_RAW_dependency(uint32_t prior, uint32_t current, uint32_t data_index, uint32_t lq_index)
{
    for (uint32_t i=0; i<MAX_INSTR_DESTINATIONS; i++) {
        if (ROB.entry[prior].destination_memory[i] == 0)
            continue;

        if (ROB.entry[prior].destination_memory[i] == ROB.entry[current].source_memory[data_index]) { //  store-to-load forwarding check

            // we need to mark this dependency in the ROB since the producer might not be added in the store queue yet
            ROB.entry[prior].memory_instrs_depend_on_me.insert (current);   // this load cannot be executed until the prior store gets executed
            ROB.entry[prior].is_producer = 1;
            LQ.entry[lq_index].producer_id = ROB.entry[prior].instr_id; 
            LQ.entry[lq_index].translated = INFLIGHT;

            //DP (if(warmup_complete[cpu]) {
            //cout << "[LQ] " << __func__ << " RAW producer instr_id: " << ROB.entry[prior].instr_id << " consumer_id: " << ROB.entry[current].instr_id << " lq_index: " << lq_index;
            //cout << hex << " address: " << ROB.entry[prior].destination_memory[i] << dec << endl; });

            return;
        }
    }
}

void O3_CPU::add_store_queue(uint32_t rob_index, uint32_t data_index)
{
    uint32_t sq_index = SQ.tail;
#ifdef SANITY_CHECK
    if (SQ.entry[sq_index].virtual_address)
        assert(0);
#endif

    /*
    // search for an empty slot 
    uint32_t sq_index = SQ.SIZE;
    for (uint32_t i=0; i<SQ.SIZE; i++) {
        if (SQ.entry[i].virtual_address == 0) {
            sq_index = i;
            break;
        }
    }

    // sanity check
    if (sq_index == SQ.SIZE) {
        cerr << "instr_id: " << ROB.entry[rob_index].instr_id << " no empty slot in the store queue!!!" << endl;
        assert(0);
    }
    */

    sim_store_gen++;

    // add it to the store queue
    ROB.entry[rob_index].sq_index[data_index] = sq_index;
    SQ.entry[sq_index].instr_id = ROB.entry[rob_index].instr_id;
    SQ.entry[sq_index].virtual_address = ROB.entry[rob_index].destination_memory[data_index];
    SQ.entry[sq_index].ip = ROB.entry[rob_index].ip;
    SQ.entry[sq_index].data_index = data_index;
    SQ.entry[sq_index].rob_index = rob_index;
    SQ.entry[sq_index].asid[0] = ROB.entry[rob_index].asid[0];
    SQ.entry[sq_index].asid[1] = ROB.entry[rob_index].asid[1];
    SQ.entry[sq_index].event_cycle = current_core_cycle[cpu] + SCHEDULING_LATENCY;
    
//    cout << "added to the sq " << sq_index << " rob " << rob_index << " data_index " << data_index << endl;

    SQ.occupancy++;
    SQ.tail++;
    if (SQ.tail == SQ.SIZE)
        SQ.tail = 0;

    // succesfully added to the store queue
    ROB.entry[rob_index].destination_added[data_index] = 1;
    
    STA[STA_head] = UINT64_MAX;
    STA_head++;
    if (STA_head == STA_SIZE)
        STA_head = 0;

    RTS0[RTS0_tail] = sq_index;
    RTS0_tail++;
    if (RTS0_tail == SQ_SIZE)
        RTS0_tail = 0;

    //DP(if(warmup_complete[cpu]) {
    //cout << "[SQ] " << __func__ << " instr_id: " << SQ.entry[sq_index].instr_id;
    //cout << " is added in the SQ translated: " << +SQ.entry[sq_index].translated << " fetched: " << +SQ.entry[sq_index].fetched << " is_producer: " << +ROB.entry[rob_index].is_producer;
    //cout << " cycle: " << current_core_cycle[cpu] << endl; });
}

void O3_CPU::operate_lsq()
{
    // handle store
    uint32_t store_issued = 0, num_iteration = 0;

//	if(cpu >= ACCELERATOR_START){
//		print_LSQ(&SQ);
	/*	cout << "RTS0 *********" ;
			for(int i= 0;i < SQ_SIZE;i++)
				cout << RTS0[i] << " ";
			cout << endl;
		
		cout << "RTS1 *********" ;
			for(int i= 0;i < SQ_SIZE;i++)
				cout << RTS1[i] << " ";
			cout << endl;
*/
//		print_LSQ(&LQ);
/*		cout << "RTl0 *********" ;
			for(int i= 0;i < LQ_SIZE;i++)
				cout << RTL0[i] << " ";
			cout << endl;
		cout << "RTl1 *********" ;
			for(int i= 0;i < LQ_SIZE;i++)
				cout << RTL1[i] << " ";
*/			
//	}
    //@Vishal: VIPT Execute store without sending translation request to DTLB.
    //@Nilesh: Translation handles by DTLB in accelerators
   // if(cpu < ACCELERATOR_START){
	    while (store_issued < SQ_WIDTH) {
		if (RTS0[RTS0_head] < SQ_SIZE) {
		    uint32_t sq_index = RTS0[RTS0_head];
		    if (SQ.entry[sq_index].event_cycle <= current_core_cycle[cpu]) {
			execute_store(SQ.entry[sq_index].rob_index, sq_index, SQ.entry[sq_index].data_index);

			RTS0[RTS0_head] = SQ_SIZE;
			RTS0_head++;
			if (RTS0_head == SQ_SIZE)
			    RTS0_head = 0;

			store_issued++;
		    }
		}
		else {
		    ////DP (if (warmup_complete[cpu]) {
		    ////cout << "[RTS0] is empty head: " << RTS0_head << " tail: " << RTS0_tail << endl; });
		    break;
		}

		num_iteration++;
		if (num_iteration == (SQ_SIZE-1))
		    break;
	    }
   // }
   /* else{ //if Accelerator
	    while (store_issued < SQ_WIDTH) {
		if (RTS0[RTS0_head] < SQ_SIZE) {
		    uint32_t sq_index = RTS0[RTS0_head];
		    if (SQ.entry[sq_index].event_cycle <= current_core_cycle[cpu]) {

			// add it to DTLB
			PACKET data_packet;

			data_packet.tlb_access = 1;
			data_packet.fill_level = FILL_L1;
			data_packet.cpu = cpu;
			data_packet.data_index = SQ.entry[sq_index].data_index;
			data_packet.sq_index = sq_index;
			if (knob_cloudsuite)
			    data_packet.address = ((SQ.entry[sq_index].virtual_address >> LOG2_PAGE_SIZE) << 9) | SQ.entry[sq_index].asid[1];
			else
			    data_packet.address = SQ.entry[sq_index].virtual_address >> LOG2_PAGE_SIZE;
			data_packet.full_addr = SQ.entry[sq_index].virtual_address;
			data_packet.full_virtual_address= SQ.entry[sq_index].virtual_address;
			data_packet.instr_id = SQ.entry[sq_index].instr_id;
			data_packet.rob_index = SQ.entry[sq_index].rob_index;
			data_packet.ip = SQ.entry[sq_index].ip;
			data_packet.type = RFO;
			data_packet.asid[0] = SQ.entry[sq_index].asid[0];
			data_packet.asid[1] = SQ.entry[sq_index].asid[1];
			data_packet.event_cycle = SQ.entry[sq_index].event_cycle;
			data_packet.tlb_start_cycle = current_core_cycle[cpu];
			//DP (if (warmup_complete[cpu]) {
			//cout << "[RTS0] " << __func__ << " instr_id: " << SQ.entry[sq_index].instr_id << " rob_index: " << SQ.entry[sq_index].rob_index << " is popped from to RTS0";
			//cout << " head: " << RTS0_head << " tail: " << RTS0_tail << endl; }); 

			int rq_index = DTLB.add_rq(&data_packet);

			if(rq_index == -2)
			    break; 
			else 
			    SQ.entry[sq_index].translated = INFLIGHT;

			RTS0[RTS0_head] = SQ_SIZE;
			RTS0_head++;
			if (RTS0_head == SQ_SIZE)
			    RTS0_head = 0;

			store_issued++;
		    }
		}
		else {
		    ////DP (if (warmup_complete[cpu]) {
		    ////cout << "[RTS0] is empty head: " << RTS0_head << " tail: " << RTS0_tail << endl; });
		    break;
		}

		num_iteration++;
		if (num_iteration == (SQ_SIZE-1))
		    break;
	    }

	    num_iteration = 0;
	    while (store_issued < SQ_WIDTH) {
		if (RTS1[RTS1_head] < SQ_SIZE) {
		    uint32_t sq_index = RTS1[RTS1_head];
		    if (SQ.entry[sq_index].event_cycle <= current_core_cycle[cpu]) {
			execute_store(SQ.entry[sq_index].rob_index, sq_index, SQ.entry[sq_index].data_index);

			RTS1[RTS1_head] = SQ_SIZE;
			RTS1_head++;
			if (RTS1_head == SQ_SIZE)
			    RTS1_head = 0;

			store_issued++;
		    }
		}
		else {
		    ////DP (if (warmup_complete[cpu]) {
		    ////cout << "[RTS1] is empty head: " << RTS1_head << " tail: " << RTS1_tail << endl; });
		    break;
		}

		num_iteration++;
		if (num_iteration == (SQ_SIZE-1))
		    break;
	    }
    }*/

    unsigned load_issued = 0;
    num_iteration = 0;

    //@Vishal: VIPT. Send request to L1D.
    //@Nilesh: Check for accelerator
   // if(cpu < ACCELERATOR_START){
/*	if(print_flag){
		for(int i=0;i<LQ_SIZE;i++){
			cout << RTL0[i] << " ";
		}
		cout << endl;
	}*/
	    while (load_issued < LQ_WIDTH) {
		if (RTL0[RTL0_head] < LQ_SIZE) {
		    uint32_t lq_index = RTL0[RTL0_head];
		    if (LQ.entry[lq_index].event_cycle <= current_core_cycle[cpu]) {

			int rq_index = execute_load(LQ.entry[lq_index].rob_index, lq_index, LQ.entry[lq_index].data_index);

			if (rq_index != -2) {
			    RTL0[RTL0_head] = LQ_SIZE;
				RTL0_head++;
				if (RTL0_head == LQ_SIZE)
				    RTL0_head = 0;

			    load_issued++;
			}
		    }
		}
		else {
		    ////DP (if (warmup_complete[cpu]) {
		    ////cout << "[RTL1] is empty head: " << RTL1_head << " tail: " << RTL1_tail << endl; });
		    break;
		}

		num_iteration++;
		if (num_iteration == (LQ_SIZE-1))
		    break;
	    }
   // }    
   /* else{	//Accelerator
	    while (load_issued < LQ_WIDTH) {
		if (RTL0[RTL0_head] < LQ_SIZE) {
		    uint32_t lq_index = RTL0[RTL0_head];
		    if (LQ.entry[lq_index].event_cycle <= current_core_cycle[cpu]) {

			// add it to DTLB
			PACKET data_packet;
			data_packet.fill_level = FILL_L1;
			data_packet.cpu = cpu;
			data_packet.data_index = LQ.entry[lq_index].data_index;
			data_packet.lq_index = lq_index;
			if (knob_cloudsuite)
			    data_packet.address = ((LQ.entry[lq_index].virtual_address >> LOG2_PAGE_SIZE) << 9) | LQ.entry[lq_index].asid[1];
			else
			    data_packet.address = LQ.entry[lq_index].virtual_address >> LOG2_PAGE_SIZE;
			data_packet.full_addr = LQ.entry[lq_index].virtual_address;
			data_packet.full_virtual_address= LQ.entry[lq_index].virtual_address;
			data_packet.instr_id = LQ.entry[lq_index].instr_id;
			data_packet.rob_index = LQ.entry[lq_index].rob_index;
			data_packet.ip = LQ.entry[lq_index].ip;
			data_packet.type = LOAD;
			data_packet.asid[0] = LQ.entry[lq_index].asid[0];
			data_packet.asid[1] = LQ.entry[lq_index].asid[1];
			data_packet.event_cycle = LQ.entry[lq_index].event_cycle;
			data_packet.tlb_start_cycle = current_core_cycle[cpu];

			//DP (if (warmup_complete[cpu]) {
			//cout << "[RTL0] " << __func__ << " instr_id: " << LQ.entry[lq_index].instr_id << " rob_index: " << LQ.entry[lq_index].rob_index << " is popped to RTL0";
			//cout << " head: " << RTL0_head << " tail: " << RTL0_tail << endl; }); 

			int rq_index = DTLB.add_rq(&data_packet);

			if (rq_index == -2)
			    break; // break here
			else  
			    LQ.entry[lq_index].translated = INFLIGHT;
			RTL0[RTL0_head] = LQ_SIZE;
			RTL0_head++;
			if (RTL0_head == LQ_SIZE)
			    RTL0_head = 0;

			load_issued++;
		    }
		}
		else {
		    ////DP (if (warmup_complete[cpu]) {
		    ////cout << "[RTL0] is empty head: " << RTL0_head << " tail: " << RTL0_tail << endl; });
		    break;
		}

		num_iteration++;
		if (num_iteration == (LQ_SIZE-1))
		    break;
	    }

	    num_iteration = 0;
	    while (load_issued < LQ_WIDTH) {
		if (RTL1[RTL1_head] < LQ_SIZE) {
		    uint32_t lq_index = RTL1[RTL1_head];
		    if (LQ.entry[lq_index].event_cycle <= current_core_cycle[cpu]) {
			int rq_index = execute_load(LQ.entry[lq_index].rob_index, lq_index, LQ.entry[lq_index].data_index);

			if (rq_index != -2) {
			    RTL1[RTL1_head] = LQ_SIZE;
			    RTL1_head++;
			    if (RTL1_head == LQ_SIZE)
				RTL1_head = 0;

			    load_issued++;
			}
		    }
		}
		else {
		    ////DP (if (warmup_complete[cpu]) {
		    ////cout << "[RTL1] is empty head: " << RTL1_head << " tail: " << RTL1_tail << endl; });
		    break;
		}

		num_iteration++;
		if (num_iteration == (LQ_SIZE-1))
		    break;
	    }
    }*/
}

void O3_CPU::execute_store(uint32_t rob_index, uint32_t sq_index, uint32_t data_index)
{
    SQ.entry[sq_index].fetched = COMPLETED;
    SQ.entry[sq_index].event_cycle = current_core_cycle[cpu];
   
    ROB.entry[rob_index].num_mem_ops--;
    ROB.entry[rob_index].event_cycle = current_core_cycle[cpu];
    if (ROB.entry[rob_index].num_mem_ops < 0) {
        cerr << "instr_id: " << ROB.entry[rob_index].instr_id << endl;
        assert(0);
    }

    if (ROB.entry[rob_index].num_mem_ops == 0)	
    	inflight_mem_executions++;
    
       //DP (if (warmup_complete[cpu]) {
    //cout << "[SQ1] " << __func__ << " instr_id: " << SQ.entry[sq_index].instr_id << hex;
    //cout << " full_address: " << SQ.entry[sq_index].physical_address << dec << " remain_mem_ops: " << ROB.entry[rob_index].num_mem_ops;
    //cout << " event_cycle: " << SQ.entry[sq_index].event_cycle << endl; });

    // resolve RAW dependency after DTLB access
    // check if this store has dependent loads
    if (ROB.entry[rob_index].is_producer) {
	ITERATE_SET(dependent,ROB.entry[rob_index].memory_instrs_depend_on_me, ROB_SIZE) {
            // check if dependent loads are already added in the load queue
            for (uint32_t j=0; j<NUM_INSTR_SOURCES; j++) { // which one is dependent?
                if (ROB.entry[dependent].source_memory[j] && ROB.entry[dependent].source_added[j]) {
                    if (ROB.entry[dependent].source_memory[j] == SQ.entry[sq_index].virtual_address) { // this is required since a single instruction can issue multiple loads
			
                         //@Vishal: count RAW forwarding
			 sim_RAW_hits++;

                        // now we can resolve RAW dependency
                        uint32_t lq_index = ROB.entry[dependent].lq_index[j];
#ifdef SANITY_CHECK
                        if (lq_index >= LQ.SIZE)
                            assert(0);
                        if (LQ.entry[lq_index].producer_id != SQ.entry[sq_index].instr_id) {
                            cerr << "[SQ2] " << __func__ << " lq_index: " << lq_index << " producer_id: " << LQ.entry[lq_index].producer_id;
                            cerr << " does not match to the store instr_id: " << SQ.entry[sq_index].instr_id << endl;
                            assert(0);
                        }
#endif
                        // update correspodning LQ entry
                        // @Vishal: Dependent load can now get the data, translation is not required
                        //LQ.entry[lq_index].physical_address = (SQ.entry[sq_index].physical_address & ~(uint64_t) ((1 << LOG2_BLOCK_SIZE) - 1)) | (LQ.entry[lq_index].virtual_address & ((1 << LOG2_BLOCK_SIZE) - 1));
                        LQ.entry[lq_index].translated = COMPLETED;
                        LQ.entry[lq_index].fetched = COMPLETED;
                        LQ.entry[lq_index].event_cycle = current_core_cycle[cpu];

                        uint32_t fwr_rob_index = LQ.entry[lq_index].rob_index;
                        ROB.entry[fwr_rob_index].num_mem_ops--;
                        ROB.entry[fwr_rob_index].event_cycle = current_core_cycle[cpu];
#ifdef SANITY_CHECK
                        if (ROB.entry[fwr_rob_index].num_mem_ops < 0) {
                            cerr << "instr_id: " << ROB.entry[fwr_rob_index].instr_id << endl;
                            assert(0);
                        }
#endif
                        if (ROB.entry[fwr_rob_index].num_mem_ops == 0)
                            inflight_mem_executions++;

                        //DP(if(warmup_complete[cpu]) {
                        //cout << "[LQ3] " << __func__ << " instr_id: " << LQ.entry[lq_index].instr_id << hex;
                        //cout << " full_addr: " << LQ.entry[lq_index].physical_address << dec << " is forwarded by store instr_id: ";
                        //cout << SQ.entry[sq_index].instr_id << " remain_num_ops: " << ROB.entry[fwr_rob_index].num_mem_ops << " cycle: " << current_core_cycle[cpu] << endl; });

                        release_load_queue(lq_index);

                        // clear dependency bit
                        if (j == (NUM_INSTR_SOURCES-1))
                            ROB.entry[rob_index].memory_instrs_depend_on_me.insert (dependent);
                    }
                }
            }
        }
    }
}

int O3_CPU::execute_load(uint32_t rob_index, uint32_t lq_index, uint32_t data_index)
{
    // add it to L1D
/*    if(ROB.entry[rob_index].instr_id ==  29024) {
    	cout << " inside execurte load for rob_index " << rob_index << endl;
    	
    }
 */   PACKET data_packet;
    data_packet.fill_level = FILL_L1;
    data_packet.cpu = cpu;
    data_packet.data_index = LQ.entry[lq_index].data_index;
    data_packet.lq_index = lq_index;

    //@Vishal: VIPT send virtual address instead of physical address
    //data_packet.address = LQ.entry[lq_index].physical_address >> LOG2_BLOCK_SIZE;
    //data_packet.full_addr = LQ.entry[lq_index].physical_address;

/*    if(cpu >= ACCELERATOR_START)
	    data_packet.address = LQ.entry[lq_index].virtual_address >> LOG2_PAGE_SIZE;
    else
*/	
/*    if(knob_cloudsuite){
	    data_packet.address =(((LQ.entry[lq_index].virtual_address >>LOG2_PAGE_SIZE ) << 9 ) | ( ROB.entry[lq_index].asid[1])) << 6; 
    }
    else*/
	    data_packet.address = LQ.entry[lq_index].virtual_address >> LOG2_BLOCK_SIZE;
    data_packet.full_addr = LQ.entry[lq_index].virtual_address;
    data_packet.full_virtual_address = LQ.entry[lq_index].virtual_address;

    data_packet.instr_id = LQ.entry[lq_index].instr_id;
    data_packet.rob_index = LQ.entry[lq_index].rob_index;
    data_packet.ip = LQ.entry[lq_index].ip;
    data_packet.type = LOAD;
    data_packet.asid[0] = LQ.entry[lq_index].asid[0];
    data_packet.asid[1] = LQ.entry[lq_index].asid[1];
    data_packet.event_cycle = LQ.entry[lq_index].event_cycle;
    
    int rq_index;
    if(cpu >= ACCELERATOR_START){	//Nilesh: add to the Scratchpad instead L1D
    /*	data_packet.address = LQ.entry[lq_index].physical_address >> LOG2_BLOCK_SIZE;
        data_packet.full_addr = LQ.entry[lq_index].physical_address;
        data_packet.fill_level = FILL_DRAM;
        rq_index = uncore.DRAM.add_rq(&data_packet);*/
	    rq_index = SCRATCHPAD.add_rq(&data_packet);

    }
    else
	    rq_index = L1D.add_rq(&data_packet);

    if (rq_index == -2)
        return rq_index;
    else 
    {
        LQ.entry[lq_index].fetched = INFLIGHT;
	
	sim_load_sent++;
    }

    return rq_index;
}

void O3_CPU::complete_execution(uint32_t rob_index)
{
	////DP ( if (warmup_complete[cpu]) {	//*******************************************************************************************
          //      //cout << "Entered complete_execution() "<< endl; });
    if (ROB.entry[rob_index].is_memory == 0) {
        if ((ROB.entry[rob_index].executed == INFLIGHT) && (ROB.entry[rob_index].event_cycle <= current_core_cycle[cpu])) {

            ROB.entry[rob_index].executed = COMPLETED; 
            inflight_reg_executions--;
            completed_executions++;

            if (ROB.entry[rob_index].reg_RAW_producer)
                reg_RAW_release(rob_index);

            if (ROB.entry[rob_index].branch_mispredicted)
	      {
		fetch_resume_cycle = current_core_cycle[cpu] + BRANCH_MISPREDICT_PENALTY;
		//fetch_stall = 0;
	      }

            //DP(if(warmup_complete[cpu]) {
            //cout << "[ROB] " << __func__ << " instr_id: " << ROB.entry[rob_index].instr_id;
            //cout << " branch_mispredicted: " << +ROB.entry[rob_index].branch_mispredicted << " fetch_stall: " << +fetch_stall;
            //cout << " event: " << ROB.entry[rob_index].event_cycle << endl; });
        }
    }
    else {
        if (ROB.entry[rob_index].num_mem_ops == 0) {
            if ((ROB.entry[rob_index].executed == INFLIGHT) && (ROB.entry[rob_index].event_cycle <= current_core_cycle[cpu])) {
                ROB.entry[rob_index].executed = COMPLETED;
                inflight_mem_executions--;
                completed_executions++;
                
                if (ROB.entry[rob_index].reg_RAW_producer)
                    reg_RAW_release(rob_index);

                if (ROB.entry[rob_index].branch_mispredicted)
		  {
		    fetch_resume_cycle = current_core_cycle[cpu] + BRANCH_MISPREDICT_PENALTY;
		    //fetch_stall = 0;
		  }

                //DP(if(warmup_complete[cpu]) {
                //cout << "[ROB] " << __func__ << " instr_id: " << ROB.entry[rob_index].instr_id;
                //cout << " is_memory: " << +ROB.entry[rob_index].is_memory << " branch_mispredicted: " << +ROB.entry[rob_index].branch_mispredicted;
                //cout << " fetch_stall: " << +fetch_stall << " event: " << ROB.entry[rob_index].event_cycle << " current: " << current_core_cycle[cpu] << endl; });
            }
        }
    }
    ////DP ( if (warmup_complete[cpu]) {	//*******************************************************************************************
      //          //cout << "Exit complete_execution() "<< endl; });
}

void O3_CPU::reg_RAW_release(uint32_t rob_index)
{
    // if (!ROB.entry[rob_index].registers_instrs_depend_on_me.empty()) 

    ITERATE_SET(i,ROB.entry[rob_index].registers_instrs_depend_on_me, ROB_SIZE) {
        for (uint32_t j=0; j<NUM_INSTR_SOURCES; j++) {
            if (ROB.entry[rob_index].registers_index_depend_on_me[j].search (i)) {
                ROB.entry[i].num_reg_dependent--;

                if (ROB.entry[i].num_reg_dependent == 0) {
                    ROB.entry[i].reg_ready = 1;
                    if (ROB.entry[i].is_memory)
                        ROB.entry[i].scheduled = INFLIGHT;
                    else {
                        ROB.entry[i].scheduled = COMPLETED;

#ifdef SANITY_CHECK
                        if (RTE0[RTE0_tail] < ROB_SIZE)
                            assert(0);
#endif
                        // remember this rob_index in the Ready-To-Execute array 0
                        RTE0[RTE0_tail] = i;

                        //DP (if (warmup_complete[cpu]) {
                        //cout << "[RTE0] " << __func__ << " instr_id: " << ROB.entry[i].instr_id << " rob_index: " << i << " is added to RTE0";
                        //cout << " head: " << RTE0_head << " tail: " << RTE0_tail << endl; }); 

                        RTE0_tail++;
                        if (RTE0_tail == ROB_SIZE)
                            RTE0_tail = 0;

                    }
                }

                //DP (if (warmup_complete[cpu]) {
                //cout << "[ROB] " << __func__ << " instr_id: " << ROB.entry[rob_index].instr_id << " releases instr_id: ";
                //cout << ROB.entry[i].instr_id << " reg_index: " << +ROB.entry[i].source_registers[j] << " num_reg_dependent: " << ROB.entry[i].num_reg_dependent << " cycle: " << current_core_cycle[cpu] << endl; });
            }
        }
    }
}

void O3_CPU::operate_cache()
{
    if(cpu >= ACCELERATOR_START){	//Nilesh: scratchpad operate
	    SCRATCHPAD.operate();
    }
    ITLB.operate();
    DTLB.operate();
    if(cpu <= ACCELERATOR_START){	//Nilesh:  for acclerators
	    STLB.operate();
#ifdef INS_PAGE_TABLE_WALKER
	    PTW.operate();
#endif
    }
  /*  if(cpu == NUM_CPUS -1){		//Nilesh: STLB will be called only once after completing all the accelerators
	    STLB.operate();
#ifdef INS_PAGE_TABLE_WALKER
	    PTW.operate();
#endif
    
    }*/
    if(cpu < ACCELERATOR_START){	//Nilesh: Caches will not be called for accelerators
	    L1I.operate();
	    L1D.operate();
	    L2C.operate();
    }

}

void O3_CPU::update_rob()
{
    //@Vishal: VIPT ITLB processed entries will be handled by L1I cache.
    //@Nilesh: check for acclerators
    if(cpu >= ACCELERATOR_START){	//Nilesh: only check for scratchpad processed queue
		/*if (ITLB.PROCESSED.occupancy && (ITLB.PROCESSED.entry[ITLB.PROCESSED.head].event_cycle <= current_core_cycle[cpu]))
			complete_instr_fetch(&ITLB.PROCESSED, 1);
		if (DTLB.PROCESSED.occupancy && (DTLB.PROCESSED.entry[DTLB.PROCESSED.head].event_cycle <= current_core_cycle[cpu]))
			complete_data_fetch(&DTLB.PROCESSED, 1);
		if (uncore.DRAM.PROCESSED[cpu].occupancy && (uncore.DRAM.PROCESSED[cpu].entry[uncore.DRAM.PROCESSED[cpu].head].event_cycle <= current_core_cycle[cpu]) &&  uncore.DRAM.PROCESSED[cpu].entry[uncore.DRAM.PROCESSED[cpu].head].instruction )
                complete_instr_fetch(&uncore.DRAM.PROCESSED[cpu], 0);

            if  (uncore.DRAM.PROCESSED[cpu].occupancy && (uncore.DRAM.PROCESSED[cpu].entry[uncore.DRAM.PROCESSED[cpu].head].event_cycle <= current_core_cycle[cpu]) && uncore.DRAM.PROCESSED[cpu].entry[uncore.DRAM.PROCESSED[cpu].head].instruction == 0 )
                complete_data_fetch(&uncore.DRAM.PROCESSED[cpu], 0);*/

	    if(SCRATCHPAD.PROCESSED.occupancy && SCRATCHPAD.PROCESSED.entry[SCRATCHPAD.PROCESSED.head].event_cycle <= current_core_cycle[cpu]){
		if(SCRATCHPAD.PROCESSED.entry[SCRATCHPAD.PROCESSED.head].instruction)
		    complete_instr_fetch(&SCRATCHPAD.PROCESSED,0);
		else
			complete_data_fetch(&SCRATCHPAD.PROCESSED,0);
	    }


    }

    else{
	    if (L1I.PROCESSED.occupancy && (L1I.PROCESSED.entry[L1I.PROCESSED.head].event_cycle <= current_core_cycle[cpu]))
		complete_instr_fetch(&L1I.PROCESSED, 0);
	    if (L1D.PROCESSED.occupancy && (L1D.PROCESSED.entry[L1D.PROCESSED.head].event_cycle <= current_core_cycle[cpu]))
		complete_data_fetch(&L1D.PROCESSED, 0);
    }

    // update ROB entries with completed executions
    if ((inflight_reg_executions > 0) || (inflight_mem_executions > 0)) {
        if (ROB.head < ROB.tail) {
            for (uint32_t i=ROB.head; i<ROB.tail; i++) 
                complete_execution(i);
        }
        else {
            for (uint32_t i=ROB.head; i<ROB.SIZE; i++)
                complete_execution(i);
            for (uint32_t i=0; i<ROB.tail; i++)
                complete_execution(i);
        }
    }
}

void O3_CPU::complete_instr_fetch(PACKET_QUEUE *queue, uint8_t is_it_tlb)
{

	//@Vishal: VIPT, TLB request should not be handled here
	//@Nilesh: Exception for accelearator
//	if(cpu <ACCELERATOR_START)
	assert(is_it_tlb == 0);

    uint32_t index = queue->head,
             rob_index = queue->entry[index].rob_index,
             num_fetched = 0;

#ifdef SANITY_CHECK
	//DP (if (warmup_complete[cpu]) {
    //cout << "**(complete_instr_fetch)queue->entry[index].full_addr = "<< hex << queue->entry[index].full_addr << "  instr_id = "<< queue->entry[index].instr_id << " index="<<index<< " rob_index="<<queue->entry[index].rob_index<< endl; });
    int a;
    if (rob_index != (a = check_rob(queue->entry[index].instr_id)))
    {
	    //cout << "complete_instr_fetch, rob_index ="<<rob_index<< " a = " << a << endl; 
	    assert(0);
    }
#endif

    // update ROB entry
    if (is_it_tlb) {
        ROB.entry[rob_index].translated = COMPLETED;
        ROB.entry[rob_index].instruction_pa = (queue->entry[index].instruction_pa << LOG2_PAGE_SIZE) | (ROB.entry[rob_index].ip & ((1 << LOG2_PAGE_SIZE) - 1)); // translated address
    }
    else
        ROB.entry[rob_index].fetched = COMPLETED;
    ROB.entry[rob_index].event_cycle = current_core_cycle[cpu];
    num_fetched++;

    //DP ( if (warmup_complete[cpu]) {
    //cout << "[" << queue->NAME << "] " << __func__ << " cpu: " << cpu <<  " instr_id: " << ROB.entry[rob_index].instr_id;
    //cout << " ip: " << hex << ROB.entry[rob_index].ip << " address: " << ROB.entry[rob_index].instruction_pa << dec;
    //cout << " translated: " << +ROB.entry[rob_index].translated << " fetched: " << +ROB.entry[rob_index].fetched;
    //cout << " event_cycle: " << ROB.entry[rob_index].event_cycle << endl; });

    // check if other instructions were merged
    if (queue->entry[index].instr_merged) {
	ITERATE_SET(i,queue->entry[index].rob_index_depend_on_me, ROB_SIZE) {
            // update ROB entry
            if (is_it_tlb) {
                ROB.entry[i].translated = COMPLETED;
                ROB.entry[i].instruction_pa = (queue->entry[index].instruction_pa << LOG2_PAGE_SIZE) | (ROB.entry[i].ip & ((1 << LOG2_PAGE_SIZE) - 1)); // translated address
            }
            else
                ROB.entry[i].fetched = COMPLETED;
            ROB.entry[i].event_cycle = current_core_cycle[cpu] + (num_fetched / FETCH_WIDTH);
            num_fetched++;

            //DP ( if (warmup_complete[cpu]) {
            //cout << "[" << queue->NAME << "] " << __func__ << " cpu: " << cpu <<  " instr_id: " << ROB.entry[i].instr_id;
            //cout << " ip: " << hex << ROB.entry[i].ip << " address: " << ROB.entry[i].instruction_pa << dec;
            //cout << " translated: " << +ROB.entry[i].translated << " fetched: " << +ROB.entry[i].fetched << " provider: " << ROB.entry[rob_index].instr_id;
            //cout << " event_cycle: " << ROB.entry[i].event_cycle << endl; });
        }
    }

    // remove this entry
    queue->remove_queue(&queue->entry[index]);
    
}

void O3_CPU::complete_data_fetch(PACKET_QUEUE *queue, uint8_t is_it_tlb)
{

	//@Vishal: VIPT, TLB request should not be handled here
	//@Nilesh: Exception for accelerator
//	if(cpu < ACCELERATOR_START)
		assert(is_it_tlb == 0);


    uint32_t index = queue->head,
             rob_index = queue->entry[index].rob_index,
             sq_index = queue->entry[index].sq_index,
             lq_index = queue->entry[index].lq_index;

#ifdef SANITY_CHECK
    if (queue->entry[index].type != RFO) {
    	//DP (if (warmup_complete[cpu]) {
            //cout << "queue->entry[index].full_addr = "<< queue->entry[index].full_addr << endl; });
        if (rob_index != check_rob(queue->entry[index].instr_id))
        {
            assert(0);
        }
    }
#endif

    // update ROB entry
    if (is_it_tlb) { // DTLB

        if (queue->entry[index].type == RFO) {
            SQ.entry[sq_index].physical_address = (queue->entry[index].data_pa << LOG2_PAGE_SIZE) | (SQ.entry[sq_index].virtual_address & ((1 << LOG2_PAGE_SIZE) - 1)); // translated address
            SQ.entry[sq_index].translated = COMPLETED;
            SQ.entry[sq_index].event_cycle = current_core_cycle[cpu];

            RTS1[RTS1_tail] = sq_index;
            RTS1_tail++;
            if (RTS1_tail == SQ_SIZE)
                RTS1_tail = 0;

            //DP (if (warmup_complete[cpu]) {
            //cout << "[ROB] " << __func__ << " RFO instr_id: " << SQ.entry[sq_index].instr_id;
            //cout << " DTLB_FETCH_DONE translation: " << +SQ.entry[sq_index].translated << hex << " page: " << (SQ.entry[sq_index].physical_address>>LOG2_PAGE_SIZE);
            //cout << " full_addr: " << SQ.entry[sq_index].physical_address << dec << " store_merged: " << +queue->entry[index].store_merged;
            //cout << " load_merged: " << +queue->entry[index].load_merged << endl; }); 

            handle_merged_translation(&queue->entry[index]);
        }
        else { 
            LQ.entry[lq_index].physical_address = (queue->entry[index].data_pa << LOG2_PAGE_SIZE) | (LQ.entry[lq_index].virtual_address & ((1 << LOG2_PAGE_SIZE) - 1)); // translated address
            LQ.entry[lq_index].translated = COMPLETED;
            LQ.entry[lq_index].event_cycle = current_core_cycle[cpu];

            RTL1[RTL1_tail] = lq_index;
            RTL1_tail++;
            if (RTL1_tail == LQ_SIZE)
                RTL1_tail = 0;

            //DP (if (warmup_complete[cpu]) {
            //cout << "[RTL1] " << __func__ << " instr_id: " << LQ.entry[lq_index].instr_id << " rob_index: " << LQ.entry[lq_index].rob_index << " is added to RTL1";
            //cout << " head: " << RTL1_head << " tail: " << RTL1_tail << endl; }); 

            //DP (if (warmup_complete[cpu]) {
            //cout << "[ROB] " << __func__ << " load instr_id: " << LQ.entry[lq_index].instr_id;
            //cout << " DTLB_FETCH_DONE translation: " << +LQ.entry[lq_index].translated << hex << " page: " << (LQ.entry[lq_index].physical_address>>LOG2_PAGE_SIZE);
            //cout << " full_addr: " << LQ.entry[lq_index].physical_address << dec << " store_merged: " << +queue->entry[index].store_merged;
            //cout << " load_merged: " << +queue->entry[index].load_merged << endl; }); 

            handle_merged_translation(&queue->entry[index]);
        }

        ROB.entry[rob_index].event_cycle = queue->entry[index].event_cycle;
    }
    else { // L1D

        if (queue->entry[index].type == RFO)
            handle_merged_load(&queue->entry[index]);
        else { 
#ifdef SANITY_CHECK
            if (queue->entry[index].store_merged)
                assert(0);
#endif
            LQ.entry[lq_index].fetched = COMPLETED;
            LQ.entry[lq_index].event_cycle = current_core_cycle[cpu];
            ROB.entry[rob_index].num_mem_ops--;
            ROB.entry[rob_index].event_cycle = queue->entry[index].event_cycle;

#ifdef SANITY_CHECK
            if (ROB.entry[rob_index].num_mem_ops < 0) {
                cerr << "instr_id: " << ROB.entry[rob_index].instr_id << endl;
                assert(0);
            }
#endif
            if (ROB.entry[rob_index].num_mem_ops == 0)
                inflight_mem_executions++;

            //DP (if (warmup_complete[cpu]) {
            //cout << "[ROB] " << __func__ << " load instr_id: " << LQ.entry[lq_index].instr_id;
            //cout << " L1D_FETCH_DONE fetched: " << +LQ.entry[lq_index].fetched << hex << " address: " << (LQ.entry[lq_index].physical_address>>LOG2_BLOCK_SIZE);
            //cout << " full_addr: " << LQ.entry[lq_index].physical_address << dec << " remain_mem_ops: " << ROB.entry[rob_index].num_mem_ops;
            //cout << " load_merged: " << +queue->entry[index].load_merged << " inflight_mem: " << inflight_mem_executions << endl; }); 

            release_load_queue(lq_index);
            handle_merged_load(&queue->entry[index]);
        }
    }

    // remove this entry
    queue->remove_queue(&queue->entry[index]);
}

//@Vishal: This function is not used anywhere
/*
void O3_CPU::handle_o3_fetch(PACKET *current_packet, uint32_t cache_type)
{
    uint32_t rob_index = current_packet->rob_index,
             sq_index  = current_packet->sq_index,
             lq_index  = current_packet->lq_index;

    // update ROB entry
    if (cache_type == 0) { // DTLB

#ifdef SANITY_CHECK
        if (rob_index != check_rob(current_packet->instr_id))
            assert(0);
#endif
        if (current_packet->type == RFO) {
            SQ.entry[sq_index].physical_address = (current_packet->data_pa << LOG2_PAGE_SIZE) | (SQ.entry[sq_index].virtual_address & ((1 << LOG2_PAGE_SIZE) - 1)); // translated address
            SQ.entry[sq_index].translated = COMPLETED;

            RTS1[RTS1_tail] = sq_index;
            RTS1_tail++;
            if (RTS1_tail == SQ_SIZE)
                RTS1_tail = 0;

            //DP (if (warmup_complete[cpu]) {
            //cout << "[ROB] " << __func__ << " RFO instr_id: " << SQ.entry[sq_index].instr_id;
            //cout << " DTLB_FETCH_DONE translation: " << +SQ.entry[sq_index].translated << hex << " page: " << (SQ.entry[sq_index].physical_address>>LOG2_PAGE_SIZE);
            //cout << " full_addr: " << SQ.entry[sq_index].physical_address << dec << " store_merged: " << +current_packet->store_merged;
            //cout << " load_merged: " << +current_packet->load_merged << endl; }); 

            handle_merged_translation(current_packet);
        }
        else { 
            LQ.entry[lq_index].physical_address = (current_packet->data_pa << LOG2_PAGE_SIZE) | (LQ.entry[lq_index].virtual_address & ((1 << LOG2_PAGE_SIZE) - 1)); // translated address
            LQ.entry[lq_index].translated = COMPLETED;

            RTL1[RTL1_tail] = lq_index;
            RTL1_tail++;
            if (RTL1_tail == LQ_SIZE)
                RTL1_tail = 0;

            //DP (if (warmup_complete[cpu]) {
            //cout << "[RTL1] " << __func__ << " instr_id: " << LQ.entry[lq_index].instr_id << " rob_index: " << LQ.entry[lq_index].rob_index << " is added to RTL1";
            //cout << " head: " << RTL1_head << " tail: " << RTL1_tail << endl; }); 

            //DP (if (warmup_complete[cpu]) {
            //cout << "[ROB] " << __func__ << " load instr_id: " << LQ.entry[lq_index].instr_id;
            //cout << " DTLB_FETCH_DONE translation: " << +LQ.entry[lq_index].translated << hex << " page: " << (LQ.entry[lq_index].physical_address>>LOG2_PAGE_SIZE);
            //cout << " full_addr: " << LQ.entry[lq_index].physical_address << dec << " store_merged: " << +current_packet->store_merged;
            //cout << " load_merged: " << +current_packet->load_merged << endl; }); 

            handle_merged_translation(current_packet);
        }

        ROB.entry[rob_index].event_cycle = current_packet->event_cycle;
    }
    else { // L1D

        if (current_packet->type == RFO)
            handle_merged_load(current_packet);
        else { // do traditional things
#ifdef SANITY_CHECK
            if (rob_index != check_rob(current_packet->instr_id))
                assert(0);

            if (current_packet->store_merged)
                assert(0);
#endif
            LQ.entry[lq_index].fetched = COMPLETED;
            ROB.entry[rob_index].num_mem_ops--;

#ifdef SANITY_CHECK
            if (ROB.entry[rob_index].num_mem_ops < 0) {
                cerr << "instr_id: " << ROB.entry[rob_index].instr_id << endl;
                assert(0);
            }
#endif
            if (ROB.entry[rob_index].num_mem_ops == 0)
                inflight_mem_executions++;

            //DP (if (warmup_complete[cpu]) {
            //cout << "[ROB] " << __func__ << " load instr_id: " << LQ.entry[lq_index].instr_id;
            //cout << " L1D_FETCH_DONE fetched: " << +LQ.entry[lq_index].fetched << hex << " address: " << (LQ.entry[lq_index].physical_address>>LOG2_BLOCK_SIZE);
            //cout << " full_addr: " << LQ.entry[lq_index].physical_address << dec << " remain_mem_ops: " << ROB.entry[rob_index].num_mem_ops;
            //cout << " load_merged: " << +current_packet->load_merged << " inflight_mem: " << inflight_mem_executions << endl; }); 

            release_load_queue(lq_index);

            handle_merged_load(current_packet);

            ROB.entry[rob_index].event_cycle = current_packet->event_cycle;
        }
    }
}*/

void O3_CPU::handle_merged_translation(PACKET *provider)
{

	//@Vishal: VIPT, Translation are not sent from processor, so this code should not be executed.
	//@Nilesh: Exception for accelerator
//	if(cpu < ACCELERATOR_START)
		assert(0);


    if (provider->store_merged) {
	ITERATE_SET(merged, provider->sq_index_depend_on_me, SQ.SIZE) {
            SQ.entry[merged].translated = COMPLETED;
            SQ.entry[merged].physical_address = (provider->data_pa << LOG2_PAGE_SIZE) | (SQ.entry[merged].virtual_address & ((1 << LOG2_PAGE_SIZE) - 1)); // translated address
            SQ.entry[merged].event_cycle = current_core_cycle[cpu];

            RTS1[RTS1_tail] = merged;
            RTS1_tail++;
            if (RTS1_tail == SQ_SIZE)
                RTS1_tail = 0;

            //DP (if (warmup_complete[cpu]) {
            //cout << "[ROB] " << __func__ << " store instr_id: " << SQ.entry[merged].instr_id;
            //cout << " DTLB_FETCH_DONE translation: " << +SQ.entry[merged].translated << hex << " page: " << (SQ.entry[merged].physical_address>>LOG2_PAGE_SIZE);
            //cout << " full_addr: " << SQ.entry[merged].physical_address << dec << " by instr_id: " << +provider->instr_id << endl; });
        }
    }
    if (provider->load_merged) {
	ITERATE_SET(merged, provider->lq_index_depend_on_me, LQ.SIZE) {
            LQ.entry[merged].translated = COMPLETED;
            LQ.entry[merged].physical_address = (provider->data_pa << LOG2_PAGE_SIZE) | (LQ.entry[merged].virtual_address & ((1 << LOG2_PAGE_SIZE) - 1)); // translated address
            LQ.entry[merged].event_cycle = current_core_cycle[cpu];

            RTL1[RTL1_tail] = merged;
            RTL1_tail++;
            if (RTL1_tail == LQ_SIZE)
                RTL1_tail = 0;

            //DP (if (warmup_complete[cpu]) {
            //cout << "[RTL1] " << __func__ << " instr_id: " << LQ.entry[merged].instr_id << " rob_index: " << LQ.entry[merged].rob_index << " is added to RTL1";
            //cout << " head: " << RTL1_head << " tail: " << RTL1_tail << endl; }); 

            //DP (if (warmup_complete[cpu]) {
            //cout << "[ROB] " << __func__ << " load instr_id: " << LQ.entry[merged].instr_id;
            //cout << " DTLB_FETCH_DONE translation: " << +LQ.entry[merged].translated << hex << " page: " << (LQ.entry[merged].physical_address>>LOG2_PAGE_SIZE);
            //cout << " full_addr: " << LQ.entry[merged].physical_address << dec << " by instr_id: " << +provider->instr_id << endl; });
        }
    }
}

void O3_CPU::handle_merged_load(PACKET *provider)
{
    ITERATE_SET(merged, provider->lq_index_depend_on_me, LQ.SIZE) {
        uint32_t merged_rob_index = LQ.entry[merged].rob_index;

        LQ.entry[merged].fetched = COMPLETED;
        LQ.entry[merged].event_cycle = current_core_cycle[cpu];
     
	ROB.entry[merged_rob_index].num_mem_ops--;
        ROB.entry[merged_rob_index].event_cycle = current_core_cycle[cpu];

#ifdef SANITY_CHECK
        if (ROB.entry[merged_rob_index].num_mem_ops < 0) {
            cerr << "instr_id: " << ROB.entry[merged_rob_index].instr_id << " rob_index: " << merged_rob_index << endl;
            assert(0);
        }
#endif
        if (ROB.entry[merged_rob_index].num_mem_ops == 0)
            inflight_mem_executions++;
	
               //DP (if (warmup_complete[cpu]) {
        //cout << "[ROB] " << __func__ << " load instr_id: " << LQ.entry[merged].instr_id;
        //cout << " L1D_FETCH_DONE translation: " << +LQ.entry[merged].translated << hex << " address: " << (LQ.entry[merged].physical_address>>LOG2_BLOCK_SIZE);
        //cout << " full_addr: " << LQ.entry[merged].physical_address << dec << " by instr_id: " << +provider->instr_id;
        //cout << " remain_mem_ops: " << ROB.entry[merged_rob_index].num_mem_ops << endl; });

        release_load_queue(merged);
    }
}

void O3_CPU::release_load_queue(uint32_t lq_index)
{
    // release LQ entries
    //DP ( if (warmup_complete[cpu]) {
    //cout << "[LQ] " << __func__ << " instr_id: " << LQ.entry[lq_index].instr_id << " releases lq_index: " << lq_index;
    //cout << hex << " full_addr: " << LQ.entry[lq_index].physical_address << dec << endl; });

    LSQ_ENTRY empty_entry;
    LQ.entry[lq_index] = empty_entry;
    LQ.occupancy--;
}

void O3_CPU::retire_rob()
{
    for (uint32_t n=0; n<RETIRE_WIDTH; n++) {
        if (ROB.entry[ROB.head].ip == 0)
            return;

        // retire is in-order
        if (ROB.entry[ROB.head].executed != COMPLETED) { 
            //DP ( if (warmup_complete[cpu]) {
            //cout << "[ROB] " << __func__ << " instr_id: " << ROB.entry[ROB.head].instr_id << " head: " << ROB.head << " is not executed yet" << endl; });
            return;
        }

        // check store instruction
        uint32_t num_store = 0;
        for (uint32_t i=0; i<MAX_INSTR_DESTINATIONS; i++) {
            if (ROB.entry[ROB.head].destination_memory[i])
                num_store++;
        }

        if (num_store) {
            if (((L1D.WQ.occupancy + num_store) <= L1D.WQ.SIZE) || (cpu >= ACCELERATOR_START)) {	//Nilesh: check added for accelerator
                for (uint32_t i=0; i<MAX_INSTR_DESTINATIONS; i++) {
                    if (ROB.entry[ROB.head].destination_memory[i]) {

                        PACKET data_packet;
                        uint32_t sq_index = ROB.entry[ROB.head].sq_index[i];

                        // sq_index and rob_index are no longer available after retirement
                        // but we pass this information to avoid segmentation fault
                        data_packet.fill_level = FILL_L1;
                        data_packet.cpu = cpu;
                        data_packet.data_index = SQ.entry[sq_index].data_index;
                        data_packet.sq_index = sq_index;

                        //@Vishal: VIPT, send virtual address
                        //data_packet.address = SQ.entry[sq_index].physical_address >> LOG2_BLOCK_SIZE;
                        //data_packet.full_addr = SQ.entry[sq_index].physical_address;

                        data_packet.address = SQ.entry[sq_index].virtual_address >> LOG2_BLOCK_SIZE;
                        data_packet.full_addr = SQ.entry[sq_index].virtual_address;
			data_packet.full_virtual_address = SQ.entry[sq_index].virtual_address;

                        data_packet.instr_id = SQ.entry[sq_index].instr_id;
                        data_packet.rob_index = SQ.entry[sq_index].rob_index;
                        data_packet.ip = SQ.entry[sq_index].ip;
                        data_packet.type = RFO;
                        data_packet.asid[0] = SQ.entry[sq_index].asid[0];
                        data_packet.asid[1] = SQ.entry[sq_index].asid[1];
                        data_packet.event_cycle = current_core_cycle[cpu];

			if( cpu >= ACCELERATOR_START) {		//Nilesh: check added for accelerator
//                                data_packet.address = SQ.entry[sq_index].physical_address >> LOG2_BLOCK_SIZE;
//                                data_packet.full_addr = SQ.entry[sq_index].physical_address;
//                                data_packet.fill_level = FILL_DRAM;
                                int index = uncore.DRAM.add_wq(&data_packet);

//				cout << "addwq from ooo cpu index "  << sq_index << " address " << data_packet.address << " rob " << ROB.head<< endl;
//				print_LSQ(&SQ);
                                if(index == -2)
                                       uncore.DRAM.STALL[RFO]++;

                        }
			else
				L1D.add_wq(&data_packet);

			sim_store_sent++;
                    }
                }
            }
            else {
                //DP ( if (warmup_complete[cpu]) {
                //cout << "[ROB] " << __func__ << " instr_id: " << ROB.entry[ROB.head].instr_id << " L1D WQ is full" << endl; });

                L1D.WQ.FULL++;
                L1D.STALL[RFO]++;

                return;
            }
        }

        // release SQ entries
        for (uint32_t i=0; i<MAX_INSTR_DESTINATIONS; i++) {
            if (ROB.entry[ROB.head].sq_index[i] != UINT32_MAX) {
                uint32_t sq_index = ROB.entry[ROB.head].sq_index[i];

                //DP ( if (warmup_complete[cpu]) {
                //cout << "[SQ] " << __func__ << " instr_id: " << ROB.entry[ROB.head].instr_id << " releases sq_index: " << sq_index;
                //cout << hex << " address: " << (SQ.entry[sq_index].physical_address>>LOG2_BLOCK_SIZE);
                //cout << " full_addr: " << SQ.entry[sq_index].physical_address << dec << endl; });

                LSQ_ENTRY empty_entry;
                SQ.entry[sq_index] = empty_entry;
                
                SQ.occupancy--;
                SQ.head++;
                if (SQ.head == SQ.SIZE)
                    SQ.head = 0;
            }
        }

        // release ROB entry
        //DP ( if (warmup_complete[cpu]) {
        //cout << "[ROB] " << __func__ << " instr_id: " << ROB.entry[ROB.head].instr_id << " is retired" << endl; });

        ooo_model_instr empty_entry;
        ROB.entry[ROB.head] = empty_entry;

        ROB.head++;
        if (ROB.head == ROB.SIZE)
            ROB.head = 0;
        ROB.occupancy--;
        completed_executions--;
        num_retired++;
    }
}

