#ifndef  COMMAND_H
#define  COMMAND_H

#include "Common.h"

using namespace std;

class Command{
public:
    static string run_mode;     
    static string method_name; 

	static string prefixs_file;  
	static string traces_file; 
	static string output_file;  

    static int lookup_round;   
    static int TopK;  

    static bool Set(int argc, char *argv[]); 
};

#endif