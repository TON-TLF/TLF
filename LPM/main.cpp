#include "Elements/Elements.h"
#include "Methods/Methods.h"
#include "TraceStats/TraceStats.h"
#include "Tools/Tools.h"
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <memory>  // for std::shared_ptr
#include <x86intrin.h>
#include <pthread.h>

using namespace std;

int main(int argc, char *argv[]){
    /** 获取并解析指令 */
    Command::Set(argc, argv);
    cout<<Command::output_file<<endl;

    struct timespec ts_start, ts_end; 
    uint64_t cycles_start, cycles_end;
    ProgramState *ps = new ProgramState;

    /** 1. 读取规则集与流量集 */
    vector<Prefix*> prefixs = ReadPrefixs(Command::prefixs_file);
    vector<Trace*> traces = ReadTraces(Command::traces_file);
    int prefixs_num = prefixs.size();
    int traces_num = traces.size();
    ps->prefixs_num = prefixs_num;
    ps->traces_num = traces_num;

    /** 2. 准备流量信息 TopK统计*/
    /** 使用 sketch 对流量进行统计 */
    string *trace_str = new string[traces_num + 10];
    clock_gettime(CLOCK_MONOTONIC, &ts_start); 
    TDHeavyKeeper tdhk(Command::TopK, 0.4 * 1024 * 1024 / 16);
    for(int i = 0; i < traces_num; i+=1){ 
        trace_str[i] = traceToString(traces[i]);  
        tdhk.insert(trace_str[i]);
    }  
    clock_gettime(CLOCK_MONOTONIC, &ts_end);
    ps->sketch_build_and_update_time = GetTimeInSeconds(ts_start, ts_end);
    ps->avg_insert_time = GetTimeInMicroSeconds(ts_start, ts_end) / (traces_num * 1.0);

    /** 计算 topk 流量 */
    clock_gettime(CLOCK_MONOTONIC, &ts_start); 
    vector<TFNode> tdhkTF = tdhk.work();
    clock_gettime(CLOCK_MONOTONIC, &ts_end);
    ps->sketch_calculate_topk_time = GetTimeInSeconds(ts_start, ts_end);

    /** 构建 topk 查询矩阵 */
    clock_gettime(CLOCK_MONOTONIC, &ts_start);
    TSL::InitTopKStats(tdhkTF);
    clock_gettime(CLOCK_MONOTONIC, &ts_end);
    ps->topk_tracesMat_init_time = GetTimeInSeconds(ts_start, ts_end);

    ps->rules_memory_size = prefixs_num * sizeof(Prefix) / 1024.0 / 1024.0;
    ps->traces_memory_size = traces_num * sizeof(Trace) / 1024.0 / 1024.0;
    ps->sketch_memory_size = 1.0 * tdhk.calculateMemory() / 1024 / 1024;
    ps->topk_tracesFreq_memory_size = tdhkTF.size() * sizeof(TFNode) / 1024.0 / 1024.0; 
	ps->TracesMat_memory_size = TSL::CalMemory() / 1024.0 / 1024.0;
    cout << "TSL 初始化完成" << std::endl;

    /** 3. 根据参数初始化结构体 */
    bool is_Mat = false;
    Classifier *classifier;
    if (Command::method_name == "ABST" ){
        classifier = new ABST;
    } else if (Command::method_name == "ABST_TD" ){
        is_Mat = true;
        classifier = new ABST_TD;
    } else if (Command::method_name == "DIR248" ){
        classifier = new DIR248;
    } else if (Command::method_name == "DIR248_TD" ){
        is_Mat = true;
        classifier = new DIR248_TD;
    } else if (Command::method_name == "Poptrie" ){
        classifier = new Poptrie;
    } else if (Command::method_name == "Poptrie_TD" ){
        is_Mat = true;
        classifier = new Poptrie_TD;
    } else if (Command::method_name == "Auto" ){
        clock_gettime(CLOCK_MONOTONIC, &ts_start);
        unordered_map<uint8_t, int> lenCount;
        for (auto* p : prefixs) {
            lenCount[p->prefix_len]++;
        }
        int N_nz = lenCount.size();

        double sum_p2 = 0.0, entropy = 0.0;
        for (auto& pair : lenCount) {
            double p = static_cast<double>(pair.second) / prefixs_num;
            sum_p2 += p * p;
            entropy -= p * log2(p);
        }

        double N_eff = 1.0 / sum_p2;
        double H_norm = (N_nz > 0) ? entropy / log2(N_nz) : 0.0;
        double m_star = pow(N_nz, H_norm);
        clock_gettime(CLOCK_MONOTONIC, &ts_end);
        ps->rules_analyze_time = GetTimeInSeconds(ts_start, ts_end);

        is_Mat = true;
        if(m_star > 10){
            Poptrie_TD::TOP_LEVEL_STRIDE = 16;
            classifier = new Poptrie_TD;
        } else {
            classifier = new ABST_TD;
        }
    }
    
    /** 4. 创建查找结构 */
	clock_gettime(CLOCK_MONOTONIC, &ts_start);
    classifier->Create(prefixs, ps);
	clock_gettime(CLOCK_MONOTONIC, &ts_end);
    ps->tree_build_time = GetTimeInSeconds(ts_start, ts_end);
    
    ps->tree_memory_size = classifier->CalMemory() / 1024.0 / 1024.0;
    if(is_Mat){
        ps->total_build_time += ps->sketch_build_and_update_time;
        ps->total_build_time += ps->sketch_calculate_topk_time;
        ps->total_build_time += ps->topk_tracesMat_init_time;
        if (Command::method_name == "Auto" ) ps->total_build_time += ps->rules_analyze_time;
        ps->total_build_time += ps->tree_build_time;

        ps->total_memory_size += ps->sketch_memory_size;
        ps->total_memory_size += ps->topk_tracesFreq_memory_size;
        ps->total_memory_size += ps->TracesMat_memory_size; 
        ps->total_memory_size += ps->tree_memory_size;
    } else {
        ps->total_build_time = ps->tree_build_time;
        ps->total_memory_size += ps->tree_memory_size;
    }
	std::cout<<"Create over!"<<std::endl;

    /** 5.1 测试平均访存次数，并记录查询结果 */
    vector<uint32_t> Ans;
    for (int i = 0; i < traces_num; ++i){
        Ans.push_back(classifier->Lookup(traces[i], ps));
    }

    /** 5.2 测试平均查询时间以及CPU Cycle */
    int lookup_round = Command::lookup_round;
    double total_lookup_times = 0;
    clock_gettime(CLOCK_MONOTONIC, &ts_start);
	for (int k = 0; k < lookup_round; ++k){
		for (int i = 0; i < traces_num; ++i){
            classifier->Lookup(traces[i]);
		}
	}
    clock_gettime(CLOCK_MONOTONIC, &ts_end);
    total_lookup_times += GetTimeInMicroSeconds(ts_start, ts_end);
    ps->avg_lookup_time = total_lookup_times / (lookup_round * traces_num * 1.0);
    ps->throughput = (traces_num * lookup_round) / (total_lookup_times / 1e6); // pps
    cout << "lookup over!" <<endl;
    
    ps->CalInfo();
    ps->Print();
    return 0;
}