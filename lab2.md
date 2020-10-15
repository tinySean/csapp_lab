# CSAPP-LAB2
``拆弹游戏``
通过objdump -d进行反汇编代码，通过cgdb进行调试，
## phase1
本题的关键在下面两行：
```
  400ee4:	be 00 24 40 00       	mov    $0x402400,%esi
  400ee9:	e8 4a 04 00 00       	callq  401338 <strings_not_equal>
```
通过观察strings_not_equal函数，我们发现它的两个输入分别保存在edi和esi寄存器中，其中edi寄存器中保存着我们输入的值，而esi寄存器则保存着炸弹字符串。需要注意的是，这里保存的仅仅字符串的起始地址(这个问题我思考了很久。。)，因此在gdb中使用下面的命令就能解决该问题:
```
x /s 0x402400
```
## phase2
首先需要读懂read_six_numbers的作用在于将六个数字依次读入到栈中
本题的关键在下面几行：
```
  400f0a:	83 3c 24 01          	cmpl   $0x1,(%rsp) # 栈的第一个元素为1
  400f0e:	74 20                	je     400f30 <phase_2+0x34> # 跳转到循环开始处
  400f10:	e8 25 05 00 00       	callq  40143a <explode_bomb>
  400f15:	eb 19                	jmp    400f30 <phase_2+0x34>
  400f17:	8b 43 fc             	mov    -0x4(%rbx),%eax        #下面三行确保当前元素是前一个元素的两倍
  400f1a:	01 c0                	add    %eax,%eax
  400f1c:	39 03                	cmp    %eax,(%rbx)
  400f1e:	74 05                	je     400f25 <phase_2+0x29>
  400f20:	e8 15 05 00 00       	callq  40143a <explode_bomb>
  400f25:	48 83 c3 04          	add    $0x4,%rbx             # 将当前元素后移
  400f29:	48 39 eb             	cmp    %rbp,%rbx
  400f2c:	75 e9                	jne    400f17 <phase_2+0x1b>
  400f2e:	eb 0c                	jmp    400f3c <phase_2+0x40>
  400f30:	48 8d 5c 24 04       	lea    0x4(%rsp),%rbx             # 循环起点
  400f35:	48 8d 6c 24 18       	lea    0x18(%rsp),%rbp            # 循环终点
  400f3a:	eb db                	jmp    400f17 <phase_2+0x1b>
```

## phase3
在所有实现中，都需要考虑的情况:
1. allocate失败，直接返回，另外在返回前还需要将之前成功分配的内存回收
2. queue为空


q_new函数主要进行初始化工作，要同时初始化head, tail, q_size三个变量
```
queue_t *q_new()
{
  queue_t *q = malloc(sizeof(queue_t));
  if (q == NULL)
    return NULL;
  q->head = NULL;
  q->tail = q->head;
  q->q_size = 0;
  return q;
}
```

q_insert_head主要进行插入操作，需要判断队列是否为空，元素newh是否能分配，元素下的字符串newh->value是否能分配三种情况，特别需要注意的是,如果元素下的字符串不能分配，在返回的时候要回收newh的内存。
另外如果插入的时候队列为空，还要初始化tail指针

```
bool q_insert_head(queue_t *q, char *s)
{
  if (q == NULL)
    return false;
  /* allocate memory for list element */
  list_ele_t *newh;
  newh = malloc(sizeof(list_ele_t));
  if (newh == NULL)
    return false;
  /* allocate memory for value */
  newh->value = malloc(sizeof(char) * (strlen(s) + 1));
  if (newh->value == NULL){
    free(newh);
    return false;
  }
  memcpy(newh->value, s, sizeof(char) * (strlen(s) + 1));
  /* insert the element at head of queue */
  newh->next = q->head;
  q->head = newh;
  if (q->tail == NULL)
    q->tail = q->head;
  q->q_size += 1;
  return true;
}
```

q_insert_tail需要注意的点跟之前相同，除了在最后要对队列为空的情况进行额外讨论
```
bool q_insert_tail(queue_t *q, char *s)
{
  /* You need to write the complete code for this function */
  /* Remember: It should operate in O(1) time */
  if (q == NULL)
    return false;
  /* allocate memory for list element */
  list_ele_t *newh;
  newh = malloc(sizeof(list_ele_t));
  if (newh == NULL)
    return false;
  /* allocate memory for value */
  newh->value = malloc(sizeof(char) * (strlen(s) + 1));
  if (newh->value == NULL){
    free(newh);
    return false;
  }
  memccpy(newh->value, s, sizeof(char) * (strlen(s) + 1), sizeof(char) * (strlen(s) + 1));
  /* insert the element at tail of queue */
  newh->next = NULL;
  if (q->tail != NULL)
    q->tail->next = newh;
  q->tail = newh;
  if (q->head == NULL)
    q->head = q->tail;
  q->q_size += 1;
  return true;
}
```

删除首部元素时，要注意队列为空时，需要将tail指向空，另外第bufsize个元素是sp[bufsize - 1]
```
bool q_remove_head(queue_t *q, char *sp, size_t bufsize)
{
  /* Return false if queue is NULL or empty. */
  if (q == NULL || q->q_size == 0)
    return false;
  list_ele_t *t = q->head;
  q->head = q->head->next;
  if(sp != NULL){
    memccpy(sp, t->value, sizeof(char) * (strlen(t->value) + 1), bufsize - 1);
    sp[bufsize - 1] = '\0';
  }
  free(t->value);
  free(t);
  t = NULL;
  q->q_size -= 1;
  if (q->q_size == 0)
    q->tail = NULL;
  return true;
}
```

整体思路和删除首部元素类似，每次都会删除首部的一个元素，然后判断队列元素是否为空，当队列为空时，free整个队列
```
void q_free(queue_t *q)
{
  /* How about freeing the list elements and the strings? */
  /* Free queue structure */
  if(q == NULL)
    return;
  while (q->q_size > 0)
  {
    list_ele_t *t = q->head;
    q->head = q->head->next;
    free(t->value);
    free(t);
    t = NULL;
    q->q_size -= 1;
    if (q->q_size == 0) 
      q->tail = NULL; 
  }
  free(q);
}
```

翻转队列
```
void q_reverse(queue_t *q)
{
  /* You need to write the code for this function */
  if(q == NULL || q->q_size == 0)
    return;
  list_ele_t* t_head = q->head;
  list_ele_t* t_tail = q->tail;
  list_ele_t* t = q->head;
  list_ele_t* t_next = q->head->next;
  t->next = NULL; //这边需要注意，不加的话会变成循环队列的情况
  while(t_next != NULL){
    list_ele_t* t_next_next = t_next->next;
    t_next->next = t;
    t = t_next;
    t_next = t_next_next;
  }
  q->head = t_tail;
  q->tail = t_head;
}
```