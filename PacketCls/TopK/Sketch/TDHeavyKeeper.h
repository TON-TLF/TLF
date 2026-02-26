#ifndef TD_HEAVYKEEPER_H
#define TD_HEAVYKEEPER_H

#include "../../Elements/Elements.h"
#include "../Struct/TDSSummary.h"
#include "../BOBHASH64.h"

#define HK_d 2
#define HK_b 1.08

using namespace std;

class TDHeavyKeeper{
public:
    TDSSummary *ss;
    struct HKnode {
        int C,FP;
    } **HK;
    // C：计数器，用于记录当前元素的出现次数
    // FP：指纹，用于判断哈希冲突
    
    BOBHash64 * bobhash; // BOBHash64对象，用于字符串哈希
    int K, MAX_MEM; // K：保留的较重元素数量；MAX_MEM：哈希表的大小

public:
    TDHeavyKeeper(){};
    TDHeavyKeeper(int _K, int _MAX_MEM){
        K = _K;
        MAX_MEM = _MAX_MEM;
        ss = new TDSSummary(K);
        ss->clear();
        HK = new HKnode*[HK_d];  // 分配行指针数组
        for (int i = 0; i < HK_d; ++i) {
            HK[i] = new HKnode[MAX_MEM + 10];  // 为每行分配列数组
        }
        for (int  i= 0; i < HK_d; i++){
            for (int j = 0; j < MAX_MEM + 10; j++){
                HK[i][j].C = HK[i][j].FP = 0; // 重置计数器和指纹
            } 
        }
        bobhash=new BOBHash64(1005);
    }

    void clear(){
        ss->clear();
        for (int  i= 0; i < HK_d; i++){
            for (int j = 0; j < MAX_MEM + 10; j++){
                HK[i][j].C = HK[i][j].FP = 0; // 重置计数器和指纹
            } 
        }
    }

    unsigned long long Hash(string str) {
        return (bobhash->run(str.c_str(),str.size()));
    }

    int LOG2(int x){
        int l = 0,r = 31;
        while(l<r){
            int mid = (l + r + 1) >> 1;
            if((x >> mid) > 0) l = mid;
            else r = mid - 1;
        }
        return l;
    }

    int POW2(int x){
        return 1 << x;
    }

