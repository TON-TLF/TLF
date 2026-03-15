#include "EffiNode.h"

using namespace std;

EffiNode::EffiNode(){      
    rules_num = 0;        
    priority = 0;          
    leaf_node = 0;        

    cuts[0] = cuts[1] = -1;  
    dims[0] = dims[1] = -1;       
    dims_num = 0;         
    width[0] = width[1] = -1;    

    child_arr = NULL;  
    child_num = 0; 
    bounds[0][0] = bounds[1][0] =0;
	bounds[2][0] = bounds[3][0] =0;
	bounds[4][0] = 0;         
    bounds[0][1] = bounds[1][1] =0xFFFFFFFF;
	bounds[2][1] = bounds[3][1] =0xFFFF;
	bounds[4][1] = 0xFF;
}

void EffiNode::Create(ProgramState *ps, int depth) {
    if (rules_num == 0){
        cout << ">>> EffiNode::Create -> Wrong! rules_num = 0" << endl;
        exit(1);
    }
    HasBounds();

    if (EffiCuts::remove_redund && depth != 0) {
        RemoveRedund();
    }
	sort(rules.begin(), rules.end(), CmpRulePriority);
    priority = rules[0]->priority; 

    if (depth == EffiCuts::max_depth || rules_num <= EffiCuts::leaf_size) {
        leaf_node = true;      
        ps->DTI.AddNode(depth, rules_num);
        return;                      
    }

    CalDimToCuts();
    CalNumCuts();
    
    ps->DTI.AddDims(dims[0], dims[1]);

    child_num = cuts[0] * cuts[1]; 

    child_arr = new EffiNode*[child_num];
    bool *vis = new bool[child_num];               
    int max_child_rules_num = 0;                  
    
    for (int i = 0; i < cuts[0]; ++i){
        for (int j = 0; j < cuts[1]; ++j) {
            int index = i * cuts[1] + j;    
            child_arr[index] = new EffiNode(); 
            vis[index] = false; 

            for (int k = 0; k < 5; ++k) {
                if (k == dims[0] || k == dims[1]) {
                    int p = (k == dims[0]) ? 0 : 1;
                    int ret = (p == 0) ? i : j;
                    uint64_t low = bounds[k][0] + (uint64_t)ret * width[p];
                    uint64_t high = min(low + width[p] - 1, (uint64_t)bounds[k][1]);

                    if (low > high) {
                        cout<<">>> EffiNode::Create -> Wrong! low > high error!"<<endl;
                        exit(1);
                        break;
                    }
                    child_arr[index]->bounds[k][0] = low; 
                    child_arr[index]->bounds[k][1] = high;
                } else { 
                    child_arr[index]->bounds[k][0] = bounds[k][0];
                    child_arr[index]->bounds[k][1] = bounds[k][1];
                }
            }

            for (int k = 0; k < rules_num; ++k){
                if (child_arr[index]->NodeContainRule(rules[k])){
                    child_arr[index]->rules.push_back(rules[k]);
                }
            }
                
            int child_rules_num = child_arr[index]->rules.size(); 
            if (child_rules_num == 0) {
                delete child_arr[index];
                child_arr[index] = NULL;
                continue;
            }

            vis[index] = true;  
            child_arr[index]->rules_num = child_rules_num; 

            max_child_rules_num = max(max_child_rules_num, child_rules_num); 
        }
    }

    if (max_child_rules_num >= rules_num * 0.99) {
        for (int i = 0; i < child_num; ++i){
            if (child_arr[i] != NULL){
                delete child_arr[i];
            }
        }   
        if(child_arr != NULL)   
            delete child_arr; 
        child_arr = NULL; 
        child_num = 0; 

        leaf_node = true; 
        ps->DTI.AddNode(depth, rules_num);
        return;
    }
    
    for (int i = 0; i < child_num; ++i){
        if(vis[i] == true && child_arr[i] == NULL){
            cout<<">>> EffiNode::Create -> Wrong! child_arr error!"<<endl;
            exit(1);
        }
    }

    if (EffiCuts::node_merge) {
        for (int i = 0; i < child_num; ++i){
            if (vis[i] == true){
                for (int j = 0; j < i; ++j) {
                    if (vis[j] == true && SameEffiRules(child_arr[i], child_arr[j])) {
                        vis[i] = false;
                        for (int k = 0; k < 5; ++k) {
                            child_arr[j]->bounds[k][0] = min(child_arr[j]->bounds[k][0], child_arr[i]->bounds[k][0]);
                            child_arr[j]->bounds[k][1] = max(child_arr[j]->bounds[k][1], child_arr[i]->bounds[k][1]);
                        }
                        if (child_arr[i]){
                            delete child_arr[i];
                            child_arr[i] = child_arr[j];
                        }
                        break;
                    }
                }
            }
        }
    }
    
    for (int i = 0; i < child_num; ++i){
        if (vis[i]) {
            child_arr[i]->Create(ps, depth+1); 
        }
    }

    rules.clear();
    ps->DTI.AddNode(depth);
}

