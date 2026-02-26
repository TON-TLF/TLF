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

/**
 * @brief 根据给定规则创建EffiCuts树
 * @param rules 规则指针的向量
 * @param depth 当前深度
 * @return bool 表示创建是否成功
 */
void EffiNode::Create(ProgramState *ps, int depth) {
    /** 对规则数量进行必要的检查 */
    if (rules_num == 0){
        cout << ">>> EffiNode::Create -> Wrong! rules_num = 0" << endl;
        exit(1);
    }
    HasBounds();

    /** 如果启用了冗余移除且节点深度不为0，移除冗余规则 */ 
    if (EffiCuts::remove_redund && depth != 0) {
        RemoveRedund();
    }
	sort(rules.begin(), rules.end(), CmpRulePriority);
    /** 对规则按优先级进行排序，并将节点的优先级设置为最高优先级规则的优先级 */ 
    priority = rules[0]->priority; 

    /** 检查是否达到最大深度、规则数量小于等于叶节点阈值或节点无边界 */ 
    if (depth == EffiCuts::max_depth || rules_num <= EffiCuts::leaf_size) {
        leaf_node = true;      
        ps->DTI.AddNode(depth, rules_num);
        return;                      
    }

    /** 计算节点的切割维度 */ 
    CalDimToCuts();

    /** 计算切割数量 */ 
    CalNumCuts();
    
    ps->DTI.AddDims(dims[0], dims[1]);

    child_num = cuts[0] * cuts[1]; /** 计算子节点数量 */ 

    child_arr = new EffiNode*[child_num];
    bool *vis = new bool[child_num];               /** 用于标记子节点是否已被使用 */
    int max_child_rules_num = 0;                   /** 记录子节点中的最大规则数量 */ 
    
    /** 遍历每个子节点 */ 
    for (int i = 0; i < cuts[0]; ++i){
        for (int j = 0; j < cuts[1]; ++j) {
            int index = i * cuts[1] + j;   /** 计算子节点的索引 */ 
            child_arr[index] = new EffiNode(); 
            vis[index] = false; 

            for (int k = 0; k < 5; ++k) {
                /** 如果是切割维度，计算子节点的边界 */ 
                if (k == dims[0] || k == dims[1]) {
                    int p = (k == dims[0]) ? 0 : 1;
                    int ret = (p == 0) ? i : j;
                    uint64_t low = bounds[k][0] + (uint64_t)ret * width[p];
                    uint64_t high = min(low + width[p] - 1, (uint64_t)bounds[k][1]);

                    /** 如果低位大于高位，释放子节点并退出循环 */ 
                    if (low > high) {
                        cout<<">>> EffiNode::Create -> Wrong! low > high error!"<<endl;
                        exit(1);
                        break;
                    }
                    child_arr[index]->bounds[k][0] = low; 
                    child_arr[index]->bounds[k][1] = high;
                } else { 
                    /** 非切割维度，直接继承父节点的边界 */ 
                    child_arr[index]->bounds[k][0] = bounds[k][0];
                    child_arr[index]->bounds[k][1] = bounds[k][1];
                }
            }

            /** 将符合子节点边界的规则添加到子节点的规则集合中 */ 
            for (int k = 0; k < rules_num; ++k){
                if (child_arr[index]->NodeContainRule(rules[k])){
                    child_arr[index]->rules.push_back(rules[k]);
                }
            }
                
            /** 如果子节点没有规则，释放子节点并继续 */
            int child_rules_num = child_arr[index]->rules.size(); 
            if (child_rules_num == 0) {
                delete child_arr[index];
                child_arr[index] = NULL;
                continue;
            }

            vis[index] = true;  /** 标记子节点已被使用 */ 
            child_arr[index]->rules_num = child_rules_num; 

            max_child_rules_num = max(max_child_rules_num, child_rules_num); 
        }
    }

    /** 如果子节点中的规则数量接近总规则数量，则停止进一步分割 */ 
    if (max_child_rules_num >= rules_num * 0.99) {
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
            cout<<">>> EffiNode::Create -> Wrong! child_arr error!"<<endl;
            exit(1);
        }
    }

    /** 如果启用了节点合并，尝试合并相同规则的子节点 */ 
    if (EffiCuts::node_merge) {
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

/**
 * @brief 释放节点内存。
 */
EffiNode::~EffiNode() {
	map<EffiNode*, int> child_map;
	for (int i = 0; i < child_num; ++i)
		if (child_arr[i] != NULL && ++child_map[child_arr[i]] == 1)
			delete child_arr[i];
    if(child_arr != NULL) delete child_arr;
}

/**
 * @brief 移除冗余规则，保留在节点边界内的唯一规则
 * @param rules 规则指针的引用
 */
void EffiNode::RemoveRedund() {
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
void EffiNode::HasBounds() {
    for(int i = 0; i < 5; ++i){
        if(bounds[i][0] > bounds[i][1]){
            cout<<">>> EffiNode::Create -> Wrong! bounds error!"<<endl;
            exit(1);
        }
    }
}

/**
 * @brief 计算节点的切割维度
 */
void EffiNode::CalDimToCuts() {
    if (rules_num == 0){
        cout << ">>> EffiNode::CalDimToCuts -> Wrong! rules_num = 0" << endl;
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
    int index1 = -1, index2 = -1; 
    for (int d = 0; d < 5; ++d){
        //? diff_range[d] * diff_dim >= diff_sum 意义是什么（原来的代码已删）
        /** 更新 index1 和 index2，保存不同范围最多的两个维度 */ 
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

/**
 * @brief 计算节点的切割数量
 * @param node 节点指针
 * @param rules 规则指针的引用
 */
void EffiNode::CalNumCuts() {
    if (rules_num == 0){
        cout << ">>> EffiNode::CalNumCuts -> Wrong! rules_num = 0" << endl;
        exit(1);
    }

    uint64_t spmf = rules_num * EffiCuts::spfac;

    cuts[0] = cuts[1] = 1;     
    uint64_t spcount[2] = {1000000000, 1000000000}; 
    uint64_t range_num[2][2];  

    //?只有一个维度，说明其它维度没有切割价值，不应该强行选择第二个维度
    /** 如果只有一个维度，则第二个维度选择下一个 */
    if (dims_num == 1)
        dims[1] = (dims[0] + 1) % 4;

    /** 通过二分搜索来找到最优的切割数量 */ 
    while (true) {
        for (int i = 0; i < dims_num; ++i) {
            cuts[i] <<= 1; /** 切割数量翻倍 */ 
            /** 如果切割数量大于当前维度的长度，则恢复并设置为最大值 */ 
            if (cuts[i] >= (uint64_t)bounds[dims[i]][1] - bounds[dims[i]][0] + 1) {
                cuts[i] >>= 1; 
                spcount[i] = 1000000000; 
                continue; 
            }
            /** 计算每个维度的切割宽度 */ 
            for (int j = 0; j < 2; ++j) {
                width[j] = ((uint64_t)bounds[dims[j]][1] - bounds[dims[j]][0] + 1 + cuts[j] - 1) / cuts[j];
                /** 如果需要将宽度调整为2的幂次方 */ 
                if (EffiCuts::width_power2)
                    width[j] = 1ULL << (uint64_t)floor(log2(width[j]));
            }

            spcount[i] = cuts[0] * cuts[1]; 
            /** 遍历所有规则，计算每个规则在子空间中的复制次数 */ 
            for (int j = 0; j < rules_num; ++j) {
                for (int a = 0; a < 2; a++){
                    for (int b = 0; b < 2; b++) {
                        uint64_t number;
                        if (b == 0) number = max(rules[j]->range[dims[a]][b], bounds[dims[a]][b]);
                        else        number = min(rules[j]->range[dims[a]][b], bounds[dims[a]][b]);
                        /** 计算规则在切割子空间的范围索引 */ 
                        range_num[a][b] = (number - bounds[dims[a]][0]) / width[a];
                    }
                }
                /** 计算规则在子空间中的总数，累加到子空间计数 */ 
                spcount[i] += (range_num[0][1] - range_num[0][0] + 1) * (range_num[1][1] - range_num[1][0] + 1);
            }
            cuts[i] >>= 1; // 恢复切割数量
        }
        /** 如果两个维度的子空间计数都大于放大因子，跳出循环 */ 
        if (spcount[0] > spmf && spcount[1] > spmf)
            break;
        /** 根据子空间计数选择进一步加倍的维度 */ 
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

    /** 将计算结果存储到节点 */ 
    for (int i = 0; i < 2; ++i) {
        width[i] = ((uint64_t)bounds[dims[i]][1] - bounds[dims[i]][0] + 1 + cuts[i] - 1) / cuts[i];
        /** 如果需要将宽度调整为2的幂次方 */ 
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

/**
 * @brief 当前节点是否包含该规则
 * @param node 节点指针
 * @param rule 规则指针的引用
 * @return bool 返回判断结果
 */
bool EffiNode::NodeContainRule(Rule* rule) {
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
