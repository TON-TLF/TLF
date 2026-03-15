#ifndef  HINODE_H
#define  HINODE_H

#include "../../Elements/Elements.h"
#include "../../Tools/Tools.h"
#include "../TreeNode.h"
#include "HiCuts.h"

using namespace std;

class HiNode : public TreeNode {
public:    
    vector<Rule*> rules;   
    int rules_num;         
    bool leaf_node;        
    uint32_t bounds[5][2]; 

    int cut;              
    char dim;              
    uint64_t width;        

    HiNode** child_arr;    
    int child_num;         

    HiNode();
    void Create(vector<Rule*> &rules, ProgramState *ps, int depth){};  
    void Create(ProgramState *ps, int depth);                         
    uint64_t CalMemory();                                             
    void HasBounds();                                                 
    void RemoveRedund();
    void CalDimToCuts();
    void CalNumCuts();
    bool NodeContainRule(Rule* rule);
    ~HiNode();                                                      
};

bool SameEffiRules(HiNode *node1, HiNode *node2);
#endif