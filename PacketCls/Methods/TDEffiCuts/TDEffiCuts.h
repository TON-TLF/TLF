#ifndef  TD_EFFICUTS_H
#define  TD_EFFICUTS_H

#include "../../Elements/Elements.h"
#include "../../Tools/Tools.h"
#include "../../TopK/TopK.h"
#include "../Classifier.h"
#include "TDEffiNode.h"

using namespace std;

class TDEffiCuts : public Classifier {
public:
    static double spfac;          /** 空间放大因子 */
    static int max_depth;         /** 子树最大深度阈值 */
    static int leaf_size;         /** 叶子节点阈值 */
    static bool remove_redund;    /** 是否启用了冗余移除 */
    static bool node_merge;       /** 是否启用了节点合并 */
    static bool width_power2;     /** 是否优化成2的次幂 */
    int tree_num;                 /** 子树的个数 */
    struct TDEffiNode **root;  /** 指向TDEffiCuts树的根节点 */

    static int total_trace_num;   /** 总流量数量 */
    static double wfac;           /** 权重因子 */ 
    static double pop_max;
    static double pop_min;
    static double pop_add_1;
    static double pop_add_2;
    static double pop_dec_1;
    static double pop_dec_2;
    static double pop_mul_1;
    static double pop_mul_2;
    static double pop_div_1;
    static double pop_div_2;

    TDEffiCuts();
    void Create(vector<Rule*> &rules, ProgramState *ps);  
    uint32_t Lookup(Trace *trace, ProgramState *ps);     
    uint32_t Lookup(Trace *trace); 
    uint64_t CalMemory(); 
    void Setparam(double _spfac, double _wfac);                                
    ~TDEffiCuts();
};

#endif