#include "TDHyperSplit.h"

using namespace std;

uint32_t TDHyperSplit::binth = 0; 
uint32_t TDHyperSplit::total_trace_num = 0;
bool TDHyperSplit::remove_redund = true;
double TDHyperSplit::pfac = 1;
double TDHyperSplit::pop_max = 1.2;
double TDHyperSplit::pop_min = 0.8;

void TDHyperSplit::Setparam(double _pfac){
    TDHyperSplit::pfac = _pfac;
}

/**
 * @brief 根据给定规则创建HyperSplitMat2D树。
 * @param rules 规则指针的向量。
 * @param ps 程序状态指针。
 */
void TDHyperSplit::Create(vector<Rule*> &rules, ProgramState *ps) {
    /** 对规则集按照优先级进行排序 */
	int rules_num = rules.size();
	sort(rules.begin(), rules.end(), CmpRulePriority);

    /** 根据规则数量设置叶子节点阈值 */
	if      (rules_num <= 20000) TDHyperSplit::binth = 16;
	else if (rules_num <= 60000) TDHyperSplit::binth = 24;
	else                         TDHyperSplit::binth = 32;

	uint32_t bounds[2][2];
    bounds[0][0] = bounds[1][0] =0;     
    bounds[0][1] = bounds[1][1] =0xFFFFFFFF;

    TDHyperSplit::total_trace_num = traceFreqMat2d::getTracenum2D(bounds);

	root = new TDHsNode;
    root->Create(rules, ps, bounds, 0);
}

/**
 * @brief 查找并访问Trace，同时更新程序状态。
 * @param trace Trace对象的指针。
 * @param ps 程序状态指针。
 * @return 查找结果的优先级。
 */
uint32_t TDHyperSplit::Lookup(Trace *trace, ProgramState *ps) {
	TDHsNode *node = root;
	while (node->child[0] != NULL) {
        ps->lookup_access_nodes.Addcount();
		ps->lookup_access.Addcount();

		if (trace->key[node->dim] <= node->range[0][1])
			node = node->child[0];
		else
			node = node->child[1];

		ps->lookup_depth.Addcount();
	}
    ps->lookup_access_nodes.Addcount();
	ps->lookup_access.Addcount();

	uint32_t ans = 0;
	for (int i = 0; i < (int)node->rules.size(); ++i) {
		ps->lookup_access_rules.Addcount();
		ps->lookup_access.Addcount();
		
		if (MatchRuleTrace(node->rules[i], trace)) {
			ans = node->rules[i]->priority;
            break;
		}
	}
    ps->lookup_access_nodes.Cal();
    ps->lookup_access_rules.Cal();
	ps->lookup_access.Cal();
	ps->lookup_depth.Cal();
	return ans;
}

/**
 * @brief 查找并访问Trace，同时更新程序状态。
 * @param trace Trace对象的指针。
 * @return 查找结果的优先级。
 */
uint32_t TDHyperSplit::Lookup(Trace *trace) {
	TDHsNode *node = root;
	while (node->child[0] != NULL) {
		if (trace->key[node->dim] <= node->range[0][1])
			node = node->child[0];
		else
			node = node->child[1];
	}
	uint32_t ans = 0;
	for (int i = 0; i < (int)node->rules.size(); ++i) {
		if (MatchRuleTrace(node->rules[i], trace)) {
			ans = node->rules[i]->priority;
            break;
		}
	}
	return ans;
}

/**
 * @brief 计算HyperSplitMat2D树使用的内存大小。
 * @return 内存大小（字节）。
 */
uint64_t TDHyperSplit::CalMemory() {
	uint64_t memory_size = sizeof(TDHyperSplit);
	memory_size += root->CalMemory();
	return memory_size;
}