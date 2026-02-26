#ifndef IO_H
#define IO_H

#include "../Elements/Elements.h"

using namespace std;


vector<Prefix*> ReadPrefixs(string prefixs_file);
void PrintPrefixHex(const Prefix* prefix);

vector<Trace*> ReadTraces(string traces_file);

vector<uint32_t> ReadLinearAns(string ans_file);
void SaveLinearAns(vector<uint32_t> &ans, string ans_file);
#endif