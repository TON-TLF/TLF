#include "HsNode.h"
#include "HyperSplit.h"

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

/**
 * @brief HsNode 默认构造函数
 */
HsNode::HsNode(){
    dim = 255;
    range[0][0] = range[0][0] = range[0][0] = range[0][0] = -1;
    child[0] = child[1] = NULL;
}

/**
 * @brief 创建HyperSplit节点并构建决策树。
 * 
 * 该函数通过递归方式构建HyperSplit树，每个节点根据规则集选择最佳分割维度和阈值，分割后将规则分配到子节点。
 * 
 * @param rules 包含所有规则的RangeRule向量。
 * @param ps 程序状态指针。
 * @param depth 当前节点的深度。
 */
void HsNode::Create(vector<Rule*> &rules, ProgramState *ps, int depth) {
    /** 进行必要的检查 */
    int rules_num = rules.size();
    if (rules_num == 0) {
		printf(">>> HsNode::Create -> Wrong! rules_num = 0!\n");
		exit(1);
	}
    
	/** 剩余规则数量小于等于阈值，直接建立子节点 */ 
	if (rules_num <= HyperSplit::binth) {
		this->rules = rules;
        ps->DTI.AddNode(depth, rules_num);
		return;
	}

    CalHowToCut(rules);

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
    double height_avg = 1 << 30; /** 用于选择分割维度的平均高度初始值 */ 

	/** 遍历每个维度，寻找最佳分割维度 */
	for (int d = 0; d < 5; ++d) {
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
		uint64_t height_sum = 0;
		int points_num = points.size();
		vector<cut> cuts;
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
			height_sum += cnt;             /** cnt  表示当前有多少规则左端点 <  pos,并且没有有端点与其匹配 */
            cuts.push_back((cut){pos,cnt});
        }

		/** 待选的分割点的数量 */ 
		int cuts_num = cuts.size(); 
        /** 如果该维度的平均高度小于当前最小值，并且分割点数量 >= 3,则更新最优分割维度和阈值 */
		if (cuts_num >= 3 && height_avg > 1.0 * height_sum / cuts_num) {
			height_avg = 1.0 * height_sum / cuts_num;
			dim = d;
			uint32_t thresh = 0;
			uint64_t count = 0;
			/** 选择合适的分割点（不会选择第0个和最后一个）作为阈值 */ 
			for (int j = 0; j < cuts_num - 1; ++j) {
				count += cuts[j].height;
				if (count * 2 >= height_sum || j == cuts_num - 3) { 
					thresh = cuts[j + 1].pos;
					break;
				}
			}
			
			/** 确定左右子节点的范围 */ 
			range[0][0] = cuts[0].pos;
			range[0][1] = thresh - 1;
			range[1][0] = thresh;
			range[1][1] = cuts[cuts_num - 1].pos;

            /** 检查分割是否合理 */
            if(range[0][0] > range[0][1] || range[0][1] > range[1][0] || range[1][0] > range[1][1]){
                cout << ">>> HsNode::Create -> Wrong! range error! "<<endl;
                cout << "left  : " << range[0][0] << " " << range[0][1] << endl;
                cout << "right : " << range[1][0] << " " << range[1][1] << endl;
                exit(1);
            }
		}
	}
}

/**
 * @brief 计算节点使用的内存大小。
 * @return 内存大小（字节）。
 */
uint64_t HsNode::CalMemory() {
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

/**
 * @brief 释放节点内存。
 */
HsNode::~HsNode() {
    if (child[0] != nullptr) delete child[0];
    if (child[1] != nullptr) delete child[1];
}