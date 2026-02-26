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
    int K;          /** 该数据结构中存储的流量种类数，即保留Topk流量 */
    int tot;        /** 该数据结构中流量的总条数 */
    int num;        /** 当前剩余的可用ID数目 */
    int ID[TD_MAX_FLOW_SIZE + 10]; /** ID数组，记录空闲ID指向的链表节点 */

    /**
    * @brief SS的头结点
    * head[i] 表示存储流量条数为i的链表头部
    */
    struct headnode{
        int id;    /** 当前头结点指向的链表节点的ID */
        int left;  /** 指向当前头结点左边的头结点 */
        int right; /** 指向当前头结点右边的头结点 */
    } head[TD_MAX_FLOW_NUM + 10];

    /** 
    * @brief 链表结点
    * 每个链表节点存储一条流量的相关信息
    */
    struct linknode{
        string str; /** 该条流量的字符串表示 */
        int sum;    /** 该条流量的总条数 */
        int pre;    /** 该节点的前继节点编号 */
        int nxt;    /** 该节点的后继节点编号 */
        int start_sum;
    } node[TD_MAX_FLOW_SIZE + 10];


    int hhead[TD_Max_Hash + 10]; /** hhead[i]表示存储哈希值为i的元素的头结点 */
    int hnext[TD_MAX_FLOW_SIZE + 10];        /** hnext[i]表示哈希链表中编号为i的节点的后继节点 */
    BOBHash32* bobhash;       /** 哈希对象指针，用于计算字符串哈希值 */

    /** 
    * @brief 构造函数
    * @param _K 保留前K条重要流
    */
    TDSSummary(int _K){
        K = _K;
        bobhash = new BOBHash32(1000);
    }

    /** 
    * @brief 清空所有数据
    */
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

    /** 
    * @brief 设置k值
    * @param _K 保留前K条重要流
    */
    void setK(int _K) { K = _K; }

    /** 
    * @brief 获取一个空闲的ID
    */
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

    /** 
    * @brief 获取字符串str对应的哈希位置
    * @param str 要查找的字符串
    */
    // int location(string str) {
    //     return (bobhash->run(str.c_str(),str.size())) % TD_Max_Hash;
    // }
    int location(unsigned long long H) {
        return H % TD_Max_Hash;
    }

    /** 
    * @brief 在哈希表中添加元素x到位置为pos的链表中
    * @param pos 要添加到的哈希表的位置
    * @param x 待添加的元素
    */
    // void addHash(int pos,int x) {
    //     hnext[x]=hhead[pos];
    //     hhead[pos]=x;
    // }
    void addHash(unsigned long long pos,int x) {
        pos = location(pos);
        hnext[x]=hhead[pos];
        hhead[pos]=x;
    }

    /** 
    * @brief 查找字符串str对应的ID，若不存在则返回0
    * @param str 待查找的字符串
    */
    // int find(string str) {
    //     for(int id = hhead[location(str)]; id; id = hnext[id])
    //         if(node[id].str == str) 
    //             return id;
    //     return 0;
    // }
    int find(string str, unsigned long long H) {
        for(int id = hhead[location(H)]; id; id = hnext[id])
            if(node[id].str == str) 
                return id;
        return 0;
    }

    //!参数反转了
    /** 
    * @brief 将元素x插入到元素pre之后
    * @param pre 插入的位置
    * @param x 待插入的元素
    */
    void linkhead(int pre, int x) {
        head[x].left = pre;
        head[x].right = head[pre].right;
        head[pre].right = x;
        head[head[x].right].left = x;
    }

    /** 
    * @brief 从头结点链表中移除编号为id的头节点
    * @param id 待删除的头结点编号
    */
    void cuthead(int id) {
        int L = head[id].left;
        int R = head[id].right;
        head[L].right = R;
        head[R].left = L;
    }

    /** 
    * @brief 获取最小的有效ID，如果元素总数小于K则返回0
    */
    int getmin() {
        if (tot < K) return 0;
        if(head[0].right == TD_MAX_FLOW_NUM) return 1;//?为什么要这样
        return head[0].right;
    }

    /** 
    * @brief 将节点id插入到链表对应位置中
    * @param id 待插入的结点编号
    */
    void link(int prenum, int id) {
        ++tot;
        bool flag = (head[node[id].sum].id);  /** 检查待插入的链表是否为新链表 */
        node[id].nxt = head[node[id].sum].id;
        if(node[id].nxt){
            node[node[id].nxt].pre = id;      /** 如果存在下一个元素，则更新其pre指向id */ 
        } 
        node[id].pre = 0;         /** 将id的pre设为0，表示其为链表头部 */ 
        head[node[id].sum].id = id;  /** 更新链表头部为元素id */ 

        /** 如果是新链表 */
        if(!flag) {
            /** 查找比node[id].sum小且接近的非空链表 */ 
            for(int j = node[id].sum - 1; j > 0 && j > node[id].sum - 10; j--) {
                /** 找到则将当前链表插入其后 */ 
                if(head[j].id) {
                    linkhead(j, node[id].sum);
                    return;
                } 
            } 
            linkhead(prenum, node[id].sum); // 如果未找到合适的链表，则插入到0之后
        }
    }

    /** 
    * @brief 从链表中移除编号为id的节点
    * @param id 待删除结点的编号
    */
    void cut(int id) {
        --tot; 
        /** 如果id是链表头部，则更新头部为其后继节点 */ 
        if(head[node[id].sum].id == id){
            head[node[id].sum].id = node[id].nxt;
        }
        /** 如果链表为空，则从链表头部移除 */ 
        if(head[node[id].sum].id == 0){
            cuthead(node[id].sum);
        } 

        int L = node[id].pre;
        int R = node[id].nxt;
        node[L].nxt = R;
        node[R].pre = L;
    }

    /** 
    * @brief 回收节点id，将其从哈希表和链表中移除并归还ID
    * @param id 待回收结点的编号
    */
    // void recycling(int id)
    // {
    //     int pos = location(node[id].str);
    //     if (hhead[pos] == id){
    //         hhead[pos] = hnext[id];
    //     } else {
    //         for(int j = hhead[pos]; j; j = hnext[j]){
    //             if(hnext[j] == id) {
    //                 hnext[j] = hnext[id];
    //                 break;
    //             }
    //         }
    //     }
    //     ID[++num]=id;
    // }
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

        // 计算基本数据成员的内存
        total_memory += sizeof(K);
        total_memory += sizeof(tot);
        total_memory += sizeof(num);

        // 计算数组的内存大小
        total_memory += (TD_MAX_FLOW_SIZE + 10) * sizeof(int);         // ID数组
        total_memory += (TD_MAX_FLOW_NUM + 10) * sizeof(headnode);       // head数组
        total_memory += (TD_MAX_FLOW_SIZE + 10) * sizeof(linknode);      // node数组
        total_memory += (TD_Max_Hash + 10) * sizeof(int);           // hhead数组
        total_memory += (TD_MAX_FLOW_SIZE + 10) * sizeof(int);      // hnext数组

        // 计算 bobhash 对象的内存
        total_memory += sizeof(*bobhash);

        return total_memory;
    }
};
#endif