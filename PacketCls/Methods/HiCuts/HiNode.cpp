#include "HiNode.h"

using namespace std;

HiNode::HiNode(){      
    rules_num = 0;            
    leaf_node = 0;        

    cut = -1;  
    dim = -1;             
    width = -1;    

    child_arr = NULL;  
    child_num = 0; 
    bounds[0][0] = bounds[1][0] =0;
	bounds[2][0] = bounds[3][0] =0;
	bounds[4][0] = 0;         
    bounds[0][1] = bounds[1][1] =0xFFFFFFFF;
	bounds[2][1] = bounds[3][1] =0xFFFF;
	bounds[4][1] = 0xFF;
}

void HiNode::Create(ProgramState *ps, int depth) {
    if (rules_num == 0){
        cout << ">>> HiNode::Create -> Wrong! rules_num = 0!" << endl;
        exit(1);
    }
    HasBounds();

    if (HiCuts::remove_redund && depth != 0) {
        RemoveRedund();
    }
	sort(rules.begin(), rules.end(), CmpRulePriority);

    if (depth == HiCuts::max_depth || rules_num <= HiCuts::leaf_size) {
        leaf_node = true;      
        ps->DTI.AddNode(depth, rules_num);
        return;                      
    }

    CalDimToCuts();
    
    CalNumCuts();
    
    ps->DTI.AddDims(dim, -1);

    child_num = cut; 

    child_arr = new HiNode*[child_num];
    bool *vis = new bool[child_num];              
    int max_child_rules_num = 0;                  
    
    for (int i = 0; i < cut; ++i){
        child_arr[i] = new HiNode(); 
        vis[i] = false; 

        for (int k = 0; k < 5; ++k) {
            if (k == dim) {
                uint64_t low = bounds[k][0] + (uint64_t)i * width;
                uint64_t high = min(low + width - 1, (uint64_t)bounds[k][1]);

                if (low > high) {
                    cout<<">>> HiNode::Create -> Wrong! low > high error!"<<endl;
                    exit(1);
                    break;
                }
                child_arr[i]->bounds[k][0] = low; 
                child_arr[i]->bounds[k][1] = high;
            } else { 
                child_arr[i]->bounds[k][0] = bounds[k][0];
                child_arr[i]->bounds[k][1] = bounds[k][1];
            }
        }

        for (int k = 0; k < rules_num; ++k){
            if (child_arr[i]->NodeContainRule(rules[k])){
                child_arr[i]->rules.push_back(rules[k]);
            }
        }
            
        int child_rules_num = child_arr[i]->rules.size(); 
        if (child_rules_num == 0) {
            delete child_arr[i];
            child_arr[i] = NULL;
            continue;
        }

        vis[i] = true; 
        child_arr[i]->rules_num = child_rules_num; 

        max_child_rules_num = max(max_child_rules_num, child_rules_num); 
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
            cout<<">>> HiNode::Create -> Wrong! child_arr error!"<<endl;
            exit(1);
        }
    }

    if (HiCuts::node_merge) {
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

uint64_t HiNode::CalMemory() {
    uint64_t size = sizeof(HiNode);
	size += rules.capacity() * sizeof(Rule*);
    if (child_arr != nullptr){
        size += child_num * sizeof(HiNode*);
        map<HiNode*, int> child_map;
        for (int i = 0; i < child_num; ++i){
            if (child_arr[i] != NULL && ++child_map[child_arr[i]] == 1){
                size += child_arr[i]->CalMemory();
            }
        }
        child_map.clear();
    }
	return size;
}

HiNode::~HiNode() {
	map<HiNode*, int> child_map;
	for (int i = 0; i < child_num; ++i)
		if (child_arr[i] != NULL && ++child_map[child_arr[i]] == 1)
			delete child_arr[i];
    if(child_arr != NULL) delete child_arr;
}

void HiNode::RemoveRedund() {
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

void HiNode::HasBounds() {
    for(int i = 0; i < 5; ++i){
        if(bounds[i][0] > bounds[i][1]){
            cout<<">>> HiNode::HasBounds -> Wrong! bounds error!"<<endl;
            exit(1);
        }
    }
}

void HiNode::CalDimToCuts() {
    if (rules_num == 0){
        cout << ">>> HiNode::CalDimToCuts -> Wrong! rules_num = 0!" << endl;
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

    int index1 = -1; 
    for (int d = 0; d < 5; ++d){
        if (index1 == -1 || diff_range[d] > diff_range[index1]) {
            index1 = d;
        } 
    }

    if (index1 != -1){
        dim = index1;
    }

    if(index1 == -1){
        cout<<">>> HiNode::CalDimToCuts -> Wrong! index1 error!"<<endl;
        exit(1);
    }
}

void HiNode::CalNumCuts() {
    if (rules_num == 0){
        cout << ">>> HiNode::CalNumCuts -> Wrong! rules_num = 0!" << endl;
        exit(1);
    }

    uint64_t spmf = rules_num * HiCuts::spfac;

    cut = 1;     
    uint64_t spcount = 1000000000; 
    uint64_t range_num[2];  

    while (true) {
        cut <<= 1; 
        if (cut >= (uint64_t)bounds[dim][1] - bounds[dim][0] + 1) {
            cut >>= 1; 
            spcount = 1000000000; 
            break; 
        }
        width = ((uint64_t)bounds[dim][1] - bounds[dim][0] + 1 + cut - 1) / cut;
        if (HiCuts::width_power2)
            width = 1ULL << (uint64_t)floor(log2(width));

        spcount = cut; 
        for (int j = 0; j < rules_num; ++j) {
            for (int b = 0; b < 2; b++) {
                uint64_t number;
                if (b == 0) number = max(rules[j]->range[dim][b], bounds[dim][b]);
                else        number = min(rules[j]->range[dim][b], bounds[dim][b]);
                range_num[b] = (number - bounds[dim][0]) / width;
            }
            spcount += range_num[1] - range_num[0] + 1;
        }
        cut >>= 1; 
        if (spcount > spmf){
            break;
        }
        cut <<= 1;
    }

    if(cut < 0 || cut > (uint64_t)bounds[dim][1] - bounds[dim][0] + 1){
        cout<<">>> HiNode::CalNumCuts -> Wrong! cuts error!"<<endl;
        cout<<bounds[dim][0]<<" "<<bounds[dim][1]<<endl;
        cout<<cut<<" "<<(uint64_t)bounds[dim][1] - bounds[dim][0] + 1<<endl;
        exit(1);
    }

    for (int i = 0; i < 2; ++i) {
        width = ((uint64_t)bounds[dim][1] - bounds[dim][0] + 1 + cut - 1) / cut;
        if (HiCuts::width_power2) {
            width = 1ULL << (uint64_t)floor(log2(width));
            cut = ((uint64_t)bounds[dim][1] - bounds[dim][0] + 1) / width;
            if(((uint64_t)bounds[dim][1] - bounds[dim][0] + 1) % width)
                cut++;
        }
    }

    if(width == 0 ){
        cout<<">>> HiNode::CalNumCuts -> Wrong! width error!"<<endl;
        exit(1);
    }
}

bool HiNode::NodeContainRule(Rule* rule) {
	for (int i = 0; i < 5; i++)
		if(rule->range[i][1] < bounds[i][0] || bounds[i][1] < rule->range[i][0])
			return false;
	return true;
}

bool SameEffiRules(HiNode *node1, HiNode *node2) {
    if(node1 == NULL || node2 == NULL){
        cout<<">>> HiNode::SameEffiRules -> NULL error!"<<endl;
        exit(1);
    }
	if (node1->rules_num != node2->rules_num)
		return false;
	for (int i = 0; i < node1->rules_num; ++i)
		if (node1->rules[i] != node2->rules[i])
			return false;
	return true;
}
