#ifndef  TRACEFREQMAT2D_H
#define  TRACEFREQMAT2D_H

#include "../../Elements/Elements.h"
#include <eigen3/Eigen/Dense>
#include <eigen3/Eigen/Sparse>

using namespace std;

/**
 * @brief 流量频数矩阵，存储每个流量频数的矩阵
 */
class traceFreqMat2d{
public:
    /** key[i]存储所有流量第i维度去重后的所有key值，并且从小到大排序 */
    static vector<uint32_t> key[2];  

    /** 流量频数矩阵 */
    static Eigen::SparseMatrix<uint32_t> mat;

    /** 设置稀疏矩阵大小 */
    static void init(vector<TraceFreq> &topktraces);

    /** 插入新的流量 */
    static void insert(uint32_t row, uint32_t col,uint32_t value){
        mat.coeffRef(row, col) = value;
    }

    /** 获取某个流量的频数 */
    static uint32_t getValue(uint32_t row, uint32_t col){
        return mat.coeff(row, col);
    }

    /** 获取在子空间范围里的流量的频数之和 */
    static double getTracenum2D(uint32_t bounds[][2]);

    static uint64_t CalMemory();

    static void clear();
};
#endif