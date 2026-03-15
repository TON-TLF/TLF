#include "TDHsNode.h"
#include "TDHyperSplit.h"

using namespace std;

struct point{
    uint32_t pos;   
	bool flag;      
	point(uint32_t _pos, bool _flag) : pos(_pos), flag(_flag) {}
    bool operator<(const point &tmp) const{
		if(pos == tmp.pos) return flag < tmp.flag;
        return pos < tmp.pos;
    }
	bool operator == (const point &tmp) const {
        return pos == tmp.pos && flag == tmp.flag;
    }
};

struct cut{
    uint32_t pos,height;
};

TDHsNode::TDHsNode(){
    dim = 255;
    range[0][0] = range[0][1] = 0;
	range[1][1] = range[1][1] = 0xFFFFFFFF;
    child[0] = child[1] = NULL;
}

void TDHsNode::Create(vector<Rule*> &rules, ProgramState *ps, uint32_t bounds[][2], int depth) {
    int rules_num = rules.size();
    if (rules_num == 0) {
		printf(">>> TDHsNode::Create -> Wrong! rules_num = 0!\n");
		exit(1);
	}
    
	int traces_sum = 0;
	double pop = 1;
	if(traceFreqMat2d::mat.nonZeros() != 0){
		traces_sum = traceFreqMat2d::getTracenum2D(bounds);
		pop = (TDHyperSplit::total_trace_num * 1.0 / (1 << (depth - 1)));
		if(pop != 0) pop = traces_sum / pop;
		pop = max(min(pop, TDHyperSplit::pop_max), TDHyperSplit::pop_min);	
	}

	if (rules_num * pop <= TDHyperSplit::binth) {
		this->rules = rules;
        ps->DTI.AddNode(depth, rules_num);
		return;
	}

    CalHowToCut(rules, traces_sum, bounds);

	if (dim == 255) {
		this->rules = rules;
        ps->DTI.AddNode(depth, rules_num);
		return;
	}

    ps->DTI.AddDims(dim, -1);

	for (int k = 0; k < 2; ++k) {
		vector<Rule*> child_rules;
		map<Rule, int> rule_map;

		for (int i = 0; i < rules_num; ++i){
			if ((k == 0 && rules[i]->range[dim][0] <= range[0][1]) ||
				(k == 1 && rules[i]->range[dim][1] >= range[1][0])) {
				Rule* rule_temp = new Rule;
                rule_temp->SetRule(rules[i]);
				
				rule_temp->range[dim][0] = max(rule_temp->range[dim][0], range[k][0]);
				rule_temp->range[dim][1] = min(rule_temp->range[dim][1], range[k][1]);

				if (!TDHyperSplit::remove_redund)
					child_rules.push_back(rule_temp);
				else if (++rule_map[*rule_temp] <= 1) {
					child_rules.push_back(rule_temp);
				}	
			}
		}
		if(dim == 0 || dim == 1){
			swap(range[k][0],bounds[dim][0]);
			swap(range[k][1],bounds[dim][1]);
		}
		child[k] = new TDHsNode;
		child[k]->Create(child_rules, ps, bounds, depth + 1);
		if(dim == 0 || dim == 1){
			swap(range[k][0],bounds[dim][0]);
			swap(range[k][1],bounds[dim][1]);
		}
	}
	ps->DTI.AddNode(depth);
}

