#ifndef  DIR248_H
#define  DIR248_H

#include "../../Elements/Elements.h"
#include "../../Tools/Tools.h"
#include "../Classifier.h"

using namespace std;

// 优化后：16字节大小，16字节对齐
typedef struct TableEntry {
    uint32_t next_hop;          // 4字节（下一跳端口）
    uint8_t  has_subtable;      // 1字节（是否有子表）
    uint8_t  is_valid;          // 1字节（条目有效性）
    uint8_t  padding[2];        // 填充2字节（4+1+1+2=8，凑齐前8字节）
    struct DirTable* subtable;  // 8字节（子表指针，64位系统）
} __attribute__((aligned(16))) TableEntry;  // 强制16字节对齐

typedef struct DirTable {
    uint32_t entry_count;   // 4字节（条目数量）
    uint32_t mask;          // 4字节（掩码，与entry_count配套）
    TableEntry* entries;    // 8字节（条目数组指针，高频访问成员）
    uint8_t  stride;        // 1字节（步长，低频访问放后面）
    uint8_t  padding[15];   // 填充15字节（4+4+8+1+15=32，凑齐32字节）
} __attribute__((aligned(64))) DirTable;  // 强制64字节对齐

class DIR248 : public Classifier {
public:
    DIR248();
    void Create(vector<Prefix*> &prefixs, ProgramState *ps);   
    uint32_t Lookup(Trace *trace, ProgramState *ps);
    uint32_t Lookup(Trace *trace);       
    uint64_t CalMemory();                              
    ~DIR248();

private:
    DirTable* root;            // DIR248 根目录表（步长24）
};

class DIR248_TD : public Classifier {
public:
    DIR248_TD();
    void Create(vector<Prefix*> &prefixs, ProgramState *ps);   
    uint32_t Lookup(Trace *trace, ProgramState *ps);
    uint32_t Lookup(Trace *trace);       
    uint64_t CalMemory();                              
    ~DIR248_TD();

private:
    DirTable* root;            // DIR248 根目录表（步长24）
};
#endif