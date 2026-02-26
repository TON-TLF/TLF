#ifndef IO_H
#define IO_H

#include "../Elements/Elements.h"

using namespace std;

/**
 * @brief:读取规则文件，并根据需要是否打乱规则顺序
 * @param rules_file:规则文件名
 * @param rules_shuffle:控制是否打乱的参数，大于0打乱，否则不打乱
 * 
 * @return vector<Rule*> 返回装有规则的vector容器
 */
vector<Rule*> ReadRules(string output_file, int rules_shuffle);

/**
 * @brief:读取规则容器，并打印到指定文件
 * @param rules 装有规则的容器
 * @param output_file 输出到的文件名
 * @param rules_shuffle 控制是否打乱的参数，大于0打乱，否则不打乱
 */
void PrintRules(vector<Rule*> &rules, string output_file, bool print_priority);
void PrintRule(Rule* &rule, bool print_priority);
/**
 * @brief 读取流量数据文件并返回
 * @param traces_file 流量文件名
 * 
 * @return vector<Trace*> 返回装有流量的vector容器
 */
vector<Trace*> ReadTraces(string traces_file);

/**
 * @brief 读取流量容器，并打印到指定文件
 * @param traces 装有流量的容器
 * @param output_file 输出到的文件名
 */
void PrintTraces(vector<Trace*> &traces, string output_file);


void PrintTrace(Trace* &trace);
/**
 * @brief 读取答案容器，并打印到指定文件
 * @param ans 装有答案的容器
 * @param output_file 输出到的文件名
 */
void PrintAns(vector<int> &ans, string output_file);
void PrintAns(vector<double> &ans, string output_file);
void PrintProgramState(Command command, ProgramState *program_state);
#endif