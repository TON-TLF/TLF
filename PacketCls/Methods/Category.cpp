#include "Category.h"

using namespace std;

const int A1[5]  = {11110, 11101, 11011, 10111,  1111};
const int A2[10] = {11100, 11010, 11001, 
                    10110, 10101, 10011,
                    1110,  1101,  1011,
                    111};
const int A3[10] = {11000, 10100, 10010, 10001, 
                    1100,  1010,  1001,
                    110,   101,
                    11};

void Category::Init(){
    int sub_nums[4] = {5, 10, 10, 1};
    for(int i = 0; i < 4 ;i++){
        sub_num[i] = sub_nums[i];
        for (int j = 0; j < sub_num[i]; j++) {
            SubCategory *_sub = new SubCategory();
            sub[i].push_back(_sub);
        }
    }
    
    for (int i = 0; i < 5; i++){
        sub[0][i]->wildcard = A1[i];
    }
	for (int i = 0; i < 10; i++) {
		sub[1][i]->wildcard = A2[i];
		sub[2][i]->wildcard = A3[i];
	}
}

void Category::ProcessRule(vector<Rule*> &rules) {
    uint64_t field_max[5] = {1ULL << 32, 1ULL << 32, 65536, 65536, 256};
    int rules_num = rules.size();
    for(int i = 0; i < rules_num; ++i){
        int wildcard = 0;  
        int wild_num = 0;  
        double field[5];    

        for (int j = 0; j < 5; j++){
            field[j] = (double)(rules[i]->range[j][1] - rules[i]->range[j][0]) / field_max[j];
        }
        if (field[0] >= 0.05) wildcard += 10000, ++wild_num;
        if (field[1] >= 0.05) wildcard += 1000, ++wild_num;
        if (field[2] >= 0.5)  wildcard += 100, ++wild_num;
        if (field[3] >= 0.5)  wildcard += 10, ++wild_num;
        if (field[4] >= 0.5)  wildcard += 1, ++wild_num;

        if (wild_num >= 4) {
            int index = 0;
            for (int j = 0; j < 5; j++){
                if (field[j] < field[index]){
                    index = j;
                }
            }
            sub[0][4 - index]->rules.push_back(rules[i]);
        } else if (wild_num == 3) {
            for (int j = 0; j < 10; j++){
                if (sub[1][j]->wildcard == wildcard){
                    sub[1][j]->rules.push_back(rules[i]);
                }
            }
        } else if (wild_num == 2) {
            for (int j = 0; j < 10; j++){
                if (sub[2][j]->wildcard == wildcard){
                    sub[2][j]->rules.push_back(rules[i]);
                } 
            }
        } else {
            sub[3][0]->rules.push_back(rules[i]);
        }
    }
}

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

int Category::SelectCategorySecond(int id, int wildcard) {
    int idx = -1,diff = 1000000; 
    for (int i = 0; i < sub_num[id]; ++i) {
        if (sub[id][i]->used == 1)
            continue;
        
        int d = wildcard - sub[id][i]->wildcard;
        if ((d == 10000 || d == 1000 || d == 100 || d == 10 || d == 1) && d < diff) {
            diff = d; 
            idx = i;  
        }
    }
    return idx;
}

vector<pair<int,int>> Category::TreeMerge(){
    int index = 0;        
    int tree_num = 0;
    vector<pair<int,int>> ans;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < sub_num[i]; j++) {
            if (sub[i][j]->rules.size() == 0){
                sub[i][j]->used = true;
            }
        }
    }

    while (index < 4) {
        int id = this->SelectCategoryFirst(index);
        if (id == -1) {
            ++index;
            continue;
        }

        int nxt = (index == 3 ? -1 : this->SelectCategorySecond(index + 1,sub[index][id]->wildcard));
        if (nxt >= 0) {
            sub[index + 1][nxt]->used = true;
            int rules_num = sub[index + 1][nxt]->rules.size();
            for (int i = 0; i < rules_num; ++i)
                sub[index][id]->rules.push_back(sub[index + 1][nxt]->rules[i]);
        }

        sub[index][id]->used = true;
        ans.push_back(make_pair(index,id));
    }
    return ans;
}