uint64_t EffiNode::CalMemory() {
    uint64_t size = sizeof(EffiNode);
    size += rules.capacity() * sizeof(Rule*);
    if (child_arr != nullptr) {
        size += child_num * sizeof(EffiNode*);
        map<EffiNode*, int> child_map;
        for (int i = 0; i < child_num; ++i) {
            if (child_arr[i] != nullptr &&  ++child_map[child_arr[i]] == 1) {
                size += child_arr[i]->CalMemory();
            }
        }
        child_map.clear();
    }
    return size;
}

EffiNode::~EffiNode() {
	map<EffiNode*, int> child_map;
	for (int i = 0; i < child_num; ++i)
		if (child_arr[i] != NULL && ++child_map[child_arr[i]] == 1)
			delete child_arr[i];
    if(child_arr != NULL) delete child_arr;
}

void EffiNode::RemoveRedund() {
    map<Rule, int> rule_redund;
    Rule rule; 
    vector<Rule*> new_rules; 

    for (int i = 0; i < rules_num; ++i) {
        for (int j = 0; j < 5; ++j) {
            rule.range[j][0] = max(rules[i]->range[j][0], bounds[j][0]);
            rule.range[j][1] = min(rules[i]->range[j][1], bounds[j][1]);
        }
        if (++rule_redund[rule] == 1)
            new_rules.push_back(rules[i]);
    }

    rule_redund.clear(); 
    rules = new_rules;
    rules_num = rules.size();
}

void EffiNode::HasBounds() {
    for(int i = 0; i < 5; ++i){
        if(bounds[i][0] > bounds[i][1]){
            cout<<">>> EffiNode::Create -> Wrong! bounds error!"<<endl;
            exit(1);
        }
    }
}

void EffiNode::CalDimToCuts() {
    if (rules_num == 0){
        cout << ">>> EffiNode::CalDimToCuts -> Wrong! rules_num = 0" << endl;
        exit(1);
    }

    int diff_range[5]={0};        
    map<uint64_t, int> range_map;
    for (int d = 0; d < 5; ++d) {
        for (int i = 0; i < rules_num; ++i) {
            uint64_t low = max(rules[i]->range[d][0], bounds[d][0]);
            uint64_t high = min(rules[i]->range[d][1], bounds[d][1]); 
            uint64_t range_hash = (low << 32) | high;

            if (++range_map[range_hash] == 1)
                ++diff_range[d];
        }
        range_map.clear();
    }

    int index1 = -1, index2 = -1; 
    for (int d = 0; d < 5; ++d){
        if (index1 == -1 || diff_range[d] > diff_range[index1]) {
            index2 = index1;
            index1 = d;
        } else if (index2 == -1 || diff_range[d] > diff_range[index2]) {
            index2 = d;
        }
    }

    dims_num = 0;
    if (index1 != -1){
        dims[dims_num] = index1;
        dims_num++;
    }
    if (index2 != -1 ){
        dims[dims_num] = index2;
        dims_num++;
    }

    if(index1 == -1){
        cout<<">>> EffiNode::CalDimToCuts -> Wrong! index1 error!"<<endl;
        exit(1);
    }

    if(index2 == -1){
        cout<<">>> EffiNode::CalDimToCuts -> Wrong! index2 error!"<<endl;
        exit(1);
    }
}

