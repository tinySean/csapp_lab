# CSAPP-LAB4

``cache lab``
手写cache模拟器，优化矩阵转置函数

## stage1

#### 命令行输入处理

命令行输入处理主要使用了`getopt`函数，具体的使用方法参加个人博客转载的一篇[文章](https://tinysean.gitee.io/blog/2020/05/15/Linux%20C/getopt/)

```c
#include <unistd.h>
#include <getopt.h>
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
```

### 文件行读取处理

文件处理的时候首先要判断文件是否存在，随后每次读取一行进行解析并操作cache

```c
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
```
c
### cache数据结构

```c
// 由于不需要存储具体内容，因此我们没把data数组加入到结构体中
typedef struct sCache
{
    int valid_bit;  // 有效位
    int tag; // 标记位
    int count; // 访问时间戳
} Cache;
```

### 访问cache操作

```c
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
```

## stage2

该部分主要采用blocking技术对转置矩阵进行优化
cache的s为5, E为1, b为5

### 32*32

在32*32这一小题中，一个int为4个byte，B为32，因此cache中的一行最多存储8个int，因此为了达到最大性能，我们将blocking的size设置为8。
至于最内存循环为啥要用临时变量，假设我们不这么做,使用注释中的方法，那么会出现一种现象，A在cache中缓存第1行，B之后也会在cache中缓存第1行，它会冲掉A在cache中缓存的第1行，由于A还有其他元素没读取完，A之后又会冲掉B在cache中缓存的第1行，由此产生一种抖动现象，最终导致性能的低消。

```c
// int i, j, k;
// for (i = 0; i < M; i += 8) {
//     for (j = 0; j < N; j ++){
//         for(k = i; k < i + 8; k++){
//             B[k][j] = A[j][k];
//         } 
//     }
// }
int i, j;
int temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7;
for (i = 0; i < M; i += 8) {
    for (j = 0; j < N; j ++){
        temp0 = A[j][i];
        temp1 = A[j][i+1];
        temp2 = A[j][i+2];
        temp3 = A[j][i+3];
        temp4 = A[j][i+4];
        temp5 = A[j][i+5];
        temp6 = A[j][i+6];
        temp7 = A[j][i+7];
        B[i][j] = temp0;
        B[i+1][j] = temp1;
        B[i+2][j] = temp2;
        B[i+3][j] = temp3;
        B[i+4][j] = temp4;
        B[i+5][j] = temp5;
        B[i+6][j] = temp6;
        B[i+7][j] = temp7;
    }
}
```

### 64*64

对于64*64，我们首先尝试使用跟上面类似的代码，但是发现结果不好, 经过分析， 主要是由于现在第1和第5行都会映射到同一行cache，产生冲突，引起抖动。
![avt](https://pic1.zhimg.com/80/v2-237d1caf9fc3d241668e31c26885c990_1440w.jpg)
为了降低上面这种效应，将blocking的size设置为4，这样只能拿一半的分，想要拿全分比较困难，思考了半天没思路。

```c
int i, j;
int temp0, temp1, temp2, temp3;
for (i = 0; i < M; i += 4) {
    for (j = 0; j < N; j ++){
        temp0 = A[j][i];
        temp1 = A[j][i+1];
        temp2 = A[j][i+2];
        temp3 = A[j][i+3];
        B[i+1][j] = temp1;
        B[i+2][j] = temp2;
        B[i][j] = temp0;
        B[i+3][j] = temp3;
    }
}
```

### 61*67

对于61*67，由于M和N不一样，打乱了顺序，因此第1行和第5行不会产生冲突，我们使用32*32相同的代码就可以拿到全部的分数了。