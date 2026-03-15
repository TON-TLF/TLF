#ifndef  TD_HINODE_H
#define  TD_HINODE_H

#include "../../Elements/Elements.h"
#include "../../Tools/Tools.h"
#include "../TreeNode.h"
#include "TDHiCuts.h"

using namespace std;

class TDHiNode : public TreeNode {
public:    
    vector<Rule*> rules;      
    int rules_num;            
    bool leaf_node;            
    uint32_t bounds[5][2];    

    int cut;                 
    char dim;                 
    uint64_t width;           

    TDHiNode** child_arr;  
    int child_num;           

    TDHiNode();
    void Create(vector<Rule*> &rules, ProgramState *ps, int depth){}; 
    void Create(ProgramState *ps, int depth);                          
    uint64_t CalMemory();                                              
    void HasBounds();                                                 
    void RemoveRedund();
    void CalDimToCuts(int traces_num);
    void CalNumCuts(double pop);
    bool NodeContainRule(Rule* rule);
    ~TDHiNode();                                                   
};

bool SameEffiRules(TDHiNode *node1, TDHiNode *node2);
#endif