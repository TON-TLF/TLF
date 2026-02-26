#include "Category.h"

using namespace std;

/** A1、A2、A3 是三组整型数组，用于存储建树策略中相关的值 */ 
const int A1[5]  = {11110, 11101, 11011, 10111,  1111};
const int A2[10] = {11100, 11010, 11001, 
                    10110, 10101, 10011,
                    1110,  1101,  1011,
                    111};
const int A3[10] = {11000, 10100, 10010, 10001, 
                    1100,  1010,  1001,
                    110,   101,
                    11};

/**
 * @brief 创建一个新的 Category 对象，并根据 sub_num 创建相应数量的 SubCategory 对象
 * @param sub_num SubCategory 对象数量
 * @return Category* 返回创建的Category 对象
 */
void Category::Init(){
    /** 创建好类别结构体，为规则分类做准备 */
    int sub_nums[4] = {5, 10, 10, 1};
    for(int i = 0; i < 4 ;i++){
        sub_num[i] = sub_nums[i];
        for (int j = 0; j < sub_num[i]; j++) {
            SubCategory *_sub = new SubCategory();
            sub[i].push_back(_sub);
        }
    }
    
    /** 初始化各个子类别下的 wildcard */
    for (int i = 0; i < 5; i++){
        sub[0][i]->wildcard = A1[i];
    }
	for (int i = 0; i < 10; i++) {
		sub[1][i]->wildcard = A2[i];
		sub[2][i]->wildcard = A3[i];
	}
}

/**
 * @brief 预处规则，将规则分别添加各个子类别中
 * @param rule 规则指针
 * @param cate 类别数组
 */
void Category::ProcessRule(vector<Rule*> &rules) {
    /** 表示每一字段的最大值 */
    uint64_t field_max[5] = {1ULL << 32, 1ULL << 32, 65536, 65536, 256};
    int rules_num = rules.size();
    for(int i = 0; i < rules_num; ++i){
        int wildcard = 0;  
        int wild_num = 0;  /** 记录满足通配符条件的字段数 */ 
        double field[5];   /** 用于存储比值的数组 */ 

        /** 计算该条规则每一字段范围与最大范围的比值，用于标准化 */ 
        for (int j = 0; j < 5; j++){
            field[j] = (double)(rules[i]->range[j][1] - rules[i]->range[j][0]) / field_max[j];
        }
        if (field[0] >= 0.05) wildcard += 10000, ++wild_num;
        if (field[1] >= 0.05) wildcard += 1000, ++wild_num;
        if (field[2] >= 0.5)  wildcard += 100, ++wild_num;
        if (field[3] >= 0.5)  wildcard += 10, ++wild_num;
        if (field[4] >= 0.5)  wildcard += 1, ++wild_num;

        if (wild_num >= 4) {
            int index = 0; /** 用于记录比值最小字段的索引 */ 
            for (int j = 0; j < 5; j++){
                if (field[j] < field[index]){
                    index = j;
                }
            }
            /** 根据最小字段的索引，将规则添加到 cate[0] 对应的子类别 */ 
            sub[0][4 - index]->rules.push_back(rules[i]);
        } else if (wild_num == 3) {
            /** 遍历 cate[1] 的子类别，找到通配符值匹配的子类别，并将规则添加 */ 
            for (int j = 0; j < 10; j++){
                if (sub[1][j]->wildcard == wildcard){
                    sub[1][j]->rules.push_back(rules[i]);
                }
            }
        } else if (wild_num == 2) {
            /** 遍历 cate[2] 的子类别，找到通配符值匹配的子类别，并将规则添加 */ 
            for (int j = 0; j < 10; j++){
                if (sub[2][j]->wildcard == wildcard){
                    sub[2][j]->rules.push_back(rules[i]);
                } 
            }
        } else {
            /** 将剩余的规则添加到 cate[3] 的第一个子类别 */ 
            sub[3][0]->rules.push_back(rules[i]);
        }
    }
}

/**
 * @brief 选择当前类别中规则数量最少且未使用的子类别
 * @param category 类别指针
 * @return int 返回所选择的子类别，-1表示未找到
 */
int Category::SelectCategoryFirst(int id){
    int ans = -1,minn = 1000000;      
    for (int i = 0; i < sub_num[id]; ++i) {
        if (!sub[id][i]->used && sub[id][i]->rules.size() < minn) {
            minn = sub[id][i]->rules.size(); 
            ans = i;  
        }
    }
    return ans;
}

/**
 * @brief 择与给定通配符值最匹配的子类别
 * @param wildcard 通配符值
 * @param category 类别指针
 * @return int 返回所选择的子类别，-1表示未找到
 */
int Category::SelectCategorySecond(int id, int wildcard) {
    int idx = -1,diff = 1000000; 
    for (int i = 0; i < sub_num[id]; ++i) {
        if (sub[id][i]->used == 1)
            continue;
        
        //? 是否取绝对值
        /** 计算当前子类别通配符与给定通配符的差值 */ 
        int d = wildcard - sub[id][i]->wildcard;

        /** 检查差值是否符合特定条件，表示仅相差一个字段且小于当前最小值 */ 
        if ((d == 10000 || d == 1000 || d == 100 || d == 10 || d == 1) && d < diff) {
            diff = d; 
            idx = i;  
        }
    }
    return idx;
}

/**
 * @brief 分别构建每一颗子树，其中可能对部分子树进行合并
 * @param root 树根指针数组
 * @param cate 类别数组指针
 * @return int 返回最终创建的子树的个数
 */
vector<pair<int,int>> Category::TreeMerge(){
    int index = 0;        // 当前处理的类别索引
    int tree_num = 0;
    vector<pair<int,int>> ans;

    /** 标记为空的子类别 */
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < sub_num[i]; j++) {
            if (sub[i][j]->rules.size() == 0){
                sub[i][j]->used = true;
            }
        }
    }

    /** 依次处理每一个类别 */
    while (index < 4) {
        /** 选择当前类别中规则数量最少的子类别,如果没有找到合适的子类别，则处理下一个类别 */
        int id = this->SelectCategoryFirst(index);
        if (id == -1) {
            ++index;
            continue;
        }

        /** 寻找与当前子类别通配符相匹配的下一个类别中的子类别 */ 
        int nxt = (index == 3 ? -1 : this->SelectCategorySecond(index + 1,sub[index][id]->wildcard));

        /** 如果找到匹配的下一个子类别 */ 
        if (nxt >= 0) {
            sub[index + 1][nxt]->used = true;
            /** 将下一个子类别的规则合并到当前子类别中 */ 
            int rules_num = sub[index + 1][nxt]->rules.size();
            for (int i = 0; i < rules_num; ++i)
                sub[index][id]->rules.push_back(sub[index + 1][nxt]->rules[i]);
        }

        /** 使用当前子类别的规则创建树，并将树的根指针存储在 root 数组中 */
        sub[index][id]->used = true;
        ans.push_back(make_pair(index,id));
    }
    return ans;
}

