#ifndef  DIR248_H
#define  DIR248_H

#include "../../Elements/Elements.h"
#include "../../Tools/Tools.h"
#include "../Classifier.h"

using namespace std;

typedef struct TableEntry {
    uint32_t next_hop;          
    uint8_t  has_subtable;      
    uint8_t  is_valid;          
    uint8_t  padding[2];        
    struct DirTable* subtable; 
} __attribute__((aligned(16))) TableEntry;  

typedef struct DirTable {
    uint32_t entry_count;   
    uint32_t mask;          
    TableEntry* entries;    
    uint8_t  stride;       
    uint8_t  padding[15];   
} __attribute__((aligned(64))) DirTable; 

class DIR248 : public Classifier {
public:
    DIR248();
    void Create(vector<Prefix*> &prefixs, ProgramState *ps);   
    uint32_t Lookup(Trace *trace, ProgramState *ps);
    uint32_t Lookup(Trace *trace);       
    uint64_t CalMemory();                              
    ~DIR248();

private:
    DirTable* root;            
};

class DIR248_TD : public Classifier {
public:
    DIR248_TD();
    void Create(vector<Prefix*> &prefixs, ProgramState *ps);   
    uint32_t Lookup(Trace *trace, ProgramState *ps);
    uint32_t Lookup(Trace *trace);       
    uint64_t CalMemory();                              
    ~DIR248_TD();

private:
    DirTable* root;           
};
#endif