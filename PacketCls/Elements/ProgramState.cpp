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
/*                      DecisionTreeInfo                       */
/***************************************************************/
/**
 * @brief 默认构造函数  
 */
DecisionTreeInfo::DecisionTreeInfo(){
	tree_memory_size = 0;
}

/**
 * @brief 统计分割维度信息  
 * @param dim1 选择的第一个维度
 * @param dim2 选择的第二个维度
 */
void DecisionTreeInfo::AddDims(int dim1, int dim2){
    dimInfo[dim1]++;
    dimInfo[dim2]++;
}

/**
 * @brief 增加内部节点  
 * @param depth 要增加节点的深度
 */
void DecisionTreeInfo::AddNode(int depth){
    depthInfo[depth]++;
}

/**
 * @brief 增加叶子节点  
 * @param depth 要增加节点的深度
 */
void DecisionTreeInfo::AddNode(int depth, int rules_num){
    depthInfo[depth]++;
    leafInfo[rules_num]++;
}

/**
 * @brief 输出当前统计信息到指定文件  
 */
void DecisionTreeInfo::Print(FILE *fp){
    fprintf(fp, "dim chosen times:\n");
    for(int d = 0; d < 5; d++){
        fprintf(fp, "dim %d : %d\n", d, dimInfo[d]);
    }

    fprintf(fp, "\ndepth node numbers:\n");
    for(auto it : depthInfo){
        fprintf(fp, "depth %d : %d\n", it.first, it.second);
    }

    fprintf(fp, "\nleafnode Info:\n");
    for(auto it : leafInfo){
        fprintf(fp, "rules_num %d : %d\n", it.first, it.second);
    }
}

/**
 * @brief 清除当前统计信息  
 */
void DecisionTreeInfo::Clear(){
    tree_memory_size = 0;
    depthInfo.clear();
    dimInfo.clear();
    leafInfo.clear();
}

/***************************************************************/
/*                        ProgramState                         */
/***************************************************************/


/**
 * @brief 默认构造函数
 */
ProgramState::ProgramState(){
    rules_num = traces_num = topk_traces_num = 0;         
	rules_memory_size = traces_memory_size = sketch_memory_size = topk_tracesFreq_memory_size = TracesMat_memory_size = total_memory_size = 0;              
    sketch_build_and_update_time = sketch_calculate_topk_time = topk_tracesMat_init_time = rules_analyze_time = tree_build_time = total_build_time = 0; 

    avg_insert_time = 0;
    avg_lookup_time = 0;

    avg_lookup_access = max_lookup_access = 0;
    avg_lookup_depth = 0;
    avg_lookup_access_rules = max_lookup_access_rules = 0;
}

/**
 * @brief 计算统计信息
 */
void ProgramState::CalInfo(){
    avg_lookup_access = lookup_access.avg;
    max_lookup_access = lookup_access.maxn;
    avg_lookup_depth = lookup_depth.avg;
    avg_lookup_access_rules = lookup_access_rules.avg;
    max_lookup_access_rules = lookup_access_rules.maxn;
}

/**
 * @brief 清除当前统计信息
 */
void ProgramState::Clear(){
    rules_num = traces_num = topk_traces_num = 0;         

	rules_memory_size = traces_memory_size = sketch_memory_size = topk_tracesFreq_memory_size = TracesMat_memory_size = total_memory_size = 0;              
    sketch_build_and_update_time = sketch_calculate_topk_time = topk_tracesMat_init_time = rules_analyze_time = tree_build_time = total_build_time = 0; 
    
    avg_insert_time = 0;
    avg_lookup_time = 0;

    avg_lookup_access = max_lookup_access = 0;
    avg_lookup_depth = 0;
    avg_lookup_access_rules = max_lookup_access_rules = 0;

    lookup_access_nodes.Clear(); 
	lookup_access_rules.Clear(); 
	lookup_access.Clear(); 
    lookup_depth.Clear();
    DTI.Clear();
}

/**
 * @brief 清除当前统计信息
 */
void ProgramState::ClearAccess(){
    lookup_access_nodes.Clear(); 
	lookup_access_rules.Clear(); 
	lookup_access.Clear(); 
    lookup_depth.Clear();
}
/**
 * @brief 输出当前统计信息到指定文件
 */
void ProgramState::Print(){
    FILE *fp = fopen(Command::output_file.c_str(), "w");
    fprintf(fp, "rules_filename: %s\n", Command::rules_file.c_str());
    fprintf(fp, "method_name:    %s\n\n", Command::method_name.c_str());

    fprintf(fp, "rules_num:       %d\n", rules_num);
    fprintf(fp, "traces_num:      %d\n", traces_num);
    fprintf(fp, "topk_traces_num: %d\n\n", topk_traces_num);

    fprintf(fp, "rules_memory_size:           %.8lf MB\n", rules_memory_size);
    fprintf(fp, "traces_memory_size:          %.8lf MB\n", traces_memory_size);
    fprintf(fp, "sketch_memory_size:          %.8lf MB\n", sketch_memory_size);
    fprintf(fp, "topk_tracesFreq_memory_size: %.8lf MB\n", topk_tracesFreq_memory_size);
    fprintf(fp, "TracesMat_memory_size:       %.8lf MB\n", TracesMat_memory_size);
    fprintf(fp, "tree_memory_size:            %.8lf MB\n", DTI.tree_memory_size);
    fprintf(fp, "total_memory_size:           %.8lf MB\n\n", total_memory_size);

    fprintf(fp, "sketch_build_and_update_time: %.8lf S\n", sketch_build_and_update_time);
    fprintf(fp, "sketch_calculate_topk_time:   %.8lf S\n", sketch_calculate_topk_time);
    fprintf(fp, "topk_tracesMat_init_time:     %.8lf S\n", topk_tracesMat_init_time);
    fprintf(fp, "rules_analyze_time:           %.8lf S\n", rules_analyze_time);
    fprintf(fp, "tree_build_time:              %.8lf S\n", tree_build_time);
    fprintf(fp, "total_build_time:             %.8lf S\n\n", total_build_time);

    fprintf(fp, "avg_lookup_time:        %.8lf US\n", avg_lookup_time);
    fprintf(fp, "avg_insert_time:        %.8lf US\n", avg_insert_time);

    fprintf(fp, "avg_lookup_access:        %.8lf\n", avg_lookup_access);
    fprintf(fp, "max_lookup_access:        %.0lf\n", max_lookup_access);
    fprintf(fp, "avg_lookup_depth:         %.8lf\n", avg_lookup_depth);

    fprintf(fp, "avg_lookup_access_rules:  %.8lf\n", avg_lookup_access_rules);
    fprintf(fp, "max_lookup_access_rules:  %.0lf\n\n", max_lookup_access_rules);
    
    DTI.Print(fp);

    fclose(fp);
    fflush(stdout);
}