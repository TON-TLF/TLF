#ifndef  TD_EFFINODE_H
#define  TD_EFFINODE_H

#include "../../Elements/Elements.h"
#include "../../Tools/Tools.h"
#include "../TreeNode.h"
#include "TDEffiCuts.h"

using namespace std;

class TDEffiNode : public TreeNode {
public:    
    vector<Rule*> rules;        
    int rules_num;               
    int priority;               
    bool leaf_node;              
    uint32_t bounds[5][2];      

    int cuts[2];                 
    char dims[2];               
    char dims_num;               
    uint64_t width[2];         

    TDEffiNode** child_arr;  
    int child_num;              

    TDEffiNode();
    void Create(vector<Rule*> &rules, ProgramState *ps, int depth){};  
    void Create(ProgramState *ps, int depth);                          
    uint64_t CalMemory();                                              
    void HasBounds();                                                  
    void RemoveRedund();
    void CalDimToCuts(int traces_num);
    void CalNumCuts(double pop);
    bool NodeContainRule(Rule* rule);
    ~TDEffiNode();                                                 
};

bool SameEffiRules(TDEffiNode *node1, TDEffiNode *node2);
#endif