#include "TDHiCuts.h"
#include "TDHiNode.h"
#include "../Category.h"

using namespace std;

double TDHiCuts::spfac = 8.0;
int TDHiCuts::max_depth = 0;
int TDHiCuts::leaf_size = 16;
bool TDHiCuts::remove_redund = true;
bool TDHiCuts::node_merge = true;
bool TDHiCuts::width_power2 = true;

int TDHiCuts::total_trace_num = 0;
double TDHiCuts::wfac = 1;  
double TDHiCuts::pop_max = 2.0;
double TDHiCuts::pop_min = 0.6;
double TDHiCuts::pop_add_1 = 1.5;
double TDHiCuts::pop_add_2 = 2.5;
double TDHiCuts::pop_dec_1 = 0.2;
double TDHiCuts::pop_dec_2 = 0.05;
double TDHiCuts::pop_mul_1 = 1.5;
double TDHiCuts::pop_mul_2 = 2;
double TDHiCuts::pop_div_1 = 0.6;
double TDHiCuts::pop_div_2 = 0.1;

TDHiCuts::TDHiCuts(){
    tree_num = 0;
    root = NULL;
}

void TDHiCuts::Setparam(double _spfac, double _wfac){
    TDHiCuts::spfac = _spfac;
    TDHiCuts::wfac = _wfac;
}

/**
 * @brief 根据给定规则创建TDHiCuts树。
 * @param rules 规则指针的向量。
 * @return bool 表示创建是否成功。
 */
void TDHiCuts::Create(vector<Rule*> &rules, ProgramState *ps) {
    int rules_nums = rules.size();
    sort(rules.begin(), rules.end(), CmpRulePriority);

    /** 根据规则数量设置最大深度 */
	if      (rules_nums >= 90000) TDHiCuts::leaf_size = 32, TDHiCuts::max_depth = 6;
    else if (rules_nums >= 40000) TDHiCuts::leaf_size = 24, TDHiCuts::max_depth = 5;
    else                          TDHiCuts::leaf_size = 16, TDHiCuts::max_depth = 4;
    
    uint32_t bounds[2][2];
    bounds[0][0] = bounds[1][0] =0;     
    bounds[0][1] = bounds[1][1] =0xFFFFFFFF;
	TDHiCuts::total_trace_num = traceFreqMat2d::getTracenum2D(bounds);

    root = new TDHiNode;
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
uint32_t TDHiCuts::Lookup(Trace *trace, ProgramState *ps) {
    uint32_t ans = 0;
    TDHiNode *node = root;

    while (node->leaf_node != 1) {
        ps->lookup_access_nodes.Addcount();
		ps->lookup_access.Addcount();

        int dim = node->dim;
        uint32_t w = trace->key[dim];

        for(int i = 0; i < 2; ++i){
            if(node->bounds[dim][0] > w ||  node->bounds[dim][1] < w){
                cout<<"TDHiCuts::Lookup range error!"<<endl;
                exit(1);
            }
        }

        uint32_t nxt = (uint64_t)(w - node->bounds[dim][0]) / node->width;
        if (node->child_arr[nxt] == NULL){
            cout<<"TDHiCuts::Lookup node == NULL"<<endl;
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
uint32_t TDHiCuts::Lookup(Trace *trace) {
    uint32_t ans = 0;
    TDHiNode *node = root;

    while (node->leaf_node != 1) {
        int dim = node->dim;
        uint32_t w = trace->key[dim];

        for(int i = 0; i < 2; ++i){
            if(node->bounds[dim][0] > w ||  node->bounds[dim][1] < w){
                cout<<"TDHiCuts::Lookup range error!"<<endl;
                exit(1);
            }
        }

        uint32_t nxt = (uint64_t)(w - node->bounds[dim][0]) / node->width;
        if (node->child_arr[nxt] == NULL){
            cout<<"TDHiCuts::Lookup node == NULL"<<endl;
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
 * @brief 计算TDHiCuts树使用的内存大小。
 * @return 内存大小（字节）。
 */
uint64_t TDHiCuts::CalMemory() {
	uint64_t memory_cost = sizeof(TDHiCuts);
	memory_cost += root->CalMemory();
	return memory_cost;
}


TDHiCuts::~TDHiCuts() {
    if(root) delete root;
}