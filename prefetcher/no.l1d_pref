#include "cache.h"

#include <map>

//#define TRACE_DUMP		//Neelu: Addition Trace Dump prints

map<uint64_t, uint64_t> last_accessed_address;	//Neelu: Stores last accessed address per IP.  

void CACHE::l1d_prefetcher_initialize() 
{

}

void CACHE::l1d_prefetcher_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type)
{

	#ifdef TRACE_DUMP
	cout << "DEMAND:  IP: " << ip << "  CL Addr: " << (addr >> LOG2_BLOCK_SIZE) << "  Hit: " << unsigned(cache_hit) << "  Cycle: " << current_core_cycle[cpu] <<endl;
	#endif

	if(last_accessed_address.find(ip) != last_accessed_address.end())
	{
		#ifdef TRACE_DUMP
		cout << "IP: "<< ip << "  Stride: " << ((addr >> LOG2_BLOCK_SIZE) - last_accessed_address[ip]) << endl;
		#endif
	}
	
	last_accessed_address[ip] = addr; 

}

void CACHE::l1d_prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in)
{

	#ifdef TRACE_DUMP
	cout << "FILL:  CL Addr: " << (addr >> LOG2_BLOCK_SIZE) << "  Prefetch: " << unsigned(prefetch) << "  Cycle: " << current_core_cycle[cpu] <<endl;
	#endif

}

void CACHE::l1d_prefetcher_final_stats()
{

}
