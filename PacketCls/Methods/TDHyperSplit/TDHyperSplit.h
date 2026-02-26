#ifndef  TD_HYPERSPLIT_H
#define  TD_HYPERSPLIT_H

#include "../../Elements/Elements.h"
#include "../../Tools/Tools.h"
#include "../../TopK/TopK.h"
#include "../Classifier.h"
#include "TDHsNode.h"

using namespace std;

class TDHyperSplit : public Classifier {
public:
    static uint32_t binth;                                 /** 叶子节点规则数阈值 */
    static uint32_t total_trace_num;                       /** 参与优化决策树构建的流量总数 */
    static bool remove_redund;                             /** 标记是否去除冗余 */
    static double pfac;                                    /** 衡量流量在维度选择时的权重 */
    static double pop_max;                                 /** 节点流行度最大值 */
    static double pop_min;                                 /** 节点流行度最小值 */
    
    TDHsNode *root;                                     
    void Create(vector<Rule*> &rules, ProgramState *ps);   
    uint32_t Lookup(Trace *trace, ProgramState *ps); 
    uint32_t Lookup(Trace *trace);      
    uint64_t CalMemory();  
    void Setparam(double _pfac);
};

#endif