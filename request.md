输入输出及处理要求如下：

（1）需按文法规则，用yacc对文法中定义的语法成分进行分析；

（2）为了方便进行自动评测，输入的被编译源文件统一命名为testfile.txt（注意不要写错文件名）；输出的结果文件统一命名为output.txt（注意不要写错文件名）

结果文件中包含：

    在文法中出现的非终结符（除了<BlockItem>, <Decl>, <BType> 之外）

    分析结束前，另起一行输出当前语法成分的名字，形如“<Stmt>”

    （注：未要求输出的语法成分仍需要进行分析，但无需输出）

【重要提示】

（1）不应输出词法分析结果，否则会导致评测机无法匹配

（2）仅输出SysY文法定义中的非终结符。终结符和自行定义的非终结符都不在输出范围

（3）应当采用左递归原则实现，确保输出顺序一致

（4）对于文法中 {...} 重复结构，父节点标签只输出一次。推荐使用"辅助列表 + 包装器"模式实现；也可采用其他实现方式，只要满足输出次数与顺序要求即可


        以 FuncFParams → FuncFParam { ',' FuncFParam } 为例：

                   FuncFParams : FuncFParamsList { print("<FuncFParams>"); } ;

                   FuncFParamsList : FuncFParam | FuncFParamsList COMMA FuncFParam ;

        FuncFParamsList 作为辅助列表非终结符，不在输出范围。

        对于 int add(int a, int b)，应输出 2 次 <FuncFParam>，但只输出 1 次 <FuncFParams>。

        适用范围包括 `FuncFParams`、`FuncRParams`、`LVal` 的数组维度、`ConstDefDims` 等。

【调试建议】提交时仅输出语法成分标签，但建议在代码中保留Token输出逻辑，便于自行DEBUG。可以通过条件编译宏（如 #define DEBUG_TOKENS 0 ）控制。

【输入形式】testfile.txt中的符合文法要求的测试程序。

【输出形式】按如上要求将语法分析结果输出至output.txt中。

【特别提醒】（1）本次作业只考核对正确程序的处理，但需要为今后可能出现的错误情况预留接口。

                 （2）当前要求的输出只是为了便于评测，完成编译器中无需出现这些信息，请设计为方便打开/关闭这些输出的方案。

【样例输入】

int main(){
    return 3;
}
【样例输出】

<FuncType>
<Number>
<PrimaryExp>
<UnaryExp>
<MulExp>
<AddExp>
<Exp>
<Stmt>
<Block>
<FuncDef>
<CompUnit>
【评分标准】按与预期结果不一致的行数扣分，每项扣5%。

【开发语言及环境】评测机所采用的编译学生代码的版本是：C/C++ gcc 8.1.0，平台支持 C++11 标准。    

【提交形式】将flex yacc生成的语法分析程序的源文件（.cpp/.c/.h，不含工程文件，不含.l/.y）打包为zip或rar后提交。

