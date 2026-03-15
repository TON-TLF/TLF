#ifndef  TD_EFFICUTS_H
#define  TD_EFFICUTS_H

#include "../../Elements/Elements.h"
#include "../../Tools/Tools.h"
#include "../../TopK/TopK.h"
#include "../Classifier.h"
#include "TDEffiNode.h"

using namespace std;

class TDEffiCuts : public Classifier {
public:
    static double spfac;         
    static int max_depth;       
    static int leaf_size;         
    static bool remove_redund;    
    static bool node_merge;       
    static bool width_power2;    
    int tree_num;                
    struct TDEffiNode **root; 

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

    TDEffiCuts();
    void Create(vector<Rule*> &rules, ProgramState *ps);  
    uint32_t Lookup(Trace *trace, ProgramState *ps);     
    uint32_t Lookup(Trace *trace); 
    uint64_t CalMemory(); 
    void Setparam(double _spfac, double _wfac);                                
    ~TDEffiCuts();
};

#endif