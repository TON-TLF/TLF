#include "TDHsNode.h"
#include "TDHyperSplit.h"

using namespace std;

/**
 * @brief 端点结构体，表示一个区间的起点或终点
 */
struct point{
    uint32_t pos;   /** 坐标 */
	bool flag;      /** 0为起点，1为终点 */
	point(uint32_t _pos, bool _flag) : pos(_pos), flag(_flag) {}
    bool operator<(const point &tmp) const{
		if(pos == tmp.pos) return flag < tmp.flag;
        return pos < tmp.pos;
    }
	bool operator == (const point &tmp) const {
        return pos == tmp.pos && flag == tmp.flag;
    }
};

/**
 * @brief 高度结构体，表示每个切割点的高度。
 */
struct cut{
    uint32_t pos,height;
};

TDHsNode::TDHsNode(){
    dim = 255;
    range[0][0] = range[0][1] = 0;
	range[1][1] = range[1][1] = 0xFFFFFFFF;
    child[0] = child[1] = NULL;
}

/**
 * @brief 创建HyperSplitMat2D节点并构建决策树。
 * 
 * 该函数通过递归方式构建HyperSplitMat2D树，每个节点根据规则集选择最佳分割维度和阈值，分割后将规则分配到子节点。
 * 
 * @param rules 包含所有规则的RangeRule向量。
 * @param ps 程序状态指针。
 * @param depth 当前节点的深度。
 */
