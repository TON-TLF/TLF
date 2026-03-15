#ifndef  TREENODE_H
#define  TREENODE_H

#include "../Elements/Elements.h"

using namespace std;

class TreeNode {
public:
    virtual void Create(vector<Rule*> &rules, ProgramState *ps, int depth) = 0;   
    virtual uint64_t CalMemory() = 0;                               
};

#endif