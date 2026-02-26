#include "RuleAnalyze.h"

using namespace std;

double calculate_range(vector<Rule*> &rules,int dim){
    int rules_num = rules.size();
    set<pair<uint32_t,uint32_t>> range_set;
    for(int i = 0; i < rules_num; i++){
        range_set.insert(make_pair(rules[i]->range[dim][0],rules[i]->range[dim][1]));
    }
    return (double)range_set.size();
}

double calculate_weight(vector<Rule*> &rules, int dim){
    int rules_num = rules.size();
    set<uint32_t> points_set;
    for(int i = 0; i < rules_num; i++){
        points_set.insert(rules[i]->range[dim][0]);
        points_set.insert(rules[i]->range[dim][1]);
    }

    vector<uint32_t> points;
    for(auto it : points_set){
        points.push_back(it);
    }
    int points_num = points.size();

    uint32_t start, end, L, R;
    vector<uint32_t> count(points_num + 10, 0);
    for(int i = 0; i < rules_num; i++){
        start = rules[i]->range[dim][0];
        end = rules[i]->range[dim][1];
        L = lower_bound(points.begin(), points.end(), start) - points.begin();
        R = upper_bound(points.begin(), points.end(), end) - points.begin();
        count[L]++;
        count[R]--;
    }

    double total_intersection_count = 0;
    for(int i = 1; i < points_num; i++){
        count[i] += count[i - 1];
        total_intersection_count += count[i];
    }

    double average_weight = 0;
    if(points_num > 1) average_weight = total_intersection_count / (points_num - 1.0);

    return average_weight;
    //return total_intersection_count;
}

int CollisionCounter(vector<Rule*> &rules, Rule* &rule){
    int ans = 0;
    bool overlap = false;
    int rules_num = rules.size();
    for(int i = 0; i < rules_num; i++){
        overlap = true;
        for(int d = 0; d < 5; d++){
            if(rules[i]->range[d][0] > rule->range[d][1] || rules[i]->range[d][1] < rule->range[d][0]){
                overlap = false;
                break;
            }
        }
        if(overlap) ans++;
    }
    return ans;
}

double MonteCarloCollisionCounter(vector<Rule*> &rules,int n){
    double cnt = 0;
    int rules_num = rules.size();

    random_device rd;  // 获取随机数种子
    mt19937 gen(rd()); // 以 rd() 初始化 Mersenne Twister 引擎
    uniform_int_distribution<> distrib(0, rules_num-1); // 定义分布范围 [0, rules_num-1]

    for(int i = 1; i < n; i++){
        int k = distrib(gen);// 生成随机数
        if(k >= rules_num) {
            cout<<"MonteCarloCollisionCounter random error! "<<endl;
            cout<<k<<" "<<rules_num<<endl;
        }
        cnt += CollisionCounter(rules, rules[k]);
    }

    return cnt / n;
}