void TDHsNode::CalHowToCut(vector<Rule*> &rules, int traces_sum, uint32_t bounds[][2]){
    int rules_num = rules.size();   
    uint64_t height_sum[5];         
	double height_avgs[5];           
	vector<cut> cuts[5];		    
    vector<uint32_t> trace_seg[5]; 
	double uniformity[5];          

	for (int d = 0; d < 5; ++d) {
		height_sum[d] = 0;
		height_avgs[d] = -1;
		uniformity[d] = -1;

		vector<point> points;
		for (int i = 0; i < rules_num; ++i){
			points.push_back({rules[i]->range[d][0],0});
			points.push_back({rules[i]->range[d][1],1});
        }
		sort(points.begin(), points.end());

        int i = 0;
		uint32_t cnt = 0;
		int points_num = points.size();
        while(i < points_num){
            uint32_t pos = points[i].pos;  
			int lnum = 0;                 
            int rnum = 0;                
            
			while(i < points_num && points[i].pos == pos){
				if(points[i].flag == 0){
					++lnum;
				} else {
					++rnum;
				}
				++i;
			}

			cnt += lnum - rnum;
			height_sum[d] += cnt;          
            cuts[d].push_back((cut){pos,cnt});
        }

        int cuts_num = cuts[d].size(); 
		if (cuts_num < 3) continue; 
		
		height_avgs[d] = 1.0 * height_sum[d] / cuts_num;

		if(traces_sum == 0 || d >= 2) continue;

		i = 0;
		double traces_uniformity = 0;
		uint32_t seglow = 0, seghigh = 0;
        while(i < cuts_num ){
			seglow = cuts[d][i].pos;
            seghigh = cuts[d][min(i+1, cuts_num-1)].pos-1;
            
            swap(seglow,bounds[d][0]);
            swap(seghigh,bounds[d][1]);
            double segTraceNum = traceFreqMat2d::getTracenum2D(bounds);
            swap(seglow,bounds[d][0]);
            swap(seghigh,bounds[d][1]);
            
            trace_seg[d].push_back(segTraceNum);
			double percentage = segTraceNum / traces_sum;
			traces_uniformity += abs(percentage - (1.0 / cuts_num));
			i++;
        }

		if(cuts_num != trace_seg[d].size()){
			cout<<">>> TDHsNode::CalHowToCut -> cuts_num error!"<<endl;
			cout<<cuts_num<<" "<<trace_seg[d].size()<<endl;
			exit(1);
		}

		uniformity[d] = traces_uniformity;
	}
	
	double height_avgs_sum = 0;
    double uniformity_sum = 0;
	for (int d = 0; d < 5; ++d){
		if(height_avgs[d] != -1) height_avgs_sum += height_avgs[d];
        if(uniformity[d] != -1) uniformity_sum += uniformity[d];
	}

	double score = 1e9;       
	for (int d = 0; d < 5; ++d){
		if(height_avgs_sum == 0){
			break;
		}

        if(height_avgs[d] == -1){
            continue;
        }
		
        double tmp = 0;
		if(uniformity_sum != 0 && uniformity[d] != -1){
			tmp += TDHyperSplit::pfac * (height_avgs[d] / height_avgs_sum);
			tmp += (1.0 - TDHyperSplit::pfac) * (uniformity[d] / uniformity_sum);
		} else {
			tmp = height_avgs[d] / height_avgs_sum;
		}
		if(tmp < score){
			dim = d;
			score = tmp;
		}
	}

	if(dim == 255) return;

	uint32_t thresh = 0;
	int cutsdim_num = cuts[dim].size();
	if(traces_sum == 0 || uniformity[dim] == -1 || dim >= 2){
		uint64_t count = 0;
		for (int i = 0; i < cutsdim_num - 1; ++i) {
			count += cuts[dim][i].height;
			if (count * 2 >= height_sum[dim] || i == cutsdim_num - 3) { 
				thresh = cuts[dim][i + 1].pos;
				break;
			}
		}
	} else {
		double p=0,q=0,Min=1e9;
		int threshw = 0, threshp = 0;
		
		uint64_t count = 0;
		for (int i = 0; i < cutsdim_num ; ++i) {
			count += cuts[dim][i].height;
			if (count * 2 >= height_sum[dim] || i == cutsdim_num - 3) { 
				threshw = i;
				break;
			}
		}

		count = 0;
		for (int i = 0; i < cutsdim_num ; ++i) {
			count += trace_seg[dim][i];
			if (count * 2 >= traces_sum || i == cutsdim_num - 3) { 
				threshp = i;
				break;
			}
		}

		int L_bound = threshw * 0.15 + threshp * 0.85 + 0.1;
		int R_bound = threshw * 0.05 + threshp * 0.95 + 0.1;
		if(L_bound > R_bound) swap(L_bound, R_bound);

		if(L_bound < 0 || R_bound >= cutsdim_num || L_bound > R_bound){
			cout<<">>> TDHsNode::CalHowToCut -> LR Bound error!"<<endl;
			cout<<L_bound<<" "<<R_bound<<" "<<cutsdim_num<<endl;
			exit(1);
		}
		
		p = q = 0;
		for (int i = 0; i <= R_bound ; ++i) {
			p += (double)cuts[dim][i].height / height_sum[dim] ;
			q += (double)trace_seg[dim][i] / traces_sum;
			double tmp = abs(p - 0.5) + abs(q - 0.5);

			if (tmp <= Min && L_bound <= i && i <= R_bound) { 
				thresh = cuts[dim][i + 1].pos;
				Min = tmp;
			}

			if(i == cutsdim_num - 3){
				thresh = cuts[dim][i + 1].pos;
				break;
			}
		}
	}
	
	range[0][0] = cuts[dim][0].pos;
	range[0][1] = thresh - 1;
	range[1][0] = thresh;
	range[1][1] = cuts[dim][cutsdim_num - 1].pos;

	if(!(range[0][0] <= range[0][1] && range[0][1] <= range[1][0] && range[1][0] <= range[1][1])){
		cout <<">>> TDHsNode::CalHowToCut ->  range error! "<<endl;
		cout <<"dim = "<<(int)dim << "  thresh = "<< thresh << endl;
		cout <<range[0][0]<<" "<<range[0][1]<<" "<<range[1][0]<<" "<<range[1][1]<<endl;
		exit(1);
	}
}

uint64_t TDHsNode::CalMemory() {
    uint64_t size = 0;
	size += sizeof(dim);      
	size += sizeof(range);    
	size += sizeof(child);   

    size += sizeof(rules);                     
    size += rules.capacity() * sizeof(Rule*);  

	for (int i = 0; i < 2; ++i) {
		if (child[i] != nullptr) {
			size += child[i]->CalMemory();
		}
	}

    return size;
}

TDHsNode::~TDHsNode() {
    if (child[0] != NULL) delete child[0];
    if (child[1] != NULL) delete child[1];
}