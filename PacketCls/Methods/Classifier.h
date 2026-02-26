#ifndef  CLASSIFIER_H
#define  CLASSIFIER_H

#include "../Elements/Elements.h"

using namespace std;

/**
 * @brief 分类器类，定义了分类器的接口，所有分类器必须实现这些接口
 */
class Classifier {
public:
    virtual void Create(vector<Rule*> &rules, ProgramState *ps) = 0; /** 创建分类器 */ 
    virtual uint32_t Lookup(Trace *trace, ProgramState *ps) = 0;     /** 查找规则 */
    virtual uint32_t Lookup(Trace *trace) = 0;                       /** 查找规则 */
    virtual uint64_t CalMemory() = 0;                                /** 获取分类器占用的内存大小 */
};

#endif