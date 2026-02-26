#ifndef  ABST_H
#define  ABST_H

#include "../../Elements/Elements.h"
#include "../../Tools/Tools.h"
#include "../../TraceStats/TraceStats.h"
#include "../Classifier.h"
#include "HashTable.h"
#include "AbstTrie.h"

using namespace std;

/* 节点结构体 */
class AbstNode {
public:
    uint8_t prefix_len;
    HashTable* table;
    AbstNode* left;
    AbstNode* right;

    AbstNode();
    AbstNode(uint8_t prefix_len);
};

class ABST : public Classifier {
public:
    ABST();
    void Create(vector<Prefix*> &prefixs, ProgramState *ps);   
    uint32_t Lookup(Trace *trace, ProgramState *ps);
    uint32_t Lookup(Trace *trace);       
    uint64_t CalMemory();                              
    ~ABST();

private:
    AbstNode *root;
    AbstTrieNode *aux;
};

class ABST_TD : public Classifier {
public:
    ABST_TD();
    void Create(vector<Prefix*> &prefixs, ProgramState *ps);   
    uint32_t Lookup(Trace *trace, ProgramState *ps);
    uint32_t Lookup(Trace *trace);       
    uint64_t CalMemory();                              
    ~ABST_TD();

private:
    AbstNode *root;
    AbstTrieNode *aux;
};
#endif