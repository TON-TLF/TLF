#ifndef  TD_HCNODE_H
#define  TD_HCNODE_H

#include "../../Elements/Elements.h"
#include "../../Tools/Tools.h"
#include "../TreeNode.h"
#include "TDHyperCuts.h"

using namespace std;

class TDHcNode : public TreeNode {
public:    
    vector<Rule*> rules;   
    int rules_num;          
    bool leaf_node;         
    uint32_t bounds[5][2];  

    int cuts[2];            
    char dims[2];          
    char dims_num;          
    uint64_t width[2];      

    TDHcNode** child_arr;  
    int child_num;        

    TDHcNode();
    void Create(vector<Rule*> &rules, ProgramState *ps, int depth){}; 
    void Create(ProgramState *ps, int depth);                         
    uint64_t CalMemory();                                            
    void HasBounds();                                                
    void RemoveRedund();
    void CalDimToCuts(int traces_num);
    void CalNumCuts(double pop);
    bool NodeContainRule(Rule* rule);
    ~TDHcNode();                                                     
};

bool SameEffiRules(TDHcNode *node1, TDHcNode *node2);
#endif