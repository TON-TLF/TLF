#ifndef  HICUTS_H
#define  HICUTS_H

#include "../../Elements/Elements.h"
#include "../../Tools/Tools.h"
#include "../Classifier.h"
#include "HiNode.h"

using namespace std;

/**
 * @brief HiCuts分类器类。
 */
class HiCuts : public Classifier {
public:
    static double spfac;                              /** 空间放大因子 */
    static int max_depth;                             /** 子树最大深度阈值 */
    static int leaf_size;                             /** 叶子节点阈值 */
    static bool remove_redund;                        /** 是否启用了冗余移除 */
    static bool node_merge;                           /** 是否启用了节点合并 */
    static bool width_power2;                         /** 是否优化成2的次幂 */
    int tree_num;                                     /** 子树的个数 */
    struct HiNode *root;                              /** 指向HiCuts树的根节点 */

    HiCuts();
    void Create(vector<Rule*> &rules, ProgramState *ps);
    uint32_t Lookup(Trace *trace, ProgramState *ps);
    uint32_t Lookup(Trace *trace);
    uint64_t CalMemory();
    void Setparam(double _spfac);
    ~HiCuts();
};

#endif