    // 插入元素str到数据结构中
    void insert(string str)
    {
        //cout<<"begin "<<str<<endl;
        
        //cout<<"p = "<<p<<endl;
        //cout<<"insert "<<mon<<" "<<p<<" "<<ss->node[p].sum<<endl;

        int maxv=0;
        unsigned long long H = Hash(str); // 获取元素str的哈希值
        int FP= (H>>56); // 取哈希值的高位作为指纹

        bool mon = false;
        //int p = ss->find(str); // 查找元素str是否在ssummary中
        int p = ss->find(str, H);
        if (p) mon = true; // 如果找到了，设置标志位mon为true

        for (int j = 0; j < HK_d; j++) {
            int Hsh = H%(MAX_MEM - (2*HK_d) + 2*j + 3); // 计算哈希表的索引位置
            int c = HK[j][Hsh].C; // 获取当前计数器值
            if (HK[j][Hsh].FP == FP) // 如果指纹匹配
            {
                //cout<<1<<endl;
                if (mon || c <= POW2(ss->getmin() + 1)){// 如果元素已在ssummary中或计数器值不大于ssummary中最小值
                    HK[j][Hsh].C++; // 增加计数器
                } 
                maxv=max(maxv,HK[j][Hsh].C); // 更新最大计数值
            } else // 如果指纹不匹配
            {
                int powans = pow(HK_b, HK[j][Hsh].C);
                if(powans <= 0){
                    cout<<"pow error! "<<HK_b<<" "<<HK[j][Hsh].C<<" "<<powans<<endl;
                    exit(1);
                }
                if (!(rand() % powans)) // 以一定概率减少计数器
                {
                    //cout<<"2 "<<HK[j][Hsh].C<<" "<<powans<<endl;
                    if (HK[j][Hsh].C > 0) { // 仅当 C 大于 0 时才减少
                        HK[j][Hsh].C--;
                    }
                    if (HK[j][Hsh].C <= 0) // 如果计数器小于等于0
                    {
                        HK[j][Hsh].FP = FP; // 更新指纹为当前元素的指纹
                        HK[j][Hsh].C = 1; // 重置计数器为1
                        maxv = max(maxv, 1); // 更新最大计数值
                    }
                }
            }
        }
        //cout<<"maxv = "<<maxv<<"  "<<log2(maxv + 1)<<endl;
        int log_maxv = LOG2(maxv + 1);
        
        if (!mon) {// 如果元素不在ssummary中
            if (log_maxv - ss->getmin() == 1 || ss->tot < K) // 如果最大计数值比ssummary中的最小值大1或者ssummary未满
            {
                int id = ss->getid(); // 获取一个空闲ID
                //cout<<"id = "<<id<<endl;
                //ss->addHash(ss->location(str), id); // 将元素添加到ssummary的哈希表中
                ss->addHash(H, id);
                ss->node[id].str = str;  // 存储元素的字符串表示
                ss->node[id].sum = log_maxv; // 设置计数值
                ss->node[id].start_sum = maxv;
                ss->link(0, id); // 将元素链接到链表中
                while(ss->tot > K) // 如果ssummary中元素数量超过K
                {
                    int t = ss->getmin(); // 获取最小值对应的链表位置
                    int tmp = ss->head[t].id; // 获取链表头部
                    ss->cut(ss->head[t].id); // 从链表中移除元素
                    //ss->recycling(tmp); // 回收元素
                    ss->recycling(tmp, Hash(ss->node[tmp].str));
                }
            }
        } else if (log_maxv > ss->node[p].sum) {// 如果元素在ssummary中且计数值大于当前值
            int tmp=ss->head[ss->node[p].sum].left; // 获取左邻的值
            //cout<<"tmp = "<<tmp<<" maxv = "<<maxv<<" "<<ss->node[p].sum<<endl;
            ss->cut(p); // 从链表中移除元素p
            if(ss->head[ss->node[p].sum].id) tmp=ss->node[p].sum; // 如果链表头部存在，更新tmp
            ss->node[p].sum=log_maxv; // 更新计数值为最大值
            ss->node[p].start_sum = maxv;
            ss->link(tmp,p); // 将元素重新链接到链表中
        } else {
            ss->node[p].start_sum = maxv;
        }
        //cout<<"over!"<<endl;
    }

    vector<TFNode> work() {  
        vector<TFNode> result;
        for(int i = TD_MAX_FLOW_NUM; i; i = ss->head[i].left) {// 所有链表中的元素遍历
            for(int j = ss->head[i].id; j; j = ss->node[j].nxt) // 遍历每个链表中的节点
            {
                result.push_back({ss->node[j].str, (uint64_t)ss->node[j].start_sum});
            }
        } 
        sort(result.begin(), result.end()); // 对所有记录进行排序
        return result;
    }

    uint64_t CalMemory(){
        uint64_t total_memory = 0;

        // 计算 ss 指针对象的内存大小
        total_memory += sizeof(ss);
        if (ss != nullptr) {
            total_memory += ss->CalMemory();  
        }

        // 计算 HK 指针的内存大小
        total_memory += sizeof(HKnode*) * HK_d;  // HK 行指针数组的内存大小
        for (int i = 0; i < HK_d; ++i) {
            total_memory += sizeof(HKnode) * (MAX_MEM + 10);  // 每行的 HKnode 数组的内存大小
        }

        // 计算 bobhash 对象的内存大小
        total_memory += sizeof(*bobhash);

        // 计算类本身的其他成员变量的内存大小
        total_memory += sizeof(K);
        total_memory += sizeof(MAX_MEM);

        return total_memory;
    }

    void printSS(){
        for(int i = TD_MAX_FLOW_NUM; i; i = ss->head[i].left) {// 所有链表中的元素遍历
            cout<<"---------------------------------------"<<endl;
            cout<<ss->head[i].id<<" "<<ss->head[i].left<<" "<<ss->head[i].right<<" "<<endl;
            for(int j = ss->head[i].id; j; j = ss->node[j].nxt) // 遍历每个链表中的节点
            {   
                cout<<ss->node[j].str<<" "<<ss->node[j].sum<<endl;
            }
            cout<<"---------------------------------------"<<endl;
        } 
    }
};
#endif