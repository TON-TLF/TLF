#include "ABST.h"

using namespace std;

/***************************************
*                AbstNode              *
***************************************/
AbstNode::AbstNode(){
    prefix_len = 0;
    table = NULL;
    left = right = NULL;
}

AbstNode::AbstNode(uint8_t _prefix_len){
    prefix_len = _prefix_len;
    table = HashTable_create();
    left = right = NULL;
}


/***************************************
*                  ABST                *
***************************************/
// 前缀统计节点
typedef struct PrefixNode {
    uint8_t prefix_len;
    double prefix_score;
    double trace_score;
} PrefixNode;

bool cmp_for_abst(const PrefixNode &a, const PrefixNode &b) {
    return a.prefix_score > b.prefix_score;
}

bool cmp_for_abst_td(const PrefixNode &a, const PrefixNode &b) {
    return a.prefix_score * 0.5 + a.trace_score * 0.5 > b.prefix_score * 0.5 + b.trace_score * 0.5; 
}

// 递归构建ABST树（返回子树根节点）
AbstNode* build_tree(vector<PrefixNode> nodes) {
    if (nodes.size() == 0) return NULL;
    
    // 选择当前最优节点作为根（已排序，首元素即最优）
    AbstNode* root = new AbstNode(nodes[0].prefix_len);
    
    // 划分左右子树（左：更短前缀，右：更长前缀）
    vector<PrefixNode> left, right;
    for (size_t i = 1; i < nodes.size(); ++i) {
        if (nodes[i].prefix_len < root->prefix_len) left.push_back(nodes[i]);
        else right.push_back(nodes[i]);
    }
    
    // 递归构建子树（保持降序特性）
    root->left = build_tree(left);
    root->right = build_tree(right);
    return root;
}

void ABST_insert_prefix(AbstNode* root, AbstTrieNode* aux, Entry entry) {
    AbstNode* node = root;
    // printf("ABST_insert_prefix: %016llx%016llx\n",
    //     (unsigned long long)(entry.prefix >> 64), 
    //     (unsigned long long)(entry.prefix & 0xFFFFFFFFFFFFFFFF));
    // printf("ABST_insert_prefix : %d %d\n",entry.prefix_len ,node->prefix_len);
    // printf("%d\n",entry.prefix_len == node->prefix_len);

    while (node) {
        // printf("123");
        if (entry.prefix_len < node->prefix_len) {
            // printf("left\n");
            node = node->left;
        } else if (entry.prefix_len == node->prefix_len){
            // if(entry.prefix_len == 0)
            //     printf("insert markey\n");  
            HashTable_insert(node->table, entry);
            break;
        } else if (entry.prefix_len > node->prefix_len){
            // 插入标记条目
            Entry marker = {
                .label = MARKER,
                .port = entry.port,
                .prefix_len = node->prefix_len,
                .prefix = trim_prefix(entry.prefix, node->prefix_len), 
                .bmp = NULL
            };
            marker.bmp = AbstTrie_lookup_bmp(aux, marker);
            if(marker.bmp == NULL){
                cout << "ABST_insert_prefix() : marker.bmp is NULL!!!" << endl;
                exit(1);
            }
            // printf("insert perfix\n");
            HashTable_insert(node->table, marker);
            
            node = node->right;
        }
    }
}

ABST::ABST(){
    root = NULL;
    aux = NULL;
}

