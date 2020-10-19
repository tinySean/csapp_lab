#include "cachelab.h"
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <math.h>
#include <stdlib.h>
extern int optind, opterr, optopt;
extern char *optarg;
int help_flag = 0;
int verbose_flag = 0;
int parse_fail_flag = 0;
int S, s, E, B, b; // s, E, b
int address;
char *tracefile; // tracefile name
int hits = 0, misses = 0, evictions = 0;
int count = 0;

// 由于不需要存储具体内容，因此我们没把data数组加入到结构体中
typedef struct sCache
{
    int valid_bit; // 有效位
    int tag;       // 标记位
    int count;     // 访问时间戳
} Cache;

// 返回1表示命中，返回2表示存在空行，返回3表示淘汰了某行
int load(Cache *cache, unsigned long tag_bits, unsigned long set_index_bits, unsigned long block_offset_bits)
{
    int hit_flag = 0;                // hit标记
    int empty_flag = 0;              // 是否存在某一行为空
    int empty_index = -1;            // 空的index
    int small_count = __INT32_MAX__; // 最小的时间戳，用于LRU淘汰
    int small_count_index = -1;      // 最小时间戳的index
    // 遍历该组的所有行
    for (int i = set_index_bits * E; i < (set_index_bits + 1) * E; i++)
    {
        // 如果命中，则需要更新该行的时间戳
        if (cache[i].valid_bit == 1 && cache[i].tag == tag_bits)
        {
            hit_flag = 1;
            cache[i].count = count;
            hits++;
            break;
        }
        // 用于找出某一空行
        if (cache[i].valid_bit == 0)
        {
            empty_flag = 1;
            empty_index = i;
        }
        // 用于LRU淘汰
        if (cache[i].count < small_count)
        {
            small_count = cache[i].count;
            small_count_index = i;
        }
    }
    // 如果没有命中
    if (hit_flag == 0)
    {
        misses++;
        // 如果存在空行，则将数据存入空行
        if (empty_flag)
        {
            cache[empty_index].tag = tag_bits;
            cache[empty_index].valid_bit = 1;
            cache[empty_index].count = count;
        }
        // 如果不存在空行，需要使用LRU进行淘汰，将数据存入淘汰掉的行
        else
        {
            evictions++;
            cache[small_count_index].tag = tag_bits;
            cache[small_count_index].valid_bit = 1;
            cache[small_count_index].count = count;
        }
    }
    if (hit_flag)
        return 1;
    else if (empty_flag)
        return 2;
    else
        return 3;
}

void print_status(int status, char access_type, unsigned long address, unsigned long size)
{
    if (verbose_flag == 0)
        return;
    printf("%c %lx,%ld", access_type, address, size);
    if (status == 1)
    {
        printf(" hit");
    }
    else if (status == 2)
    {
        printf(" miss");
    }
    else
    {
        printf(" miss eviction");
    }
}

void simulate(Cache *cache)
{
    FILE *pFile;
    pFile = fopen(tracefile, "r");
    // 文件不存在则报错
    if (pFile == NULL)
    {
        fprintf(stderr, "tracefile not FOUND\n");
        exit(EXIT_FAILURE);
    }
    char access_type;
    // 内存地址
    unsigned long address;
    // 依次标记，组id和便宜量
    unsigned long tag_bits, set_index_bits, block_offset_bits;
    int size;
    int status;
    // 每次读取一行，至于为啥这边要在%c前面加上空格，没搞太清楚，不加空格会出大问题
    while (fscanf(pFile, " %c %lx, %d", &access_type, &address, &size) > 0)
    {
        printf("%c:%lx:%d\n", access_type, address, size);
        // count用来记录时间戳信息，用于LRU淘汰算法
        count++;
        // 通过操作bit位来获取tag_bits, set_index_bits, block_offset_bits
        tag_bits = address >> (b + s);
        set_index_bits = (address >> b) & ((unsigned long)(pow(2, s) - 1));
        block_offset_bits = address & ((unsigned long)(pow(2, b) - 1));
        switch (access_type)
        {
        // load data，需要访问一次cache
        case 'L':
            status = load(cache, tag_bits, set_index_bits, block_offset_bits);
            print_status(status, access_type, address, size);
            printf("\n");
            break;
        // modify data，需要访问两次cache
        case 'M':
            status = load(cache, tag_bits, set_index_bits, block_offset_bits);
            print_status(status, access_type, address, size);
            hits++;
            printf(" hit\n");
            break;
        // store data，需要访问一次cache
        case 'S':
            status = load(cache, tag_bits, set_index_bits, block_offset_bits);
            print_status(status, access_type, address, size);
            printf("\n");
            break;
        // 对于普通的instruction，直接ignore
        case 'I':
            break;
        default:
            parse_fail_flag = 1;
            break;
        }
    }
    if (parse_fail_flag)
    {
        fprintf(stderr, "operation [ILMS]\n");
        exit(EXIT_FAILURE);
    }
    fclose(pFile); // Clean up Resources
}

int main(int argc, char *argv[])
{
    /* 解析命令行 */
    int option = 0; // 用于接收选项
    // 循环处理参数
    while (EOF != (option = getopt(argc, argv, "hvs:E:b:t:")))
    {
        switch (option)
        {
        // 打印帮助信息
        case 'h':
            help_flag = 1;
            break;
        // 开启verbose模式，即展示中间的详细过程
        case 'v':
            verbose_flag = 1;
            break;
        // s是表示组信息的位数，S是组的个数
        case 's':
            s = atoi(optarg);
            S = pow(2, s);
            break;
        // E是每组行数
        case 'E':
            E = atoi(optarg);
            break;
        // b是表示单行内的偏移量所占的位数, B是偏移量的数值
        case 'b':
            b = atoi(optarg);
            B = pow(2, b);
            break;
        // 表示tracefile的名字
        case 't':
            tracefile = optarg;
            break;
        // 表示选项不支持
        case '?':
            parse_fail_flag = 1;
            // printf("unknow option:%option\n",optopt);
            break;
        default:
            break;
        }
    }
    if (parse_fail_flag)
    {
        fprintf(stderr, "Usage: %s [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    if (help_flag)
    {
        printf("Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>\n");
        printf("Options:");
        printf("  -h         Print this help message.");
        printf("  -v         Optional verbose flag.");
        printf("  -s <num>   Number of set index bits.");
        printf("  -E <num>   Number of lines per set.");
        printf("  -b <num>   Number of block offset bits.");
        printf("Examples:");
        printf("  linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace");
        printf("  linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace");
    }
    /* Cache初始化 */
    Cache *cache = (Cache *)malloc(sizeof(Cache) * E * S);
    simulate(cache);
    printSummary(hits, misses, evictions);
    return 0;
}