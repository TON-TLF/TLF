#include "io.h"

using namespace std;

vector<string> StrSplit(const string& str, const string& pattern) {
    vector<string> ret;
    if(pattern.empty())
        return ret;
    int start = 0, index = str.find_first_of(pattern, 0);
    while(index != str.npos) {
        if(start != index)
            ret.push_back(str.substr(start, index-start));
        start = index + 1;
        index = str.find_first_of(pattern, start);
    }
    if(!str.substr(start).empty())
        ret.push_back(str.substr(start));
    return ret;
}

uint32_t GetIp(string str) {
	uint32_t num1, num2, num3, num4;
	sscanf(str.c_str(),"%u.%u.%u.%u", &num1, &num2, &num3, &num4);
	return (num1 << 24) | (num2 << 16) | (num3 << 8) | num4;
}

string GetIpStr(uint32_t ip) {
    char str[100];
    sprintf(str, "%d.%d.%d.%d", ip >> 24, ip >> 16 & 255, ip >> 8 & 255, ip & 255);
    return str;
}

int Count1(uint64_t num) {
	int ans = 0;
	while (num) {
		ans += num & 1;
		num /= 2;
	}
	return ans;
}

vector<Rule*> ReadRules(string rules_file, int rules_shuffle) {
	FILE *fp = fopen(rules_file.c_str(), "rb");
	if (!fp){
        printf("Cannot open the file %s\n", rules_file.c_str());
        exit(0);
    }
		
    vector<Rule*> rules;
	int rules_num = 0;
    char buf[1025];

    uint32_t priority = 0;
    while (fgets(buf,1000,fp)!=NULL)
        ++priority;
    
    fp = fopen(rules_file.c_str(), "rb");
	while (fgets(buf,1000,fp)!=NULL) { 
        string str = buf + 1;
        vector<string> vc = StrSplit(str, "\t /:");

        Rule *rule = (Rule*)malloc(sizeof(Rule));
        memset(rule, 0, sizeof(Rule));
        // src_ip dst_ip
        for (int i = 0; i < 2; ++i) {
            rule->range[i][0] = GetIp(vc[i * 2]);
            rule->prefix_len[i] = atoi(vc[i * 2 + 1].c_str());
            uint32_t mask = (1LL << (32 - rule->prefix_len[i])) - 1;
            rule->range[i][0] -= rule->range[i][0] & mask;
			rule->range[i][1] = rule->range[i][0] | mask;
            //cout<<rule->range[i][0]<<" "<<rule->range[i][1]<<endl;
        }
        
        // src_port dst_port
        rule->range[2][0] = atoi(vc[4].c_str());
        rule->range[2][1] = atoi(vc[5].c_str());
        rule->range[3][0] = atoi(vc[6].c_str());
        rule->range[3][1] = atoi(vc[7].c_str());
        // protocol
        rule->range[4][0] = strtoul(vc[8].c_str(), NULL, 0);
        uint32_t mask = 255 - strtoul(vc[9].c_str(), NULL, 0);
		rule->range[4][0] -= rule->range[4][0] & mask;
		rule->range[4][1] = rule->range[4][0] | mask;
		rule->prefix_len[4] = Count1(255 - mask);
        // priority
        rule->priority = priority--;

        rules.push_back(rule);
        ++rules_num;
    }
    if (rules_shuffle > 0) {
        random_shuffle(rules.begin(),rules.end());
    }
    return rules;
}

void PrintRules(vector<Rule*> &rules, string output_file, bool print_priority) {
    int rules_num = rules.size();
    FILE *fp = fopen(output_file.c_str(), "w");
    for (int i = 0; i < rules_num; ++i) {
        fprintf(fp, "@");
        for (int j = 0; j < 2; ++j) {
            fprintf(fp, "%s/%d\t", GetIpStr(rules[i]->range[j][0]).c_str(), rules[i]->prefix_len[j]);
        }
        for (int j = 2; j < 4; ++j){
            fprintf(fp, "%d : %d\t", rules[i]->range[j][0], rules[i]->range[j][1]);
        }
        if (rules[i]->range[4][0] == rules[i]->range[4][1]){
            fprintf(fp, "0x%02x/0xFF\t", rules[i]->range[4][0]);
        } else {
            fprintf(fp, "0x%02x/0x00\t", rules[i]->range[4][0]);
        }
        if (print_priority){
            fprintf(fp, "%d\t", rules[i]->priority);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
}

void PrintRule(Rule* &rule, bool print_priority) {
    printf("@");
    for (int j = 0; j < 2; ++j) {
        printf("%s/%d\t", GetIpStr(rule->range[j][0]).c_str(), rule->prefix_len[j]);
    }
    for (int j = 2; j < 4; ++j){
        printf("%d : %d\t", rule->range[j][0], rule->range[j][1]);
    }
    if (rule->range[4][0] == rule->range[4][1]){
        printf("0x%02x/0xFF\t", rule->range[4][0]);
    } else {
        printf("0x%02x/0x00\t", rule->range[4][0]);
    }
    if (print_priority){
        printf("%d\t", rule->priority);
    }
    printf("\n");
}

vector<Trace*> ReadTraces(string traces_file) {
	FILE *fp = fopen(traces_file.c_str(), "rb");
	if (!fp){
        printf("Cannot open the file %s\n", traces_file.c_str());
        exit(0);
    }

	vector<Trace*> traces;
	int traces_num = 0;
    char buf[1025];

	while (fgets(buf,1000,fp)!=NULL) { 
        string str = buf;
        vector<string> vc = StrSplit(str, "\t");
        Trace *trace = (Trace*)malloc(sizeof(Trace));
        for (int i = 0; i < 5; ++i){
            trace->key[i] = atoi(vc[i].c_str());
        }
        traces.push_back(trace);
        ++traces_num;
    }
    return traces;
}

void PrintTraces(vector<Trace*> &traces, string output_file) {
    int traces_num = traces.size();
    FILE *fp = fopen(output_file.c_str(), "w");
    for (int i = 0; i < traces_num; ++i) {
        for(int j = 0; j < 5; ++j){
            fprintf(fp, "%u\t",traces[i]->key[j]);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
}

void PrintTrace(Trace* &trace) {
    for(int j = 0; j < 5; ++j){
        printf("%u\t",trace->key[j]);
    }
    printf("\n");
}

void PrintAns(vector<int> &ans, string output_file) {
    int ans_num = ans.size();
    FILE *fp = fopen(output_file.c_str(), "w");
    for (int i = 0; i < ans_num; ++i) {
        fprintf(fp, "%d\n",ans[i]);
    }
    fclose(fp);
}

void PrintAns(vector<double> &ans, string output_file) {
    int ans_num = ans.size();
    FILE *fp = fopen(output_file.c_str(), "w");
    for (int i = 0; i < ans_num; ++i) {
        fprintf(fp, "%lf\n",ans[i]);
    }
    fclose(fp);
}