void ABST::Create(vector<Prefix*> &prefixs, ProgramState *ps){
    int prefixs_num = prefixs.size();
    if(prefixs_num == 0){
        cout << "ABST::Create() : prefixs is empty!!!" << endl;
        return;
    }
    
    /** 构建辅助结构体 */
    aux = AbstTrieNode_create();
    for (size_t i = 0; i < prefixs_num; i++) {
        Entry entry = {
            .label = PREFIX,
            .port = prefixs[i]->port,
            .prefix_len = prefixs[i]->prefix_len,
            .prefix = trim_prefix(
                ((__uint128_t)((__uint128_t)prefixs[i]->ip6_upper << 64) | (__uint128_t)prefixs[i]->ip6_lower), 
                prefixs[i]->prefix_len
            ),
            .bmp = NULL
        };
        AbstTrie_insert(aux, entry);
    }
    
    /** 统计前缀长度 */ 
    int prefix_cnt[129] = {0}; // 0-128
    for (size_t i = 0; i < prefixs_num; i++) {
        prefix_cnt[prefixs[i]->prefix_len]++;
    }

    vector<PrefixNode> prefixNodes;
    for (int i = 0; i <= 128; i++) {
        if (prefix_cnt[i] != 0) {
            prefixNodes.push_back((PrefixNode){(uint8_t)i, 1.0 * prefix_cnt[i], 0.0});
        }
    }

    sort(prefixNodes.begin(), prefixNodes.end(),cmp_for_abst);
    // for(auto node : prefixNodes){
    //     printf("prefix_len : %d  \tnum = %d\n",node.prefix_len ,node.count);
    // }

    root = build_tree(prefixNodes);
    for (size_t i = 0; i < prefixs_num; i++) {
        Entry entry = {
            .label = PREFIX,
            .port = prefixs[i]->port,
            .prefix_len = prefixs[i]->prefix_len,
            .prefix = trim_prefix(
                ((__uint128_t)((__uint128_t)prefixs[i]->ip6_upper << 64) | (__uint128_t)prefixs[i]->ip6_lower), 
                prefixs[i]->prefix_len
            ),
            .bmp = NULL
        };
        ABST_insert_prefix(root, aux, entry);

    }
}

uint32_t ABST::Lookup(Trace *trace, ProgramState *ps){
    __uint128_t ip = ((__uint128_t)trace->ip6_upper << 64) | trace->ip6_lower;
    AbstNode *node = root;
    uint32_t result = 0;
    // printf("Lookup Begin!\n");
    // printf("Looking up prefix: %016llx%016llx",
    //             (unsigned long long)(ip >> 64), 
    //             (unsigned long long)(ip & 0xFFFFFFFFFFFFFFFF));
    // cout<<endl;
    while(node != NULL) {
        // printf("%d",node->prefix_len);
        // cout<<endl;
        // 在当前节点的哈希表中查找截断后的前缀
        Entry* temp = HashTable_lookup(node->table, trim_prefix(ip, node->prefix_len), ps);
		ps->lookup_access.Addcount();
        ps->lookup_depth.Addcount();

        if(temp == NULL){
            node = node->left;   // 向左子树（更短前缀方向）查找
            continue;
        } 
        
        bool markey_hit = (temp->label & MARKER) != 0;
        bool prefix_hit = (temp->label & PREFIX) != 0;
        
        // if(markey_hit){
        //     printf("markey_hit\n");
        //     cout<<endl;
        //     printf("Looking up prefix: %016llx%016llx\n",
        //         (unsigned long long)(temp->prefix >> 64), 
        //         (unsigned long long)(temp->prefix & 0xFFFFFFFFFFFFFFFF));
        //     cout<<endl;
        //     printf("Looking up prefix: %016llx%016llx\n",
        //         (unsigned long long)(temp->bmp->prefix >> 64), 
        //         (unsigned long long)(temp->bmp->prefix & 0xFFFFFFFFFFFFFFFF));
        //     cout<<endl;
        // } 
        // if(prefix_hit) printf("prefix_hit");
        // printf("\n");

        if(markey_hit && prefix_hit){
            result = temp->port;
            node = node->right;  // 向右子树（更长前缀方向）查找
        } else if(markey_hit){
            result = temp->bmp->port;
            node = node->right;  // 向右子树（更长前缀方向）查找
        } else if(prefix_hit){
            result = temp->port;
            break;
        }
    }

	ps->lookup_access.Cal();
    ps->lookup_depth.Cal();
    ps->lookup_access_entry.Cal();
    return result;
}

