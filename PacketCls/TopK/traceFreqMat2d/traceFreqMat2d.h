#ifndef  TRACEFREQMAT2D_H
#define  TRACEFREQMAT2D_H

#include "../../Elements/Elements.h"
#include <eigen3/Eigen/Dense>
#include <eigen3/Eigen/Sparse>

using namespace std;

class traceFreqMat2d{
public:
    static vector<uint32_t> key[2];  
    static Eigen::SparseMatrix<uint32_t> mat;
    static void init(vector<TraceFreq> &topktraces);
    static void insert(uint32_t row, uint32_t col,uint32_t value){
        mat.coeffRef(row, col) = value;
    }
    static uint32_t getValue(uint32_t row, uint32_t col){
        return mat.coeff(row, col);
    }
    static double getTracenum2D(uint32_t bounds[][2]);
    static uint64_t CalMemory();
    static void clear();
};
#endif