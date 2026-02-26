#ifndef  PROGRAMSTATE_H
#define  PROGRAMSTATE_H

#include "Common.h"
#include "Command.h"

using namespace std;

/**
 * @brief 次数类，用于记录各种访问次数和相关统计数据
 */
class CountState {
public:
	long long count;   /** 总次数 */ 
	long long cnt;     /** 单次次数 */ 
	long long calNum;  /** 统计次数 */ 
	long long maxn;    /** 单次最大次数 */  
	long long minn;    /** 单次最小次数 */  
	double    avg;     /** 平均单次次数 */  

    CountState();      /** 默认构造函数 */
	void Addcount();   /** 增加次数 */
	void Cal();        /** 计算最大最小平均值 */
	void Clear();      /** 清除当前统计信息 */
};

/**
 * @brief 程序状态类，用于记录程序执行过程中的各种状态信息
 */
class ProgramState {
public:
	int prefixs_num;                          /** 规则数 */ 
	int traces_num;                         /** 流量数 */ 
	
	double rules_memory_size;               /** 规则内存大小（MB） */
	double traces_memory_size;              /** 流量内存大小（MB） */
	double sketch_memory_size;              /** 流量统计模块大小（MB） */
	double topk_tracesFreq_memory_size;     /** topk流量内存大小（MB） */
	double TracesMat_memory_size;           /** topk流量频数矩阵内存大小（MB） */
	double tree_memory_size;                /** 决策树内存大小（MB） */
	double total_memory_size;               /** 总内存大小（MB） */

	double sketch_build_and_update_time;    /** 构建时间（秒）*/
	double sketch_calculate_topk_time;      /** 构建时间（秒）*/
	double topk_tracesMat_init_time;        /** 构建时间（秒）*/
	double rules_analyze_time;              /** 构预处理时间（秒）*/ 
	double tree_build_time;                 /** 构建时间（秒）*/
	double total_build_time;                /** 构建时间（秒）*/ 

	double avg_lookup_time;
	double avg_insert_time;
	double throughput;

	CountState lookup_access_entry;         /** 查询时规则匹配次数 */ 
	CountState lookup_access;				/** 查询时访存次数 */
	CountState lookup_depth;				/** 查询时访存深度 */

	double avg_lookup_access_entry;         /** 查询时平均规则匹配次数 */
	double max_lookup_access_entry;         /** 查询时最大规则匹配次数 */
	double avg_lookup_access;               /** 平均访存次数 */
	double max_lookup_access;               /** 最大访存次数 */
	double avg_lookup_depth;

	ProgramState();                         /** 默认构造函数 */
	void CalInfo();                         /** 计算统计信息 */
	void Clear();                           /** 清除当前所有统计信息 */
	void ClearAccess();                     /** 清除当前访存的统计信息 */
	void Print();                           /** 输出当前统计信息到指定文件 */
};

#endif