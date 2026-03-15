#include "Command.h"

using namespace std;

string Command::run_mode = ""; 
string Command::method_name = ""; 
string Command::prefixs_file = ""; 
string Command::traces_file = ""; 
string Command::output_file = ""; 

int Command::lookup_round = 0;  
int Command::TopK = 0;   

bool Command::Set(int argc, char *argv[]) {
	char short_opts[] = "";   
	struct option long_opts[] = { 
        {"run_mode",      required_argument, NULL, 1},  
        {"method_name",   required_argument, NULL, 2},  
        {"prefixs_file",  required_argument, NULL, 3},  
        {"traces_file",   required_argument, NULL, 4},   
        {"output_file",   required_argument, NULL, 5},  
        {"lookup_round",  required_argument, NULL, 6},  
        {"TopK",          required_argument, NULL, 7},
        {0,               0,                 0,    0} 
    };

    int opt, opt_index;
    while((opt = getopt_long(argc, argv, short_opts, long_opts, &opt_index)) != -1){
        switch(opt){
        case 1:
            run_mode = optarg;
            break;
        case 2:
            method_name = optarg;
            break;
        case 3:
            prefixs_file = optarg;
            break;
        case 4:
            traces_file = optarg;
            break;
        case 5:
            output_file = optarg;
            break;
        case 6:
            lookup_round = strtoul(optarg, NULL, 0);
            break;
        case 7:
            TopK = strtoul(optarg, NULL, 0);
            break;
        default:
            cout << "Wrong command " << opt << endl;
            exit(0);
            return false;
        }
    }
    return true;
}