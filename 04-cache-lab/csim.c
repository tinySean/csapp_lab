#include "cachelab.h"
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
extern int optind,opterr,optopt;
extern char *optarg;
int help_flag = 0;
int verbose_flag = 0;

void simulate(set_number, associativity, block_bit_number, tracefile){

}

int main(int argc, char* argv[])
{
    int option = 0; // 用于接收选项
    int set_number, associativity, block_bit_number; 
    char* tracefile;
    // 循环处理参数
    while(EOF != (option = getopt(argc,argv,"hvs:E:b:t:")))
    {
        //打印处理的参数
        // printf("start to process %d para\n",optind);
        switch(option)
        {
            case 'h':
                help_flag = 1;
                // printf("we get option -h\n");
                break;
            case 'v':
                verbose_flag = 1;
                // printf("we get option -v\n");
                break;
            case 's':
                set_number = atoi(optarg);
                // printf("we get option -s,para is %s\n",optarg);
                break;
            case 'E':
                associativity = atoi(optarg);
                // printf("we get option -E,para is %s\n",optarg);
                break;
            case 'b':
                block_bit_number = atoi(optarg);
                // printf("we get option -b,para is %s\n",optarg);
                break;
            case 't':
                tracefile = optarg;
                // printf("we get option -t,para is %s\n",optarg);
                break;
            //表示选项不支持
            case '?':
                printf("unknow option:%option\n",optopt);
                break;
            default:
                break;
        }    
    }
    simulate(set_number, associativity, block_bit_number, tracefile);
    printSummary(0, 0, 0);
    return 0;
}
