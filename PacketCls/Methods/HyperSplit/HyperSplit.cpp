#include "HyperSplit.h"

using namespace std;

int HyperSplit::binth = 0; 
bool HyperSplit::remove_redund = true;

/**
 * @brief 根据给定规则创建HyperSplit树。
 * @param rules 规则指针的向量。
 * @param ps 程序状态指针。
 */
void HyperSplit::Create(vector<Rule*> &rules, ProgramState *ps) {
    /** 对规则集按照优先级进行排序 */
	int rules_num = rules.size();
	sort(rules.begin(), rules.end(), CmpRulePriority);

    /** 根据规则数量设置叶子节点阈值 */
	if      (rules_num <= 20000) HyperSplit::binth = 16;
	else if (rules_num <= 60000) HyperSplit::binth = 24;
	else                         HyperSplit::binth = 32;

	root = new HsNode;
    root->Create(rules, ps, 0);
}

/**
 * @brief 查找并访问Trace，同时更新程序状态。
 * @param trace Trace对象的指针。
 * @param ps 程序状态指针。
 * @return 查找结果的优先级。
 */
uint32_t HyperSplit::Lookup(Trace *trace, ProgramState *ps) {
	HsNode *node = root;
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
uint32_t HyperSplit::Lookup(Trace *trace) {
	HsNode *node = root;
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
 * @brief 计算HyperSplit树使用的内存大小。
 * @return 内存大小（字节）。
 */
uint64_t HyperSplit::CalMemory() {
	uint64_t memory_size = sizeof(HyperSplit);
	memory_size += root->CalMemory();
	return memory_size;
}