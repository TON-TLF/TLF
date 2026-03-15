#ifndef  HSNODE_H
#define  HSNODE_H

#include "../../Elements/Elements.h"
#include "../../Tools/Tools.h"
#include "../TreeNode.h"

using namespace std;

class HsNode : public TreeNode {
public:
    uint8_t dim;           
    uint32_t range[2][2];  
    HsNode *child[2];      
    vector<Rule*> rules;   

    HsNode();
    void Create(vector<Rule*> &rules, ProgramState *ps, int depth);   
    void CalHowToCut(vector<Rule*> &rules);                          
    uint64_t CalMemory();                                            
    ~HsNode();
};

#endif