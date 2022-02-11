[toc]

![](https://cdn.jsdelivr.net/gh/G-ghy/cloudImages@master/202202091541814.png)

> 提供的工具：

- `parseline`：获取参数列表`char **argv`，返回是否为后台运行命令（`true`）。
- `clearjob`：清除`job`结构。
- `initjobs`：初始化`jobs`链表。
- `maxjid`：返回`jobs`链表中最大的`jid`号。
- `addjob`：在`jobs`链表中添加`job`
- `deletejob`：在`jobs`链表中删除`pid`的`job`。
- `fgpid`：返回当前前台运行`job`的`pid`号。
- `getjobpid`：返回`pid`号的`job`。
- `getjobjid`：返回`jid`号的`job`。
- `pid2jid`：将`pid`号转化为`jid`。
- `listjobs`：打印`jobs`。
- `sigquit_handler`：处理`SIGQUIT`信号。

> 整体调用关系：

1. main内循环读取命令行输入
2. main调用eval并将命令行输入作为参数
3. eval调用parseline对命令行输入进行解析，将命令和参数等信息进行分离，同时返回1代表任务执行采用后台模式，返回0表示任务执行采用前台模式
4. eval调用builtin_cmd()判断是否为内置函数，若为内置函数，则在判断时候直接执行，否则返回到eval中进行处理

提供了16个测试点，我们可以把整体的过程看作一个补漏的过程，根据测试内容对程序不断进行完善，这样就降低了难度，否则，从0开始自己想确实不知道怎么入手

1. eval

   书上有一个eval的框架，但是提示说并不会回收后台子进程，需要使用信号机制进行改进，我们先把框架搬下来

   ```c
   void eval(char *cmdline) 
   {
       char *argv[MAXARGS];/*Argument list execve() */
       char buf[MAXLINE];/*Holds modified command line */
       int bg;/*Should the job run in bg or fg? */
       pid_t pid;/*Process id */
   
       strcpy(buf, cmdline);
       bg = parseline(buf, argv);
       if (argv[0] == NULL)
           return;/* Ignore empty lines */
   
       if (!builtin_cmd(argv)) {
           if ((pid = fork()) == 0) {/* Child runs user job */
               execve(argv[0], argv, environ);
           }/* Parent waits for foreground job to terminate */
           if (!bg) {
               int status;
               if (waitpid(pid, &status,0) < 0)
                   unix_error("waitfg: waitpid error");
           }
           else
               printf("%d %s", pid, cmdline);
       }
       return;
   }
   
   ```

   

2. builtin_cmd

   比较容易理解且实现是`builtin_cmd()`，只是一个判断函数

   判断输入的命令是否为指定的4个的内置函数，同时结果`eval()`内对于此函数的调用方式，可以得知，在判断为内置函数后需要做出对应处理

   > 属于内置函数，立即执行对应操作，返回1
   >
   > 不属于内置函数，返回0

   书上有一个样本，模仿着更改即可
    ```c
    int builtin_cmd(char **argv)
    {
        if (!strcmp(argv[0], "quit"))
            exit(0);
   
        if (!strcmp(argv[0], "&"))
            return 1;
   
        // 三者的命令格式是相同的，均可通过do_bgfg解决
        if (!strcmp(argv[0], "fg") || !strcmp(argv[0], "bg") || !strcmp(argv[0], "kill")) {
            do_bgfg(argv);
            return 1;
        }
   
        if (!strcmp(argv[0], "jobs")) {
            listjobs(jobs); // 提供的现成工具
            return 1;
        }
   
        return 0;
    }
    ```

## 各个测试点需要修改的内容

### trace01

检查在读入到达EOF时是否会停止，即使不添加任何内容，仅通过main的内容即可达到这一点

### trace02

需要能够处理quit指令，即需要实现内置函数，即需要完善builtin_cmd函数，自然也就需要完善eval函数

### trace03

需要能够运行可执行程序

eval内的execve能够处理，不需要添加其他内容

### trace04

发现与标准输出在输出格式上存在差异

执行的命令为：`./myspin 1 &`

对于最后的`&`，pdf中做出如下解释

> If the command line ends with an ampersand &, then tsh should run the job in the background.

即，对应代码为eval中最后的print函数，对于其中多出来的数字，在上方提供的工具中只有一个`pid2jid`更有可能

### trace05

从测试结果来看，本次需要实现`jobs`相关命令

虽然在builtin_cmd()中，我们调用了listjobs(jobs)函数，但是它只是负责打印jobs，就是等价于一个遍历链表的操作，可是链表中是空的，所以我们现在要实现向链表中添加job，删除job的操作



#### 一些前置知识

1. 向链表中添加job(`addjob`)是在开辟一个子进程时进行

   从链表中删除job(`deletejob`)就是在子进程结束返回父进程时进行

2. fork()开辟子进程后，在子进程中会执行execve()，这个调用会使得开辟出来的子进程的内容被修改为要执行程序，所以if下面的那些东西在子进程中都没有了，而且子进程会自己结束进程，因为替换成的进程是包含exit的，所以子进程也就会结束
3. sigint_handler，sigtstp_handler 和 sigchld_handler是异步处理函数，可以理解为当3个信号出现时，它们在后台进行一些操作



我们现在遇到的问题是，在输出job时，内容为空，但是我们确实是有job的，之所以没有显示是因为我们没有加入进去

我们现在的主要任务是考虑什么时候向jobs的链表中添加一个job，什么时候删除它

只要不是内置命令，就一定会开辟一个子进程去执行

显然，我们一旦开辟一个子进程，就需要执行一次addjob操作

> addjob操作难道不是只需要在父进程中执行一次就好了吗？
>
> 难道会出现什么错误吗？

与以往编程的不同点在于，现在的运行状况是，多个进程在并发地执行，我们无法得知cpu具体的调度策略，这也就意味着到底是父进程先进行还是子进程先进行我们无法得知，那么进行分类讨论

- 父进程先进行。在开辟了子进程后，父进程先进行，则执行addjob，之后子进程进行，子进程完成后会发出SIGCHLD信号，sigchld_handler会进行响应，既然子进程已经完成了，那么就应当从任务列表中删除，所以sigchld_handler进行的操作包括从jobs中删除这个job
- 子进程先进行。子进程首先完成，那么就需要从任务列表中删除这项任务，可是父进程还没执行，addjob操作还没进行，所以delejob不会产生什么效果，但是转到父进程，addjob仍然会执行，相当于添加了一个不存在的作业，这也就导致了问题

带来这种问题的原因就是，父进程中的代码在fork子进程后不再具有严格的执行顺序了，不能保证addjob和delejob的执行先后顺序

解决方法就是通过信号进制来保证addjob和delejob的执行先后顺序

#### 信号机制

待处理信号：发出但是没有被接受

一种类型至多有一个待处理信号

pending 向量维护待处理信号集合

blocked 向量维护被阻塞信号集合

待处理信号相当于隐式阻塞机制，阻塞相当于显式阻塞机制

隐式是由于之前出现了同类型的待处理信号，所以再出现时选择不处理

显示是人为指定不处理哪些信号

人为设置涉及到函数

以下所说的集合set其实指的就是维护被阻塞信号集合的blocked向量

```c
int sigemptyset(sigset_t *set); // 初始化set为空集合
int sigfillset(sigset_t *set); // 把所有信号加入到集合
int sigaddset(sigset_t *set,  int signum); // 把信号signum添加到set集合中
int sigdelset(sigset_t *set, int signum); // 把信号signum从set集合中删除
int sigprocmask(int how, const sigset_t *set, sigset_t *set); // 根据how改变阻塞信号集合set
```

通过对于被阻塞信号集合的控制可以控制是否接收某些信号，这样操作的用途是，假设我们不希望一些操作进行过程中收到一些信号的影响，那么我们就可以在操作之前将信号阻塞，在操作完成后对信号取消阻塞

![](https://cdn.jsdelivr.net/gh/G-ghy/cloudImages@master/202202091930781.png)

我目前理解的是，每一个进程都有一个信号屏蔽字，但是它属于进程控制块(PCB)的一个字段，我们定义的信号屏蔽字只是为了设置PCB中的那个字段

希望delejob在addjob之后执行，那么需要保证在addjob之前即使子进程发送了SIGCHLD信号，sigchld_handler也不会做出处理，在addjob之后才会对此信号做出处理

所以需要首先将SIGCHLD信号阻塞，它的阻塞位置应当在fork之前，我们需要保证的是fork之后的信号处理在addjob之后进行，在fork之前就要准备好信号处理相关事宜，将信号阻塞放到fork之后，可能已经执行信号处理了，但是还没执行阻塞，那么阻塞就变得毫无意义了

> 子进程是否需要去掉对SIGCHLD的阻塞？
>
> 需要知道的一点：对于阻塞信号集合，子进程继承父进程的集合
>
> 对于子进程而言，我目前觉得即使不取消这个信号的阻塞好像也没有问题，因为它并不会涉及到接收SIGCHLD信号的问题
>
> 
>
> 可是书上说的是：“我们必须在调用execve之前，小心地解除子进程阻塞的SIGCHLD信号”
>
> 
>
> 至于原因我并没有想出来



在父进程中，要什么时候取消对于这个信号的阻塞呢？既然阻塞信号是为了addjob在信号处理之前进行，那么addjob执行后这个信号阻塞的任务也就结束了，即addjob之后解除阻塞即可

具体的实现方式，在书上给出了示例，先选择仿照着来写

[sigprocmask用法](https://jason--liu.github.io/2019/04/15/use-sigprocmask/)

```c
void eval(char *cmdline) 
{
    char *argv[MAXARGS];/*Argument list execve() */
    char buf[MAXLINE];/*Holds modified command line */
    int bg;/*Should the job run in bg or fg? */
    pid_t pid;/*Process id */
    sigset_t mask_all,mask_one,prev_one;

    strcpy(buf, cmdline);
    bg = parseline(buf, argv);
    if (argv[0] == NULL)
        return;/* Ignore empty lines */

    if (!builtin_cmd(argv)) {
        sigfillset(&mask_all);
        sigemptyset(&mask_one);
        sigaddset(&mask_one, SIGCHLD);

        sigprocmask(SIG_BLOCK, &mask_one, &prev_one);
        // 只有子进程的pid==0，即只有子进程执行下面的if
        if ((pid = fork()) == 0) {/* Child runs user job */
            sigprocmask(SIG_SETMASK, &prev_one, NULL);
            execve(argv[0], argv, environ);
        }

        sigprocmask(SIG_BLOCK, &mask_all, NULL);
        int mode = (bg == 0) ? FG : BG;
        addjob(jobs,pid,mode,cmdline);
        sigprocmask(SIG_SETMASK, &prev_one, NULL);

        /* Parent waits for foreground job to terminate */
        if (!bg) {
            int status;
            if (waitpid(pid, &status,0) < 0)
                unix_error("waitfg: waitpid error");
        }
        else
            printf("[%d] (%d) %s", pid2jid(pid), pid, cmdline);
    }

    return;
}
```

最后设定信号屏蔽字，既取消了对SIGCHLD信号的阻塞，也取消了为避免addjob被终端所设置的阻塞全部信号

写到这里，添加job已经完成了，删除job是信号处理函数sigchld_handler()负责的，由于不再eval函数中暂时不作处理

#### waitfg

注意到eval在bg == 0，即子进程为前台任务的情况，需要等待子进程执行结束，题目要求实现waitfg函数以完成该过程

不过不完善的eval已经处理了bg == 0，即子进程为前台任务的情况，会阻塞父进程，不知道为何还提供了一个waitfg

1. 书上的8.5.7一节，介绍了如何显式地等待信号，提出的一种思路是，使用一个变量，这个变量存在一个初始值，同时这个变量在SIGCHLD信号处理函数中会被修改值，通过这两个不同的值判断子进程是否结束，需要使用while循环一直判断

   ![](https://cdn.jsdelivr.net/gh/G-ghy/cloudImages@master/202202101541842.png)

   这里还使用到sigprocmask确保pid赋0在处理SIGCHLD之前进行，waitpid会返回一个非0值

2. 在上面操作的基础上，另一种方式是

   ```C
   while (!pid)
   	pause();
   ```

   pause()的作用是让进程暂停直到信号出现，如果在pause()执行之前信号已经出现，那么就永远无法让pause唤醒，则会永远休眠

3. 一个合适的解决方案：使用sigsuspend

   ![](https://cdn.jsdelivr.net/gh/G-ghy/cloudImages@master/202202101548783.png)

   这一个函数相当于3个函数一同执行

   对于单纯使用pause遇到的问题，如果在while之前阻塞SIGCHLD信号

   可以避免在pause之前执行信号处理函数，也就避免了pause永远无法被唤醒的问题

   这样做解决了冲突的问题，可是pause就无法起到作用了

   pause必须能够接收到SIGCHLD信号，所以在pause之前可以先解除对SIGCHLD信号的阻塞，pause之后再还原，这样可以实现pause的功能

   但是需要注意的是一旦SIGCHLD信号在紧接着pause之前的sigprocmask语句之后，pause之前出现，那么pause还是会永远休眠

   而sigsuspend语句可以保证sigprocmask和pause不可中断地被执行

   ```c
   void waitfg(pid_t pid)
   {
       sigset_t mask, prev;
       
       sigemptyset(&mask);
       sigaddset(&mask, SIGCHLD);
       sigprocmask(SIG_BLOCK, &mask, &prev);
       while (fgpid(jobs) != 0){ // 存在前台进程则等待
           sigsuspend(&prev);
       }
       sigprocmask(SIG_SETMASK, &prev, NULL);
       return;
   }
   ```

   在完成waitfg之后，对应eval也要修改

   ```c
           if (!bg) {
               waitfg(pid);
           }
           else
               printf("[%d] (%d) %s", pid2jid(pid), pid, cmdline);
   ```

   需要注意的是只做完以上部分，程序会无法正常处理前台进程，因为我们还没有写信号处理函数，所以前台程序执行结束后，无法删除jobs内job，所以waitfg一直休眠，程序无法继续执行

### trace06，trace07， trace08(存疑)

查看trace06.txt内容，可以看出它执行了前台程序，同时后续输入了ctrl-c所带来的中断

#### sigint_handler  & sigtstp_handler

即需要我们实现sigint_handler信号处理函数，经查看，sigtstp_handler类似，所以在此一并解决

sigint_handler 负责处理ctrl-c带来的SIGINT信号，sigtstp_handler负责处理ctrl-z带来的SIGTSTP信号，pdf上是这么说的

> 1. Typing ctrl-c causes a SIGINT signal to be delivered to each process in the foreground job. The default action for SIGINT is to terminate the process.
> 2. typing ctrl-z causes a SIGTSTP signal to be delivered to each process in the foreground job. The default action for SIGTSTP is to place a process in the stopped state

这里面一共有4条有效信息：

1. 当按下ctrl-c时，需要我们将信号传送到每一个前台作业
2. SIGINT信号的作用是终止每一个进程
3. 当按下ctrl-z时，同样需要我们将信号传送到每一个前台作业
4. SIGTSTP的作用是修改进程的状态为停止状态

对于信号状态，书籍中有如下内容

![](https://cdn.jsdelivr.net/gh/G-ghy/cloudImages@master/202202102252864.png)

这里在函数上方的注释已经写明了我们需要做的事情

> sigint_handler: Catch it and send it along to the foreground job.
>
> sigtstp_handler: Catch it and suspend the foreground job by sending it a SIGTSTP.

大概意思就是，我们只需要将信号传递给所有前台作业

 Signal(): 设置函数来处理信号

![](https://cdn.jsdelivr.net/gh/G-ghy/cloudImages@master/202202102259675.png)

结合main中的Signal函数，我想到，这里修改这几个信号的处理程序，收到影响的只有tsh这个进程，而由tsh开辟的进程的信号处理函数并没有被修改且它们是有默认的操作的

之前我一直疑惑，如果我们只负责将信号传递给其他进程，还是没办法处理呀，这才想到它们的信号处理函数没有改变，是具有默认操作的

把资源传递给前台作业，对于这项任务，需要理解现在的进程分布情况

关于pid和jid的区别，pid是进程id，jid是作业id

一个作业可能包含多个进程，例如一个前台作业 `ls | sort`

这属于一项作业，但是ls需要开辟一个子进程，sort需要开辟一个子进程

由此来看，一个作业可以看为是一个进程组

![](https://cdn.jsdelivr.net/gh/G-ghy/cloudImages@master/202202092253514.png)

以图中左侧前台作业为例，整个框属于一个进程组，也就是一个作业，

其中包含3个进程，pid分别为20，21，22；整个进程组id，或者说作业id(jid)为20

在本实验中，jobs像是一个任务列表，每当新开一个子进程时都会加入到jobs中,

jobId只是说明是第几个任务，并不同于进程组id

对于目前的情况进行分析，可以得出：tsh本身属于bash(shell)前台作业的一个子进程，只不过tsh一直处在死循环中，所以tsh一直占用着前台作业，通过tsh执行的其他命令，就是tsh开辟的子进程，**所有进程目前都处于同一个进程组中**

当执行内置命令时，就在tsh这个进程中进行，否则会开辟一个子进程。当子进程作为前台执行时，当我们执行ctrl-c时，子进程执行的任务应当结束，tsh进程不应受到影响，后台作业也不应当受到影响，例如那些后台的守护进程

首先我们需要使用kill函数来发送信号

> int kill(pid_t pid, int sig)
>
> - pid > 0: 信号sig -> pid
> - pid == 0: 信号sig -> pid进程所在进程组的每个进程
> - pid < 0: 信号sig -> 进程组|pid|内每个进程

下面两个信号处理函数都是在tsh父进程中执行，对于pid这个参数的选取是个问题，按照目前的情况，由于是向多个进程发送消息，pid>0不可选，如果我们选择pid=0，那么tsh会将所有作业kill掉，包括tsh本身，不可行，pid<0，由于前台作业与tsh同属于一个进程组，tsh本身还是会被kill掉

所以面对现在这种情况，我们需要修改子进程所属的进程组，维护一种局面：

> bash的前台作业进程组中，包含tsh进程以及tsh开辟的后台子进程
>
> tsh开辟的前台子进程另属于一个进程组

这样我们才可以正确地令信号发生作用

在eval中做出修改,

> int setpgid(pid_t pid,pid_t pgid);
>
> - pid==0:将当前进程的pid作为进程组ID
> - pgid==0:将pid作为进程组ID

tsh开辟的前台作业至多1个，所以使用kill(-pid, sig)即可将kill前台进程

```c
void eval(char *cmdline) 
{
    char *argv[MAXARGS];/*Argument list execve() */
    char buf[MAXLINE];/*Holds modified command line */
    int bg;/*Should the job run in bg or fg? */
    pid_t pid;/*Process id */
    sigset_t mask_all,mask_one,prev_one;

    strcpy(buf, cmdline);
    bg = parseline(buf, argv);
    if (argv[0] == NULL)
        return;/* Ignore empty lines */

    if (!builtin_cmd(argv)) {
        sigfillset(&mask_all);
        sigemptyset(&mask_one);
        sigaddset(&mask_one, SIGCHLD);

        sigprocmask(SIG_BLOCK, &mask_one, &prev_one);
        // 只有子进程的pid==0，即只有子进程执行下面的if
        if ((pid = fork()) == 0) {/* Child runs user job */
            sigprocmask(SIG_SETMASK, &prev_one, NULL);
            if (!bg)
            	setpgid(0, 0);
            execve(argv[0], argv, environ);
        }

        sigprocmask(SIG_BLOCK, &mask_all, NULL);
        int mode = (bg == 0) ? FG : BG;
        addjob(jobs,pid,mode,cmdline);
        sigprocmask(SIG_SETMASK, &prev_one, NULL);

        /* Parent waits for foreground job to terminate */
        if (!bg) {
            waitfg(pid);
        }
        else
            printf("[%d] (%d) %s", pid2jid(pid), pid, cmdline);
    }

    return;
}
```

```c
void sigint_handler(int sig)
{
    pid_t pid;
    if ((pid = fgpid(jobs)) > 0)
        kill(-pid, sig);
    return;
}

void sigtstp_handler(int sig)
{
    pid_t pid;
    if ((pid = fgpid(jobs)) > 0)
        kill(-pid, sig);
    return;
}
```

不过我感觉既然最多只有1个前台进程，那么我们直接kill掉它即可，没必要牵扯到进程组

如果有多个前台进程，一个信号需要中断所有进程，则需要涉及到进程组，现在不太需要

所以可以尝试的另一种方案是，eval中不做出修改

```c
void sigint_handler(int sig)
{
    pid_t pid;
    if ((pid = fgpid(jobs)) > 0)
        kill(pid, sig);
    return;
}

void sigtstp_handler(int sig)
{
    pid_t pid;
    if ((pid = fgpid(jobs)) > 0)
        kill(pid, sig);
    return;
}
```

> 这里有问题，必须使用setpgid将子进程单独设置一个进程组trace08才可以正常退出，下面不管是-pid还是pid，只要加上setpgid就可以且都可以执行，不知道是什么原因，从主观上想，这个操作应该是无所谓的才对呀

#### sigchld_handler

实现以上过程，在使用ctrl-c时，可以kill掉前台子进程，但是代码的执行结果并没有改变，因为waitfg中的循环仍然无法结束，因为前台子进程虽然结束了，但是还并没有从jobs中删除对应的job，换句话说子进程在发除SIGCHLD信号后，还缺少一些处理，即我们接下来需要完善sigchld_handler函数

```c
    while (fgpid(jobs) != 0){ // 存在前台进程则等待
        sigsuspend(&prev);
    }
```

到此为止，在eval函数中，添加job和等待前台进程的操作已经完成了，接下来需要编写删除job的函数sigchld_handler

在当前这个环境下，有3种情况需要处理

1. 进程正常结束产生SIGCHLD

   从jobs中删除任务即可，不需要输出信息

2. 由于ctrl-c使得进程结束产生SIGCHLD

   从jobs中删除job，同时还需要输出以下信息

   > Job [1] (6241) terminated by signal 2

3. 由于ctrl-z使得进程被挂起，处于停止状态产生SIGCHLD

   不需要删除job，将job状态更改为ST，同时输出以下信息

   > Job [2] (26276) stopped by signal 20

   ![](https://cdn.jsdelivr.net/gh/G-ghy/cloudImages@master/202202102252864.png)

   ![](https://cdn.jsdelivr.net/gh/G-ghy/cloudImages@master/202202111024830.png)

> 如何判断一个SIGCHLD信号的发生属于哪种情况

![](https://cdn.jsdelivr.net/gh/G-ghy/cloudImages@master/202202111034146.png)

status的获取方式，需要使用的是`waitpid`

> waitpid函数: 等待它的子进程终止或者停止
>
> pid_t waitpid(pid_t pid, int *statusp, int options)
>
> 1. pid
>
>    - pid > 0: 等待单独的进程id为pid的子进程
>    - pid == -1：等待集合为父进程的所有子进程
>
>    在出现SIGCHLD信号时，父进程是无法得知是哪个子进程触发的，所以只能选择使用-1
>
> 2. statusp
>
>    就是我们获取status的途径
>
> 3. options
>
>    有3种行为，但是书上给出一个例子，功能是符合我们的需求的
>
>    ![](https://cdn.jsdelivr.net/gh/G-ghy/cloudImages@master/202202111039211.png)
>
> 返回值：在我们的传参下
>
> - 返回值为0：没有子进程停止或终止
> - 返回值>0：停止或终止的子进程PID

对于waitpid，不能使用if仅判断一次，因为等待集合中可能有多个子进程停止或终止，应当使用while

同时还需要注意的是，为了防止addjob和delejob发生竞争，要把信号进行阻塞

```c
void sigchld_handler(int sig) 
{
    sigset_t mask_all, prev;
    int status;
    pid_t pid;
    struct job_t *job;
    
    sigfillset(&mask_all);
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        sigprocmask(SIG_BLOCK, &mask_all, &prev);
        if (WIFEXITED(status)) {
            deletejob(jobs, pid);
        } else if (WIFSIGNALED(status)) {
            printf("Job [%d] (%d) terminated by signal %d\n", pid2jid(pid), pid, WTERMSIG(status));
            deletejob(jobs, pid);
        } else if (WIFSTOPPED(status)) {
            printf("Job [%d] (%d) stoped by signal %d\n", pid2jid(pid), pid, WSTOPSIG(status));
            job = getjobpid(jobs, pid);
            job->state = ST;
        }
        sigprocmask(SIG_SETMASK, &prev, NULL);
    }
    return;
}
```

### trace09，trace10，trace11，trace12，trace13

查看trace09.txt，需要实现bg和fg命令，这两个命令的功能如下

> bg  \<job\>: Change a stopped background job to a running background job.
>
> fg \<job\> : Change a stopped or running background job to a running in the foreground.

对于一个job，分类如下：

- 前台
  - 运行态
  - 挂起态
- 后台
  - 运行态
  - 挂起态

bg命令是使后台挂起态进程转变为后台运行态进程

fg命令是使后台挂起态进程转变为前台运行态进程 或 后台运行态进程转变为前台运行态进程

通过SIGCONT信号可以实现唤醒一个stopped状态的进程

两个命令指定作业的方式有2种

1. 指定进程id，5 -> PID 5
2. 指定作业id，%5 -> JID 5

首先我们查看tshref.out，检查一下有哪些输出情况

- bg命令通过指定JID正常输出

  > [2] (26283) ./myspin 5

- fg命令通过指定JID正常输出

  > 输出为空

- fg和bg命令缺少参数

  > fg(bg) command requires PID or %jobid argument

  ```
  if(!argv[1]){
          printf("%s command requires PID or %%jobid argument\n", argv[0]);
          return;
      }
  ```

- 参数为非法值

  > fg(bg): argument must be a PID or %jobid

- 参数合法但没有找到对应进程

  > (9999999): No such process
  >
  > %2: No such job

  对于这两项内容，一种较巧妙的方式是通过sscanf解析获取pid和jid，使用sscanf，从字符串中分别按照pid和jid的方式进行数据读取，如果两种方式都没有查找到job，则代表参数为非法值

  ```C
  if (sscanf(argv[1], "%d", &pid) > 0) {
          if ((job = getjobpid(jobs, pid)) == NULL) { // 参数合法但是没有找到对应进程
              printf ("(%s): No such process\n", argv[1]);
              return;
          }
      } else if (sscanf(argv[1], "%%%d", &jid) > 0) {
          if ((job = getjobjid(jobs, jid)) == NULL) { // 参数合法但是没有找到对应进程
              printf ("%s: No such job\n", argv[1]);
              return;
          }
      } else { // 参数非法
          printf ("%s: argument must be a PID or %%jobid\n", argv[0]);
          return;
      }
  ```

很神奇的感觉只需要修改job的state字段就可以改变它的表现状态,不太懂这个进程的调度方式，还有发送这个信号到底起到什么实际作用了

```
    kill(-(job->pid), SIGCONT);
    if (!strcmp(argv[0], "bg")) {
        job->state = BG;
        printf ("[%d] (%d) %s", job->jid, job->pid, job->cmdline);
    } else {
        job->state = FG;
        waitfg(job->pid);
    }
```

### trace14, trace15, trace16

第一个命令是一个不存在的命令，需要输出提示内容

> ./bogus: Command not found

执行外部命令用到了execve，执行的结果如何就要看execve的反馈结果了

> On success, execve() does not return, on error -1 is returned

```c
            if (execve(argv[0], argv, environ) < 0) {
                printf("%s: Command not found\n", argv[0]);
                exit(0);
            }
```



最后剩下的疑惑就是进程组的问题了

为什么开辟子进程时一定要修改它的进程组

而且相当于所有子进程所在组都是不同的

最后一个测试点用到的程序mystop，在发送信号时为什么要向进程组发送，而不是向自己的进程发送

那些不能被中断的执行过程，有一个共同的特征是都使用到了公共资源，例如addjob和delejob，它们都使用到了jobs这个资源，如果不能保证进程推进顺序的确定性，则对资源的操作顺序就是不确定的，从而可能引发差错

为什么需要修改子进程的进程组号，假设此时子进程正在执行，但是我们希望停止它的执行，于是我们会按下ctrl+c键，此操作带来的效果是会把当前进程所有进程组内的所有进程全部停止，包括这个进程，子进程是tsh进程开辟的子进程，所以子进程和tsh进程同属于一个进程组，这就会导致将tsh一同终止，这显然不是我们的目的效果，所以我们需要修改它的进程组id
