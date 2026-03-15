#ifndef  EFFICUTS_H
#define  EFFICUTS_H

#include "../../Elements/Elements.h"
#include "../../Tools/Tools.h"
#include "../Classifier.h"
#include "EffiNode.h"

using namespace std;

class EffiCuts : public Classifier {
public:
    static double spfac;         
    static int max_depth;        
    static int leaf_size;        
    static bool remove_redund;   
    static bool node_merge;      
    static bool width_power2;    
    int tree_num;                
    struct EffiNode **root;      

    EffiCuts();
    void Create(vector<Rule*> &rules, ProgramState *ps);    
    uint32_t Lookup(Trace *trace, ProgramState *ps);
    uint32_t Lookup(Trace *trace);         
    uint64_t CalMemory();   
    void Setparam(double _spfac);
    ~EffiCuts();
};

#endif