void TDHsNode::Create(vector<Rule*> &rules, ProgramState *ps, uint32_t bounds[][2], int depth) {
    /** 进行必要的检查 */
    int rules_num = rules.size();
    if (rules_num == 0) {
		printf(">>> TDHsNode::Create -> Wrong! rules_num = 0!\n");
		exit(1);
	}
    
	/** 计算节点流行度 */ 
	int traces_sum = 0;
	double pop = 1;
	if(traceFreqMat2d::mat.nonZeros() != 0){
		traces_sum = traceFreqMat2d::getTracenum2D(bounds);
		pop = (TDHyperSplit::total_trace_num * 1.0 / (1 << (depth - 1)));
		if(pop != 0) pop = traces_sum / pop;
		pop = max(min(pop, TDHyperSplit::pop_max), TDHyperSplit::pop_min);	
	}

	/** 剩余规则数量小于等于阈值，直接建立子节点 */ 
	if (rules_num * pop <= TDHyperSplit::binth) {
		this->rules = rules;
        ps->DTI.AddNode(depth, rules_num);
		return;
	}

    CalHowToCut(rules, traces_sum, bounds);

	/** 如果没有成功选择一个维度，表明这些规则已经无法再次切割了，直接建立叶子节点 */
	if (dim == 255) {
		this->rules = rules;
        ps->DTI.AddNode(depth, rules_num);
		return;
	}

    ps->DTI.AddDims(dim, -1);

	/** 为左右子节点创建规则集并递归调用Create函数 */ 
	for (int k = 0; k < 2; ++k) {
		vector<Rule*> child_rules;
		map<Rule, int> rule_map;

		/** 根据分割维度和阈值，将规则分配给相应的子节点 */ 
		for (int i = 0; i < rules_num; ++i){
			if ((k == 0 && rules[i]->range[dim][0] <= range[0][1]) ||
				(k == 1 && rules[i]->range[dim][1] >= range[1][0])) {
				Rule* rule_temp = new Rule;
                rule_temp->SetRule(rules[i]);
				
				/** 调整规则范围以匹配子节点的区间 */ 
				rule_temp->range[dim][0] = max(rule_temp->range[dim][0], range[k][0]);
				rule_temp->range[dim][1] = min(rule_temp->range[dim][1], range[k][1]);

				/** 如果不移除冗余规则，直接加入子规则集，否则检测冗余后加入 */ 
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
    uint64_t height_sum[5];         /** 用于存储选择分割维度的总高度 */
	double height_avgs[5];          /** 用于存储选择分割维度的平均高度 */ 
	vector<cut> cuts[5];		    /** 用于存储每个维度的每个区间的高度值和区间切割点 */
    vector<uint32_t> trace_seg[5];  /** 用于存储每个维度的每个区间的流量频数值 */
	double uniformity[5];           /** 用于存储选择分割维度的分布均匀性 */ 

	/** 遍历每个维度，寻找最佳分割维度 */
	for (int d = 0; d < 5; ++d) {
		height_sum[d] = 0;
		height_avgs[d] = -1;
		uniformity[d] = -1;

        /** 收集所有规则在当前维度范围区间的端点 */
		vector<point> points;
		for (int i = 0; i < rules_num; ++i){
			points.push_back({rules[i]->range[d][0],0});
			points.push_back({rules[i]->range[d][1],1});
        }
		sort(points.begin(), points.end());

		/** 计算去重后的每个分割点的高度 */
        int i = 0;
		uint32_t cnt = 0;
		int points_num = points.size();
        while(i < points_num){
            uint32_t pos = points[i].pos;  /** pos  表示当前分割点，其将区间划分为[0,pos-1],与[pos,...]*/
			int lnum = 0;                  /** lnum 表示当前有多少规则左端点 <  pos */
            int rnum = 0;                  /** rnum 表示当前有多少规则右端点 >= pos */
            
			while(i < points_num && points[i].pos == pos){
				if(points[i].flag == 0){
					++lnum;
				} else {
					++rnum;
				}
				++i;
			}

			cnt += lnum - rnum;
			height_sum[d] += cnt;          /** cnt  表示当前有多少规则左端点 <  pos,并且没有有端点与其匹配 */
            cuts[d].push_back((cut){pos,cnt});
        }

        /** 待选的分割点的数量 */
        int cuts_num = cuts[d].size(); 
		if (cuts_num < 3) continue; 
		
        /** 计算该维度的平均高度 */ 
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

		/** 记录该维度均匀性，越小表示越均匀 */
		uniformity[d] = traces_uniformity;
	}
	
	/** 计算高度和流行度的总值 */
	double height_avgs_sum = 0;
    double uniformity_sum = 0;
	for (int d = 0; d < 5; ++d){
		if(height_avgs[d] != -1) height_avgs_sum += height_avgs[d];
        if(uniformity[d] != -1) uniformity_sum += uniformity[d];
	}

	/** 选择最佳切割维度 */
	double score = 1e9;        /** 初始维度得分 */
	for (int d = 0; d < 5; ++d){
        /** 如果高度之和为0,表示没有维度可以在分割 */
		if(height_avgs_sum == 0){
			break;
		}

        /** 如果该维度平均高度为0,表示该维度不可以在分割 */
        if(height_avgs[d] == -1){
            continue;
        }
		
        /** 计算当前维度得分，如果均匀性之和为0表示当前无高频流量到访此节点 */
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
		/** 选择合适的分割点（不会选择第0个和最后一个）作为阈值 */ 
		for (int i = 0; i < cutsdim_num - 1; ++i) {
			count += cuts[dim][i].height;
			if (count * 2 >= height_sum[dim] || i == cutsdim_num - 3) { 
				thresh = cuts[dim][i + 1].pos;
				break;
			}
		}
	} else {
		/** 选择合适的分割点作为阈值 */ 
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

/**
 * @brief 计算节点使用的内存大小。
 * @return 内存大小（字节）。
 */
uint64_t TDHsNode::CalMemory() {
    uint64_t size = 0;
	size += sizeof(dim);      
	size += sizeof(range);    
	size += sizeof(child);   

    size += sizeof(rules);                     /** vector本身的大小 */ 
    size += rules.capacity() * sizeof(Rule*);  /** vector中指针所占的实际内存 */ 

	for (int i = 0; i < 2; ++i) {
		if (child[i] != nullptr) {
			size += child[i]->CalMemory();  // 递归计算子节点的内存大小
		}
	}

    return size;
}

TDHsNode::~TDHsNode() {
    if (child[0] != NULL) delete child[0];
    if (child[1] != NULL) delete child[1];
}