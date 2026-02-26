#ifndef  COMMAND_H
#define  COMMAND_H

#include "Common.h"

using namespace std;

/**
 * @brief 命令类
 */
class Command{
public:
    static string run_mode;     /** 运行模式 */
    static string method_name;  /** 包分类方法名称 */

	static string prefixs_file;   /** 规则文件路径 */
	static string traces_file;  /** 流量文件路径 */
	static string output_file;  /** 输出文件路径 */

    static int lookup_round;    /** 查找轮数 */
    static int TopK;    /** TopK */

    static bool Set(int argc, char *argv[]);  /** 自定义构造函数 */
};

#endif