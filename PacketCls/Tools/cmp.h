#ifndef  CMP_H
#define  CMP_H

#include "../Elements/Elements.h"

using namespace std;

/**
 * @brief 判断规则是否匹配流量
 * @param rule 规则
 * @param trace 流量
 * @return bool 返回说明
 * -# true 匹配
 * -# false 不匹配
 */
bool MatchRuleTrace(Rule *rule, Trace *trace);

/**
 * @brief 比较规则优先级
 * @param rule1 规则1
 * @param rule2 规则2
 * @return bool 返回说明
 * -# true 规则1优先级大于规则2
 * -# false 规则1优先级小于等于规则2
 */
bool CmpRulePriority(Rule *rule1, Rule *rule2);
#endif