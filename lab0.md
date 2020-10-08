# CSAPP-LAB0
``任务是实现一个queue``

## 数据结构
```
typedef struct ELE {
    char *value;
    struct ELE *next;
} list_ele_t;

/* Queue structure */
typedef struct {
    list_ele_t *head; 
    int32_t q_size;
    list_ele_t *tail;
} queue_t;
```
## 操作接口
```
queue_t *q_new();
void q_free(queue_t *q);
bool q_insert_head(queue_t *q, char *s);
bool q_insert_tail(queue_t *q, char *s);
bool q_remove_head(queue_t *q, char *sp, size_t bufsize);
int q_size(queue_t *q);
void q_reverse(queue_t *q);
```

## 具体实现
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