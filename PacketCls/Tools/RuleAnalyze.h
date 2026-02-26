#ifndef  RULEANALYZE_H
#define  RULEANALYZE_H

#include "../Elements/Elements.h"

using namespace std;
double calculate_range(vector<Rule*> &rules,int dim);
double calculate_weight(vector<Rule*> &rules,int dim);
double MonteCarloCollisionCounter(vector<Rule*> &rules,int n);

#endif