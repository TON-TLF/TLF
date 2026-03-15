#include "EffiCuts.h"
#include "EffiNode.h"
#include "../Category.h"

using namespace std;

double EffiCuts::spfac = 8.0;
int EffiCuts::max_depth = 0;
int EffiCuts::leaf_size = 16;
bool EffiCuts::remove_redund = true;
bool EffiCuts::node_merge = true;
bool EffiCuts::width_power2 = true;

bool CmpEffiNodePriority(EffiNode *node1, EffiNode *node2) {
	return node1->priority > node2->priority;
}

EffiCuts::EffiCuts(){
    tree_num = 0;
    root = NULL;
}

void EffiCuts::Setparam(double _spfac){
    EffiCuts::spfac = _spfac;
}

void EffiCuts::Create(vector<Rule*> &rules, ProgramState *ps) {
    int rules_nums = rules.size();
    sort(rules.begin(), rules.end(), CmpRulePriority);

	if      (rules_nums >= 90000) EffiCuts::max_depth = 6;
    else if (rules_nums >= 40000) EffiCuts::max_depth = 5;
    else                          EffiCuts::max_depth = 4;
    
    Category category;
    category.Init();
    category.ProcessRule(rules);
    vector<pair<int,int>> ans = category.TreeMerge();
    
    tree_num = ans.size();
    root = new EffiNode*[tree_num];
    for(int i = 0; i < tree_num; i++){
        root[i] = new EffiNode;
        root[i]->rules = category.sub[ans[i].first][ans[i].second]->rules;
        root[i]->rules_num = root[i]->rules.size();
        root[i]->Create(ps, 0);
    }
	sort(root, root + tree_num, CmpEffiNodePriority);
}

uint32_t EffiCuts::Lookup(Trace *trace, ProgramState *ps) {
    uint32_t ans = 0;
	for (int i = 0; i < tree_num; ++i) {
        EffiNode *node = root[i];
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
                    cout<<"EffiCuts::Lookup range error!"<<endl;
                    exit(1);
                }
            }

            if(node->width[0] == 0 || node->width[1] == 0){
                cout<<"EffiCuts::Lookup width error!"<<endl;
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
                ans = node->rules[j]->priority;
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

uint32_t EffiCuts::Lookup(Trace *trace) {
    uint32_t ans = 0;
	for (int i = 0; i < tree_num; ++i) {
        EffiNode *node = root[i];
        if (node == NULL || ans >= node->priority)
            continue;

        bool flag = false;
        while (node->leaf_node != 1) {
            int dims[2] = {node->dims[0], node->dims[1]};
            uint64_t w[2] = {trace->key[dims[0]], trace->key[dims[1]]};

            for(int j = 0; j < 2; ++j){
                if(node->bounds[dims[j]][0] > w[j] ||  node->bounds[dims[j]][1] < w[j]){
                    cout<<"EffiCuts::Lookup range error!"<<endl;
                    exit(1);
                }
            }

            if(node->width[0] == 0 || node->width[1] == 0){
                cout<<"EffiCuts::Lookup width error!"<<endl;
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
                ans = node->rules[j]->priority;
                break;
            }
        }
	}
	return ans;
}

uint64_t EffiCuts::CalMemory() {
	uint64_t memory_cost = sizeof(EffiCuts);
	for (int i = 0; i < EffiCuts::tree_num; ++i)
		memory_cost += root[i]->CalMemory();
	return memory_cost;
}

EffiCuts::~EffiCuts() {
    if(root){
        for (int i = 0; i < EffiCuts::tree_num; ++i){
            if(root[i]){
                delete root[i];
            }
        }
        delete root;
    }
}