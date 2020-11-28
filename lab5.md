# CSAPP-LAB5

``shell lab``
实现一个tiny shell，不包括管道功能
主要是实现六个函数
```
* eval: Main routine that parses and interprets the command line. 
* builtincmd:  Recognizes and interprets the built-in * commands:quit,fg,bg, andjobs.  
* dobgfg: Implements thebgandfgbuilt-in commands. 
* waitfg: Waits for a foreground job to complete. 
* sigchldhandler: Catches SIGCHILD signals. 
* siginthandler: Catches SIGINT (ctrl-c) signals. 
* sigtstphandler: Catches SIGTSTP (ctrl-z) signals. 
```
## 三个信号函数的实现

```c
/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 */
void sigchld_handler(int sig) 
{
    // 首先保存错误信号
    int olderrno = errno;
    sigset_t mask_all, prev_all;
    pid_t pid;
    // 这里使用信号集mask_all在关键的位置BLOCK其他信号，类似一种互斥锁
    sigfillset(&mask_all);
    int status;
    // 注意这边要加WNOHANG, 否则父进程会一直等待，直至它所有子进程都被回收
    // 注意这边需要加WUNTRACED，为了让进程被ctrl-z时也能进入到循环中来 
    // 另外注意：在使用ctrl+c时，首先发出SIGINT信号，随后使用kill函数让进程终止，进程终止后会发出SIGCHLD信号（这一步跟kill函数直接相关）
    // 另外注意：在使用ctrl+z时，首先发出SIGTSNP信号，随后使用kill函数让进程暂停，进程暂停后会发出SIGCHLD信号 （这一步跟kill函数直接相关）
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED )) > 0) { /* Reap a zombie child */
        // 正常退出，删除任务列表中的任务
        if(WIFEXITED(status)){
            sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
            deletejob(jobs, pid); 
            sigprocmask(SIG_SETMASK, &prev_all, NULL);
        }
        // 异常终止，即ctrl+c, 使用WTERMSIG(status)来表示引发进程终止的信号值，删除任务列表中的任务
        else if(WIFSIGNALED(status)){
            printf("Job [%d] (%d) terminated by signal %d\n", pid2jid(pid), pid, WTERMSIG(status));
            sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
            deletejob(jobs, pid); 
            sigprocmask(SIG_SETMASK, &prev_all, NULL);
        }
        // 暂停，即ctrl+z, 使用WSTOPSIG(status)来表示引发进程暂停的信号值，修改任务的状态
        else if(WIFSTOPPED(status)){
            printf("Job [%d] (%d) stopped by signal %d\n", pid2jid(pid), pid, WSTOPSIG(status));
            struct job_t* job = getjobpid(jobs, pid);
            sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
            job->state = ST;
            sigprocmask(SIG_SETMASK, &prev_all, NULL);
        }
    }
    errno = olderrno;
}

/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
void sigint_handler(int sig) 
{
    int olderrno = errno;
    sigset_t mask_all, prev_all;
    pid_t pid;
    sigfillset(&mask_all);
    while ((pid = fgpid(jobs)) > 0) {
        sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
        // 通过kill函数来发出信号，从而进入sigchld，负号表示向整个进程组发出命令
        kill(-pid, SIGINT);
        sigprocmask(SIG_SETMASK, &prev_all, NULL);
    }
    errno = olderrno;
    return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 */
void sigtstp_handler(int sig) 
{
    int olderrno = errno;
    sigset_t mask_all, prev_all;
    pid_t pid;
    sigfillset(&mask_all);
    // 这边括号位置得放对了，不然可能导致会出现 pid = fgpid(jobs) > 0， pid每次都会1，最终让-1进程组暂停！！
    if((pid = fgpid(jobs)) > 0){
        sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
        // 负号表示向整个进程组发出命令
        kill(-pid, SIGTSTP);
        sigprocmask(SIG_SETMASK, &prev_all, NULL);
    }
    errno = olderrno;
    return;
}
```

## 内置指令

