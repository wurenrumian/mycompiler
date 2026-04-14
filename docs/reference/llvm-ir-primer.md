中间代码⽣成 - LLVM
对于⼤部分同学来说 , 可能是第⼀次接触 LLVM, 虽然看上去上⼿⽐较困难 , 但了解了基本语法后
还是⽐较容易的。本教程将分模块为⼤家讲解 LLVM IR 主要特点和语法结构 , 实验⽂法、C 语⾔
函数与 LLVM IR 之间的对应转换关系 , 以及如何使⽤ LLVM IR 进⾏代码⽣成。建议学习过程中可
以结合语法分析和中间代码⽣成。
⼀、基本概念
(1) LLVM?
LLVM 最早叫底层虚拟机(Low Level Virtual Machine), 最初是伊利诺伊⼤学的⼀个研究项⽬ , ⽬
的是提供⼀种现代的、基于 SSA(Static Single Assignment, 静态单⼀赋值)的编译策略 , 能够⽀
持任意编程语⾔的静态和动态编译。到现在 ,LLVM 已经发展成为⼀个由多个⼦项⽬组成的伞式
项⽬ , 其中许多⼦项⽬被各种各样的商业和开源项⽬⽤于⽣产 , 并被⼴泛⽤于学术研究。
现在 ,LLVM 被⽤作实现各种静态和运⾏时编译语⾔的通⽤基础设施(例如 C/C++、
Java、.NET、Python、Ruby、Scheme、Haskell、D 以及⽆数鲜为⼈知的语⾔所⽀持的语⾔
族)。下⾯是⼀些参考资料 :
• https://aosabook.org/en/v1/llvm.htm
• https://llvm.org/
(2) 三端设计
LLVM 流⾏的⼀个⾮常重要的原因就是它采⽤了三端设计 , 并提供了⼀个⾮常灵活⽽强⼤的中间
代码表⽰形式。在三端设计中 , 主要组件是前端、优化器和后端。前端解析源代码 , 检查其错
误 , 并构建特定语⾔的抽象语法树(Abstract Syntax Tree,AST)来表⽰输⼊的源代码。AST 可以
进⼀步转化为新的⽬标码进⾏优化 , 最终由后端⽣成具体的⼆进制可执⾏⽂件。优化器的作⽤
是提⾼代码的运⾏效率 , 例如消除冗余计算。后端负责将代码映射到⽬标指令集 , 其常⻅的功能
包括指令选择、寄存器分配和指令调度。
这种设计最重要的优点在于对多种源语⾔和⽬标体系结构的⽀持。不难发现 , 对于 M 种源语
⾔ ,N 种体系结构 , 如果前后端不分开 , 将会需要 M × N 个编译器 , ⽽在三端设计中 , 只需要 M 种
前端和 N 种后端 , 实现了复⽤。这⼀特点的官⽅表述是 Retargetablity, 即可重定向性或可移植
性。同学们熟悉的 GCC、Java 中的 JIT, 都是采⽤这种设计。下⾯这张图反映了 LLVM 中三端设
计的应⽤。
⼆、LLVM 快速上⼿
⽐起指导书 , 亲⾃体验的学习效率要⾼很多。这⼀节中会为⼤家介绍如何在本地⽣成并运⾏
LLVM IR。
在平时的使⽤中 ,LLVM ⼀般指的是 LLVM IR, 即中间代码。中间代码往往不能单独存在 , 因此我
们需要有⼀个能⽣成 LLVM IR 的编译器 , 所以我们需要使⽤ GCC 之外的另⼀款编译器 Clang, 也
是 LLVM 的⼦项⽬之⼀。
在我们的实验中 , 不同版本的 Clang 输出的 LLVM 可能有少许区别 , 但不会对正确性造成影响，
同学们下载最新的版本即可。
(1) 开发环境准备
在我们的实验中 , 主要会⽤到两个⼯具 :clang 和 lli。clang 是 LLVM 项⽬中 C/C++ 语⾔的前端 ,
其⽤法与 GCC 基本相同 , 可以为源代码⽣成 LLVM IR。lli 会解释执⾏ .bc 和 .ll 程序 , ⽅便⼤家验
证⽣成的 LLVM IR 的正确性。
1. Linux
Linux 下的环境配置相对更⽅便 , 使⽤ Windows 的话推荐⼤家使⽤ WSL 2+Ubuntu, 其他 Linux
环境如虚拟机、云服务器也都是可以的。
⾸先更新包信息。
sudo apt update
sudo apt upgrade
1
2
Bash
如果不在意版本 , 直接进⾏安装即可。
如果⽐较在意版本 , 可以使⽤ apt search 查找当前发⾏版包含的 Clang 版本。在 Ubuntu 20.04
中 , 有 7-12; 在 Ubuntu 22.04 中 , 有 11-15, ⼤家可以选择⾃⼰喜欢的版本 , 当然⾼版本会更好。
安装特定版本时 , 可以⼿动修改默认的版本 , 从⽽避免每次命令都包含版本号。
sudo apt install llvm
sudo apt install clang
1
2
$ apt search clang | grep -P ^clang-[0-9]+/
clang-10/focal 1:10.0.0-4ubuntu1 amd64
clang-11/focal-updates 1:11.0.0-2~ubuntu20.04.1 amd64
clang-12/focal-updates,focal-security 1:12.0.0-3ubuntu1~20.04.5 amd64
clang-7/focal 1:7.0.1-12 amd64
clang-8/focal 1:8.0.1-9 amd64
clang-9/focal 1:9.0.1-12 amd64
$ apt search llvm | grep -P ^llvm-[0-9]+/
llvm-10/focal 1:10.0.0-4ubuntu1 amd64
llvm-11/focal-updates 1:11.0.0-2~ubuntu20.04.1 amd64
llvm-12/focal-updates,focal-security 1:12.0.0-3ubuntu1~20.04.5 amd64
llvm-7/focal 1:7.0.1-12 amd64
llvm-8/focal 1:8.0.1-9 amd64
llvm-9/focal 1:9.0.1-12 amd64
1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
Bash
Bash
安装完成后 , 输⼊指令查看版本。如果出现版本信息则说明安装成功。
2. Windows
Windows 安装 Clang ⽐较⿇烦 , 但是我们可以借助强⼤的 Visual Studio。还没有 Visual
Studio? 可以去官⽹下载免费的社区版。安装时 , 需要在 Individual components ⾥勾选对
Clang 的⽀持。
安装完成后 , 你的电脑中应该会多出 Visual Studio 的命令⾏⼯具 , 就可以在其中使⽤ clang
了。
sudo apt install llvm-12
sudo apt install clang-12
sudo update-alternatives --install /usr/bin/lli lli /usr/bin/lli-12 100
sudo update-alternatives --install /usr/bin/llvm-link llvm-link /usr/bi
n/llvm-link-12 100
sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-
12 100
sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/cl
ang++-12 100
1
2
3
4
5
6
clang -v
lli --version
1
2
Bash
Bash
不幸的是 ,Visual Studio 并没有直接对 lli 的⽀持 , ⼿动构建⽐较⿇烦 , 感兴趣的同学可以⾃⾏尝
试。
3. MacOS
MacOS 上 LLVM 的安装需要 XCode 或 XCode Command Line Tools, 其默认⾃带 Clang ⽀持。
安装完成后 , 需要添加 LLVM 到 $PATH。
这时候可以仿照之前查看版本的⽅法 , 如果显⽰版本号则证明安装成功。
上⽂指导安装的是 LLVM 的 Dev 版本 , 只能编译运⾏优化 LLVM 代码 , 不⽀持代码调试。LLVM
的 Debug 版本需要的空间较⼤(包括相关配置⼯具在内共约 30G), 且安装操作复杂 ; 实际调试时
还需要针对性的编写 LLVM pass。因此 , 课程组不推荐同学们使⽤ LLVM 的系统调试⼯具。阶段
性打印信息到控制台不失为⼀种快捷有效的 debug ⽅法。
(2) 基本命令
LLVM IR 具有三种表⽰形式 , ⾸先当然是代码中的数据结构 , 其次是作为输出的⼆进制位码
(Bitcode)格式 .bc 和⽂本格式 .ll。在我们的实验中 , 要求⼤家能够输出正确的 .ll ⽂件。下⾯是
⼀些常⽤指令 , 针对编译执⾏的不同阶段输出相应结果 :
xcode-select --install
brew install llvm
1
2
1 echo 'export PATH="/usr/local/opt/llvm/bin:$PATH"' >> ~/.bash_profile
Bash
Bash
作为⼀⻔全新的语⾔ , 与其讲过于理论的语法 , 不如直接看⼀个实例来得直观 , 也⽅便⼤家快速
⼊⻔。例如 , 给出源程序 main.c 如下。
使⽤命令 后 , 会在同⽬录下⽣成⼀个 main.ll 的
⽂件。在 LLVM IR 中 , 注释以 打头。
clang -ccc-print-phases main.c # 查看编译的过程
clang -E -Xclang -dump-tokens main.c # ⽣成 tokens（词法分析）
clang -fsyntax-only -Xclang -ast-dump main.c # ⽣成抽象语法树
clang -S -emit-llvm main.c -o main.ll -O0 # ⽣成 LLVM ir (不开优化)
clang -S -emit-llvm main.m -o main.ll -Os # ⽣成 LLVM ir (中端优化)
clang -S main.c -o main.s # ⽣成汇编
clang -S main.bc -o main.s -Os # ⽣成汇编（后端优化）
clang -c main.c -o main.o # ⽣成⽬标⽂件
clang main.c -o main # 直接⽣成可执⾏⽂件
1
2
3
4
5
6
7
8
9
int a = 1;
int add(int x, int y) {
return x + y;
}
int main() {
int b = 2;
return add(a, b);
}
1
2
3
4
5
6
7
8
9
10
clang -S -emit-llvm main.c -o main.ll
;
Bash
Plain Text
; ModuleID = 'main.c'
source_filename = "main.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:
128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"
; 从下⼀⾏开始，是实验需要⽣成的部分，注释不要求⽣成。
@a = dso_local global i32 1, align 4
; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @add(i32 %0, i32 %1) #0 {
%3 = alloca i32, align 4
%4 = alloca i32, align 4
store i32 %0, i32* %3, align 4
store i32 %1, i32* %4, align 4
%5 = load i32, i32* %3, align 4
%6 = load i32, i32* %4, align 4
%7 = add i32 %5, %6
ret i32 %7
}
; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 {
%1 = alloca i32, align 4
%2 = alloca i32, align 4
store i32 0, i32* %1, align 4
store i32 2, i32* %2, align 4
%3 = load i32, i32* @a, align 4
%4 = load i32, i32* %2, align 4
%5 = call i32 @add(i32 %3, i32 %4)
ret i32 %5
}
; 实验要求⽣成的代码到上⼀⾏即可
attributes #0 = { noinline nounwind optnone uwtable ...}
; 实际的 attributes 很多，这⾥进⾏了省略
!llvm.module.flags = !{!0}
1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
16
17
18
19
20
21
22
23
24
25
26
27
28
29
30
31
32
33
34
35
36
37
38
Markdown
⽤ lli main.ll 解释执⾏⽣成的 .ll ⽂件。如果⼀切正常，输⼊ echo $? 查看上⼀条指令的返回
值。
LLVM IR 使⽤的是三地址码。下⾯对上述代码进⾏简要注释。
• Module ID：指明 Module 的标识
• source_filename：表明该 Module 是从什么⽂件编译得到的。如果是通过链接得到的，
此处会显⽰ llvm-link
• target datalayout 和 target triple 是程序标签属性说明，和硬件 / 系统有关。
• ：全局变量，名称是 a，类型是 i32，初始值
是 1，对⻬⽅式是 4 字节。dso_local 表明该变量会在同⼀个链接单元（llvm-link 命令的源
⽂件范围）内解析符号。
◦ 这⾥的 align 其实并不是必须的，⼤家在中间代码⽣成中也不⼀定需要⽣成。
• ：函数定义。其中第⼀个 i32 是返
回值类型，@add 是函数名；第⼆个和第三个 i32 是形参类型，%0，%1 是形参名。
◦ LLVM 中的标识符分为两种类型：全局的和局部的。全局的标识符包括函数名和全局变
量，会加⼀个 @ 前缀，局部的标识符会加⼀个 % 前缀。
◦ #0 指出了函数的 attribute group。由于每个函数的 attribute 很多，⽽且不同函数的
attributes 往往相同，因此将相同的 attributes 合并为⼀个 attribute group，从⽽使 IR
更加简洁清晰。
• ⼤括号中间的是函数体，是由⼀系列 BasicBlock 组成的。每个 BasicBlock 都有⼀个
label，label 使得该 BasicBlock 有⼀个符号表的⼊⼝点。BasicBlock 以 terminator
instruction（ret、br 等）结尾。每个 BasicBlock 由⼀系列 Instruction 组成，Instruction
是 LLVM IR 的基本指令。
• ：随便拿上⾯⼀条指令来说，%7 是⼀个临时寄存器，是
Instruction 的实例，它的操作数⾥⾯有两个值，⼀个是 %5，⼀个是 %6。%5 和 %6 也是
临时寄存器，即前两条 Instruction 的实例。
（3）LLVM IR 指令概览
对于⼀些常⽤的 Instructions，下⾯给出⽰例。对于⼀些没有给出的，可以参考 LLVM IR 指令
集。
!llvm.ident = !{!1}
!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 10.0.0-4ubuntu1 "}
39
40
41
42
@a = dso_local global i32 1, align 4
define dso_local i32 @add(i32 %0, i32 %1) #0
%7 = add i32 %5, %6
LLVM IR 指令 使⽤⽅法 简介
add <result> = add <ty> <op1>,
<op2>
/
sub <result> = sub <ty> <op1>,
<op2>
/
mul <result> = mul <ty> <op1>,
<op2>
/
sdiv <result> = sdiv <ty> <op1>,
<op2>
有符号除法
srem <result> = srem <type>
<op1>, <op2>
有符号取余
icmp <result> = icmp <cond> <ty>
<op1>, <op2>
⽐较指令
and <result> = and <ty> <op1>,
<op2>
按位与
or <result> = or <ty> <op1>,
<op2>
按位或
call <result> = call [ret attrs]
<ty> <name>(<...args>)
函数调⽤
alloca <result> = alloca <type> 分配内存
load <result> = load <ty>, ptr
<pointer>
读取内存
store store <ty> <value>, ptr
<pointer>
写内存
getelementptr <result> = getelementptr
<ty>, ptr <ptrval>{, <ty>
<idx>}*
计算⽬标元素的位置（数组部分会单
独详细说明）
phi <result> = phi [fast-math￾flags] <ty> [<val0>,
<label0>], ...
/
（4）LLVM IR 构建单元
在 C/C++ 中，⼀个 .c/.cpp ⽂件是⼀个编译单元，在 LLVM IR 中也是⼀样，⼀个 .ll ⽂件对应⼀
个 Module。在前的⽰例中⼤家可能已经注意到了，LLVM IR 有着⾮常严格清晰的结构，如下
图。
细⼼的同学可能注意到了图中使⽤的箭头和斜体，不⽤怀疑，它们分别表⽰的就是⾯向对象中
的继承与抽象类。
在 LLVM IR 中，⼀个 Module 由若⼲ GlobalValue 组成，⽽⼀个 GlobalValue 可以是全局变量
（GlobalVariable），也可以是函数（Function）。⼀个函数由若⼲基本块（BasicBlock）组
成，与⼤家在理论课上学习的基本块是⼀样的。在基本块内部，则是由若⼲指令
（Instruction）组成，也是 LLVM IR 的基本组成。
zext..to <result> = zext <ty> <value>
to <ty2>
将 ty 的 value 的 type 扩充为 ty2
（zero extend）
trunc..to <result> = trunc <ty>
<value> to <ty2>
将 ty 的 value 的 type 缩减为 ty2
（truncate）
br br i1 <cond>, label
<iftrue>, label <iffalse>
br label <dest>
改变控制流
ret ret <type> <value> , ret
void
退出当前函数，并返回值
可以发现，LLVM IR 中所有类都直接或间接继承⾃ Value，因此在 LLVM IR 中，有"⼀切皆
Value"的说法。通过规整的继承关系，我们就得到了 LLVM IR IR 的类型系统。为了表达 Value
之间的引⽤关系，LLVM IR 中还有⼀种特殊的 Value 叫做 User，其将其他 Value 作为参数。例
如，对于下⾯的代码：
其中，A 是⼀条 Instruction，更具体的，是⼀个 BinaryOperator，它在代码中的体现就是
%add1，即指令的返回值，也称作⼀个虚拟寄存器。Instruction 继承⾃ User，因此它可以将
其他 Value（%a 和 %b）作为参数。因此，在 %add1 与 %a、%b 之间分别构成了 Use 关
系。紧接着，%add1 ⼜和 %add2 ⼀起作为 %sum 的参数，从⽽形成了⼀条 Use 链。
LLVM IR 是可以⽤⾮数字作为变量标识符的，但⼆者不能混⽤。
这种指令间的关系正是 LLVM IR 的核⼼之⼀，实现了 SSA 形式的中间代码。这样的形式可以⽅
便 LLVM IR 进⾏分析和优化，例如，在这样的形式下，可以很容易地识别出，%add1 和
%add2 实际上是同⼀个值，因此可以进⾏替换。同时，如果⼀个 Value 没有 Use 关系，那么
它很可能就是可以删除的冗余代码。
同学们可以根据⾃⼰的需要⾃⾏设计数据类型。但如果对代码优化效果要求不⾼，这部分内容
其实没有那么重要，可以直接⾯向 AST ⽣成代码。（你甚⾄不需要中间代码就能⽣成⽬标代
码。）
（5）⼀些说明
LLVM 是⼀个⾮常庞⼤的系统，这⾥介绍的仅仅是它的冰⼭⼀⻆，下⾯给出⼀些 LLVM 的⽂档供
⼤家参考。
• Value Class Reference：完整的 Value 继承结构。
• Type Class Reference：完整的 Type 继承结构。
• llvm-mirror/llvm：⽐较旧，但可读性更⾼的 LLVM 版本，可以参考其中的实现。
• llvm/llvm-project：最新的 LLVM 源代码。
这⼀部分代码⽣成的⽬标是根据语法、语义分析⽣成的 AST，构建出 LLVM IR（或是四元式）。
想继续⽣成 MIPS 代码的同学也可以先⽣成 LLVM IR 来检验⾃⼰中间代码的正确性。
A: %add1 = add nsw i32 %a, %b
B: %add2 = add nsw i32 %a, %b
C: %sum = add nsw i32 %add1, %add2
1
2
3
Plain Text
⼤家可能会发现，clang ⽣成的 LLVM IR 中，虚拟寄存器是按数字命名的。LLVM IR 限制了⼀个
函数内所有数字命名的虚拟寄存器必须严格从 0 开始递增。其实，这些数字就是每个（对应了
虚拟寄存器的） Value 在 Function 内的顺序，实现起来并没有想象中那么复杂，可以参考
LLVM IR 中的 SlotTracker 类。如果不想严格按照数字命名，那么就需要使⽤⾮数字的字符串命
名，两种⽅式不能混⽤。
LLVM IR 的架构⽐较复杂，为了⽅便同学们理解，我们定义了⼀个较为简单的语法 tolangc，并
为它实现了⼀套相对简单的 LLVM IR 数据结构，⼤家可以进⾏参考。
三、主函数与常量表达式
（1）主函数
⾸先从最基本的开始，即只包含 return 语句的主函数 main（或没有参数的函数）：
主要包含⽂法如下。
对于⼀个⽆参的函数，⾸先需要从 AST 获取函数的名称，返回值类型。然后分析函数体的
Block。BlockItem 中的 Stmt 可以是 return 语句，也可以是其他语句，但是这⾥只考虑
int main() {
return 0;
}
1
2
3
CompUnit → MainFuncDef
MainFuncDef → 'int' 'main' '(' ')' Block
Block → '{' { BlockItem } '}'
BlockItem → Stmt
Stmt → 'return' Exp ';'
Exp → AddExp
AddExp → MulExp
MulExp → UnaryExp
UnaryExp → PrimaryExp
PrimaryExp → Number
1
2
3
4
5
6
7
8
9
10
Plain Text
Plain Text
return 语句。return 语句中的 Number 在现在默认是常数。所以对于⼀个这样的⼀个代码⽣成
器，同学们需要实现的功能有：
• 遍历 AST，遍历到函数时，获取函数的名称、返回值类型
• 遍历到 BlockItem 内的 Stmt 时，如果是 return 语句，⽣成对应的指令
（2）常量表达式
新增内容有
对于常量表达式，这⾥只包含常数的四则运算，正负号操作。这时候就需要⽤到之前的 Value
思想。举个例⼦，对于 ，根据⽂法⽣成的 AST 样式如下：
Stmt ! 'return' [Exp] ';'
Exp ! AddExp
AddExp ! MulExp | AddExp ('+'|'−') MulExp
MulExp ! UnaryExp | MulExp ('*'|'/'|'%') UnaryExp
UnaryExp ! PrimaryExp | UnaryOp UnaryExp
PrimaryExp ! '(' Exp ')' | Number
UnaryOp ! '+'|'−'
1
2
3
4
5
6
7
1+2+3*4
Plain Text
按最左推导的⽣成顺序，先⽣成 1，然后⽣成 2，然后⽣成 1+2，然后⽣成 3，然后⽣成 4，然
后⽣成 34，最后⽣成 1+2+34。
同理，对于数字前的正负，可以看做是 0 和其做⼀次运算，即 +1 其实就是 0+1（其实正号甚⾄
都不⽤去管他），-1 其实就是 0-1。所以在⽣成代码的时候，可以当作⼀个特殊的 Add Exp 来处
理。
(3)测试样例
源程序如下。
⽣成代码参考，由于不同编译器处理的细节可能不同，因此⽣成的代码不⼀定要完全⼀样。
当然，这些仅包含常量的计算也可以由编译器完成，即直接⽣成 。
四、全局变量与局部变量
(1)全局变量
本章实验涉及的⽂法包括：
int main(){
return -3+9*66/99%17;
}
1
2
3
define dso_local i32 @main(){
%1 = sub nsw i32 0, 3
%2 = mul nsw i32 9, 66
%3 = sdiv i32 %2, 99
%4 = srem i32 %3, 17
%5 = add nsw i32 %1, %4
ret i32 %5
}
1
2
3
4
5
6
7
8
ret i32 3
Plain Text
Plain Text
在 LLVM IR 中，全局变量使⽤的是和函数⼀样的全局标识符 ，所以全局变量的写法其实和函
数的定义⼏乎⼀样。在本次的实验中，全局变 / 常量声明中指定的初值表达式必须是常量表达
式，例如：
⽣成的 LLVM IR 如下所⽰：
可以看到，对于全局变量中的常量表达式，在⽣成的 LLVM IR 中需要直接算出其具体的值，同
时，也需要完成必要的类型转换。此外，全局变量对应的是⼀个地址，使⽤时需要结合
load/store 指令。
需要注意的是，我们的⽂法中存在 int 和 char 两个不同⼤⼩的类型，因此我们需要选择 LLVM
IR 中合适的类型，即 和 。LLVM IR 中，不同类型的变量不能直接运算，因此会涉及类
型转换，具体内容⻅类型转换部分。
CompUnit ! {Decl} MainFuncDef
Decl ! ConstDecl | VarDecl
ConstDecl ! 'const' BType ConstDef {',' ConstDef } ';'
BType ! 'int' | 'char'
ConstDef ! Ident '=' ConstInitVal
ConstInitVal ! ConstExp
ConstExp ! AddExp
VarDecl ! BType VarDef {',' VarDef } ';'
VarDef ! Ident | Ident '=' InitVal
InitVal ! Exp
1
2
3
4
5
6
7
8
9
10
@
//以下都是全局变量
int a = 'a';
char b = 2+3;
1
2
3
@a = dso_local global i32 97
@b = dso_local global i8 5
1
2
i32 i8
Plain Text
Plain Text
Plain Text
(2)局部变量与作⽤域
本章内容涉及⽂法包括：
局部变量使⽤的标识符是 。与全局变量不同，局部变量在赋值前需要申请⼀块内存。在对局
部变量操作的时候也需要采⽤ load/store 来对内存进⾏操作。同样的，举个例⼦来说明⼀下。
⽣成的 LLVM IR 如下所⽰：
注意到，对于局部变量，我们⾸先需要通过 指令分配⼀块内存，才能对其进⾏
load/store 操作。
与全局变量相同，我们同样需要为 int 和 char 选择对应的类型。
(3)符号表设计
这⼀章将主要考虑变量，包括全局变量和局部变量以及作⽤域的说明。不可避免地，同学们需
要进⾏符号表的设计。
涉及到的⽂法如下：
1 BlockItem ! Decl | Stmt
%
//以下都是局部变量
int a = 1+2;
1
2
%1 = alloca i32
%2 = add i32 1, 2
store i32 %2, i32* %1
1
2
3
alloca
Plain Text
Plain Text
Plain Text
举个最简单的例⼦：
如果需要将上述代码转换为 LLVM IR，应当怎么考虑呢？直观来看，a 和 b 是全局变量，c 是局
部变量。直观上来说，过程是⾸先将全局变量 a 和 b 进⾏赋值，然后进⼊到 main 函数内部，
对 c 进⾏赋值。那么⽣成的 main 函数的 LLVM IR 可能如下。
Stmt ! LVal '=' Exp ';'
| [Exp] ';'
| 'return' Exp ';'
LVal ! Ident
PrimaryExp ! '(' Exp ')' | LVal | Number
1
2
3
4
5
int a = 1;
int b = 3;
int main(){
int c = b + 4;
return a + b + c;
}
1
2
3
4
5
6
7
Plain Text
Plain Text
这时候就有问题了，在 AST 中，不同地⽅的标识符之间是毫⽆关系的，那么如何确定代码中出
现的 a 是谁，以及出现的多个 b 是不是同⼀个变量？这时候符号表的作⽤就体现出来了。简单
来说，符号表类似于⼀个索引，通过它可以很快速的找到标识符对应的变量。
在 SysY 中，我们遵循先声明，后使⽤的原则，因此可以在第⼀次遇⻅变量声明时，将标识符与
对应的 LLVM IR 中的 Value 添加到符号表中，那么之后再次遇到标识符时就可以获得最初的声
明，找不到的话说明出现了使⽤未定义变量的错误。
注意，虚拟寄存器的数字并不是符号表的⼀部分，它们只是输出时的标记。因此，只需要在输
出 LLVM IR 时，遍历⼀遍 Function 内的所有 Value，对其标号即可。
对于符号表的⽣命周期，可以是遍历 AST 后，⽣成⼀张完整的符号表，然后再进⾏代码⽣成；
也可以是在遍历过程中创建栈式符号表，随着遍历过程创建和销毁，同时进⾏代码⽣成。
在此基础上，我们还需要注意变量的作⽤域。语句块内声明的变量的⽣命周期在该语句块内，
且内层代码块覆盖外层代码块。
@a = dso_local global i32 1
@b = dso_local global i32 3
define dso_local i32 @main(){
%1 = alloca i32 ;分配c
%2 = load i32, i32* @b ;加载b 到内存
%3 = add nsw i32 %2, 4 ;b + 4
store i32 %3, i32* %1 ;c = b + 4
%4 = load i32, i32* @a ;加载a 到内存
%5 = load i32, i32* @b ;加载b 到内存
%6 = add nsw i32 %4, %5 ;a + b
%7 = load i32, i32* %1 ;加载c 到内存
%8 = add nsw i32 %6, %7 ;a + b + c
ret i32 %8 ;返回a + b + c
}
1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
Plain Text
在上⾯的程序中，在 BlockA 中，a 的值为 7，覆盖了全局变量 a = 1，e 覆盖了 main 中的 e =
5，⽽在 main 的最后⼀⾏，f 并不存在覆盖，因为 main 外层不存在其他 f 的定义。
同样的，下⾯给出上述程序的 LLVM IR 代码：
int a = 1;
int b = 2;
int c = 3;
int main(){
int d = 4;
int e = 5;
{ //BlockA
int a = 7;
int e = 8;
int f = 9;
}
int f = 10;
return 0;
}
1
2
3
4
5
6
7
8
9
10
11
12
13
14
Plain Text
符号表的存储格式同学们可以⾃⼰设计，下⾯给出符号表的简略⽰例，同学们在实验中可以根
据⾃⼰需要⾃⾏设计。其中需要注意作⽤域与符号表的对应关系，以及必要信息的保存。
@a = dso_local global i32 1
@b = dso_local global i32 2
@c = dso_local global i32 3
define dso_local i32 @main(){
%1 = alloca i32
store i32 4, i32* %1
%2 = alloca i32
store i32 5, i32* %2
%3 = alloca i32
store i32 7, i32* %3
%4 = alloca i32
store i32 8, i32* %4
%5 = alloca i32
store i32 9, i32* %5
%6 = alloca i32
store i32 10, i32* %6
ret i32 0
}
1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
16
17
18
19
Plain Text
所有的变量都会唯⼀对应⼀个中间代码的值，因此在中间代码⾥不会有重名问题。
(4)测试样例
源程序如下。
LLVM IR 参考如下：
int a = 1;
int b = 3;
int c = 5;
int main(){
int d = 4 + c;
int e = 5 * d;
{
a = a + 5;
int b = a * 2;
a = b;
int f = 20;
e = e + a * 20;
}
int f = 10;
return e * f;
}
1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
16
Plain Text
@a = dso_local global i32 1
@b = dso_local global i32 3
@c = dso_local global i32 5
define dso_local i32 @main(){
%1 = alloca i32
%2 = load i32, i32* @c
%3 = add nsw i32 4, %2
store i32 %3, i32* %1
%4 = alloca i32
%5 = load i32, i32* %1
%6 = mul nsw i32 5, %5
store i32 %6, i32* %4
%7 = load i32, i32* @a
%8 = add nsw i32 %7, 5
store i32 %8, i32* @a
%9 = alloca i32
%10 = load i32, i32* @a
%11 = mul nsw i32 %10, 2
store i32 %11, i32* %9
%12 = load i32, i32* %9
store i32 %12, i32* @a
%13 = alloca i32
store i32 20, i32* %13
%14 = load i32, i32* %4
%15 = load i32, i32* @a
%16 = mul nsw i32 %15, 20
%17 = add nsw i32 %14, %16
store i32 %17, i32* %4
%18 = alloca i32
store i32 10, i32* %18
1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
16
17
18
19
20
21
22
23
24
25
26
27
28
29
30
31
32
33
34
35
36
37
38
39
Plain Text
⽤ lli 执⾏后， 的结果为 34。
相信各位如果去⼿动计算的话，会算出来结果是 2850。然⽽由于 的返回值只截取最
后⼀个字节，也就是 8 位，所以 2850 mod 256 = 198
五、函数的定义及调⽤
本章主要涉及不含数组的函数的定义，调⽤等。
(1)库函数
涉及⽂法有：
⾸先添加库函数的调⽤，在实验的 LLVM IR 代码中，库函数的声明如下：
注意 getchar()的返回值为 int 类型，是为了和 C 标准
%19 = load i32, i32* %4
%20 = load i32, i32* %18
%21 = mul nsw i32 %19, %20
ret i32 %21
}
40
41
42
43
44
echo $?
echo $?
Stmt → LVal '=' 'getint' '(' ')' ';'
| 'getchar' '(' ')' ';'
| LVal '=' 'getchar' '(' ')' ';'
| 'printf' '(' FormatString {',' Exp} ')' ';'
1
2
3
4
declare i32 @getint()
declare i32 @getchar()
declare void @putint(i32)
declare void @putch(i32)
declare void @putstr(i8*)
1
2
3
4
5
Plain Text
Plain Text
只要在 LLVM IR 代码开头加上这些声明，就可以在后续代码中使⽤这些库函数。同时对于⽤到
库函数的 LLVM 代码，在编译时也需要使⽤ llvm-link 命令将库函数链接到⽣成的代码中。
对于库函数的使⽤，在⽂法中其实就包含三句，即 getint，getchar 和 printf。其中，printf 包
含了有 Exp 和没有 Exp 的情况。同样的，这⾥给出⼀个简单的例⼦：
⽣成的 LLVM IR 代码如下：
int main(){
int a;
char b;
a = getint();
b = getchar();
printf("Hello:%d,%c", a, b);
return 0;
}
1
2
3
4
5
6
7
8
Plain Text
不难看出， 即为调⽤ getint 的语句， 即为调
⽤ getchar 的语句，对于其他的任何函数的调⽤也是像这样去写。
declare i32 @getint()
declare i32 @getchar()
declare void @putint(i32)
declare void @putch(i32)
declare void @putstr(i8*)
@.str = private unnamed_addr constant [8 x i8] c"Hello:\00", align 1
@.str.1 = private unnamed_addr constant [3 x i8] c",\00", align 1
define dso_local i32 @main(){
%1 = alloca i32
%2 = alloca i8
%3 = call i32 @getint()
store i32 %3, i32* %1
%4 = call i32 @getchar() ; 注意getchar 的返回值类型
%5 = trunc i32 %4 to i8 ; 不可避免地进⾏⼀次类型转换
store i8 %5, i8* %2
%6 = load i8, i8* %2
%7 = load i32, i32* %1
call void @putstr(i8* getelementptr inbounds ([8 x i8], [8 x i8]*
@.str, i64 0, i64 0))
call void @putint(i32 %7)
call void @putstr(i8* getelementptr inbounds ([3 x i8], [3 x i8]*
@.str.1, i64 0, i64 0))
%8 = zext i8 %6 to i32
call void @putch(i32 %8)
ret i32 0
}
1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
16
17
18
19
20
21
22
23
24
25
26
27
28
29
30
31
32
call i32 @getint() call i32 @getchar()
Plain Text
对于 printf，则需要将其转化为多条语句，将格式字符串与变量值分别输出。对于格式字符串
的输出，可以直接调⽤ putstr，⽐较⽅便，之后也可以直接编译为 MIPS 中的 4 号系统调⽤。
这种⽅式需要配合全局的字符串常量，不过由于仅需考虑输出字符串这⼀种情况，因此可以将
输出硬编码。注意 LLVM IR 中需要对 和 等进⾏转义。当然，也可以转化为多个 putch
的调⽤。
(2)函数定义与调⽤
涉及⽂法如下：
其实之前的 main 函数也是⼀个函数，即主函数。这⾥将其拓⼴到⼀般函数。对于⼀个函数，
其特征包括函数名，函数返回类型和参数。在本实验中，函数返回类型有 char、int 和 void 三
种，参数类型有 char 和 int 两种。
FuncFParams 之后的 Block 则与之前主函数内处理⽅法⼀样。值得⼀提的是，由于每个临时寄
存器和基本块占⽤⼀个编号，所以没有参数的函数的第⼀个临时寄存器的编号应该从 1 开始，
因为函数体⼊⼝基本块占⽤了⼀个编号 0。⽽有参数的函数，参数编号从 0 开始，进⼊ Block
后需要跳过⼀个基本块⼊⼝的编号（可以参考测试样例）。
• 如果存在⽤编号的命名寄存器，则命名规则要与系统默认的编号分配规则相同；如果全部采
⽤字符串编号寄存器，上述问题都不会存在。
• 针对上述问题，如果函数⼊⼝基本块已经由字符串命名，则编号 0 按顺序分配给后续寄存
器。
• 推荐使⽤字符串命名寄存器。
对于函数的调⽤，参考之前库函数的处理，不难发现，函数的调⽤其实和全局变量的调⽤基本
是⼀样的，即⽤@函数名表⽰。所以函数部分和符号表有着密切关联。同学们需要在函数定义
和函数调⽤的时候对符号表进⾏操作。对于有参数的函数调⽤，则在调⽤的函数内传⼊参数。
对于没有返回值的函数，则直接 call 即可，不⽤为语句赋⼀个实例。
\n \0
CompUnit → {Decl} {FuncDef} MainFuncDef
FuncDef → FuncType Ident '(' [FuncFParams] ')' Block
FuncType → 'void' | 'int' | 'char'
FuncFParams → FuncFParam {',' FuncFParam }
FuncFParam → BType Ident
BType → 'int' | 'char'
UnaryExp → PrimaryExp | Ident '(' [FuncRParams] ')' | UnaryOp UnaryExp
1
2
3
4
5
6
7
Plain Text
(3)测试样例
源代码如下。
LLVM IR 输出参考 :
int a = 1000;
int foo(int a, int b) {
return a + b;
}
void bar() {
a = 1200;
return;
}
int main() {
bar();
int b = a;
a = getint();
printf("%d\n", foo(a, b));
return 0;
}
1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
Plain Text
declare i32 @getint()
declare i32 @getchar()
declare void @putint(i32)
declare void @putch(i32)
declare void @putstr(i8*)
@a = dso_local global i32 1000
@.str = private unnamed_addr constant [2 x i8] c"\0A\00", align 1
define dso_local i32 @foo(i32 %0, i32 %1) {
%3 = alloca i32
%4 = alloca i32
store i32 %0, i32* %3
store i32 %1, i32* %4
%5 = load i32, i32* %3
%6 = load i32, i32* %4
%7 = add nsw i32 %5, %6
ret i32 %7
}
define dso_local void @bar() {
store i32 1200, i32* @a
ret void
}
define dso_local i32 @main() {
call void @bar()
%1 = alloca i32
%2 = load i32, i32* @a
store i32 %2, i32* %1
%3 = call i32 @getint()
store i32 %3, i32* @a
%4 = load i32, i32* @a
%5 = load i32, i32* %1
%6 = call i32 @foo(i32 %4, i32 %5)
call void @putint(i32 %6)
call void @putstr(i8* getelementptr inbounds ([2 x i8], [2 x i8]*
@.str, i64 0, i64 0))
1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
16
17
18
19
20
21
22
23
24
25
26
27
28
29
30
31
32
33
34
35
36
37
Plain Text
输⼊：1000 输出：2200
六、条件语句与短路求值
(1) 条件语句
涉及⽂法如下。
在条件语法中，需要同学们进⾏条件的判断与选择。这时采⽤数字编号的同学可能会对基本块
的标号产⽣疑惑，因为需要跳转到之后还未构建的基本块。你可能回想到回填操作，但是之后
每次优化后 LLVM IR 都会发⽣变化，因此并不实际。所以最佳的做法还是使⽤ LLVM IR 中的
SlotTracker，因为编号其实只⽤在输出中，所以只需要在输出前，遍历每个 Function 中的所
有 Value，按顺序为它们分配编号即可。
要写出条件语句，⾸先要理清楚逻辑。在上述⽂法中，最重要的莫过于下⾯这⼀条语法：
为了⽅便说明，对上述⽂法的两个 Stmt 编号为 Stmt1 和 Stmt2。在这条语句之后基本块假设
叫 BasicBlock3。不难发现，条件判断的逻辑如左下图。
ret i32 0
}
38
39
Stmt → 'if' '(' Cond ')' Stmt ['else' Stmt]
Cond → LOrExp
RelExp → AddExp | RelExp ('<' | '>' | '<=' | '>=') AddExp
EqExp → RelExp | EqExp ('==' | '!=') RelExp
LAndExp → EqExp | LAndExp '&&' EqExp
LOrExp → LAndExp | LOrExp '||' LAndExp
1
2
3
4
5
6
1 Stmt → 'if' '(' Cond ')' Stmt1 ['else' Stmt2]
Plain Text
Plain Text
⾸先进⾏ Cond 结果的判断，如果结果为 1 则进⼊ Stmt1，如果 Cond 结果为 0，若⽂法有
else 则将进⼊ Stmt2，否则进⼊下⼀条⽂法的基本块 BasicBlock3。在 Stmt1 或 Stmt2 执⾏完
成后都需要跳转到 BasicBlock3。对于⼀个 LLVM IR 程序来说，对⼀个含 else 的条件分⽀，其
基本块构造可以如右上图所⽰。如果能够理清楚基本块跳转的逻辑，那么在写代码的时候就会
变得⼗分简单。
这时候再回过头去看 Cond ⾥⾯的代码，即 LOr 和 LAnd，Eq 和 Rel。不难发现，其处理⽅式
和加减乘除⾮常像，除了运算结果都是 1 位 (i1) ⽽⾮ 32 位 (i32)。同学们可能需要⽤到 trunc
或者 zext 指令进⾏类型转换。
(2) 短路求值
可能有的同学会认为，反正对于 LLVM IR 来说，跳转与否只看 Cond 的值，所以只要把 Cond
算完结果就⾏，不会影响正确性。不妨看⼀下下⾯这个例⼦：
如果要将上⾯这段代码翻译为 LLVM IR，同学们会怎么做？如果按照传统⽅法，即先统⼀计算
Cond，则⼀定会执⾏⼀次 change() 函数，把全局变量的值变为 6。但事实上，由于短路求值
的存在，在读完 1 后，整个 Cond 的值就已经被确定了，即⽆论 1 || 后⾯跟的是什么，都不影
响 Cond 的结果，那么根据短路求值，后⾯的东西就不应该执⾏。所以上述代码的输出应当为
5 ⽽不是 6，也就是说，LLVM IR 不能够单纯的把 Cond 计算完后再进⾏跳转。这时候就需要对
Cond 的跳转逻辑进⾏改写。
改写之前同学们不妨思考⼀个问题，即什么时候跳转。根据短路求值，只要条件判断出现"短
路"，即不需要考虑后续与或参数的情况下就已经能确定值的时候，就可以进⾏跳转。或者更简
单的来说，当 LOrExp 值为 1 或者 LAndExp 值为 0 的时候，就已经没有必要再进⾏计算了。
这⾥提供⼀个短路求值的思路，以类似综合属性、继承属性计算的⽅式实现短路求值。
对于条件（对应⾮终结符 Cond）的解析，主要涉及三个基本块：条件为真跳转的⽬标块，条
件为假跳转的⽬标块，以及条件的运算所属的基本块。这三个块根据表达式的不同（如 if、
for）⽽有⼀些区别，但是都符合这⼀模式。那么在解析前，我们应该准备好这三个基本块，但
暂时不将其插⼊函数中。
int a = 5;
int change() {
a = 6;
return a;
}
int main() {
if (1 || change()) {
printf("%d", a);
}
return 0;
}
1
2
3
4
5
6
7
8
9
10
11
Cond → LOrExp
LAndExp → LAndExp '&&' EqExp
LOrExp → LOrExp '||' LAndExp
1
2
3
Plain Text
Plain Text
需要注意的是，对于 if，其条件运算所属的基本块可以是当前基本块，即不⽤新建⼀个基本块
专⻔放第⼀个条件。当然也可以在优化时再消除冗余的基本块。
接下来，将三个块作为节点属性，开始解析，这⾥主要涉及 Cond、OrExp、AndExp，和
EqExp（作为条件对应的 Value）。下图为 中 Cond 的解析过程，
解析时进⾏先序遍历。
以图中 COND 节点为例，T（左上⻆）代表条件为真要跳转到的基本块，F（右上⻆）代表条件
为假要跳转到的基本块，N（左下⻆）代表将要⽣成的条件所在的基本块，右下⻆（COND 没
有）代表当前节点新创建的基本块。↓ 表⽰该基本块由⽗节点传递下来，↑ 表⽰进⼊时即将该
基本块插⼊函数，并作为当前基本块，即之后⽣成的指令（条件计算）会添加到该基本块中。
这⾥为了顺序清晰，将 ↑ 操作放在了 EqExp 节点⾥，实际上在 AndExp 节点⾥就可以根据解
析 EqExp ⽣成的 Value ⽣成 br 指令了。
那么可以看到，当 OrExp/AndExp 在有多个⼦节点时，通过为右节点预先创建基本块的⽅式，
解决了左⼦树的跳转问题，并通过传递该节点的⽅式使得右节点⽣成的指令可以添加到该基本
块。具体地，对于如下代码：
if (a || b && c || d)
(3) 测试样例
源代码如下。
参考 LLVM IR 如下：
int a = 1;
int func() {
a = 2;
return 1;
}
int func2() {
a = 4;
return 10;
}
int func3() {
a = 3;
return 0;
}
int main() {
if (0 || func() && func3() || func2()) {
printf("%d--1", a);
}
if (1 || func3()) {
printf("%d--2", a);
}
if (0 || func3() || func() < func2()) {
printf("%d--3", a);
}
return 0;
}
1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
16
17
18
19
20
21
22
23
24
25
26
Plain Text
declare i32 @getint()
declare i32 @getchar()
declare void @putint(i32)
declare void @putch(i32)
declare void @putstr(i8*)
@a = dso_local global i32 1
@.str = private unnamed_addr constant [4 x i8] c"--1\00", align 1
@.str.1 = private unnamed_addr constant [4 x i8] c"--2\00", align 1
@.str.2 = private unnamed_addr constant [4 x i8] c"--3\00", align 1
define dso_local i32 @func() {
store i32 2, i32* @a
ret i32 1
}
define dso_local i32 @func2() {
store i32 4, i32* @a
ret i32 10
}
define dso_local i32 @func3() {
store i32 3, i32* @a
ret i32 0
}
define dso_local i32 @main() {
br label %1
1: ; preds = %0
%2 = call i32 @func()
%3 = icmp ne i32 %2, 0
br i1 %3, label %4, label %7
4: ; preds = %1
%5 = call i32 @func3()
%6 = icmp ne i32 %5, 0
br i1 %6, label %10, label %7
1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
16
17
18
19
20
21
22
23
24
25
26
27
28
29
30
31
32
33
34
35
36
37
38
39
Plain Text
7: ; preds = %4, %1
%8 = call i32 @func2()
%9 = icmp ne i32 %8, 0
br i1 %9, label %10, label %12
10: ; preds = %7, %4
%11 = load i32, i32* @a
call void @putint(i32 %11)
call void @putstr(i8* getelementptr inbounds ([4 x i8], [4 x i8]*
@.str, i64 0, i64 0))
br label %12
12: ; preds = %10, %7
br label %16
13:
%14 = call i32 @func3()
%15 = icmp ne i32 %14, 0
br i1 %15, label %16, label %18
16: ; preds = %13, %12
%17 = load i32, i32* @a
call void @putint(i32 %17)
call void @putstr(i8* getelementptr inbounds ([4 x i8], [4 x i8]*
@.str.1, i64 0, i64 0))
br label %18
18: ; preds = %16, %13
br label %19
19: ; preds = %18
%20 = call i32 @func3()
%21 = icmp ne i32 %20, 0
br i1 %21, label %26, label %22
22: ; preds = %19
%23 = call i32 @func()
%24 = call i32 @func2()
%25 = icmp slt i32 %23, %24
br i1 %25, label %26, label %28
40
41
42
43
44
45
46
47
48
49
50
51
52
53
54
55
56
57
58
59
60
61
62
63
64
65
66
67
68
69
70
71
72
73
74
75
76
77
78
输出：4--14--24--3
七、条件判断与循环
（1）循环
涉及⽂法如下：
如果经过了上⼀章的学习，这⼀章其实难度就⼩了不少。对于这条⽂法，同样可以改写为
如果查询 C 语⾔的 for 循环，其中对 for 循环的描述为：
26: ; preds = %22, %19
%27 = load i32, i32* @a
call void @putint(i32 %27)
call void @putstr(i8* getelementptr inbounds ([4 x i8], [4 x i8]*
@.str.2, i64 0, i64 0))
br label %28
28: ; preds = %26, %22
ret i32 0
}
79
80
81
82
83
84
85
86
87
88
Stmt → 'for' '(' [ForStmt] ';' [Cond] ';' [ForStmt] ')' Stmt
| 'break' ';'
| 'continue' ';'
ForStmt → LVal '=' Exp
1
2
3
4
Stmt → 'for' '(' [ForStmt1] ';' [Cond] ';' [ForStmt2] ')' Stmt (BasicBl
ock)
1
Plain Text
Plain Text
不难发现，实验⽂法中的 ForStmt1、Cond、ForStmt2 分别表⽰了上述 for 循环中的初始化
（initialization），条件（condition）和更新（step）。同学们去搜索 C 语⾔的 for 循环逻辑的
话也会发现，for 循环的逻辑可以表述为
执⾏初始化表达式 ForStmt1
执⾏条件表达式 Cond，如果为 1 执⾏循环体 Stmt，否则结束循环执⾏ BasicBlock
执⾏完循环体 Stmt 后执⾏更新表达式 ForStmt2
重复执⾏步骤 2 和步骤 3
（2）break/continue
对于 break 和 continue，直观理解为，break 跳出循环，continue 跳过本次循环。再通俗点
说就是，break 跳转到的是 BasicBlock，⽽ continue 跳转到的是 ForStmt2。这样就能达到⽬
for (initialization ; condition ; step){
// code to be executed
}
1
2
3
1.
2.
3.
4.
Plain Text
的了。所以，对于循环⽽⾔，跳转的位置很重要。这也是同学们在编码的时候需要着重注意的
点。
同样的，针对这两条指令，对上图作出⼀定的修改，就是整个循环的流程图了。
（3）测试样例
源代码如下，本质为输出偶数项的斐波那契数列。
参考 LLVM IR 如下。
int main() {
int a1 = 1, a2;
a2 = a1;
int temp;
int n, i;
n = getint();
for (i = a1 * a1; i < n + 1; i = i + 1) {
temp = a2;
a2 = a1 + a2;
a1 = temp;
if (i % 2 == 1) {
continue;
}
printf("round %d: %d\n", i, a1);
if (i > 19) {
break;
}
}
return 0;
}
1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
16
17
18
19
20
Plain Text
源代码如下 , 本质为输出偶数项的斐波那契数列。
declare i32 @getint()
declare i32 @getchar()
declare void @putint(i32)
declare void @putch(i32)
declare void @putstr(i8*)
@.str = private unnamed_addr constant [7 x i8] c"round \00", align 1
@.str.1 = private unnamed_addr constant [3 x i8] c": \00", align 1
@.str.2 = private unnamed_addr constant [2 x i8] c"\0A\00", align 1
define dso_local i32 @main() {
%1 = alloca i32
store i32 1, i32* %1
%2 = alloca i32
%3 = load i32, i32* %1
store i32 %3, i32* %2
%4 = alloca i32
%5 = alloca i32
%6 = alloca i32
%7 = call i32 @getint()
store i32 %7, i32* %5
%8 = load i32, i32* %1
%9 = load i32, i32* %1
%10 = mul nsw i32 %8, %9
store i32 %10, i32* %6
br label %11
1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
16
17
18
19
20
21
22
23
24
25
26
Plain Text
参考 LLVM IR 如下。
int main(){
int a1=1,a2;
a2=a1;
int temp;
int n,i;
n =getint();
for (i =a1*a1;i <n +1;i =i +1){
temp =a2;
a2=a1+a2;
a1=temp;
if (i %2==1){
continue;
}
printf("round %d:%d\n",i,a1);
if (i >19){
break;
}
}
return 0;
}
1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
16
17
18
19
20
Plain Text
declare i32@getint()
declare i32@getchar()
declare void @putint(i32)
declare void @putch(i32)
declare void @putstr(i8*)
@.str =private unnamed_addr constant [7x i8]c"round \00",align 1
@.str.1=private unnamed_addr constant [3x i8]c":\00",align 1
@.str.2=private unnamed_addr constant [2x i8]c"\0A\00",align 1
define dso_local i32@main(){
%1=alloca i32
store i321,i32*%1
%2=alloca i32
%3=load i32,i32*%1
store i32%3,i32*%2
%4=alloca i32
%5=alloca i32
%6=alloca i32
%7=call i32@getint()
store i32%7,i32*%5
%8=load i32,i32*%1
%9=load i32,i32*%1
%10=mul nsw i32%8,%9
store i32%10,i32*%6
br label %11
11:;preds =%33,%0
%12=load i32,i32*%6
%13=load i32,i32*%5
%14=add nsw i32%13,1
%15=icmp slt i32%12,%14
br i1%15,label %16,label %36
16:;preds =%11
%17=load i32,i32*%2
store i32%17,i32*%4
%18=load i32,i32*%1
%19=load i32,i32*%2
1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
16
17
18
19
20
21
22
23
24
25
26
27
28
29
30
31
32
33
34
35
36
37
38
39
Plain Text
%20=add nsw i32%18,%19
store i32%20,i32*%2
%21=load i32,i32*%4
store i32%21,i32*%1
%22=load i32,i32*%6
%23=srem i32%22,2
%24=icmp eq i32%23,1
br i1%24,label %25,label %26
25:;preds =%16
br label %33
26:;preds =%16
%27=load i32,i32*%1
%28=load i32,i32*%6
call void @putstr(i8*getelementptr inbounds ([7x i8],[7x i8]*@.str,
i640,i640))
call void @putint(i32%28)
call void @putstr(i8*getelementptr inbounds ([3x i8],[3x i8]*@.str.
1,i640,i640))
call void @putint(i32%27)
call void @putstr(i8*getelementptr inbounds ([2x i8],[2x i8]*@.str.
2,i640,i640))
%29=load i32,i32*%6
%30=icmp sgt i32%29,19
br i1%30,label %31,label %32
31:;preds =%26
br label %36
32:;preds =%26
br label %33
33:;preds =%32,%25
%34=load i32,i32*%6
%35=add nsw i32%34,1
store i32%35,i32*%6
br label %11
36:;preds =%31,%11
40
41
42
43
44
45
46
47
48
49
50
51
52
53
54
55
56
57
58
59
60
61
62
63
64
65
66
67
68
69
70
71
72
73
74
75
76
输⼊ :10 输出 :
输⼊ :40 输出 :
⼋、数组与函数
(1)数组
数组涉及的⽂法相当多 , 包括以下⼏条 :
ret i320
}
77
78
round 2:2
round 4:5
round 6:13
round 8:34
round 10:89
1
2
3
4
5
round 2:2
round 4:5
round 6:13
round 8:34
round 10:89
round 12:233
round 14:610
round 16:1597
round 18:4181
round 20:10946
1
2
3
4
5
6
7
8
9
10
Plain Text
Plain Text
在数组的编写中 , 同学们会频繁⽤到 getelementptr 指令 , 故先系统介绍⼀下这个指令的⽤法。
getelementptr 指令的⼯作是计算地址。其本⾝不对数据做任何访问与修改。其语法如下 :
也可以为 getelementptr 的参数添加括号 , 如下。
现在来理解⼀下上⾯这⼀条指令。第⼀个 表⽰的是第⼀个索引所指向的类型 , 有时也是
返回值的类型。第⼆个 表⽰的是后⾯的指针基地址 的类型 ,
表⽰的是⼀组索引的类型和值 , 在本实验中索引的类型为 i32。索引指向的基本类型确定的是增
加索引值时指针的偏移量。
说完理论 , 不如结合⼀个实例来讲解。考虑数组 a[5], 需要获取 a[3]的地址 , 有如下写法 :
ConstDef → Ident {'['ConstExp ']'}'='ConstInitVal
ConstInitVal → ConstExp |'{'[ConstExp {','ConstExp }]'}'|ConstString
VarDef → Ident {'['ConstExp ']'}|Ident {'['ConstExp ']'}'='InitVal
InitVal → Exp |'{'[Exp {','Exp }]'}'|ConstString
FuncFParam → BType Ident ['['']']
LVal → Ident {'['Exp ']'}
1
2
3
4
5
6
1 <result>=getelementptr <ty>,<ty>*<ptrval>{,[inrange]<ty><idx>}*
1 <result>=getelementptr (<ty>,<ty>*<ptrval>{,[inrange]<ty><idx>}*)
<ty>
<ty> <ptrval> <ty><index>
%1=getelementptr [5x i32],[5x i32]*@a,i320,i323
%2=getelementptr [5x i32],[5x i32]*@a,i320
%3=getelementptr i32,i32*%2,i323
%3=getelementptr i32,i32*@a,i323
1
2
3
4
Plain Text
Plain Text
Plain Text
Plain Text
(2)数组定义与调⽤
这⼀章将主要讲述数组定义和调⽤ , 包括全局数组 , 局部数组的定义 , 以及函数中的数组调⽤。
对于全局数组定义 , 与全局变量⼀样 , 同学们需要将所有量全部计算到特定的值。对于⼀个维度
内全是 0 的地⽅ , 可以采⽤ zeroinitializer 来统⼀置 0。
例如 , 我们有如下的全局数组。
对应的 LLVM IR 中的数组如下。
对于字符串数组 , 我们还有另⼀种声明⽅式 , 即与整数数组相同。
当然 ,zeroinitializer 不是必须的 , 同学们完全可以⼀个个 i320 写进去 , 但对于⼀些很阴间的样例
点 , 不⽤ zeroinitializer 可能会导致 TLE , 例如全局数组 , 不使⽤该指令就需要
输出 1000 次 i320, 必然导致 TLE, 所以还是推荐同学们使⽤ zeroinitializer。
对于局部数组 , 在定义的时候同样需要使⽤ alloca 指令 , 其存取指令同样采⽤ load 和 store, 只
是在此之前需要采⽤ getelementptr 获取数组内应位置的地址。
字符数组的字符串常量初始化 , 可以⾃⾏设计实现。LLVM IR 中 , 全局字符数组的字符串常量初
始化可以直接通过字符串赋值 , 局部字符数组则也需要通过 alloca 指令分配内存空间 , 逐个元素
初始化。不要忘记字符串常量末尾的结束符 和填充符号 。
int a[1+2+3+4]={1,1+1,1+3-1,0,0,0,0,0,0,0};
int b[20];
char c[8]="foobar";
1
2
3
@a =dso_local global [10x i32][i321,i322,i323,i320,i320,i320,i320,i320,
i320,i320]
@b =dso_local global [20x i32]zeroinitializer
@c =dso_local global [8x i8]c"foobar\00\00",align 1
1
2
3
1 @c =dso_local global [8x i8][i8102,i8111,i8111,i898,i897,i8114,i80,i80]
int a[1000];
'\00' '\00'
Plain Text
Plain Text
Plain Text
对于数组传参 , 其中涉及到维数的变化问题 , 例如 , 对于参数中含维度的数组 , 同学们可以参考上
述 getelementptr 指令⾃⾏设计 , 因为该指令很灵活 , 所以下⾯的测试样例仅仅当⼀个参考。同
学们可以将⾃⼰⽣成的 LLVM IR 使⽤ lli 编译后⾃⾏查看输出⽐对。
此外 , 你可能注意到 , 教程中⽣成的 getelementptr 指令中添加了 inbound。当指定 inbound
时 , 如果下标访问超过了数组的实际⼤⼩ , 那么 getelementptr 就会返回⼀个"poison"值 , ⽽不
是正常计算得到的地址 , 算是⼀种越界检查。官⽹对此的描述如下。
"With the inbounds keyword,the result value of the GEP is poison if the address is out
side the actual underlying allocated object and not the address
(3)测试样例
源代码如下。
参考 LLVM IR 如下。
int a[3+3]={1,2,3,4,5,6};
int foo(int x,int y[]){
return x+y[1];
}
int main(){
int c[3]={1,2,3};
int x =foo(a[4],a);
printf("%d -%d\n",x,foo(c[0],c));
return 0;
}
1
2
3
4
5
6
7
8
9
10
11
12
Plain Text
declare i32@getint()
declare i32@getchar()
declare void @putint(i32)
declare void @putch(i32)
declare void @putstr(i8*)
@a =dso_local global [6x i32][i321,i322,i323,i324,i325,i326]
@.str =private unnamed_addr constant [4x i8]c"-\00",align 1
@.str.1=private unnamed_addr constant [2x i8]c"\0A\00",align 1
define dso_local i32@foo(i32%0,i32*%1){
%3=alloca i32
%4=alloca i32*
store i32%0,i32*%3
store i32*%1,i32**%4
%5=load i32,i32*%3
%6=load i32*,i32**%4
%7=getelementptr inbounds i32,i32*%6,i322
%8=load i32,i32*%7
%9=add nsw i32%5,%8
ret i32%9
}
define dso_local i32@main(){
%1=alloca [3x i32]
%2=getelementptr inbounds [3x i32],[3x i32]*%1,i320,i320
store i321,i32*%2
%3=getelementptr inbounds i32,i32*%2,i321
store i322,i32*%3
%4=getelementptr inbounds i32,i32*%3,i321
store i323,i32*%4
%5=alloca i32
%6=getelementptr inbounds [6x i32],[6x i32]*@a,i320,i324
%7=load i32,i32*%6
%8=getelementptr inbounds [6x i32],[6x i32]*@a,i320,i320
%9=call i32@foo(i32%7,i32*%8)
store i32%9,i32*%5
%10=getelementptr inbounds [3x i32],[3x i32]*%1,i320,i320
1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
16
17
18
19
20
21
22
23
24
25
26
27
28
29
30
31
32
33
34
35
36
37
38
39
Plain Text
输出 :8-4
九、类型转换
我们的⽂法中包含 char 和 int 两种算数类型 , 因此在计算中涉及到类型转换的问题。例如 , 对于
下⾯的代码 , 在运算中会涉及到类型转换和溢出。
当 int 类型值赋给 char 类型变量时 , 需要进⾏截断。在 LLVM IR 中 , 我们可以直接使⽤ trunc 指
令将 i32 转换为 i8。同理 , 当 char 类型值赋给 int 类型变量时 , 需要进⾏扩展。由于我们⽂法中
限制了 char 的取值⼤于零 , 所以我们使⽤⽆符号扩展 zext 即可。因此那么对于上⾯的源代码 ,
可能的 LLVM IR 如下。
%11=load i32,i32*%10
%12=getelementptr inbounds [3x i32],[3x i32]*%1,i320,i320
%13=call i32@foo(i32%11,i32*%12)
%14=load i32,i32*%5
call void @putint(i32%14)
call void @putstr(i8*getelementptr inbounds ([4x i8],[4x i8]*@.str,
i640,i640))
call void @putint(i32%13)
call void @putstr(i8*getelementptr inbounds ([2x i8],[2x i8]*@.str.
1,i640,i640))
ret i320
}
40
41
42
43
44
45
46
47
48
49
int main()
{
int a =255;
char b =2;
printf("a =%d,b =%d\n",a,b);
b =a +b;
printf("b =%d\n",b);
return 0;
}
1
2
3
4
5
6
7
8
9
Plain Text
declare i32@getint()
declare i32@getchar()
declare void @putint(i32)
declare void @putch(i32)
declare void @putstr(i8*)
@.str =private unnamed_addr constant [5x i8]c"a =\00",align 1
@.str.1=private unnamed_addr constant [7x i8]c",b =\00",align 1
@.str.2=private unnamed_addr constant [2x i8]c"\0A\00",align 1
@.str.3=private unnamed_addr constant [5x i8]c"b =\00",align 1
define dso_local i32@main(){
%1=alloca i32
store i32255,i32*%1
%2=alloca i8
store i82,i8*%2;初始化时即确保类型匹配
%3=load i8,i8*%2
%4=load i32,i32*%1
call void @putstr(i8*getelementptr inbounds ([5x i8],[5x i8]*@.str,
i640,i640))
call void @putint(i32%4)
call void @putstr(i8*getelementptr inbounds ([7x i8],[7x i8]*@.str.
1,i640,i640))
%5=zext i8%3to i32;输出时char 转int
call void @putint(i32%5)
call void @putstr(i8*getelementptr inbounds ([2x i8],[2x i8]*@.str.
2,i640,i640))
%6=load i32,i32*%1
%7=load i8,i8*%2
%8=zext i8%7to i32;char 参与运算时转int
%9=add nsw i32%6,%8
%10=trunc i32%9to i8;保存时int 转char
store i8%10,i8*%2
%11=load i8,i8*%2
call void @putstr(i8*getelementptr inbounds ([5x i8],[5x i8]*@.str.
3,i640,i640))
%12=zext i8%11to i32;输出时char 转int
call void @putint(i32%12)
call void @putstr(i8*getelementptr inbounds ([2x i8],[2x i8]*@.str.
1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
16
17
18
19
20
21
22
23
24
25
26
27
28
29
30
31
32
33
34
35
Plain Text
上述 LLVM IR 的输出为 :
此外 , 参数传递也可以视为特殊的赋值 , 当 int 类型值传递给 char 类型参数(或反之)时 , 同样需
要进⾏类型转换。当然 , 如果是数组 , 那么是没法进⾏转换的 , 如果出现 , 则应该报错。
2,i640,i640))
ret i320
}
36
37
a =255,b =2
b =1
1
2
Plain Text


中间代码数据结构 - 以 LLVM IR 为例
对于优化来说，一个设计良好的中间代码数据结构能够提供极大的帮助。而即便不做优化，定
义中间代码相关数据结构也能为中间代码的生成提供更加结构化的操作接口。在 tolangc 中，
我们参考 LLVM 的实现，提供了一个相对简单，但足以支持 tolangc 实现的 LLVM 后端，希望
能够为各位同学们提供参考。
本章节只介绍了 tolangc 中 LLVM IR 的实现方式，对于 LLVM 的更多介绍见《中间代码生成 -
LLVM》。
一、文法涉及的 LLVM IR 语法结构
根据 tolang 的文法和语义，我们可以得知 tolangc 所需要的 LLVM IR 语法结构有：
函数块
指令集 Instruction
局部变量 / 字面量
针对以上语法结构，我们设计了对应的 类存储上下文信息。
在实验编译器的实现中，也可以按需定义编译器中的 LLVM IR 数据结构。
二、架构设计
1.
1 define dso_local i32 @add(i32 %0, i32 %1) {}
2.
指令类型 包含的 LLVM IR 指令形式
UnaryInstruction：1 个操作数 + - ! number load i32, i32* %1
BinaryInstruction：两个操作数 a + - / % b a > < >= <= == != b store i32 %0,
i32* %1
Instruction：其他指令 alloca i32 ret i32 %7 call i32 @add(i32 @i1, i32
@i2) call i32 @get() call void @put(i32 %5)
3.
Value
Plain Text
根据涉及的语法结构，我们可以把 LLVM 语法结构由粒度从高到低划分为：
整个模块
函数
基本代码块 （以 label 和跳转语句为界限）
指令
变量 / 常量 && 符号
变量：参数、局部变量；常量：字面量
整体 LLVM 语法层级结构如下图所示：
先解释一下图中的 和 类的意义。
对于模块中不同粒度的所有语法结构，我们借鉴 LLVM 官方库（https://github.com/llvm/llvm
-project）的实现方法，将它们都作为 类的子类。其中， 类是 的一个
特殊的子类，是一种可以使用其他 对象的 类。 、
和 都有使用的语法结构，都是 的子类，也是 的子类。
和 之间的配对用 类记录，通过 类建立起了语法结构的上下级关系
双向索引。这样所有的语法结构都可以统一成 类，语法结构间的使用关系都可以统一
成 类。 关系是 LLVM 编译器实现的核心架构，索引关系的抽象使其能够在全局保
存，可以大大提高代码优化的效率。
1. Module
2. Function
3. BasicBlock
4. Instruction
5.
6.
User Value
Value User Value
Value Value Function BasicBlock
Instruction User Value
User Value Use Use
Value
Use Use
对于 和 的具体对应，可以参考以下实例代码。A 语句被看作是一个
，此 继承自 。作为 ，它引用了 和 两个
，又被 C 语句所引用。
在我们的设计中， 的主要属性值有：
可以看到， 类中的几个重要属性成员分别为：
• 语法类型
• 值类型
• 使用关系索引列表
• 名称
接下来我们围绕 成员讲解中端总体架构设计。
（1）语法类型 ValueType
中， 是 的类型枚举。所有语法结构都是
的子类，不同子类通过修改 值区分：
A: %add = add nsw i32 %a, %b1
B: %add1 = add nsw i32 %a, %b2
C: %add2 = add nsw i32 %add, %add13
1
2
3
Value User
Instruction Instruction User User a b
Value
Value
class Value {
protected:
TypePtr _type;
std::string _name;
UseList _useList;
UseList _userList;
private:
ValueType _valueType;
};
1
2
3
4
5
6
7
8
9
10
11
Value
ValueType _valueType
Type _type
UseList _useList
std::string _name
Value
ValueType _valueType ValueType Value
Value _valueType
Plain Text
Plain Text
（2）值类型 Type
记录的是返回值类型，与 不属于一个系统：
• 用于区分我们自行实现的 LLVM 架构中定义的不同数据类型。
• 记录的是 LLVM 本身语法结构中的数据类型，可以理解为一个指令的返回值类型。
比如对于指令对象： ，返回值类型 为 的类型
，指令类型 为二元操作数指令类型 。
可以区分的数据类型包括：
enum class ValueType {
// Value
ArgumentTy, // 参数
BasicBlockTy, // 基本块
// Value -> Constant
ConstantTy, // 常量标识符
ConstantDataTy, // 字面量
// Value -> Constant -> GlobalValue
FunctionTy,
GlobalVariableTy,
// Value -> User -> Instruction
BinaryOperatorTy,
CompareInstTy,
BranchInstTy,
ReturnInstTy,
StoreInstTy,
CallInstTy,
InputInstTy,
OutputInstTy,
AllocaInstTy,
LoadInstTy,
UnaryOperatorTy,
};
1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
16
17
18
19
20
21
22
23
24
25
26
TypePtr _type ValueType
ValueType
Type
%add = add i32 %a, %b Type %add
i32 ValueType BinaryOperatorTy
Type
Plain Text
（3）索引 Use & User
是 的子类。通过 和 的配对，索引 间的使用和被使用
关系。
目前 关系只用于记录指令和操作数的关系。每条指令建立的同时，我们将操作数作为
，指令作为 ，创建 对象，将 对象同时保存在全局记录和 中
的 属性中。指令和每个操作数之间都是一个 关系。每个 的 关
系数量不定（一条指令中的操作数数量不定，参数调用中参数个数不限）。
我们使用 语法结构记录全局的 关系。每个编译单元 中只有一
个 ：
存储的是对象的指针（引用）。
enum TypeID {
// Primitive types
VoidTyID, // 空返回值
LabelTyID, // 标签类型
// Derived types
IntegerTyID, // 整数类型
FloatTyID, // 浮点数类型
FunctionTyID, // 函数类型
PointerTyID // 指针类型
};
1
2
3
4
5
6
7
8
9
10
11
User Value User Value Value
Use
Usee User Use Use User
uselist Use User Use
LlvmContext Use Module
LlvmContext
class LlvmContext {
std::vector<FunctionTypePtr> _functionTypes;
std::vector<PointerTypePtr> _pointerTypes;
std::vector<ValuePtr> _values;
std::vector<UsePtr> _uses;
};
1
2
3
4
5
6
7
LlvmContext
Plain Text
Plain Text
记录对象之间的索引关系为之后的代码优化提供便利，让我们能查找到某个变量的所有使用地
点，更方便地进行变量替换。设计的代码优化方法有活跃变量分析、公共子表达式删除等。
（4）名称记录
中记录语法结构的名称。名称记录只在全局变量
（ 、 ）中有意义。
本文法编译不涉及全局变量。但代码库中也完成了全局变量的指令实现，可以作为其他编译器
实现的参考。
局部变量的名称在 LLVM 中没有意义，因为在翻译过后需要用数字对虚拟寄存器重新命名——
按出现顺序为虚拟寄存器编号（LLVM 的虚拟寄存器相关知识见课程教程）。 对象中通
过 保存域内各个成员结构的指针索引，成员间的使用关系中无需语法对象名称信息，即
整个翻译过程与对象名称无关，与虚拟寄存器编号也无关。因此，分配虚拟寄存器编号不用在
翻译过程中实现，可以推迟到代码打印的部分。
在我们的实现中， 类作为一个工具类，在中间代码打印时为一个函数域分配虚
拟寄存器，记录各语句对应的虚拟寄存器编号。
在中间代码打印前使用 方法，按出现顺序建立 和编号值
之间的索引；打印时通过 方法获取对应寄存器的编号。
三、LLVM 文件结构说明
最后，我们给出 tolangc 中 LLVM 模块的文件结构说明。
std::string _name
GlobalVariable Function
Value
Use
SlotTracker
class SlotTracker final {
std::unordered_map<ValuePtr, int> _slot;
};
1
2
3
SlotTracker::Trace ValuePtr
SlotTracker::Slot
Plain Text
+---asm
| AsmWriter.h // 存储中间代码字符串
| AsmPrinter.h // 打印 writer 中存储的结构化字符串
\---ir
| IrForward.h // 各个语法结构前向声明（防止交叉引用报错）：定义指针类型
| Llvm.h // 罗列 LLVM 翻译过程中所需的头文件集合
| Module.h // LLVM 的一个编译单元，按文法定义存储：函数列表、main
函数、上下文语义记录 LlvmContext
| LlvmContext.h // LLVM 编译单元 Module 的语义记录，保存类型字典、Us
e 关系（一个 Module 对应一个 LlvmContext）
| SlotTracker.h // 为虚拟寄存器编号和输出的工具类
| Type.h // 定义 LLVM 的对象类型和类型子类，类型包括：Void、Lab
el、Integer、Function、Pointer
\---value // LLVM 翻译中自行定义的语法单元
| Value.h // 所有语法单元的父类，主要记录语法单元类型
和 Use 关系
| Use.h // 一个 Use 对象记录一个 User 和 Value 对
| User.h // Value 的子类
| ArgRegTy.h // 参数 Value
| BasicBlock.h // 代码块 Value
| Constant.h // 常量 Value
| ConstantData.h // 字面量 Value -> Constant
| GlobalValue.h // 全局变量 Value -> Constant
| Function.h // 函数 Value -> Constant -> GlobalValu
e
\---inst // 指令 Value -> User
Instruction.h // 指令基本类
InstructionTypes.h // 定义存在类型区分的指令类：UnaryIn
struction -> UnaryOperator, BinaryInstruction -> BinaryOperator & Compa
reInstruction
Instructions.h // 定义 LLVM 固有指令：AllocaIns
t、LoadInst、StoreInst、BranchInst、JumpInst、ReturnInst、CallInst
ExtendedInstructions.h // 定义外部库指令：InputInst、Outp
utInst
1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
16
17
18
19
20
21
22
23
24
25
Plain Text