uint32_t ABST::Lookup(Trace *trace){
    __uint128_t ip = ((__uint128_t)trace->ip6_upper << 64) | trace->ip6_lower;
    AbstNode *node = root;
    uint32_t result = 0;
    
    while(node != NULL) {
        Entry* temp = HashTable_lookup(node->table, trim_prefix(ip, node->prefix_len));
        if(temp == NULL){
            node = node->left;   // 向左子树（更短前缀方向）查找
            continue;
        } 
        
        bool markey_hit = (temp->label & MARKER) != 0;
        bool prefix_hit = (temp->label & PREFIX) != 0;

        if(markey_hit && prefix_hit){
            result = temp->port;
            node = node->right;  // 向右子树（更长前缀方向）查找
        } else if(markey_hit){
            result = temp->bmp->port;
            node = node->right;  // 向右子树（更长前缀方向）查找
        } else if(prefix_hit){
            result = temp->port;
            break;
        }
    }

    return result;
}

uint64_t calculateAbstNodeMemory(AbstNode* node) {
    if (node == nullptr) {
        return 0;
    }
    
    uint64_t total = 0;
    
    // 1. 当前节点本身的大小
    total += sizeof(AbstNode);
    
    // 2. 节点中哈希表的内存开销（如果存在）
    if (node->table != nullptr) {
        total += HashTable_cal_memory(node->table);
    }
    
    // 3. 递归计算左子树和右子树
    total += calculateAbstNodeMemory(node->left);
    total += calculateAbstNodeMemory(node->right);
    
    return total;
}

uint64_t ABST::CalMemory(){
    uint64_t total = 0;
    
    // 1. ABST对象本身的大小（包含root和aux指针等成员）
    total += sizeof(AbstNode *);
    
    // 2. 计算整个AbstNode二叉树的内存开销（包含所有节点和哈希表）
    total += calculateAbstNodeMemory(root);

    return total;
}

ABST::~ABST(){}

/***************************************
*               ABST_TD                *
***************************************/
ABST_TD::ABST_TD(){
    root = NULL;
    aux = NULL;
}

void ABST_TD::Create(vector<Prefix*> &prefixs, ProgramState *ps){
    int prefixs_num = prefixs.size();
    if(prefixs_num == 0){
        cout << "ABST::Create() : prefixs is empty!!!" << endl;
        return;
    }
    
    /** 构建辅助结构体 */
    aux = AbstTrieNode_create();

    for (size_t i = 0; i < prefixs_num; i++) {
        Entry entry = {
            .label = PREFIX,
            .port = prefixs[i]->port,
            .prefix_len = prefixs[i]->prefix_len,
            .prefix = trim_prefix(
                ((__uint128_t)((__uint128_t)prefixs[i]->ip6_upper << 64) | (__uint128_t)prefixs[i]->ip6_lower), 
                prefixs[i]->prefix_len
            ),
            .bmp = NULL
        };
        AbstTrie_insert(aux, entry);
    }
    
    /** 统计前缀长度 */ 
    int prefix_cnt[129] = {0}; // 0-128
    for (size_t i = 0; i < prefixs_num; i++) {
        prefix_cnt[prefixs[i]->prefix_len]++;
    }

    /* 查询TopK匹配*/
    int total_count = 0;
    int trace_cnt[129] = {0}; // 0-128
    for(auto it : TSL::TopKStats){
        uint8_t ret = AbstTrie_lookup_lpm(aux, (__uint128_t)it.addr);
        trace_cnt[ret] += it.count;
        total_count += it.count;
    }

    /** 生成节点 */ 
    vector<PrefixNode> prefixNodes;
    for (int i = 0; i <= 128; i++) {
        if (prefix_cnt[i] != 0) {
            // prefixNodes.push_back((PrefixNode){
            //     .prefix_len = (uint8_t)i, 
            //     .prefix_score = prefix_cnt[i] / (1.0 * prefixs_num),
            //     .trace_score = TSL::abstPrefixSorce[i]}
            // );
            prefixNodes.push_back((PrefixNode){
                .prefix_len = (uint8_t)i, 
                .prefix_score = prefix_cnt[i] / (1.0 * prefixs_num),
                .trace_score = total_count == 0 ? 0.0 : trace_cnt[i] / (1.0 * total_count)}
            );
        }
    }

    sort(prefixNodes.begin(), prefixNodes.end(),cmp_for_abst_td);
    // for(auto node : prefixNodes){
    //     printf("prefix_len : %d\tnum = %d\thit = %d\n",node.prefix_len ,node.num, node.hit_count);
    // }

    root = build_tree(prefixNodes);
    for (size_t i = 0; i < prefixs_num; i++) {
        Entry entry = {
            .label = PREFIX,
            .port = prefixs[i]->port,
            .prefix_len = prefixs[i]->prefix_len,
            .prefix = trim_prefix(
                ((__uint128_t)((__uint128_t)prefixs[i]->ip6_upper << 64) | (__uint128_t)prefixs[i]->ip6_lower), 
                prefixs[i]->prefix_len
            ),
            .bmp = NULL
        };
        ABST_insert_prefix(root, aux, entry);

    }
}

