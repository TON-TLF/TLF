#ifndef  HYPERSPLIT_H
#define  HYPERSPLIT_H

#include "../../Elements/Elements.h"
#include "../../Tools/Tools.h"
#include "../Classifier.h"
#include "HsNode.h"

using namespace std;

class HyperSplit : public Classifier {
public:
    static int binth;                                      /** 叶子节点规则数阈值 */
    static bool remove_redund;                             /** 标记是否去除冗余 */

    HsNode *root;      
    void Create(vector<Rule*> &rules, ProgramState *ps);
    uint32_t Lookup(Trace *trace, ProgramState *ps);
    uint32_t Lookup(Trace *trace);
    uint64_t CalMemory();
};

#endif