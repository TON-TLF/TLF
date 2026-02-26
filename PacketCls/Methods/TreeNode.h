#ifndef  TREENODE_H
#define  TREENODE_H

#include "../Elements/Elements.h"

using namespace std;

/**
 * @brief 决策树节点类
 */
class TreeNode {
public:
    virtual void Create(vector<Rule*> &rules, ProgramState *ps, int depth) = 0;    /** 根据给定的规则和深度创建节点 */
    virtual uint64_t CalMemory() = 0;                                  /** 计算节点使用的内存大小 */
};

#endif