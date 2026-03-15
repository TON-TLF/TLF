#include "io.h"

using namespace std;

bool ipv6_str_to_uint128(const string &ip6_str, uint64_t &upper, uint64_t &lower) {
    struct in6_addr addr;
    if (inet_pton(AF_INET6, ip6_str.c_str(), &addr) != 1) {
        return false;
    }
    
    uint32_t part0 = ntohl(addr.__in6_u.__u6_addr32[0]);
    uint32_t part1 = ntohl(addr.__in6_u.__u6_addr32[1]);
    uint32_t part2 = ntohl(addr.__in6_u.__u6_addr32[2]);
    uint32_t part3 = ntohl(addr.__in6_u.__u6_addr32[3]);
    
    upper = (static_cast<uint64_t>(part0) << 32) | part1;
    lower = (static_cast<uint64_t>(part2) << 32) | part3;
    
    return true;
}

vector<Prefix*> ReadPrefixs(string prefixs_file) {
    vector<Prefix*> prefixs;
    ifstream file(prefixs_file);
    string line;
    
    if (!file.is_open()) {
        cerr << "Error opening prefix file: " << prefixs_file << endl;
        return prefixs;
    }
    
    int port_id = 1;
    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        size_t slash_pos = line.find('/');
        if (slash_pos == string::npos) {
            cerr << "Invalid prefix format: " << line << endl;
            continue;
        }
        
        string ip_str = line.substr(0, slash_pos);
        string len_str = line.substr(slash_pos + 1);
        
        uint64_t upper, lower;
        if (!ipv6_str_to_uint128(ip_str, upper, lower)) {
            cerr << "Invalid IPv6 address: " << ip_str << endl;
            continue;
        }
        
        int prefix_len = stoi(len_str);
        if (prefix_len < 0 || prefix_len > 128) {
            cerr << "Invalid prefix length: " << prefix_len << endl;
            continue;
        }
    
        Prefix* prefix = new Prefix();
        prefix->ip6_upper = upper;
        prefix->ip6_lower = lower;
        prefix->prefix_len = static_cast<uint8_t>(prefix_len);
        prefix->port = port_id; 
        
        prefixs.push_back(prefix);
        port_id++;
    }
    
    file.close();
    return prefixs;
}

void PrintPrefixHex(const Prefix* prefix) {
    if (!prefix) return;
    
    printf("IPv6 Upper: 0x%016" PRIx64 "\n", prefix->ip6_upper);
    printf("IPv6 Lower: 0x%016" PRIx64 "\n", prefix->ip6_lower);
    printf("Prefix Length: %u\n", prefix->prefix_len);
    printf("Port: %u\n", prefix->port);
}

vector<Trace*> ReadTraces(string traces_file) {
    vector<Trace*> traces;
    ifstream file(traces_file);
    string line;
    
    if (!file.is_open()) {
        cerr << "Error opening traces file: " << traces_file << endl;
        return traces;
    }
    
    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        uint64_t upper, lower;
        if (!ipv6_str_to_uint128(line, upper, lower)) {
            cerr << "Invalid IPv6 address: " << line << endl;
            continue;
        }
        
        Trace* trace = new Trace();
        trace->ip6_upper = upper;
        trace->ip6_lower = lower;
        
        traces.push_back(trace);
    }
    
    file.close();
    return traces;
}

vector<uint32_t> ReadLinearAns(string ans_file) {
    vector<uint32_t> ans;
    ifstream file(ans_file);
    string line;
    
    if (!file.is_open()) {
        return ans; 
    }
    
    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        uint32_t port = stoul(line);
        ans.push_back(port);
    }
    
    file.close();
    return ans;
}

void SaveLinearAns(vector<uint32_t> &ans, string ans_file) {
    ofstream file(ans_file);
    if (!file.is_open()) {
        cerr << "Error opening answer file: " << ans_file << endl;
        return;
    }
    
    for (uint32_t port : ans) {
        file << port << endl;
    }
    
    file.close();
}