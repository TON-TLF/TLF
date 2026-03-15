#include "cmp.h"

using namespace std;

bool MatchRuleTrace(Rule *rule, Trace *trace) {
	for (int i = 0; i < 5; i++){
        if (trace->key[i] < rule->range[i][0] || trace->key[i] > rule->range[i][1])
			return false;
    }
	return true; 
}

bool CmpRulePriority(Rule *rule1, Rule *rule2){
    return rule1->priority > rule2->priority;
}