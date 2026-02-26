#include "TDEffiCuts.h"
#include "TDEffiNode.h"
#include "../Category.h"

using namespace std;

double TDEffiCuts::spfac = 8.0;
int TDEffiCuts::max_depth = 0;
int TDEffiCuts::leaf_size = 16;
bool TDEffiCuts::remove_redund = true;
bool TDEffiCuts::node_merge = true;
bool TDEffiCuts::width_power2 = true;

int TDEffiCuts::total_trace_num = 0;
double TDEffiCuts::wfac = 1;   
double TDEffiCuts::pop_max = 2.0;
double TDEffiCuts::pop_min = 0.6;
double TDEffiCuts::pop_add_1 = 1.5;
double TDEffiCuts::pop_add_2 = 2.5;
double TDEffiCuts::pop_dec_1 = 0.2;
double TDEffiCuts::pop_dec_2 = 0.01;
double TDEffiCuts::pop_mul_1 = 1.5;
double TDEffiCuts::pop_mul_2 = 2;
double TDEffiCuts::pop_div_1 = 0.5;
double TDEffiCuts::pop_div_2 = 0.1;


bool CmpTDEffiNodePriority(TDEffiNode *node1, TDEffiNode *node2) {
	return node1->priority > node2->priority;
}

TDEffiCuts::TDEffiCuts(){
    tree_num = 0;
    root = NULL;
}

void TDEffiCuts::Setparam(double _spfac, double _wfac){
    TDEffiCuts::spfac = _spfac;
    TDEffiCuts::wfac = _wfac;
}

/**
 * @brief 根据给定规则创建TDEffiCuts树。
 * @param rules 规则指针的向量。
 * @return bool 表示创建是否成功。
 */
void TDEffiCuts::Create(vector<Rule*> &rules, ProgramState *ps) {
    /** 根据规则数量设置最大深度 */
    int rules_nums = rules.size();
    sort(rules.begin(), rules.end(), CmpRulePriority);

	if      (rules_nums >= 90000) TDEffiCuts::max_depth = 6;
    else if (rules_nums >= 40000) TDEffiCuts::max_depth = 5;
    else                          TDEffiCuts::max_depth = 4;

    uint32_t bounds[2][2];
    bounds[0][0] = bounds[1][0] =0;     
    bounds[0][1] = bounds[1][1] =0xFFFFFFFF;
	TDEffiCuts::total_trace_num = traceFreqMat2d::getTracenum2D(bounds);

    /** 预处理规则 */
    Category category;
    category.Init();
    category.ProcessRule(rules);
    vector<pair<int,int>> ans = category.TreeMerge();
    
    tree_num = ans.size();
    root = new TDEffiNode*[tree_num];
    for(int i = 0; i < tree_num; i++){
        root[i] = new TDEffiNode;
        root[i]->rules = category.sub[ans[i].first][ans[i].second]->rules;
        root[i]->rules_num = root[i]->rules.size();
        root[i]->Create(ps, 0);
    }
	sort(root, root + tree_num, CmpTDEffiNodePriority);
}

/**
 * @brief 查找并访问Trace，同时更新程序状态。
 * @param trace Trace对象的指针。
 * @param ps 程序状态指针。
 * @return 查找结果的优先级。
 */
uint32_t TDEffiCuts::Lookup(Trace *trace, ProgramState *ps) {
    uint32_t ans = 0;
	for (int i = 0; i < tree_num; ++i) {
        TDEffiNode *node = root[i];
        if (node == NULL || ans >= node->priority)
            continue;

        bool flag = false;
        while (node->leaf_node != 1) {
            ps->lookup_access_nodes.Addcount();
		    ps->lookup_access.Addcount();

            int dims[2] = {node->dims[0], node->dims[1]};
            uint64_t w[2] = {trace->key[dims[0]], trace->key[dims[1]]};

            for(int j = 0; j < 2; ++j){
                if(node->bounds[dims[j]][0] > w[j] ||  node->bounds[dims[j]][1] < w[j]){
                    cout<<"TDEffiCuts::Lookup range error!"<<endl;
                    exit(1);
                }
            }

            if(node->width[0] == 0 || node->width[1] == 0){
                cout<<"TDEffiCuts::Lookup width error!"<<endl;
                exit(1);
            }

            uint32_t index0 = (w[0] - (uint64_t)node->bounds[dims[0]][0]) / node->width[0];
            uint32_t index1 = (w[1] - (uint64_t)node->bounds[dims[1]][0]) / node->width[1];

            uint32_t nxt = index0 * node->cuts[1] + index1;
            
            if (node->child_arr[nxt] == NULL){
                flag = true;
                break;
            }
            node = node->child_arr[nxt];
            ps->lookup_depth.Addcount();
        }
        ps->lookup_access_nodes.Addcount();
		ps->lookup_access.Addcount();

        if(flag) continue;

        for (int j = 0; j < node->rules_num; ++j) {
            ps->lookup_access_rules.Addcount();
		    ps->lookup_access.Addcount();
            if(ans >= node->rules[j]->priority){
                break;
            }
            if (MatchRuleTrace(node->rules[j], trace) && ans < node->rules[j]->priority){
                ans = max(ans,node->rules[j]->priority);
                break;
            }
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
uint32_t TDEffiCuts::Lookup(Trace *trace) {
    uint32_t ans = 0;
	for (int i = 0; i < tree_num; ++i) {
        TDEffiNode *node = root[i];
        if (node == NULL || ans >= node->priority)
            continue;

        bool flag = false;
        while (node->leaf_node != 1) {
            int dims[2] = {node->dims[0], node->dims[1]};
            uint64_t w[2] = {trace->key[dims[0]], trace->key[dims[1]]};

            for(int j = 0; j < 2; ++j){
                if(node->bounds[dims[j]][0] > w[j] ||  node->bounds[dims[j]][1] < w[j]){
                    cout<<"TDEffiCuts::Lookup range error!"<<endl;
                    exit(1);
                }
            }

            if(node->width[0] == 0 || node->width[1] == 0){
                cout<<"TDEffiCuts::Lookup width error!"<<endl;
                exit(1);
            }

            uint32_t index0 = (w[0] - (uint64_t)node->bounds[dims[0]][0]) / node->width[0];
            uint32_t index1 = (w[1] - (uint64_t)node->bounds[dims[1]][0]) / node->width[1];

            uint32_t nxt = index0 * node->cuts[1] + index1;
            
            if (node->child_arr[nxt] == NULL){
                flag = true;
                break;
            }
            node = node->child_arr[nxt];
        }

        if(flag) continue;

        for (int j = 0; j < node->rules_num; ++j) {
            if(ans >= node->rules[j]->priority){
                break;
            }
            if (MatchRuleTrace(node->rules[j], trace) && ans < node->rules[j]->priority){
                ans = max(ans,node->rules[j]->priority);
                break;
            }
        }
	}

	return ans;
}

/**
 * @brief 计算TDEffiCuts树使用的内存大小。
 * @return 内存大小（字节）。
 */
uint64_t TDEffiCuts::CalMemory() {
	uint64_t memory_cost = sizeof(TDEffiCuts);
	for (int i = 0; i < TDEffiCuts::tree_num; ++i)
		memory_cost += root[i]->CalMemory();
	return memory_cost;
}

/**
 * @brief 释放HyperSplit树的内存。
 */
TDEffiCuts::~TDEffiCuts() {
    if(root){
        for (int i = 0; i < TDEffiCuts::tree_num; ++i){
            if(root[i]){
                delete root[i];
            }
        }
        delete root;
    }
}