#ifndef POPTRIE_H
#define POPTRIE_H

#include "../../Elements/Elements.h"
#include "../../Tools/Tools.h"
#include "../Classifier.h"

using namespace std;

class Poptrie : public Classifier {
public:
    Poptrie();
    ~Poptrie();

    void Create(vector<Prefix*> &prefixes, ProgramState *ps) override;
    uint32_t Lookup(Trace *trace, ProgramState *ps) override;
    uint32_t Lookup(Trace *trace) override;
    uint64_t CalMemory() override;

private:
    struct InternalNode {
        uint8_t stride;            
        uint64_t bitVector[8];      
        uint32_t* directResult;     
        InternalNode* childNodes;   
    };

    struct DirectPointer {
        uint32_t* indexTable;     
        uint32_t* directResult;   
        InternalNode* internalNodes;
    } topLevel;

    static const uint8_t TOP_LEVEL_STRIDE; 
    uint64_t totalMemory;                  

    void destroyInternalNode(InternalNode* node);
    void clear();

    void buildTopLevel(const vector<Prefix*>& prefixes);
    void buildInternalNode(InternalNode* node, const vector<Prefix*>& prefixes, uint8_t currentDepth);
    unordered_map<uint32_t, vector<Prefix*>> groupPrefixesByRange(
        const vector<Prefix*>& prefixes, uint8_t currentDepth, uint8_t stride);
    inline uint32_t extractBits(__uint128_t ip, uint8_t offset, uint8_t length) const;
    inline bool isBitSet(const InternalNode* node, uint32_t index) const;
    inline void setBit(InternalNode* node, uint32_t index);
    inline uint32_t countSetBits(const InternalNode* node, uint32_t index) const;
};

#endif // POPTRIE_H