#ifndef  CATEGORY_H
#define  CATEGORY_H

#include "../Elements/Elements.h"
#include "../Tools/Tools.h"
#include "TreeNode.h"

using namespace std;

struct SubCategory {
    vector<Rule*> rules;     
    int wildcard;             
    bool used;               
};

class Category {
public:
    vector<SubCategory*> sub[4]; 
    int sub_num[4];             

    void Init();
    void ProcessRule(vector<Rule*> &rules);
    int SelectCategoryFirst(int id);
    int SelectCategorySecond(int id, int wildcard);
    vector<pair<int,int>> TreeMerge();
};

#endif