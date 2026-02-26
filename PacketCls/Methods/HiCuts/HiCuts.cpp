#include "HiCuts.h"
#include "HiNode.h"
#include "../Category.h"

using namespace std;

double HiCuts::spfac = 8.0;
int HiCuts::max_depth = 0;
int HiCuts::leaf_size = 16;
bool HiCuts::remove_redund = true;
bool HiCuts::node_merge = true;
bool HiCuts::width_power2 = true;

HiCuts::HiCuts(){
    tree_num = 0;
    root = NULL;
}

void HiCuts::Setparam(double _spfac){
    HiCuts::spfac = _spfac;
}
/**
 * @brief 根据给定规则创建HiCuts树。
 * @param rules 规则指针的向量。
 * @return bool 表示创建是否成功。
 */
void HiCuts::Create(vector<Rule*> &rules, ProgramState *ps) {
    /** 根据规则数量设置最大深度 */
    int rules_nums = rules.size();
	if      (rules_nums >= 90000) HiCuts::leaf_size = 32, HiCuts::max_depth = 6;
    else if (rules_nums >= 40000) HiCuts::leaf_size = 24, HiCuts::max_depth = 5;
    else                          HiCuts::leaf_size = 16, HiCuts::max_depth = 4;

    sort(rules.begin(), rules.end(), CmpRulePriority);

    root = new HiNode;
    root->rules = rules;
    root->rules_num = rules_nums;
    root->Create(ps, 0);
}

/**
 * @brief 查找并访问Trace，同时更新程序状态。
 * @param trace Trace对象的指针。
 * @param ps 程序状态指针。
 * @return 查找结果的优先级。
 */
uint32_t HiCuts::Lookup(Trace *trace, ProgramState *ps) {
    uint32_t ans = 0;
    HiNode *node = root;

    while (node->leaf_node != 1) {
        ps->lookup_access_nodes.Addcount();
		ps->lookup_access.Addcount();

        int dim = node->dim;
        uint32_t w = trace->key[dim];

        for(int i = 0; i < 2; ++i){
            if(node->bounds[dim][0] > w ||  node->bounds[dim][1] < w){
                cout<<"HiCuts::Lookup range error!"<<endl;
                exit(1);
            }
        }

        uint32_t nxt = (uint64_t)(w - node->bounds[dim][0]) / node->width;
        if (node->child_arr[nxt] == NULL){
            cout<<"HiCuts::Lookup node == NULL"<<endl;
            exit(1);
        }
        node = node->child_arr[nxt];
        ps->lookup_depth.Addcount();
    }

    ps->lookup_access_nodes.Addcount();
	ps->lookup_access.Addcount();

    for (int j = 0; j < node->rules_num; ++j) {
        ps->lookup_access_rules.Addcount();
		ps->lookup_access.Addcount();
        if (MatchRuleTrace(node->rules[j], trace)){
            ans = node->rules[j]->priority;
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
uint32_t HiCuts::Lookup(Trace *trace) {
    uint32_t ans = 0;
    HiNode *node = root;

    while (node->leaf_node != 1) {

        int dim = node->dim;
        uint32_t w = trace->key[dim];

        for(int i = 0; i < 2; ++i){
            if(node->bounds[dim][0] > w ||  node->bounds[dim][1] < w){
                cout<<"HiCuts::Lookup range error!"<<endl;
                exit(1);
            }
        }

        uint32_t nxt = (uint64_t)(w - node->bounds[dim][0]) / node->width;
        if (node->child_arr[nxt] == NULL){
            cout<<"HiCuts::Lookup node == NULL"<<endl;
            exit(1);
        }
        node = node->child_arr[nxt];
    }

    for (int j = 0; j < node->rules_num; ++j) {
        if (MatchRuleTrace(node->rules[j], trace)){
            ans = node->rules[j]->priority;
            break;
        }
    }

	return ans;
}

/**
 * @brief 计算HiCuts树使用的内存大小。
 * @return 内存大小（字节）。
 */
uint64_t HiCuts::CalMemory() {
	uint64_t memory_cost = sizeof(HiCuts);
	memory_cost += root->CalMemory();
	return memory_cost;
}

HiCuts::~HiCuts() {
    if(root) delete root;
}