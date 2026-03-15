#ifndef TD_SSUMMARY_H
#define TD_SSUMMARY_H

#include "../../Elements/Elements.h"
#include "BOBHASH32.h"

#define TD_Max_Hash 9973
#define TD_MAX_FLOW_NUM 64  // maximum flow
#define TD_MAX_FLOW_SIZE 300  // maximum size of stream-summary

using namespace std;

class TDSSummary{
public:
    int K;         
    int tot;      
    int num;       
    int ID[TD_MAX_FLOW_SIZE + 10]; 

    struct headnode{
        int id;   
        int left; 
        int right; 
    } head[TD_MAX_FLOW_NUM + 10];


    struct linknode{
        string str; 
        int sum;    
        int pre;    
        int nxt;    
        int start_sum;
    } node[TD_MAX_FLOW_SIZE + 10];


    int hhead[TD_Max_Hash + 10]; 
    int hnext[TD_MAX_FLOW_SIZE + 10];       
    BOBHash32* bobhash;       

    TDSSummary(int _K){
        K = _K;
        bobhash = new BOBHash32(1000);
    }

    void clear(){
        K = tot = 0;
        num = TD_MAX_FLOW_SIZE + 2;
        for(int i = 1; i <= TD_MAX_FLOW_SIZE + 2; i++) ID[i] = i;
        for(int i = 0; i <= TD_MAX_FLOW_NUM + 9; i++) head[i].id = head[i].left = head[i].right = 0;
        for(int i = 0; i <= TD_MAX_FLOW_SIZE + 9; i++) {
            node[i].str = "";
            node[i].sum = node[i].pre = node[i].nxt = 0;
            node[i].start_sum = 0;
        }
        memset(hhead,0,sizeof(hhead));
        memset(hnext,0,sizeof(hnext));

        //?为什么要这样 
        head[0].right = TD_MAX_FLOW_NUM;
    }


    void setK(int _K) { K = _K; }


    int getid() {
        if(num == 0){
            cout<<"TDSSummary id over!"<<endl;
            exit(1);
        }
        int id = ID[num--];
        node[id].str = "";
        node[id].sum = node[id].pre = node[id].nxt = 0;
        node[id].start_sum = 0;
        hnext[id]=0;
        return id;
    }

    int location(unsigned long long H) {
        return H % TD_Max_Hash;
    }

    void addHash(unsigned long long pos,int x) {
        pos = location(pos);
        hnext[x]=hhead[pos];
        hhead[pos]=x;
    }

    int find(string str, unsigned long long H) {
        for(int id = hhead[location(H)]; id; id = hnext[id])
            if(node[id].str == str) 
                return id;
        return 0;
    }

    void linkhead(int pre, int x) {
        head[x].left = pre;
        head[x].right = head[pre].right;
        head[pre].right = x;
        head[head[x].right].left = x;
    }

    void cuthead(int id) {
        int L = head[id].left;
        int R = head[id].right;
        head[L].right = R;
        head[R].left = L;
    }

    int getmin() {
        if (tot < K) return 0;
        if(head[0].right == TD_MAX_FLOW_NUM) return 1;//?为什么要这样
        return head[0].right;
    }

    void link(int prenum, int id) {
        ++tot;
        bool flag = (head[node[id].sum].id);  
        node[id].nxt = head[node[id].sum].id;
        if(node[id].nxt){
            node[node[id].nxt].pre = id;       
        } 
        node[id].pre = 0;          
        head[node[id].sum].id = id;   

        if(!flag) {
            for(int j = node[id].sum - 1; j > 0 && j > node[id].sum - 10; j--) {
                if(head[j].id) {
                    linkhead(j, node[id].sum);
                    return;
                } 
            } 
            linkhead(prenum, node[id].sum); 
        }
    }

    void cut(int id) {
        --tot; 
        if(head[node[id].sum].id == id){
            head[node[id].sum].id = node[id].nxt;
        }
        if(head[node[id].sum].id == 0){
            cuthead(node[id].sum);
        } 

        int L = node[id].pre;
        int R = node[id].nxt;
        node[L].nxt = R;
        node[R].pre = L;
    }

    void recycling(int id, unsigned long long H)
    {
        int pos = location(H);
        if (hhead[pos] == id){
            hhead[pos] = hnext[id];
        } else {
            for(int j = hhead[pos]; j; j = hnext[j]){
                if(hnext[j] == id) {
                    hnext[j] = hnext[id];
                    break;
                }
            }
        }
        ID[++num]=id;
    }

    uint64_t CalMemory(){
        uint64_t total_memory = 0;

        total_memory += sizeof(K);
        total_memory += sizeof(tot);
        total_memory += sizeof(num);
        total_memory += (TD_MAX_FLOW_SIZE + 10) * sizeof(int);        
        total_memory += (TD_MAX_FLOW_NUM + 10) * sizeof(headnode);       
        total_memory += (TD_MAX_FLOW_SIZE + 10) * sizeof(linknode);    
        total_memory += (TD_Max_Hash + 10) * sizeof(int);          
        total_memory += (TD_MAX_FLOW_SIZE + 10) * sizeof(int);      
        total_memory += sizeof(*bobhash);

        return total_memory;
    }
};
#endif