void EffiNode::CalNumCuts() {
    if (rules_num == 0){
        cout << ">>> EffiNode::CalNumCuts -> Wrong! rules_num = 0" << endl;
        exit(1);
    }

    uint64_t spmf = rules_num * EffiCuts::spfac;

    cuts[0] = cuts[1] = 1;     
    uint64_t spcount[2] = {1000000000, 1000000000}; 
    uint64_t range_num[2][2];  

    if (dims_num == 1)
        dims[1] = (dims[0] + 1) % 4;

    while (true) {
        for (int i = 0; i < dims_num; ++i) {
            cuts[i] <<= 1;
            if (cuts[i] >= (uint64_t)bounds[dims[i]][1] - bounds[dims[i]][0] + 1) {
                cuts[i] >>= 1; 
                spcount[i] = 1000000000; 
                continue; 
            }
            for (int j = 0; j < 2; ++j) {
                width[j] = ((uint64_t)bounds[dims[j]][1] - bounds[dims[j]][0] + 1 + cuts[j] - 1) / cuts[j];
                if (EffiCuts::width_power2)
                    width[j] = 1ULL << (uint64_t)floor(log2(width[j]));
            }

            spcount[i] = cuts[0] * cuts[1]; 
            for (int j = 0; j < rules_num; ++j) {
                for (int a = 0; a < 2; a++){
                    for (int b = 0; b < 2; b++) {
                        uint64_t number;
                        if (b == 0) number = max(rules[j]->range[dims[a]][b], bounds[dims[a]][b]);
                        else        number = min(rules[j]->range[dims[a]][b], bounds[dims[a]][b]);
                        range_num[a][b] = (number - bounds[dims[a]][0]) / width[a];
                    }
                }
                spcount[i] += (range_num[0][1] - range_num[0][0] + 1) * (range_num[1][1] - range_num[1][0] + 1);
            }
            cuts[i] >>= 1; 
        }
        if (spcount[0] > spmf && spcount[1] > spmf)
            break;
        if (spcount[1] > spmf || spcount[0] <= spcount[1])
            cuts[0] <<= 1;
        else
            cuts[1] <<= 1;
    }
    
    if(cuts[0] <= 0 || cuts[0] > (uint64_t)bounds[dims[0]][1] - bounds[dims[0]][0] + 1){
        cout<<bounds[dims[0]][0]<<" "<<bounds[dims[0]][1]<<endl;
        cout<<cuts[0]<<" "<<(uint64_t)bounds[dims[0]][1] - bounds[dims[0]][0] + 1<<endl;
        cout<<">>> EffiNode::CalNumCuts -> Wrong! cuts[0] error!"<<endl;
        exit(1);
    }
    if(cuts[1] <= 0 || cuts[1] > (uint64_t)bounds[dims[1]][1] - bounds[dims[1]][0] + 1){
        cout<<">>> EffiNode::CalNumCuts -> Wrong! cuts[1] error!"<<endl;
        exit(1);
    }

    for (int i = 0; i < 2; ++i) {
        width[i] = ((uint64_t)bounds[dims[i]][1] - bounds[dims[i]][0] + 1 + cuts[i] - 1) / cuts[i];
        if (EffiCuts::width_power2) {
            width[i] = 1ULL << (uint64_t)floor(log2(width[i]));
            cuts[i] = ((uint64_t)bounds[dims[i]][1] - bounds[dims[i]][0] + 1) / width[i];
            if(((uint64_t)bounds[dims[i]][1] - bounds[dims[i]][0] + 1) % width[i])
                cuts[i]++;
        }
    }

    if(width[0] == 0 || width[1] == 0){
        cout<<">>> EffiNode::CalNumCuts -> Wrong! width error!"<<endl;
        exit(1);
    }
}

bool EffiNode::NodeContainRule(Rule* rule) {
	for (int i = 0; i < 5; i++)
		if(rule->range[i][1] < bounds[i][0] || bounds[i][1] < rule->range[i][0])
			return false;
	return true;
}

bool SameEffiRules(EffiNode *node1, EffiNode *node2) {
    if(node1 == NULL || node2 == NULL){
        cout<<">>> EffiNode::SameEffiRules -> NULL error!"<<endl;
        exit(1);
    }
	if (node1->rules_num != node2->rules_num)
		return false;
	for (int i = 0; i < node1->rules_num; ++i)
		if (node1->rules[i] != node2->rules[i])
			return false;
	return true;
}
