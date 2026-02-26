#include "TDHyperCuts.h"
#include "TDHcNode.h"
#include "../Category.h"

using namespace std;

double TDHyperCuts::spfac = 8.0;
int TDHyperCuts::max_depth = 0;
int TDHyperCuts::leaf_size = 16;
bool TDHyperCuts::remove_redund = true;
bool TDHyperCuts::node_merge = true;
bool TDHyperCuts::width_power2 = true;

int TDHyperCuts::total_trace_num = 0;
double TDHyperCuts::wfac = 1;  
double TDHyperCuts::pop_max = 2.0;
double TDHyperCuts::pop_min = 0.8;
double TDHyperCuts::pop_add_1 = 1.5;
double TDHyperCuts::pop_add_2 = 2.5;
double TDHyperCuts::pop_dec_1 = 0.2;
double TDHyperCuts::pop_dec_2 = 0.05;
double TDHyperCuts::pop_mul_1 = 1.5;
double TDHyperCuts::pop_mul_2 = 2;
double TDHyperCuts::pop_div_1 = 0.6;
double TDHyperCuts::pop_div_2 = 0.1;

TDHyperCuts::TDHyperCuts(){
    tree_num = 0;
    root = NULL;
}

void TDHyperCuts::Setparam(double _spfac, double _wfac){
    TDHyperCuts::spfac = _spfac;
    TDHyperCuts::wfac = _wfac;
}

/**
 * @brief 根据给定规则创建TDHyperCuts树。
 * @param rules 规则指针的向量。
 * @return bool 表示创建是否成功。
 */
void TDHyperCuts::Create(vector<Rule*> &rules, ProgramState *ps) {
    /** 根据规则数量设置最大深度 */
    int rules_nums = rules.size();
    sort(rules.begin(), rules.end(), CmpRulePriority);

	if      (rules_nums >= 90000) TDHyperCuts::leaf_size = 32, TDHyperCuts::max_depth = 6;
    else if (rules_nums >= 40000) TDHyperCuts::leaf_size = 24, TDHyperCuts::max_depth = 5;
    else                          TDHyperCuts::leaf_size = 16, TDHyperCuts::max_depth = 4;

    uint32_t bounds[2][2];
    bounds[0][0] = bounds[1][0] =0;     
    bounds[0][1] = bounds[1][1] =0xFFFFFFFF;
	TDHyperCuts::total_trace_num = traceFreqMat2d::getTracenum2D(bounds);

    root = new TDHcNode;
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
uint32_t TDHyperCuts::Lookup(Trace *trace, ProgramState *ps) {
    
    uint32_t ans = 0;
    TDHcNode *node = root;

    while (node->leaf_node != 1) {
        ps->lookup_access_nodes.Addcount();
	    ps->lookup_access.Addcount();

        int dims[2] = {node->dims[0], node->dims[1]};
        uint64_t w[2] = {trace->key[dims[0]], trace->key[dims[1]]};

        for(int j = 0; j < 2; ++j){
            if(node->bounds[dims[j]][0] > w[j] ||  node->bounds[dims[j]][1] < w[j]){
                cout<<"TDHyperCuts::Lookup range error!"<<endl;
                exit(1);
            }
        }

        if(node->width[0] == 0 || node->width[1] == 0){
            cout<<"TDHyperCuts::Lookup width error!"<<endl;
            exit(1);
        }

        uint32_t index0 = (w[0] - (uint64_t)node->bounds[dims[0]][0]) / node->width[0];
        uint32_t index1 = (w[1] - (uint64_t)node->bounds[dims[1]][0]) / node->width[1];

        uint32_t nxt = index0 * node->cuts[1] + index1;
        
        if (node->child_arr[nxt] == NULL){
            cout<<"TDHyperCuts::Lookup node == NULL"<<endl;
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
uint32_t TDHyperCuts::Lookup(Trace *trace) {
    
    uint32_t ans = 0;
    TDHcNode *node = root;

    while (node->leaf_node != 1) {
        int dims[2] = {node->dims[0], node->dims[1]};
        uint64_t w[2] = {trace->key[dims[0]], trace->key[dims[1]]};

        for(int j = 0; j < 2; ++j){
            if(node->bounds[dims[j]][0] > w[j] ||  node->bounds[dims[j]][1] < w[j]){
                cout<<"TDHyperCuts::Lookup range error!"<<endl;
                exit(1);
            }
        }

        if(node->width[0] == 0 || node->width[1] == 0){
            cout<<"TDHyperCuts::Lookup width error!"<<endl;
            exit(1);
        }

        uint32_t index0 = (w[0] - (uint64_t)node->bounds[dims[0]][0]) / node->width[0];
        uint32_t index1 = (w[1] - (uint64_t)node->bounds[dims[1]][0]) / node->width[1];

        uint32_t nxt = index0 * node->cuts[1] + index1;
        
        if (node->child_arr[nxt] == NULL){
            cout<<"TDHyperCuts::Lookup node == NULL"<<endl;
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
 * @brief 计算TDHyperCuts树使用的内存大小。
 * @return 内存大小（字节）。
 */
uint64_t TDHyperCuts::CalMemory() {
	uint64_t memory_cost = sizeof(TDHyperCuts);
	memory_cost += root->CalMemory();
	return memory_cost;
}

/**
 * @brief 释放HyperSplit树的内存。
 */
TDHyperCuts::~TDHyperCuts() {
    if(root) delete root;
}