```c
/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 */
int builtin_cmd(char **argv) 
{
    if(!strcmp(argv[0], "quit")){
        _exit(0);
    }else if(!strcmp(argv[0], "fg")){
        do_bgfg(argv);
        return 1;
    }else if(!strcmp(argv[0], "bg")){
        do_bgfg(argv);
        return 1;
    }else if(!strcmp(argv[0], "jobs")){
        listjobs(jobs);
        return 1;
    }
    return 0;     /* not a builtin command */
}

/*
 * isdigits - Determine whether the string is all numbers 
 */
int isdigits(char* str){
    int i = 0;
    while(str[i] != '\0'){
        if(!isdigit(str[i])){
            return 0;
        }
        // 注意要写++， 不然死循环
        i++;
    }
    return 1;
}

/* 
 * do_bgfg - Execute the builtin bg and fg commands
 */
void do_bgfg(char **argv) 
{
    // bg fg后面必须要有pid或者%jid
    if(argv[1] == NULL){
        printf("%s command requires PID or %%jobid argument\n", argv[0]);
        return;
    }
    struct job_t* job;
    if(argv[1][0] == '%'){
        // 判断后面的jid是否合法
        if(!isdigits(argv[1] + 1)){
            printf("%s: argument must be a PID or %%jobid\n", argv[0]);
            return;
        }
        // 获取job
        int jid = atoi(argv[1] + 1);
        job = getjobjid(jobs, jid);
    }else{
        // 判断后面的pid是否合法
        if(!isdigits(argv[1])){
            printf("%s: argument must be a PID or %%jobid\n", argv[0]);
            return;
        }
        // 获取job
        int pid = atoi(argv[1]);
        job = getjobpid(jobs, pid);
    }
    // 判断jobs是否存在，即job是否在jobs列表里面
    if(argv[1][0] == '%' && job == NULL){
        printf("%s: No such job\n", argv[1]);
        return;
    }
    if(argv[1][0] != '%' && job == NULL){
        printf("(%s): No such process\n", argv[1]);
        return;
    }
    // 下面的kill注意都要用负号，以向进程组发送信号
    // 处理fg
    if(!strcmp(argv[0], "fg")){
        // 对于暂停的job，发送SIGCONT信号
        if(job->state == ST){
            kill(-job->pid, SIGCONT);
        }
        // 改变job的状态，让它在前台工作，并且等待它运行结束
        if(job->state == BG || job->state == ST){
            job->state = FG;
            waitfg(job->pid);
        }
    // 处理bg
    }else if(!strcmp(argv[0], "bg")){
        // 对于暂停的job，发送SIGCONT信号，并改变它的状态，让其工作在后台
        if(job->state == ST){
            kill(-job->pid, SIGCONT);
            printf("[%d] (%d) %s", job->jid, job->pid, job->cmdline);
            job->state = BG;
        }
    }
    return;
}

/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid)
{
    while(fgpid(jobs)){
        sleep(0);
    }
    return; 
}
```

## eval函数

```c
/* 
 * eval - Evaluate the command line that the user has just typed in
 * 
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
*/
void eval(char *cmdline) 
{
    char *argv[MAXARGS];
    char buf[MAXLINE];
    int bg;
    pid_t pid;
    sigset_t mask_all, prev_mask, mask_two;
    sigfillset(&mask_all);
    sigaddset(&mask_two, SIGCHLD);
    sigaddset(&mask_two, SIGINT);
    sigaddset(&mask_two, SIGTSTP);
    strcpy(buf, cmdline);
    bg = parseline(buf, argv);
    if(argv[0] == NULL){
        return;
    }
    if(!builtin_cmd(argv)){
        sigprocmask(SIG_BLOCK, &mask_two, &prev_mask);
        if((pid = fork()) == 0){
            // 改变组id，不这么做的话，ctrl+c会杀掉所有进程
            setpgid(0, 0);
            sigprocmask(SIG_SETMASK, &prev_mask, NULL);
            // 判断路径是否有该程序，如果执行不成功，则说明不存在
            if(execve(argv[0], argv, environ) < 0){
                // 这里不能用buf，因为execve在会把原有的地址空间中的变量给刷掉，只能用argv[0]
                printf("%s: Command not found\n", argv[0]);
                exit(1);
            }
        }
        sigprocmask(SIG_BLOCK, &mask_all, NULL);
        if(!bg){
            addjob(jobs, pid, FG, buf);
            sigprocmask(SIG_SETMASK, &prev_mask, NULL);
            // 等待前台任务执行完成
            waitfg(pid);
        }else{
            printf("[%d] (%d) %s", nextjid, pid, buf);
            addjob(jobs, pid, BG, buf);
            sigprocmask(SIG_SETMASK, &prev_mask, NULL);
        }
    }
}
```