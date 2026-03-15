#include "HsNode.h"
#include "HyperSplit.h"

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

HsNode::HsNode(){
    dim = 255;
    range[0][0] = range[0][0] = range[0][0] = range[0][0] = -1;
    child[0] = child[1] = NULL;
}

void HsNode::Create(vector<Rule*> &rules, ProgramState *ps, int depth) {
    int rules_num = rules.size();
    if (rules_num == 0) {
		printf(">>> HsNode::Create -> Wrong! rules_num = 0!\n");
		exit(1);
	}
    
	if (rules_num <= HyperSplit::binth) {
		this->rules = rules;
        ps->DTI.AddNode(depth, rules_num);
		return;
	}

    CalHowToCut(rules);

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

				if (!HyperSplit::remove_redund)
					child_rules.push_back(rule_temp);
				else if (++rule_map[*rule_temp] <= 1) {
					child_rules.push_back(rule_temp);
				}
			}
		}
		
		child[k] = new HsNode;
		child[k]->Create(child_rules, ps, depth + 1);
	}
	ps->DTI.AddNode(depth);
}

void HsNode::CalHowToCut(vector<Rule*> &rules){
	int rules_num = rules.size();
    double height_avg = 1 << 30;  

	for (int d = 0; d < 5; ++d) {
		vector<point> points;
		for (int i = 0; i < rules_num; ++i){
            points.push_back({rules[i]->range[d][0],0});
			points.push_back({rules[i]->range[d][1],1});
        }
		sort(points.begin(), points.end());

        int i = 0;
		uint32_t cnt = 0;
		uint64_t height_sum = 0;
		int points_num = points.size();
		vector<cut> cuts;
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
			height_sum += cnt;             
            cuts.push_back((cut){pos,cnt});
        }

		int cuts_num = cuts.size(); 
		if (cuts_num >= 3 && height_avg > 1.0 * height_sum / cuts_num) {
			height_avg = 1.0 * height_sum / cuts_num;
			dim = d;
			uint32_t thresh = 0;
			uint64_t count = 0;
			for (int j = 0; j < cuts_num - 1; ++j) {
				count += cuts[j].height;
				if (count * 2 >= height_sum || j == cuts_num - 3) { 
					thresh = cuts[j + 1].pos;
					break;
				}
			}
			
			range[0][0] = cuts[0].pos;
			range[0][1] = thresh - 1;
			range[1][0] = thresh;
			range[1][1] = cuts[cuts_num - 1].pos;

            if(range[0][0] > range[0][1] || range[0][1] > range[1][0] || range[1][0] > range[1][1]){
                cout << ">>> HsNode::Create -> Wrong! range error! "<<endl;
                cout << "left  : " << range[0][0] << " " << range[0][1] << endl;
                cout << "right : " << range[1][0] << " " << range[1][1] << endl;
                exit(1);
            }
		}
	}
}

uint64_t HsNode::CalMemory() {
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

HsNode::~HsNode() {
    if (child[0] != nullptr) delete child[0];
    if (child[1] != nullptr) delete child[1];
}