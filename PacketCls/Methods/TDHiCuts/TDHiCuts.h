#ifndef  TD_HICUTS_H
#define  TD_HICUTS_H

#include "../../Elements/Elements.h"
#include "../../Tools/Tools.h"
#include "../../TopK/TopK.h"
#include "../Classifier.h"
#include "TDHiNode.h"

using namespace std;

class TDHiCuts : public Classifier {
public:
    static double spfac;           
    static int max_depth;          
    static int leaf_size;          
    static bool remove_redund;    
    static bool node_merge;        
    static bool width_power2;     
    int tree_num;                  
    struct TDHiNode *root;       

    static int total_trace_num;    
    static double wfac;           
    static double pop_max;
    static double pop_min;
    static double pop_add_1;
    static double pop_add_2;
    static double pop_dec_1;
    static double pop_dec_2;
    static double pop_mul_1;
    static double pop_mul_2;
    static double pop_div_1;
    static double pop_div_2;

    TDHiCuts();
    void Create(vector<Rule*> &rules, ProgramState *ps);  
    uint32_t Lookup(Trace *trace, ProgramState *ps);   
    uint32_t Lookup(Trace *trace);   
    uint64_t CalMemory();
    void Setparam(double _spfac, double _wfac);
    ~TDHiCuts();
};

#endif