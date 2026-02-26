#include "HyperCuts.h"
#include "HcNode.h"
#include "../Category.h"

using namespace std;

double HyperCuts::spfac = 8.0;
int HyperCuts::max_depth = 0;
int HyperCuts::leaf_size = 16;
bool HyperCuts::remove_redund = true;
bool HyperCuts::node_merge = true;
bool HyperCuts::width_power2 = true;

HyperCuts::HyperCuts(){
    tree_num = 0;
    root = NULL;
}

void HyperCuts::Setparam(double _spfac){
    HyperCuts::spfac = _spfac;
}

/**
 * @brief 根据给定规则创建HyperCuts树。
 * @param rules 规则指针的向量。
 * @return bool 表示创建是否成功。
 */
void HyperCuts::Create(vector<Rule*> &rules, ProgramState *ps) {
    int rules_nums = rules.size();
    sort(rules.begin(), rules.end(), CmpRulePriority);

    /** 根据规则数量设置最大深度 */
	if      (rules_nums >= 90000) HyperCuts::leaf_size = 32, HyperCuts::max_depth = 6;
    else if (rules_nums >= 40000) HyperCuts::leaf_size = 24, HyperCuts::max_depth = 5;
    else                          HyperCuts::leaf_size = 16, HyperCuts::max_depth = 4;
    
    root = new HcNode;
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
uint32_t HyperCuts::Lookup(Trace *trace, ProgramState *ps) {
    uint32_t ans = 0;
    HcNode *node = root;

    while (node->leaf_node != 1) {
        ps->lookup_access_nodes.Addcount();
		ps->lookup_access.Addcount();

        int dims[2] = {node->dims[0], node->dims[1]};
        uint64_t w[2] = {trace->key[dims[0]], trace->key[dims[1]]};

        for(int j = 0; j < 2; ++j){
            if(node->bounds[dims[j]][0] > w[j] ||  node->bounds[dims[j]][1] < w[j]){
                cout<<">>> HyperCuts::Lookup -> Wrong! range error!"<<endl;
                exit(1);
            }
        }

        if(node->width[0] == 0 || node->width[1] == 0){
            cout<<">>> HyperCuts::Lookup -> Wrong! width error!"<<endl;
            exit(1);
        }

        uint32_t index0 = (w[0] - (uint64_t)node->bounds[dims[0]][0]) / node->width[0];
        uint32_t index1 = (w[1] - (uint64_t)node->bounds[dims[1]][0]) / node->width[1];

        uint32_t nxt = index0 * node->cuts[1] + index1;
        
        if (node->child_arr[nxt] == NULL){
            cout<<">>> HyperCuts::Lookup -> Wrong! node == NULL!"<<endl;
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
uint32_t HyperCuts::Lookup(Trace *trace) {
    uint32_t ans = 0;
    HcNode *node = root;

    while (node->leaf_node != 1) {

        int dims[2] = {node->dims[0], node->dims[1]};
        uint64_t w[2] = {trace->key[dims[0]], trace->key[dims[1]]};

        for(int j = 0; j < 2; ++j){
            if(node->bounds[dims[j]][0] > w[j] ||  node->bounds[dims[j]][1] < w[j]){
                cout<<">>> HyperCuts::Lookup -> Wrong! range error!"<<endl;
                exit(1);
            }
        }

        if(node->width[0] == 0 || node->width[1] == 0){
            cout<<">>> HyperCuts::Lookup -> Wrong! width error!"<<endl;
            exit(1);
        }

        uint32_t index0 = (w[0] - (uint64_t)node->bounds[dims[0]][0]) / node->width[0];
        uint32_t index1 = (w[1] - (uint64_t)node->bounds[dims[1]][0]) / node->width[1];

        uint32_t nxt = index0 * node->cuts[1] + index1;
        
        if (node->child_arr[nxt] == NULL){
            cout<<">>> HyperCuts::Lookup -> Wrong! node == NULL!"<<endl;
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
 * @brief 计算HyperCuts树使用的内存大小。
 * @return 内存大小（字节）。
 */
uint64_t HyperCuts::CalMemory() {
	uint64_t memory_cost = sizeof(HyperCuts);
	memory_cost += root->CalMemory();
	return memory_cost;
}

HyperCuts::~HyperCuts() {
    if(root) delete root;
}