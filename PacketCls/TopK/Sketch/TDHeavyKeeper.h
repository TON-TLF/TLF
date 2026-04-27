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

    BOBHash64 * bobhash; 
    int K, MAX_MEM; 

public:
    TDHeavyKeeper(){};
    TDHeavyKeeper(int _K, int _MAX_MEM){
        K = _K;
        MAX_MEM = _MAX_MEM;
        ss = new TDSSummary(K);
        ss->clear();
        HK = new HKnode*[HK_d];  
        for (int i = 0; i < HK_d; ++i) {
            HK[i] = new HKnode[MAX_MEM + 10]; 
        }
        for (int  i= 0; i < HK_d; i++){
            for (int j = 0; j < MAX_MEM + 10; j++){
                HK[i][j].C = HK[i][j].FP = 0; 
            } 
        }
        bobhash=new BOBHash64(1005);
    }

    void clear(){
        ss->clear();
        for (int  i= 0; i < HK_d; i++){
            for (int j = 0; j < MAX_MEM + 10; j++){
                HK[i][j].C = HK[i][j].FP = 0;
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

    void insert(string str)
    {
        int maxv=0;
        unsigned long long H = Hash(str); 
        int FP= (H>>56); 

        bool mon = false;
        int p = ss->find(str, H);
        if (p) mon = true; 

        for (int j = 0; j < HK_d; j++) {
            int Hsh = H%(MAX_MEM - (2*HK_d) + 2*j + 3);
            int c = HK[j][Hsh].C; 
            if (HK[j][Hsh].FP == FP)
            {
                if (mon || c <= POW2(ss->getmin() + 1)){
                    HK[j][Hsh].C++; 
                } 
                maxv=max(maxv,HK[j][Hsh].C); 
            } else 
            {
                int powans = pow(HK_b, HK[j][Hsh].C);
                if(powans <= 0){
                    cout<<"pow error! "<<HK_b<<" "<<HK[j][Hsh].C<<" "<<powans<<endl;
                    exit(1);
                }
                if (!(rand() % powans)) 
                {
                    if (HK[j][Hsh].C > 0) { 
                        HK[j][Hsh].C--;
                    }
                    if (HK[j][Hsh].C <= 0) 
                    {
                        HK[j][Hsh].FP = FP; 
                        HK[j][Hsh].C = 1; 
                        maxv = max(maxv, 1); 
                    }
                }
            }
        }
        
        int log_maxv = LOG2(maxv + 1);
        
        if (!mon) {
            if (log_maxv - ss->getmin() == 1 || ss->tot < K) 
            {
                int id = ss->getid(); 
                ss->addHash(H, id);
                ss->node[id].str = str;  
                ss->node[id].sum = log_maxv; 
                ss->node[id].start_sum = maxv;
                ss->link(0, id); 
                while(ss->tot > K) 
                {
                    int t = ss->getmin(); 
                    int tmp = ss->head[t].id; 
                    ss->cut(ss->head[t].id); 
                    ss->recycling(tmp, Hash(ss->node[tmp].str));
                }
            }
        } else if (log_maxv > ss->node[p].sum) {
            int tmp=ss->head[ss->node[p].sum].left; 
            ss->cut(p); 
            if(ss->head[ss->node[p].sum].id) tmp=ss->node[p].sum; 
            ss->node[p].sum=log_maxv; 
            ss->node[p].start_sum = maxv;
            ss->link(tmp,p); 
        } else {
            ss->node[p].start_sum = maxv;
        }
    }

    vector<TraceFreq> work() {  
        vector<TraceFreq> result;
        for(int i = TD_MAX_FLOW_NUM; i; i = ss->head[i].left) {
            for(int j = ss->head[i].id; j; j = ss->node[j].nxt) 
            {
                TraceFreq tmp;
                const unsigned char* ptr = reinterpret_cast<const unsigned char*>(ss->node[j].str.data());

                tmp.freq = ss->node[j].start_sum;
                tmp.trace.key[0] =  (static_cast<uint32_t>(ptr[0]) << 24) |
                                    (static_cast<uint32_t>(ptr[1]) << 16) |
                                    (static_cast<uint32_t>(ptr[2]) << 8)  |
                                    (static_cast<uint32_t>(ptr[3]));
                tmp.trace.key[1] =  (static_cast<uint32_t>(ptr[4]) << 24) |
                                    (static_cast<uint32_t>(ptr[5]) << 16) |
                                    (static_cast<uint32_t>(ptr[6]) << 8)  |
                                    (static_cast<uint32_t>(ptr[7]));
                tmp.trace.key[2] =  (static_cast<uint32_t>(ptr[8]) << 8)  |
                                    (static_cast<uint32_t>(ptr[9]));
                tmp.trace.key[3] =  (static_cast<uint32_t>(ptr[10]) << 8) |
                                    (static_cast<uint32_t>(ptr[11]));
                tmp.trace.key[4] =   static_cast<uint32_t>(ptr[12]);

                result.push_back(tmp);
            }
        } 
        sort(result.begin(), result.end()); 
        return result;
    }

    uint64_t CalMemory(){
        uint64_t total_memory = 0;

        total_memory += sizeof(ss);
        if (ss != nullptr) {
            total_memory += ss->CalMemory();  
        }

        total_memory += sizeof(HKnode*) * HK_d;  
        for (int i = 0; i < HK_d; ++i) {
            total_memory += sizeof(HKnode) * (MAX_MEM + 10);  
        }

        total_memory += sizeof(*bobhash);
        total_memory += sizeof(K);
        total_memory += sizeof(MAX_MEM);

        return total_memory;
    }

    void printSS(){
        for(int i = TD_MAX_FLOW_NUM; i; i = ss->head[i].left) {
            cout<<"---------------------------------------"<<endl;
            cout<<ss->head[i].id<<" "<<ss->head[i].left<<" "<<ss->head[i].right<<" "<<endl;
            for(int j = ss->head[i].id; j; j = ss->node[j].nxt) 
            {   
                cout<<ss->node[j].str<<" "<<ss->node[j].sum<<endl;
            }
            cout<<"---------------------------------------"<<endl;
        } 
    }
};
#endif