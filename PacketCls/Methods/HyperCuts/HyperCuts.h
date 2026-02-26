#ifndef  HYPERCUTS_H
#define  HYPERCUTS_H

#include "../../Elements/Elements.h"
#include "../../Tools/Tools.h"
#include "../Classifier.h"
#include "HcNode.h"

using namespace std;

class HyperCuts : public Classifier {
public:
    static double spfac;          /** 空间放大因子 */
    static int max_depth;         /** 子树最大深度阈值 */
    static int leaf_size;         /** 叶子节点阈值 */
    static bool remove_redund;    /** 是否启用了冗余移除 */
    static bool node_merge;       /** 是否启用了节点合并 */
    static bool width_power2;     /** 是否优化成2的次幂 */
    int tree_num;                 /** 子树的个数 */
    struct HcNode *root;          /** 指向HyperCuts树的根节点 */

    HyperCuts();
    void Create(vector<Rule*> &rules, ProgramState *ps);   
    uint32_t Lookup(Trace *trace, ProgramState *ps);
    uint32_t Lookup(Trace *trace);       
    uint64_t CalMemory();       
    void Setparam(double _spfac);                           
    ~HyperCuts();
};

#endif