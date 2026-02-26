#include "ProgramState.h"

using namespace std;

/***************************************************************/
/*                         CountState                          */
/***************************************************************/
/**
 * @brief 默认构造函数
 */
CountState::CountState(){ 
    count = cnt = calNum = 0;
	maxn = avg = 0;
    minn = 1e9;
}

/**
 * @brief 增加单次访问次数
 */
void CountState::Addcount() { ++cnt; } 

/**
 * @brief 计算最大最小平均值
 */
void CountState::Cal() {
    maxn = max(maxn,cnt);
    minn = min(minn,cnt);
    count += cnt;
    cnt = 0;
    calNum++;
    avg = (double)count / calNum;
}

/**
 * @brief 清除当前访问信息  
 */
void CountState::Clear() { 
    count = cnt = calNum = 0;
	maxn = avg = 0;
    minn = 1e9;
}

/***************************************************************/
/*                        ProgramState                         */
/***************************************************************/


/**
 * @brief 默认构造函数
 */
ProgramState::ProgramState(){
    prefixs_num = traces_num = 0;         
	rules_memory_size = traces_memory_size = sketch_memory_size = topk_tracesFreq_memory_size = TracesMat_memory_size = tree_memory_size = total_memory_size = 0;              
    sketch_build_and_update_time = sketch_calculate_topk_time = topk_tracesMat_init_time = rules_analyze_time = tree_build_time = total_build_time = 0; 
    throughput = 0;

    avg_insert_time = 0;
    avg_lookup_time = 0;
    avg_lookup_access = max_lookup_access = 0;
    avg_lookup_depth = 0;
    avg_lookup_access_entry = max_lookup_access_entry = 0;
}

/**
 * @brief 计算统计信息
 */
void ProgramState::CalInfo(){
    avg_lookup_access = lookup_access.avg;
    max_lookup_access = lookup_access.maxn;
    avg_lookup_depth = lookup_depth.avg;
    avg_lookup_access_entry = lookup_access_entry.avg;
    max_lookup_access_entry = lookup_access_entry.maxn;
}

/**
 * @brief 清除当前统计信息
 */
void ProgramState::Clear(){
    prefixs_num = traces_num = 0;         

	rules_memory_size = traces_memory_size = sketch_memory_size = topk_tracesFreq_memory_size = TracesMat_memory_size = tree_memory_size = total_memory_size = 0;              
    sketch_build_and_update_time = sketch_calculate_topk_time = topk_tracesMat_init_time = rules_analyze_time = tree_build_time = total_build_time = 0; 
    throughput = 0;

    avg_insert_time = 0;
    avg_lookup_time = 0;
    avg_lookup_access = max_lookup_access = 0;
    avg_lookup_depth = 0;
    avg_lookup_access_entry = max_lookup_access_entry = 0;

	lookup_access_entry.Clear(); 
	lookup_access.Clear(); 
    lookup_depth.Clear();
}

/**
 * @brief 清除当前统计信息
 */
void ProgramState::ClearAccess(){
	lookup_access_entry.Clear(); 
	lookup_access.Clear(); 
    lookup_depth.Clear();
}
/**
 * @brief 输出当前统计信息到指定文件
 */
void ProgramState::Print(){
    FILE *fp = fopen(Command::output_file.c_str(), "w");
    fprintf(fp, "prefixs_filename: %s\n", Command::prefixs_file.c_str());
    fprintf(fp, "traces_filename:  %s\n", Command::traces_file.c_str());
    fprintf(fp, "method_name:      %s\n\n", Command::method_name.c_str());

    fprintf(fp, "prefixs_num:    %d\n", prefixs_num);
    fprintf(fp, "traces_num:     %d\n", traces_num);

    fprintf(fp, "rules_memory_size:           %.8lf MB\n", rules_memory_size);
    fprintf(fp, "traces_memory_size:          %.8lf MB\n", traces_memory_size);
    fprintf(fp, "sketch_memory_size:          %.8lf MB\n", sketch_memory_size);
    fprintf(fp, "topk_tracesFreq_memory_size: %.8lf MB\n", topk_tracesFreq_memory_size);
    fprintf(fp, "TracesMat_memory_size:       %.8lf MB\n", TracesMat_memory_size);
    fprintf(fp, "tree_memory_size:            %.8lf MB\n", tree_memory_size);
    fprintf(fp, "total_memory_size:           %.8lf MB\n\n", total_memory_size);

    fprintf(fp, "sketch_build_and_update_time: %.8lf S\n", sketch_build_and_update_time);
    fprintf(fp, "sketch_calculate_topk_time:   %.8lf S\n", sketch_calculate_topk_time);
    fprintf(fp, "topk_tracesMat_init_time:     %.8lf S\n", topk_tracesMat_init_time);
    fprintf(fp, "rules_analyze_time:           %.8lf S\n", rules_analyze_time);
    fprintf(fp, "tree_build_time:              %.8lf S\n", tree_build_time);
    fprintf(fp, "total_build_time:             %.8lf S\n\n", total_build_time);

    fprintf(fp, "avg_lookup_time  :        %.8lf\n", avg_lookup_time);
    fprintf(fp, "avg_insert_time:          %.8lf US\n", avg_insert_time);
    fprintf(fp, "throughput:               %.8lf pps\n\n", throughput);

    fprintf(fp, "avg_lookup_access:        %.8lf\n", avg_lookup_access);
    fprintf(fp, "max_lookup_access:        %.0lf\n", max_lookup_access);
    fprintf(fp, "avg_lookup_depth:         %.8lf\n", avg_lookup_depth);

    fprintf(fp, "avg_lookup_access_entry:  %.8lf\n", avg_lookup_access_entry);
    fprintf(fp, "max_lookup_access_entry:  %.0lf\n\n", max_lookup_access_entry);
    
    fclose(fp);
    fflush(stdout);
}