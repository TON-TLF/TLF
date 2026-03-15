#ifndef  TD_HYPERSPLIT_H
#define  TD_HYPERSPLIT_H

#include "../../Elements/Elements.h"
#include "../../Tools/Tools.h"
#include "../../TopK/TopK.h"
#include "../Classifier.h"
#include "TDHsNode.h"

using namespace std;

class TDHyperSplit : public Classifier {
public:
    static uint32_t binth;                                 
    static uint32_t total_trace_num;                       
    static bool remove_redund;                             
    static double pfac;                                    
    static double pop_max;                                 
    static double pop_min;                                 
    
    TDHsNode *root;                                     
    void Create(vector<Rule*> &rules, ProgramState *ps);   
    uint32_t Lookup(Trace *trace, ProgramState *ps); 
    uint32_t Lookup(Trace *trace);      
    uint64_t CalMemory();  
    void Setparam(double _pfac);
};

#endif