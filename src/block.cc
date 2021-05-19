#include "ooo_cpu.h"
#include "block.h"

//Nilesh: added for debug pupose
void PACKET_QUEUE::queuePrint(){
		cout << "********************************" << endl;
                if ((head == tail) && occupancy == 0)
                        ;

                        for (uint32_t i=0; i<SIZE; i++) {
				if(entry[i].full_addr == 0 )
					continue;
			/*	if(entry[i].instr_id != 91257)
					continue;
			/*	if(entry[i].rob_index == -1) 
					continue;
				if(entry[i].cpu != 3 )
					continue;*/
				cout << "index " << i << " cpu "  << entry[i].cpu << " instr id " << entry[i].instr_id << " robid  " << entry[i].rob_index << " returned " << (int)entry[i].returned << " event cycle "<< entry[i].event_cycle <<  " address " << entry[i].address  << " fullvirt add " << entry[i].full_virtual_address << " type " << (int)entry[i].type <<  " is_translation " << (int) entry[i].is_translation << " Instrcution " << (int)entry[i].instruction << " translation level " << (int) entry[i].translation_level << " scheduled " << (int)entry[i].scheduled << endl << " returned " << (int)entry[i].returned << endl; 
                        }
		cout << "********************************" << endl;

}
int PACKET_QUEUE::check_queue(PACKET *packet)
{
    if ((head == tail) && occupancy == 0)
        return -1;

    if (head < tail) {
        for (uint32_t i=head; i<tail; i++) {
            if (NAME == "L1D_WQ") {
                if (entry[i].full_addr == packet->full_addr) {
                    DP (if (warmup_complete[packet->cpu]) {
                    cout << "[" << NAME << "] " << __func__ << " cpu: " << packet->cpu << " instr_id: " << packet->instr_id << " same address: " << hex << packet->address;
                    cout << " full_addr: " << packet->full_addr << dec << " by instr_id: " << entry[i].instr_id << " index: " << i;
                    cout << " cycle " << packet->event_cycle << endl; });
                    return i;
                }
            }
            else {
	    	if ((entry[i].address == packet->address) && (entry[i].asid[0] == packet->asid[0])) {	//Nilesh: added check for ASID
			if(entry[i].cpu != packet->cpu){
					ooo_cpu[ACCELERATOR_START].STLB.stlb_merged++;
					ooo_cpu[packet->cpu].STLB.check_stlb_counter[cpu]++;
					    packet->stlb_start_cycle = current_core_cycle[cpu];

                                        entry[i].stlb_merged = true;         //Nilesh: address space is shared among accelerators
                                        entry[i].stlb_depends_on_me.push(*packet);
                                        SPRINT(cout << "instruction merged in check queue " << NAME << " with packet cpu " << entry[i].cpu << " of packet cpu "<<  packet->cpu << endl;)
                                }
                //cout << "address = "<< packet->address << " size=" <<sizeof(packet->address) << endl;
                    DP (if (warmup_complete[packet->cpu]) {
                    cout << "[" << NAME << "] " << __func__ << " cpu: " << packet->cpu << " instr_id: " << packet->instr_id << " same address: " << hex << packet->address;
                    cout << " full_addr: " << packet->full_addr << dec << " by instr_id: " << entry[i].instr_id << " index: " << i;
                    cout << " cycle " << packet->event_cycle << endl; });

//		cout << "packet address " <<packet->address << " cpu " << packet->cpu << " instrid" << packet->instr_id<< " matched with cpu " << entry[i].cpu << " instr id " << entry[i].instr_id << " robid  " << entry[i].rob_index << " returned " << (int)entry[i].returned << " event cycle "<< entry[i].event_cycle <<  " address " << entry[i].address << endl; 
                    return i;
                }
            }
        }
    }
    else {
        for (uint32_t i=head; i<SIZE; i++) {
            if (NAME == "L1D_WQ") {
                if (entry[i].full_addr == packet->full_addr) {
                //cout << "full_addr = "<< packet->full_addr << "size=" << sizeof(packet->full_addr)<<endl;
                    DP (if (warmup_complete[packet->cpu]) {
                    cout << "[" << NAME << "] " << __func__ << " cpu: " << packet->cpu << " instr_id: " << packet->instr_id << " same address: " << hex << packet->address;
                    cout << " full_addr: " << packet->full_addr << dec << " by instr_id: " << entry[i].instr_id << " index: " << i;
                    cout << " cycle " << packet->event_cycle << endl; });
                    return i;
                }
            }
            else {
	    	if ((entry[i].address == packet->address) && (entry[i].asid[0] == packet->asid[0])) {	//Nilesh: added check for accelerators
			if(entry[i].cpu != packet->cpu){
					ooo_cpu[packet->cpu].STLB.check_stlb_counter[cpu]++;
					    packet->stlb_start_cycle = current_core_cycle[cpu];
                                        entry[i].stlb_merged = true;         //Nilesh: address space is shared among accelerators
                                        entry[i].stlb_depends_on_me.push(*packet);
                                        SPRINT(cout << "instruction merged in check queue " << NAME << " with packet cpu " << entry[i].cpu << " of packet cpu "<<  packet->cpu << endl;)
                                }
                    DP (if (warmup_complete[packet->cpu]) {
                    cout << "[" << NAME << "] " << __func__ << " cpu: " << packet->cpu << " instr_id: " << packet->instr_id << " same address: " << hex << packet->address;
                    cout << " full_addr: " << packet->full_addr << dec << " by instr_id: " << entry[i].instr_id << " index: " << i;
                    cout << " cycle " << packet->event_cycle << endl; });
//		cout << "packet address " <<packet->address << " cpu " << packet->cpu << " instrid" << packet->instr_id<< " matched with cpu " << entry[i].cpu << " instr id " << entry[i].instr_id << " robid  " << entry[i].rob_index << " returned " << (int)entry[i].returned << " event cycle "<< entry[i].event_cycle <<  " address " << entry[i].address << endl; 
                    return i;
                }
            }
        }
        for (uint32_t i=0; i<tail; i++) {
            if (NAME == "L1D_WQ") {
                if (entry[i].full_addr == packet->full_addr) {
                    DP (if (warmup_complete[packet->cpu]) {
                    cout << "[" << NAME << "] " << __func__ << " cpu: " << packet->cpu << " instr_id: " << packet->instr_id << " same address: " << hex << packet->address;
                    cout << " full_addr: " << packet->full_addr << dec << " by instr_id: " << entry[i].instr_id << " index: " << i;
                    cout << " cycle " << packet->event_cycle << endl; });
                    return i;
                }
            }
            else {
	    	if ((entry[i].address == packet->address) && (entry[i].asid[0] == packet->asid[0])) {	//Nilesh: asid check added
			if(entry[i].cpu != packet->cpu){
					ooo_cpu[packet->cpu].STLB.check_stlb_counter[cpu]++;
					    packet->stlb_start_cycle = current_core_cycle[cpu];
                                        entry[i].stlb_merged = true;         //Nilesh: address space is shared among accelerators
                                        entry[i].stlb_depends_on_me.push(*packet);
                                        SPRINT(cout << "instruction merged in check queue " << NAME << " with packet cpu " << entry[i].cpu << " of packet cpu "<<  packet->cpu << endl;)
                                }
                    DP (if (warmup_complete[packet->cpu]) {
                    cout << "[" << NAME << "] " << __func__ << " cpu: " << packet->cpu << " instr_id: " << packet->instr_id << " same address: " << hex << packet->address;
                    cout << " full_addr: " << packet->full_addr << dec << " by instr_id: " << entry[i].instr_id << " index: " << i;
                    cout << " cycle " << packet->event_cycle << endl; });
//		cout << "packet address " <<packet->address << " cpu " << packet->cpu << " instrid" << packet->instr_id<< " matched with cpu " << entry[i].cpu << " instr id " << entry[i].instr_id << " robid  " << entry[i].rob_index << " returned " << (int)entry[i].returned << " event cycle "<< entry[i].event_cycle <<  " address " << entry[i].address << endl; 
                    return i;
                }
            }
        }
    }

    return -1;
}

