#ifndef  HICUTS_H
#define  HICUTS_H

#include "../../Elements/Elements.h"
#include "../../Tools/Tools.h"
#include "../Classifier.h"
#include "HiNode.h"

using namespace std;

class HiCuts : public Classifier {
public:
    static double spfac;                              
    static int max_depth;                         
    static int leaf_size;                            
    static bool remove_redund;                      
    static bool node_merge;                          
    static bool width_power2;                      
    int tree_num;                                   
    struct HiNode *root;                            

    HiCuts();
    void Create(vector<Rule*> &rules, ProgramState *ps);
    uint32_t Lookup(Trace *trace, ProgramState *ps);
    uint32_t Lookup(Trace *trace);
    uint64_t CalMemory();
    void Setparam(double _spfac);
    ~HiCuts();
};

#endif