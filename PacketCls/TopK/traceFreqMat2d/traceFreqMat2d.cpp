#include "traceFreqMat2d.h"

vector<uint32_t> traceFreqMat2d::key[2];
Eigen::SparseMatrix<uint32_t> traceFreqMat2d::mat;

double traceFreqMat2d::getTracenum2D(uint32_t bounds[][2]){
    uint32_t lid[2],rid[2];
    for (int d = 0; d < 2; ++d){
        lid[d] = lower_bound(traceFreqMat2d::key[d].begin(),traceFreqMat2d::key[d].end(),bounds[d][0]) - traceFreqMat2d::key[d].begin();
        rid[d] = upper_bound(traceFreqMat2d::key[d].begin(),traceFreqMat2d::key[d].end(),bounds[d][1]) - traceFreqMat2d::key[d].begin();
    }
    double traces_sum = 0;
    for(int a = lid[0]; a < rid[0]; a++){
        for(int b = lid[1]; b < rid[1]; b++){
            traces_sum += traceFreqMat2d::getValue(a,b);
        }
    }
    return traces_sum;
}

void traceFreqMat2d::init(vector<TraceFreq> &topktraces){
    set<uint32_t> s[2];
    int topktraces_num = topktraces.size();
	for(int i = 0; i < topktraces_num; ++i){
		for(int d = 0; d < 2; ++d){
			s[d].insert(topktraces[i].trace.key[d]);
		}
	}
    
	for(int d = 0; d < 2; ++d){
		for(auto it : s[d]){
            traceFreqMat2d::key[d].push_back(it);
		}
	}
    
    mat.resize(traceFreqMat2d::key[0].size() + 1000, traceFreqMat2d::key[1].size() + 1000);

    uint32_t tkey[2];
    for(int i = 0; i < topktraces_num; ++i){
        tkey[0] = topktraces[i].trace.key[0];
        tkey[1] = topktraces[i].trace.key[1];
        uint32_t xid = lower_bound(traceFreqMat2d::key[0].begin(),traceFreqMat2d::key[0].end(),tkey[0]) - traceFreqMat2d::key[0].begin();
        uint32_t yid = lower_bound(traceFreqMat2d::key[1].begin(),traceFreqMat2d::key[1].end(),tkey[1]) - traceFreqMat2d::key[1].begin();
        traceFreqMat2d::insert(xid, yid, topktraces[i].freq);
	}
}

uint64_t traceFreqMat2d::CalMemory() {
    uint64_t size = 0;

    for (int i = 0; i < 2; ++i) {
        size += sizeof(key[i]);  
        size += key[i].capacity() * sizeof(uint32_t); 
    }

    size += sizeof(mat);  
    size += mat.nonZeros() * sizeof(uint32_t); 
    size += mat.nonZeros() * sizeof(int);              
    size += mat.outerSize() * sizeof(int);              

    return size;
}

void traceFreqMat2d::clear(){
    traceFreqMat2d::key[0].clear();
    traceFreqMat2d::key[1].clear();
    traceFreqMat2d::mat.resize(0, 0);
}