void PACKET_QUEUE::add_queue(PACKET *packet)
{
#ifdef SANITY_CHECK
    if (occupancy && (head == tail))
        assert(0);
#endif

    // add entry
    entry[tail] = *packet;

    DP ( if (warmup_complete[packet->cpu]) {
    cout << "[" << NAME << "] " << __func__ << " cpu: " << packet->cpu << " instr_id: " << packet->instr_id;
    cout << " address: " << hex << entry[tail].address << " full_addr: " << entry[tail].full_addr << dec;
    cout << " head: " << head << " tail: " << tail << " occupancy: " << occupancy << " event_cycle: " << entry[tail].event_cycle << endl; });


#ifdef PRINT_QUEUE_TRACE
    if(packet->instr_id == QTRACE_INSTR_ID)
    {
        cout << "[" << NAME << "] " << __func__ << " cpu: " << packet->cpu << " instr_id: " << packet->instr_id;
        cout << " address: " << hex << entry[tail].address << " full_addr: " << entry[tail].full_addr << dec;
        cout << " head: " << head << " tail: " << tail << " occupancy: " << occupancy << " event_cycle: " << entry[tail].event_cycle <<endl;
    }
#endif



    occupancy++;
    tail++;
    if (tail >= SIZE)
        tail = 0;
}

void PACKET_QUEUE::remove_queue(PACKET *packet)
{
#ifdef SANITY_CHECK
    if ((occupancy == 0) && (head == tail))
        assert(0);
#endif

    DP ( if (warmup_complete[packet->cpu]) {
    cout << "[" << NAME << "] " << __func__ << " cpu: " << packet->cpu << " instr_id: " << packet->instr_id;
    cout << " address: " << hex << packet->address << " full_addr: " << packet->full_addr << dec << " fill_level: " << packet->fill_level;
    cout << " head: " << head << " tail: " << tail << " occupancy: " << occupancy << " event_cycle: " << packet->event_cycle << endl; });

#ifdef PRINT_QUEUE_TRACE
    if(packet->instr_id == QTRACE_INSTR_ID)
    {
        cout << "[" << NAME << "] " << __func__ << " cpu: " << packet->cpu << " instr_id: " << packet->instr_id;
        cout << " address: " << hex << packet->address << " full_addr: " << packet->full_addr << dec << " fill_level: " << packet->fill_level;
        cout << " head: " << head << " tail: " << tail << " occupancy: " << occupancy << " event_cycle: " << packet->event_cycle <<endl;
    }
#endif


    // reset entry
    PACKET empty_packet;
    *packet = empty_packet;

    occupancy--;
    head++;
    if (head >= SIZE)
        head = 0;
}
