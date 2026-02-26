#ifndef  CATEGORY_H
#define  CATEGORY_H

#include "../Elements/Elements.h"
#include "../Tools/Tools.h"
#include "TreeNode.h"

using namespace std;

/**
 * @brief 子类别结构体，表示规则大分类下的子分类
 */
struct SubCategory {
    vector<Rule*> rules;      /** 存储规则的向量 */ 
    int wildcard;             /** 通配符数量 */ 
    bool used;                /** 标记是否被使用 */ 
};

/**
 * @brief 类别结构体，表示规则的分类
 */
class Category {
public:
    vector<SubCategory*> sub[4]; /** 存储子类别的向量 */ 
    int sub_num[4];              /** 子类别的数量 */

    void Init();
    void ProcessRule(vector<Rule*> &rules);
    int SelectCategoryFirst(int id);
    int SelectCategorySecond(int id, int wildcard);
    vector<pair<int,int>> TreeMerge();
};

#endif