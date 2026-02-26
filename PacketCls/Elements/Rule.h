#ifndef  RULE_H
#define  RULE_H

#include "Common.h"

using namespace std;

class Rule {
public:
	uint32_t range[5][2];   
	uint8_t prefix_len[5];   
	uint32_t priority;       

	Rule(){
		for(int i = 0; i < 5; i++){
			range[i][0] = range[i][1] = prefix_len[i] = 0;
		}
		priority = 0;
	};

	void SetRule(const Rule* rule) {
        for (int i = 0; i < 5; ++i) {
            range[i][0] = rule->range[i][0];
            range[i][1] = rule->range[i][1];
            prefix_len[i] = rule->prefix_len[i];
        }
        priority = rule->priority;
    }
	
	bool operator<(const Rule& rule) const {
		for (int i = 0; i < 5; ++i)
			for (int j = 0; j < 2; ++j)
				if (range[i][j] != rule.range[i][j])
					return range[i][j] < rule.range[i][j];
		return 0;
    }

    bool operator==(const Rule& rule) const {
		for (int i = 0; i < 5; ++i)
			for (int j = 0; j < 2; ++j)
				if (range[i][j] != rule.range[i][j])
					return false;
		for (int i = 0; i < 5; ++i)
			if (prefix_len[i] != rule.prefix_len[i])
				return false;
		return priority == rule.priority;
    }
};

#endif