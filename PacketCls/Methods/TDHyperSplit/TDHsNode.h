#ifndef  TD_HSNODE_H
#define  TD_HSNODE_H

#include "../../Elements/Elements.h"
#include "../../Tools/Tools.h"
#include "../TreeNode.h"

using namespace std;

class TDHsNode : public TreeNode {
public:
    uint8_t dim;          
    uint32_t range[2][2];   
    TDHsNode *child[2];   
    vector<Rule*> rules;   

    TDHsNode();
    void Create(vector<Rule*> &rules, ProgramState *ps, int depth){};                
    void Create(vector<Rule*> &rules, ProgramState *ps, uint32_t bounds[][2], int depth);
    void CalHowToCut(vector<Rule*> &rules, int traces_num, uint32_t bounds[][2]);    
    uint64_t CalMemory();                                                             
    ~TDHsNode();
};

#endif