#ifndef  CLASSIFIER_H
#define  CLASSIFIER_H

#include "../Elements/Elements.h"
#include "../TraceStats/TraceStats.h"

using namespace std;

class Classifier {
public:
    virtual void Create(vector<Prefix*> &prefixs, ProgramState *ps) = 0; 
    virtual uint32_t Lookup(Trace *trace, ProgramState *ps) = 0;         
    virtual uint32_t Lookup(Trace *trace) = 0;                          
    virtual uint64_t CalMemory() = 0;                                   
};

#endif