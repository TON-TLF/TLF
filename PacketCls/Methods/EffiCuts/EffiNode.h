#ifndef  EFFINODE_H
#define  EFFINODE_H

#include "../../Elements/Elements.h"
#include "../../Tools/Tools.h"
#include "../TreeNode.h"
#include "EffiCuts.h"

using namespace std;

class EffiNode : public TreeNode {
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

    EffiNode** child_arr;  
    int child_num;         

    EffiNode();
    void Create(vector<Rule*> &rules, ProgramState *ps, int depth){};  
    void Create(ProgramState *ps, int depth);                          
    uint64_t CalMemory();                                              
    void HasBounds();                                                  
    void RemoveRedund();
    void CalDimToCuts();
    void CalNumCuts();
    bool NodeContainRule(Rule* rule);
    ~EffiNode();                                                       
};

bool SameEffiRules(EffiNode *node1, EffiNode *node2);
#endif