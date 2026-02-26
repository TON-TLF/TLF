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

/**
 * @brief 根据给定规则创建HiCuts树
 * @param rules 规则指针的向量
 * @param depth 当前深度
 */
void HiNode::Create(ProgramState *ps, int depth) {
    /** 对规则数量进行必要的检查 */
    if (rules_num == 0){
        cout << ">>> HiNode::Create -> Wrong! rules_num = 0!" << endl;
        exit(1);
    }
    HasBounds();

    /** 如果启用了冗余移除且节点深度不为0，移除冗余规则 */ 
    if (HiCuts::remove_redund && depth != 0) {
        RemoveRedund();
    }
	sort(rules.begin(), rules.end(), CmpRulePriority);

    /** 检查是否达到最大深度、规则数量小于等于叶节点阈值或节点无边界 */ 
    if (depth == HiCuts::max_depth || rules_num <= HiCuts::leaf_size) {
        leaf_node = true;      
        ps->DTI.AddNode(depth, rules_num);
        return;                      
    }

    /** 计算节点的切割维度 */ 
    CalDimToCuts();
    
    /** 计算切割数量 */ 
    CalNumCuts();
    
    ps->DTI.AddDims(dim, -1);

    child_num = cut; /** 计算子节点数量 */ 

    child_arr = new HiNode*[child_num];
    bool *vis = new bool[child_num];               /** 用于标记子节点是否已被使用 */
    int max_child_rules_num = 0;                   /** 记录子节点中的最大规则数量 */ 
    
    /** 遍历每个子节点 */ 
    for (int i = 0; i < cut; ++i){
        child_arr[i] = new HiNode(); 
        vis[i] = false; 

        for (int k = 0; k < 5; ++k) {
            /** 如果是切割维度，计算子节点的边界 */ 
            if (k == dim) {
                uint64_t low = bounds[k][0] + (uint64_t)i * width;
                uint64_t high = min(low + width - 1, (uint64_t)bounds[k][1]);

                /** 如果低位大于高位，释放子节点并退出循环 */ 
                if (low > high) {
                    cout<<">>> HiNode::Create -> Wrong! low > high error!"<<endl;
                    exit(1);
                    break;
                }
                child_arr[i]->bounds[k][0] = low; 
                child_arr[i]->bounds[k][1] = high;
            } else { 
                /** 非切割维度，直接继承父节点的边界 */ 
                child_arr[i]->bounds[k][0] = bounds[k][0];
                child_arr[i]->bounds[k][1] = bounds[k][1];
            }
        }

        /** 将符合子节点边界的规则添加到子节点的规则集合中 */ 
        for (int k = 0; k < rules_num; ++k){
            if (child_arr[i]->NodeContainRule(rules[k])){
                child_arr[i]->rules.push_back(rules[k]);
            }
        }
            
        /** 如果子节点没有规则，释放子节点并继续 */
        int child_rules_num = child_arr[i]->rules.size(); 
        if (child_rules_num == 0) {
            delete child_arr[i];
            child_arr[i] = NULL;
            continue;
        }

        vis[i] = true;  /** 标记子节点已被使用 */ 
        child_arr[i]->rules_num = child_rules_num; 

        max_child_rules_num = max(max_child_rules_num, child_rules_num); 
    }

    /** 如果子节点中的规则数量接近总规则数量，则停止进一步分割 */ 
    if (max_child_rules_num >= rules_num * 0.99) {
        //cout<<max_child_rules_num<<" "<<rules_num<<endl;
        /** 释放所有子节点 */ 
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

    /** 如果启用了节点合并，尝试合并相同规则的子节点 */ 
    if (HiCuts::node_merge) {
        for (int i = 0; i < child_num; ++i){
            if (vis[i] == true){
                for (int j = 0; j < i; ++j) {
                    /** 如果两个子节点具有相同的规则，合并边界并释放其中一个节点 */ 
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
    
    /** 递归构建每个有效的子节点 */ 
    for (int i = 0; i < child_num; ++i){
        if (vis[i]) {
            child_arr[i]->Create(ps, depth+1); 
        }
    }
    rules.clear();
    ps->DTI.AddNode(depth);
}

/**
 * @brief 计算节点使用的内存大小。
 * @return 内存大小（字节）。
 */
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

/**
 * @brief 移除冗余规则，保留在节点边界内的唯一规则
 * @param rules 规则指针的引用
 */
void HiNode::RemoveRedund() {
    map<Rule, int> rule_redund;

    /** 用于存储当前规则的副本 */ 
    Rule rule; 

    /** 用于存储去除冗余后的规则 */
    vector<Rule*> new_rules; 

    /** 遍历所有规则，计算规则与节点边界的交集范围 */ 
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

/**
 * @brief 检查节点是否具有有效的边界范围
 */
void HiNode::HasBounds() {
    for(int i = 0; i < 5; ++i){
        if(bounds[i][0] > bounds[i][1]){
            cout<<">>> HiNode::HasBounds -> Wrong! bounds error!"<<endl;
            exit(1);
        }
    }
}

/**
 * @brief 计算节点的切割维度
 */
void HiNode::CalDimToCuts() {
    if (rules_num == 0){
        cout << ">>> HiNode::CalDimToCuts -> Wrong! rules_num = 0!" << endl;
        exit(1);
    }

    int diff_range[5]={0};         /** 用于记录每个维度的不同范围数量 */ 
    map<uint64_t, int> range_map;
    for (int d = 0; d < 5; ++d) {
        for (int i = 0; i < rules_num; ++i) {
            /** 计算规则和节点边界在当前维度的交集范围 */ 
            uint64_t low = max(rules[i]->range[d][0], bounds[d][0]);
            uint64_t high = min(rules[i]->range[d][1], bounds[d][1]);

            /** 生成唯一的范围哈希值，将低位和高位拼接 */ 
            uint64_t range_hash = (low << 32) | high;

            /** 如果此范围首次出现，则将该维度的不同范围计数加1 */ 
            if (++range_map[range_hash] == 1)
                ++diff_range[d];
        }
        range_map.clear();
    }

    /** 用于记录不同范围数量最多的两个维度索引 */ 
    int index1 = -1; 
    for (int d = 0; d < 5; ++d){
        /** 更新 index1 和 index2，保存不同范围最多的两个维度 */ 
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

/**
 * @brief 计算节点的切割数量
 * @param node 节点指针
 * @param rules 规则指针的引用
 */
void HiNode::CalNumCuts() {
    if (rules_num == 0){
        cout << ">>> HiNode::CalNumCuts -> Wrong! rules_num = 0!" << endl;
        exit(1);
    }

    uint64_t spmf = rules_num * HiCuts::spfac;

    cut = 1;     
    uint64_t spcount = 1000000000; 
    uint64_t range_num[2];  

    /** 通过二分搜索来找到最优的切割数量 */ 
    while (true) {
        cut <<= 1; /** 切割数量翻倍 */ 
        /** 如果切割数量大于当前维度的长度，则恢复并设置为最大值 */ 
        if (cut >= (uint64_t)bounds[dim][1] - bounds[dim][0] + 1) {
            cut >>= 1; 
            spcount = 1000000000; 
            break; 
        }
        /** 计算每个维度的切割宽度 */ 
        width = ((uint64_t)bounds[dim][1] - bounds[dim][0] + 1 + cut - 1) / cut;
        /** 如果需要将宽度调整为2的幂次方 */ 
        if (HiCuts::width_power2)
            width = 1ULL << (uint64_t)floor(log2(width));

        spcount = cut; 
        /** 遍历所有规则，计算每个规则在子空间中的复制次数 */ 
        for (int j = 0; j < rules_num; ++j) {
            for (int b = 0; b < 2; b++) {
                uint64_t number;
                if (b == 0) number = max(rules[j]->range[dim][b], bounds[dim][b]);
                else        number = min(rules[j]->range[dim][b], bounds[dim][b]);
                /** 计算规则在切割子空间的范围索引 */ 
                range_num[b] = (number - bounds[dim][0]) / width;
            }
            /** 计算规则在子空间中的总数，累加到子空间计数 */ 
            spcount += range_num[1] - range_num[0] + 1;
        }
        cut >>= 1; // 恢复切割数量

        /** 如果两个维度的子空间计数都大于放大因子，跳出循环 */ 
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

    /** 将计算结果存储到节点 */ 
    for (int i = 0; i < 2; ++i) {
        width = ((uint64_t)bounds[dim][1] - bounds[dim][0] + 1 + cut - 1) / cut;
        /** 如果需要将宽度调整为2的幂次方 */ 
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

/**
 * @brief 当前节点是否包含该规则
 * @param node 节点指针
 * @param rule 规则指针的引用
 * @return bool 返回判断结果
 */
bool HiNode::NodeContainRule(Rule* rule) {
	for (int i = 0; i < 5; i++)
		if(rule->range[i][1] < bounds[i][0] || bounds[i][1] < rule->range[i][0])
			return false;
	return true;
}

/**
 * @brief 两个节点是否包含相同的规则集
 * @param node1 节点1指针
 * @param node2 节点2指针
 * @return bool 返回判断结果
 */
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
