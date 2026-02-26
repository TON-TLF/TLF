#include "Elements/Elements.h"
#include "Tools/Tools.h"
#include "Methods/Methods.h"
#include "TopK/TopK.h"
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <memory> 
#include <x86intrin.h>
#include <pthread.h>

using namespace std;

int rules_num, traces_num;
vector<Rule*> rules;
vector<Trace*> traces;
ProgramState *ps;
struct timespec ts_start, ts_end; 

int main(int argc, char *argv[]){
    Command::Set(argc, argv);
    
    ps = new ProgramState;

    /** 读取规则集与流量集 */
    rules = ReadRules(Command::rules_file, 0);
    traces = ReadTraces(Command::traces_file);
    rules_num = rules.size();
    traces_num = traces.size();

    /** 将流量转化为字符串形式存储，为插入到 sketch 中做准备 */
    string *trace_str = new string[traces_num + 10];
    for(int i = 0; i < traces_num; i++){ 
        trace_str[i] = traceToString(traces[i]);   
    }   

    /** 使用 sketch 对流量进行统计 */
    clock_gettime(CLOCK_MONOTONIC, &ts_start);
    TDHeavyKeeper sketch(Command::topk_num, 0.4 * 1024 * 1024 / 16);
    for(int i = 0; i < traces_num; i++){
        sketch.insert(trace_str[i]);
    }   
    clock_gettime(CLOCK_MONOTONIC, &ts_end);
    ps->sketch_build_and_update_time = GetTimeInSeconds(ts_start, ts_end);
    ps->avg_insert_time = GetTimeInMicroSeconds(ts_start, ts_end) / traces_num;
    cout << "Sketch insert over!" << endl;

    /** 计算 topk 流量 */
    clock_gettime(CLOCK_MONOTONIC, &ts_start);
    vector<TFNode> hkTF = sketch.work();  
    vector<TraceFreq> topktracesfreq = TFNodeToTraceFreq(hkTF);
    clock_gettime(CLOCK_MONOTONIC, &ts_end);
    ps->sketch_calculate_topk_time = GetTimeInSeconds(ts_start, ts_end);
    cout<<"Cal Topk over!"<<endl;

    /** 准备 Topk 流量数组，为后续查询测试做准备 */
    int topktracesfreq_num = topktracesfreq.size();
	std::vector<Trace*> topktraces;
	for(int i = 0; i < topktracesfreq_num; i++){
		for(int j = 0; j < (int)topktracesfreq[i].freq; j++) {
			topktraces.push_back(&topktracesfreq[i].trace);
		}
	}
    int topktraces_num = topktraces.size();

    /** 构建 topk 查询矩阵 */
    clock_gettime(CLOCK_MONOTONIC, &ts_start);
    traceFreqMat2d::init(topktracesfreq);
    clock_gettime(CLOCK_MONOTONIC, &ts_end);
    ps->topk_tracesMat_init_time = GetTimeInSeconds(ts_start, ts_end);
    cout<<"Topk TracesMat Init over!"<<endl;

    /** 定义程序状态对象，用于记录程序执行过程中的各种信息 */
    ps->rules_num = rules_num;
    ps->traces_num = traces_num;
    ps->topk_traces_num = topktraces_num;
    ps->rules_memory_size = rules_num * sizeof(Rule) / 1024.0 / 1024.0;
    ps->traces_memory_size = traces_num * sizeof(Trace) / 1024.0 / 1024.0;
    ps->sketch_memory_size = sketch.CalMemory() / 1024.0 / 1024.0;
    ps->topk_tracesFreq_memory_size = topktracesfreq_num * sizeof(TraceFreq) / 1024.0 / 1024.0; 
	ps->TracesMat_memory_size = traceFreqMat2d::CalMemory() / 1024.0 / 1024.0;  
    
    /** 使用对应算法的分类器生成查询结果 */
    bool is_Mat = false;
    std::vector<int> Ans;
    Classifier *classifier;
    if (Command::method_name == "HyperSplit" ){
        classifier = new HyperSplit;
    }
    else if (Command::method_name == "TDHyperSplit" ){
        is_Mat = true;
        classifier = new TDHyperSplit;
    }
    else if (Command::method_name == "EffiCuts" ){
        classifier = new EffiCuts;
    }
    else if (Command::method_name == "TDEffiCuts" ){
        is_Mat = true;
        classifier = new TDEffiCuts;        
    }
    else if (Command::method_name == "HyperCuts" ){
        classifier = new HyperCuts;
    }
    else if (Command::method_name == "TDHyperCuts" ){
        is_Mat = true;
        classifier = new TDHyperCuts;
    }
    else if (Command::method_name == "HiCuts" ){
        classifier = new HiCuts;
    }
    else if (Command::method_name == "TDHiCuts" ){
        is_Mat = true;
        classifier = new TDHiCuts;
    }
    else if (Command::method_name == "Auto" ){
        /** 统计时间维度 */
        clock_gettime(CLOCK_MONOTONIC, &ts_start);
        double src_ranges = calculate_range(rules, 0);
        double dst_ranges = calculate_range(rules, 1);
        double avg_src_weight = calculate_weight(rules, 0);
        double avg_dst_weight = calculate_weight(rules, 1);
        double avg_collision  = MonteCarloCollisionCounter(rules, 5000);
        clock_gettime(CLOCK_MONOTONIC, &ts_end);
        ps->rules_analyze_time = GetTimeInSeconds(ts_start, ts_end);
        // printf("src : %.0lf %.0lf   ",avg_src_weight, src_ranges);
        // printf("dst : %.0lf %.0lf   ",avg_dst_weight, dst_ranges);
        // printf("collision = %.0lf\n",avg_collision);

        is_Mat = true;
        if(avg_collision > 500){
            classifier = new TDEffiCuts;
        } else if(avg_src_weight > 300 && avg_dst_weight > 300) {
            classifier = new TDHyperSplit;
        } else {
            classifier = new TDHyperCuts;
        }
    }
    
    /** 构建查找结构 */
	clock_gettime(CLOCK_MONOTONIC, &ts_start);
    classifier->Create(rules, ps);
	clock_gettime(CLOCK_MONOTONIC, &ts_end);
	ps->tree_build_time = GetTimeInSeconds(ts_start, ts_end);

    ps->DTI.tree_memory_size = classifier->CalMemory() / 1024.0 / 1024.0;
    if(is_Mat){
        ps->total_build_time += ps->sketch_build_and_update_time;
        ps->total_build_time += ps->sketch_calculate_topk_time;
        ps->total_build_time += ps->topk_tracesMat_init_time;
        if (Command::method_name == "Auto" ) ps->total_build_time += ps->rules_analyze_time;
        ps->total_build_time += ps->tree_build_time;

        ps->total_memory_size += ps->rules_memory_size;
        ps->total_memory_size += ps->sketch_memory_size;
        ps->total_memory_size += ps->topk_tracesFreq_memory_size;
        ps->total_memory_size += ps->TracesMat_memory_size; 
        ps->total_memory_size += ps->DTI.tree_memory_size;
    } else {
        ps->total_build_time = ps->tree_build_time;

        ps->total_memory_size += ps->rules_memory_size;
        ps->total_memory_size += ps->DTI.tree_memory_size;
    }
	std::cout<<"Create over!"<<std::endl;     
    
    /** 测试查询性能 */
	int lookup_round = Command::lookup_round;
    uint64_t total_lookup_cycles = 0;
    double total_lookup_times = 0;
    /** 测试平均访存次数 */
	for (int k = 0; k < lookup_round; ++k){
		Ans.clear();
		for (int i = 0; i < traces_num; ++i){
            Ans.push_back(classifier->Lookup(traces[i], ps));
		}
	}
    ps->CalInfo();
    ps->ClearAccess();

    /** 测试平均查询时间 */
    clock_gettime(CLOCK_MONOTONIC, &ts_start);
    for (int k = 0; k < lookup_round; ++k){
		for (int i = 0; i < traces_num; ++i){
            uint32_t tmp = classifier->Lookup(traces[i]);
		}
	}
    clock_gettime(CLOCK_MONOTONIC, &ts_end);
    total_lookup_times += GetTimeInMicroSeconds(ts_start, ts_end);
    ps->avg_lookup_time = total_lookup_times / (lookup_round * traces_num * 1.0);

    ps->Print();
    cout<<"Test over!"<<endl;
    
    return 0;
}