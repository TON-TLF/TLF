#ifndef IO_H
#define IO_H

#include "../Elements/Elements.h"

using namespace std;

vector<Rule*> ReadRules(string output_file, int rules_shuffle);

void PrintRules(vector<Rule*> &rules, string output_file, bool print_priority);
void PrintRule(Rule* &rule, bool print_priority);
vector<Trace*> ReadTraces(string traces_file);
void PrintTraces(vector<Trace*> &traces, string output_file);


void PrintTrace(Trace* &trace);
void PrintAns(vector<int> &ans, string output_file);
void PrintAns(vector<double> &ans, string output_file);
void PrintProgramState(Command command, ProgramState *program_state);
#endif