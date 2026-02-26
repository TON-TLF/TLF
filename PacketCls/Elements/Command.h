#ifndef  COMMAND_H
#define  COMMAND_H

#include "Common.h"

using namespace std;

class Command{
public:
    static string run_mode;     
    static string method_name;  

	static string rules_file;   
	static string traces_file;  
	static string output_file;  

    static int lookup_round;    
    static int topk_num;        

    static bool Set(int argc, char *argv[]);  
};

#endif