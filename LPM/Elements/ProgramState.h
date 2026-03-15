#ifndef  PROGRAMSTATE_H
#define  PROGRAMSTATE_H

#include "Common.h"
#include "Command.h"

using namespace std;

class CountState {
public:
	long long count;  
	long long cnt;     
	long long calNum;  
	long long maxn;   
	long long minn;   
	double    avg;       

    CountState();      
	void Addcount();   
	void Cal();       
	void Clear();     
};

class ProgramState {
public:
	int prefixs_num;                         
	int traces_num;                        
	
	double rules_memory_size;               
	double traces_memory_size;             
	double sketch_memory_size;              
	double topk_tracesFreq_memory_size;     
	double TracesMat_memory_size;          
	double tree_memory_size;                
	double total_memory_size;               

	double sketch_build_and_update_time;    
	double sketch_calculate_topk_time;      
	double topk_tracesMat_init_time;        
	double rules_analyze_time;              
	double tree_build_time;                 
	double total_build_time;               

	double avg_lookup_time;
	double avg_insert_time;
	double throughput;

	CountState lookup_access_entry;         
	CountState lookup_access;				
	CountState lookup_depth;				

	double avg_lookup_access_entry;         
	double max_lookup_access_entry;        
	double avg_lookup_access;              
	double max_lookup_access;               
	double avg_lookup_depth;

	ProgramState();                         
	void CalInfo();                         
	void Clear();                          
	void ClearAccess();                     
	void Print();                           
};

#endif