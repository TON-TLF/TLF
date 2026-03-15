#ifndef  CMP_H
#define  CMP_H

#include "../Elements/Elements.h"

using namespace std;

bool MatchRuleTrace(Rule *rule, Trace *trace);
bool CmpRulePriority(Rule *rule1, Rule *rule2);
#endif