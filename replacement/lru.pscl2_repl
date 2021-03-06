#include "cache.h"

// initialize replacement state
void CACHE::PSCL2_initialize_replacement()
{
	cout << NAME << "LRU BASE repalcement intialize PSCL2 " << endl;

}

// find replacement victim
uint32_t CACHE::PSCL2_find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t ip, uint64_t full_addr, uint32_t type)
{
    // baseline LRU
    return lru_victim(cpu, instr_id, set, current_set, ip, full_addr, type); 
}

// called on every cache hit and cache fill
void CACHE::PSCL2_update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit)
{
//	cout << "INSIDE PSCL2 update " << NAME << endl;
    string TYPE_NAME;
    if (type == LOAD)
        TYPE_NAME = "LOAD";
    else if (type == RFO)
        TYPE_NAME = "RFO";
    else if (type == PREFETCH)
        TYPE_NAME = "PF";
    else if (type == WRITEBACK)
        TYPE_NAME = "WB";
    else if (type == TRANSLATION)
	TYPE_NAME = "TRANSLATION";
    else
        assert(0);

    if (hit)
        TYPE_NAME += "_HIT";
    else
        TYPE_NAME += "_MISS";

    if ((type == WRITEBACK) && ip)
        assert(0);

    // uncomment this line to see the PSCL2 accesses
    // cout << "CPU: " << cpu << "  PSCL2 " << setw(9) << TYPE_NAME << " set: " << setw(5) << set << " way: " << setw(2) << way;
    // cout << hex << " paddr: " << setw(12) << paddr << " ip: " << setw(8) << ip << " victim_addr: " << victim_addr << dec << endl;

    // baseline LRU
    if (hit && (type == WRITEBACK)) // writeback hit does not update LRU state
        return;

    return lru_update(set, way);
}

void CACHE::PSCL2_replacement_final_stats()
{

}
