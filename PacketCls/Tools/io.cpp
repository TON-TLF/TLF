#include "io.h"

using namespace std;
/**
 * @brief 字符串分割函数，根据指定模式将字符串拆分为子字符串的向量
 * @param str 待分割字符串
 * @param pattern 字符集，字符串可以被字符集中任意一个字符分割
 * 
 * @return ret 返回装有分割后的字符串的vector容器
 * 
 * find_first_of 会在字符串中逐个字符地寻找目标字符集中的任何一个字符，
 * 一旦找到就会告诉你它的位置。如果你找的字符一个都没有出现，函数就会告
 * 诉你没有找到。
 */
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

/**
 * @brief 将IP地址字符串转换为32位整数形式
 * @param str 字符串形式的IP
 * @return uint32_t 返回32位整数形式的IP
 */
uint32_t GetIp(string str) {
	uint32_t num1, num2, num3, num4;
	sscanf(str.c_str(),"%u.%u.%u.%u", &num1, &num2, &num3, &num4);
	return (num1 << 24) | (num2 << 16) | (num3 << 8) | num4;
}

/**
 * @brief:将32位整数IP转换为字符串形式
 * @param ip 32位整数IP
 * 
 * @return str 字符串形式的IP
 */
string GetIpStr(uint32_t ip) {
    char str[100];
    sprintf(str, "%d.%d.%d.%d", ip >> 24, ip >> 16 & 255, ip >> 8 & 255, ip & 255);
    return str;
}

/**
 * @brief 计算64位整数中二进制位为1的个数
 * @param num 待计算的整数
 * @return ans 二进制位为1的个数
 */
int Count1(uint64_t num) {
	int ans = 0;
	while (num) {
		ans += num & 1;
		num /= 2;
	}
	return ans;
}

/**
 * @brief 读取规则文件，并根据需要是否打乱规则顺序
 * @param rules_file 规则文件名
 * @param rules_shuffle 控制是否打乱的参数，大于0打乱，否则不打乱
 * 
 * @return Rules 返回装有规则的vector容器
 * 
 * atoi() 函数的作用是将一个以字符串形式表示的整数转换为一个 int 类型的整数
 * strtoul() 用于将字符串转换为无符号长整型 (unsigned long) 数字
 * ? 为什么优先级要这样设置，以及为什么每次读取1000字节 
 */
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

/**
 * @brief 读取规则容器，并打印到指定文件
 * @param rules 装有规则的容器
 * @param output_file 输出到的文件名
 * @param print_priority 控制是否打印优先级
 */
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

/**
 * @brief 打印规则
 * @param rule 规则指针
 * @param print_priority 控制是否打印优先级
 */
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

/**
 * @brief 读取流量数据文件并返回
 * @param traces_file 流量文件名
 * 
 * @return traces 返回装有流量的vector容器
 */
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

/**
 * @brief 读取流量容器，并打印到指定文件
 * @param traces 装有流量的容器
 * @param output_file 输出到的文件名
 */
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

/**
 * @brief 打印流量
 * @param traces 流量指针
 */
void PrintTrace(Trace* &trace) {
    for(int j = 0; j < 5; ++j){
        printf("%u\t",trace->key[j]);
    }
    printf("\n");
}

/**
 * @brief 读取答案容器，并打印到指定文件
 * @param ans 装有答案的容器
 * @param output_file 输出到的文件名
 */
void PrintAns(vector<int> &ans, string output_file) {
    int ans_num = ans.size();
    FILE *fp = fopen(output_file.c_str(), "w");
    for (int i = 0; i < ans_num; ++i) {
        fprintf(fp, "%d\n",ans[i]);
    }
    fclose(fp);
}

/**
 * @brief 读取答案容器，并打印到指定文件
 * @param ans 装有答案的容器
 * @param output_file 输出到的文件名
 */
void PrintAns(vector<double> &ans, string output_file) {
    int ans_num = ans.size();
    FILE *fp = fopen(output_file.c_str(), "w");
    for (int i = 0; i < ans_num; ++i) {
        fprintf(fp, "%lf\n",ans[i]);
    }
    fclose(fp);
}