uint32_t ABST_TD::Lookup(Trace *trace, ProgramState *ps){
    __uint128_t ip = ((__uint128_t)trace->ip6_upper << 64) | trace->ip6_lower;
    AbstNode *node = root;
    uint32_t result = 0;
    

    while(node != NULL) {
        // printf("%d\n",node->prefix_len);
        // 在当前节点的哈希表中查找截断后的前缀
        Entry* temp = HashTable_lookup(node->table, trim_prefix(ip, node->prefix_len), ps);
		ps->lookup_access.Addcount();
        ps->lookup_depth.Addcount();

        if(temp == NULL){
            node = node->left;   // 向左子树（更短前缀方向）查找
            continue;
        } 
        
        bool markey_hit = (temp->label & MARKER) != 0;
        bool prefix_hit = (temp->label & PREFIX) != 0;
        
        // if(markey_hit){
        //     printf("markey_hit\n");
        //     printf("Looking up prefix: %016llx%016llx\n",
        //         (unsigned long long)(temp->prefix >> 64), 
        //         (unsigned long long)(temp->prefix & 0xFFFFFFFFFFFFFFFF));
        //     printf("Looking up prefix: %016llx%016llx\n",
        //         (unsigned long long)(temp->bmp->prefix >> 64), 
        //         (unsigned long long)(temp->bmp->prefix & 0xFFFFFFFFFFFFFFFF));
        // } 
        // if(prefix_hit) printf("prefix_hit");
        // printf("\n");

        if(markey_hit && prefix_hit){
            result = temp->port;
            node = node->right;  // 向右子树（更长前缀方向）查找
        } else if(markey_hit){
            result = temp->bmp->port;
            node = node->right;  // 向右子树（更长前缀方向）查找
        } else if(prefix_hit){
            result = temp->port;
            break;
        }
    }

	ps->lookup_access.Cal();
    ps->lookup_depth.Cal();
    ps->lookup_access_entry.Cal();
    return result;
}

uint32_t ABST_TD::Lookup(Trace *trace){
    __uint128_t ip = ((__uint128_t)trace->ip6_upper << 64) | trace->ip6_lower;
    AbstNode *node = root;
    uint32_t result = 0;
    
    while(node != NULL) {
        Entry* temp = HashTable_lookup(node->table, trim_prefix(ip, node->prefix_len));
        if(temp == NULL){
            node = node->left;   // 向左子树（更短前缀方向）查找
            continue;
        } 
        
        bool markey_hit = (temp->label & MARKER) != 0;
        bool prefix_hit = (temp->label & PREFIX) != 0;
        
        if(markey_hit && prefix_hit){
            result = temp->port;
            node = node->right;  // 向右子树（更长前缀方向）查找
        } else if(markey_hit){
            result = temp->bmp->port;
            node = node->right;  // 向右子树（更长前缀方向）查找
        } else if(prefix_hit){
            result = temp->port;
            break;
        }
    }

    return result;
}

uint64_t ABST_TD::CalMemory(){
    uint64_t total = 0;
    
    // 1. ABST对象本身的大小（包含root和aux指针等成员）
    total += sizeof(AbstNode *);
    
    // 2. 计算整个AbstNode二叉树的内存开销（包含所有节点和哈希表）
    total += calculateAbstNodeMemory(root);

    return total;
}

ABST_TD::~ABST_TD(){}