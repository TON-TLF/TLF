#include "TDHiNode.h"

using namespace std;

TDHiNode::TDHiNode(){      
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

void TDHiNode::Create(ProgramState *ps, int depth) {
    if (rules_num == 0){
        cout << ">>> TDHiNode::Create -> Wrong! rules_num = 0!" << endl;
        exit(1);
    }
    HasBounds();

    if (TDHiCuts::remove_redund && depth != 0) {
        RemoveRedund();
    }
	sort(rules.begin(), rules.end(), CmpRulePriority);

	static double scopes[5] = {(double)0xFFFFFFFFll, (double)0xFFFFFFFFll, 0xFFFF, 0xFFFF, 0xFF};
	
	double pop = TDHiCuts::total_trace_num;
    double traces_sum = 0;
    if(TDHiCuts::total_trace_num != 0){
        for(int i = 0; i < 2; i++){
            pop *= (double)(this->bounds[i][1] - this->bounds[i][0] + 1) / scopes[i];
        }
        traces_sum = traceFreqMat2d::getTracenum2D(bounds);
        if(pop != 0){
            pop = traces_sum / pop;
        }
    }
	
    int _max_depth = TDHiCuts::max_depth, _leaf_size = TDHiCuts::leaf_size;
    if(TDHiCuts::total_trace_num != 0){
        if      (pop > TDHiCuts::pop_add_2) _max_depth += 2;
        else if (pop > TDHiCuts::pop_add_1) _max_depth += 1;
        else if (pop < TDHiCuts::pop_dec_1) _max_depth -= 1;
        else if (pop < TDHiCuts::pop_dec_2) _max_depth -= 2;
        _leaf_size /= max(min(pop, TDHiCuts::pop_max), TDHiCuts::pop_min);
    }
    
    if (depth >= _max_depth || rules_num <= _leaf_size) {
        leaf_node = true;      
        ps->DTI.AddNode(depth, rules_num);
        return;                      
    }

    CalDimToCuts(traces_sum);
    CalNumCuts(pop);
    
    ps->DTI.AddDims(dim, -1);
    
    child_num = cut;

    child_arr = new TDHiNode*[child_num];
    bool *vis = new bool[child_num];               
    int max_child_rules_num = 0;                   
    
    for (int i = 0; i < cut; ++i){
        child_arr[i] = new TDHiNode(); 
        vis[i] = false; 

        for (int k = 0; k < 5; ++k) {
            if (k == dim) {
                uint64_t low = bounds[k][0] + (uint64_t)i * width;
                uint64_t high = min(low + width - 1, (uint64_t)bounds[k][1]);

                if (low > high) {
                    cout<<">>> TDHiNode::Create -> Wrong! low > high error!"<<endl;
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
            cout<<">>> TDHiNode::Create -> Wrong! child_arr NULL error!"<<endl;
            exit(1);
        }
    }

    if (TDHiCuts::node_merge) {
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

uint64_t TDHiNode::CalMemory() {
    uint64_t size = sizeof(TDHiNode);
	size += rules.capacity() * sizeof(Rule*);
    if (child_arr != nullptr){
        size += child_num * sizeof(TDHiNode*);
        map<TDHiNode*, int> child_map;
        for (int i = 0; i < child_num; ++i){
            if (child_arr[i] != NULL && ++child_map[child_arr[i]] == 1){
                size += child_arr[i]->CalMemory();
            }
        }
        child_map.clear();
    }
	return size;
}

TDHiNode::~TDHiNode() {
	map<TDHiNode*, int> child_map;
	for (int i = 0; i < child_num; ++i)
		if (child_arr[i] != NULL && ++child_map[child_arr[i]] == 1)
			delete child_arr[i];
    if(child_arr != NULL) delete child_arr;
}

void TDHiNode::RemoveRedund() {
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

void TDHiNode::HasBounds() {
    for(int i = 0; i < 5; ++i){
        if(bounds[i][0] > bounds[i][1]){
            cout<<">>> TDHiNode::HasBounds -> Wrong! bounds error!"<<endl;
            exit(1);
        }
    }
}

void TDHiNode::CalDimToCuts(int traces_num) {
    if (rules_num == 0){
        cout << ">>> TDHiNode::CalDimToCuts -> Wrong! rules_num = 0!" << endl;
        exit(1);
    }

    double diff_range[5]={0};         
    double uni_sum = 0;
    double uniformity[5]={0};
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

        uniformity[d] = -1;
        uint32_t low = bounds[d][0];
		uint32_t high = bounds[d][1];
		uint32_t range = high - low + 1;
		uint32_t maxpo2 = (long long)range&(-(long long)(range));
		
		if(maxpo2 == 0 || range < 8 || traces_num == 0 || d >= 2){
			uniformity[d] = -1;
			continue;
		}

		uint32_t segcnt = min(max((uint32_t)256, range / maxpo2 + range % maxpo2), range);
		uint32_t seglen = range / segcnt;
		double avg_cnt = 1.0 / segcnt;
		double var = 0;

        for(int i = 0; i < segcnt; i++)
		{
			uint32_t seglow = low + i * seglen;
			uint32_t seghigh = min(seglow + seglen - 1, high);
            swap(seglow, bounds[d][0]);
            swap(seghigh, bounds[d][1]);
			double cnt = traceFreqMat2d::getTracenum2D(bounds);
            swap(seglow, bounds[d][0]);
            swap(seghigh, bounds[d][1]);
			var += abs((double)cnt / traces_num - avg_cnt);
		}
		uniformity[d] = 2 - var;
		uni_sum += uniformity[d];
    }

    double diff_sum = 0; 
    for (int d = 0; d < 5; ++d){
        diff_sum += diff_range[d];            
    }

    int index = -1; 
    double score[5];
	for(int d = 0; d < 5; ++d) {
        score[d] = 0;
        if(uni_sum == 0 || uniformity[d] == -1){
            score[d] = diff_range[d] / diff_sum;
        } else {
            score[d] += TDHiCuts::wfac * (diff_range[d] / diff_sum);
            score[d] += (1 - TDHiCuts::wfac) * (uniformity[d] / uni_sum );
        }

        if (index == -1 || score[d] > score[index]) {
            index = d;
        } 
	}

    if (index != -1){
        dim = index;
    }

    if(index == -1){
        cout<<">>> TDHiNode::CalDimToCuts -> Wrong! index error!"<<endl;
        exit(1);
    }
}

void TDHiNode::CalNumCuts(double pop) {
    if (rules_num == 0){
        cout << ">>> TDHiNode::CalNumCuts -> Wrong! rules_num = 0" << endl;
        exit(1);
    }

    double _spfac = TDHiCuts::spfac;
    if(TDHiCuts::total_trace_num != 0){
        if      (pop > TDHiCuts::pop_mul_2)   _spfac *= 1.5;
        else if (pop > TDHiCuts::pop_mul_1)   _spfac *= 1.25;
        else if (pop < TDHiCuts::pop_div_1)   _spfac /= 1.25;
        else if (pop < TDHiCuts::pop_div_2)   _spfac /= 1.5;
    }
    uint64_t spmf = rules_num * _spfac;

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
        if (TDHiCuts::width_power2)
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
        cout<<">>> TDHiNode::CalNumCuts -> Wrong! cuts error!"<<endl;
        cout<<bounds[dim][0]<<" "<<bounds[dim][1]<<endl;
        cout<<cut<<" "<<(uint64_t)bounds[dim][1] - bounds[dim][0] + 1<<endl;
        exit(1);
    }

    for (int i = 0; i < 2; ++i) {
        width = ((uint64_t)bounds[dim][1] - bounds[dim][0] + 1 + cut - 1) / cut;
        if (TDHiCuts::width_power2) {
            width = 1ULL << (uint64_t)floor(log2(width));
            cut = ((uint64_t)bounds[dim][1] - bounds[dim][0] + 1) / width;
            if(((uint64_t)bounds[dim][1] - bounds[dim][0] + 1) % width)
                cut++;
        }
    }

    if(width == 0 ){
        cout<<">>> TDHiNode::CalNumCuts -> Wrong! width error!"<<endl;
        exit(1);
    }
}

bool TDHiNode::NodeContainRule(Rule* rule) {
	for (int i = 0; i < 5; i++)
		if(rule->range[i][1] < bounds[i][0] || bounds[i][1] < rule->range[i][0])
			return false;
	return true;
}

bool SameEffiRules(TDHiNode *node1, TDHiNode *node2) {
    if(node1 == NULL || node2 == NULL){
        cout<<">>> TDHiNode::SameEffiRules -> Wrong! NULL error!"<<endl;
        exit(1);
    }
	if (node1->rules_num != node2->rules_num)
		return false;
	for (int i = 0; i < node1->rules_num; ++i)
		if (node1->rules[i] != node2->rules[i])
			return false;
	return true;
}