文法如下：
补充更新内容：

    1. 常量数组不能作为参数传递；

    2. 相邻的 UnaryOp 不能相同，如 i = --+4; 是不符合语义约束的；

    3. UnaryOp 为 '!' 时只能出现在条件表达式中；

    4. ConstExp -> AddExp 这条规则所涉及的 Ident 必须为常量；

    5. printf 语句中的转义字符有且仅有 '\n'，即为了与 gcc 评测结果保持一致， '\' 不能单独出现（将视为不符合文法），只可以与 'n' 结对出现。

    6. 为了简化后续同学们开发编译器的难度，有返回值的函数的最后一个 <stmt> 一定是 return 语句（注意是 <FuncDef> 中 <Block> 的最后一个语句）

    7. 对于整数数字的要求：不含前导0，但数字 0 是合法的。

    8. main 函数的返回值只能为 0。

    【提示】有不少同学的错误在于输入输出不匹配，输出文件的行尾缺少本应该输出的空格。这种情况请用电脑自带的文本编辑器来保存文件并上传，有的 IDE 如 jetbrain 那一套，保存后会自动抹去行尾空格，造成输出不一致的问题。

    编译单元 CompUnit → {Decl} {FuncDef} MainFuncDef  // 1.是否存在Decl 2.是否存在FuncDef

    声明 Decl → ConstDecl | VarDecl // 覆盖两种声明

    常量声明 ConstDecl → 'const' BType ConstDef { ',' ConstDef } ';' // 1.花括号内重复0次 2.花括号内重复多次

    基本类型 BType → 'int' // 存在即可

    常数定义 ConstDef → Ident { '[' ConstExp ']' } '=' ConstInitVal  // 包含普通变量、一维数组、二维数组共三种情况

    常量初值 ConstInitVal → ConstExp
        | '{' [ ConstInitVal { ',' ConstInitVal } ] '}' // 1.常表达式初值 2.一维数组初值 3.二维数组初值

    变量声明 VarDecl → BType VarDef { ',' VarDef } ';' // 1.花括号内重复0次 2.花括号内重复多次

    变量定义 VarDef → Ident { '[' ConstExp ']' } // 包含普通变量、一维数组、二维数组定义
        | Ident { '[' ConstExp ']' } '=' InitVal

    变量初值 InitVal → Exp | '{' [ InitVal { ',' InitVal } ] '}'// 1.表达式初值 2.一维数组初值 3.二维数组初值

    函数定义 FuncDef → FuncType Ident '(' [FuncFParams] ')' Block // 1.无形参 2.有形参

    主函数定义 MainFuncDef → 'int' 'main' '(' ')' Block // 存在main函数

    函数类型 FuncType → 'void' | 'int' // 覆盖两种类型的函数 

    函数形参表 FuncFParams → FuncFParam { ',' FuncFParam } // 1.花括号内重复0次 2.花括号内重复多次

    函数形参 FuncFParam → BType Ident ['[' ']' { '[' ConstExp ']' }] // 1.普通变量 2.一维数组变量 3.二维数组变量

    语句块 Block → '{' { BlockItem } '}' // 1.花括号内重复0次 2.花括号内重复多次

    语句块项 BlockItem → Decl | Stmt // 覆盖两种语句块项

    语句 Stmt → LVal '=' Exp ';' // 每种类型的语句都要覆盖
        | [Exp] ';'  //有无Exp两种情况
        | Block 
        | 'if' '( Cond ')' Stmt [ 'else' Stmt ] // 1.有else 2.无else
        | 'while' '(' Cond ')' Stmt
        | 'break' ';' | 'continue' ';'
        | 'return' [Exp] ';' // 1.有Exp 2.无Exp
        | LVal = 'getint''('')'';'
        | 'printf' '('FormatString {',' Exp} ')'';' // 1.有Exp 2.无Exp

    表达式 Exp → AddExp 注：SysY 表达式是int 型表达式 // 存在即可

    条件表达式 Cond → LOrExp // 存在即可

    左值表达式 LVal → Ident {'[' Exp ']'} //1.普通变量 2.一维数组 3.二维数组

    基本表达式 PrimaryExp → '(' Exp ')' | LVal | Number // 三种情况均需覆盖

    数值 Number → IntConst // 存在即可

    一元表达式 UnaryExp → PrimaryExp | Ident '(' [FuncRParams] ')' // 三种情况均需覆盖,函数调用也需要覆盖FuncRParams的不同情况
        | UnaryOp UnaryExp // 存在即可

    单目运算符 UnaryOp → '+' | '−' | '!' 注：'!'仅出现在条件表达式中 // 三种均需覆盖

    函数实参表 FuncRParams → Exp { ',' Exp } // 1.花括号内重复0次 2.花括号内重复多次 3. Exp需要覆盖数组传参和部分数组传参

    乘除模表达式 MulExp → UnaryExp | MulExp ('*' | '/' | '%') UnaryExp // 1.UnaryExp 2.* 3./ 4.% 均需覆盖

    加减表达式 AddExp → MulExp | AddExp ('+' | '−') MulExp // 1.MulExp 2.+ 3.- 均需覆盖

    关系表达式 RelExp → AddExp | RelExp ('<' | '>' | '<=' | '>=') AddExp // 1.AddExp 2.< 3.> 4.<= 5.>= 均需覆盖

    相等性表达式 EqExp → RelExp | EqExp ('==' | '!=') RelExp // 1.RelExp 2.== 3.!= 均需覆盖

    逻辑与表达式 LAndExp → EqExp | LAndExp '&&' EqExp // 1.EqExp 2.&& 均需覆盖

    逻辑或表达式 LOrExp → LAndExp | LOrExp '||' LAndExp // 1.LAndExp 2.|| 均需覆盖

    常量表达式 ConstExp → AddExp 注：使用的Ident 必须是常量 // 存在即可

    格式化字符 FormatChar → %d

    普通字符 NormalChar → 十进制编码为32,33,40-126的ASCII字符

    字符 Char → FormatChar | NormalChar 

    格式化字符串 FormatString → '"'{ Char }'"'