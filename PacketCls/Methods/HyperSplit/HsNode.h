#ifndef  HSNODE_H
#define  HSNODE_H

#include "../../Elements/Elements.h"
#include "../../Tools/Tools.h"
#include "../TreeNode.h"

using namespace std;

/**
 * @brief 表示HyperSplitNew树中的一个节点。
 */
class HsNode : public TreeNode {
public:
    uint8_t dim;           /** 分割维度 */
    uint32_t range[2][2];  /** 左右孩子范围 */  
    HsNode *child[2];      /** 指向子节点的指针数组 */
    vector<Rule*> rules;   /** 与节点相关联的规则向量 */

    HsNode();
    void Create(vector<Rule*> &rules, ProgramState *ps, int depth);   /** 根据给定的规则创建节点 */
    void CalHowToCut(vector<Rule*> &rules);                           /** 计算如何划分子结点 */
    uint64_t CalMemory();                                             /** 计算以该节点为根的树的内存大小 */
    ~HsNode();
};

#endif