# Parser 实现说明

## 职责

语法模块负责将 token 流解析为 AST，并在需要时输出文法标签，服务 Homework 2 与后续 codegen 前端。

## 核心文件

- `src/sysy.y`：Bison 文法、语义动作、`yylex()` / `yyerror()`
- `include/ParserFrontend.h`、`src/ParserFrontend.cpp`：对外文件到 AST 的封装接口
- `include/Ast.h`、`src/Ast.cpp`：AST 容器与构建辅助
- `include/Lexer.h`、`src/Lexer.cpp`：词法来源

## 调用链

1. 上层调用 `parse_file_to_ast(input_path, &root, &error)`。
2. `ParserFrontend` 创建 `Lexer`，通过 `set_lexer()` 绑定到 Bison 全局上下文。
3. 调用 `yyparse()` 进行规约。
4. 成功后调用 `ast::take_ast_root()` 获取 AST。

## 关键实现

### 1) Bison 解析器模式

- `sysy.y` 使用 `%glr-parser`，提升冲突处理弹性。
- 通过 `%nonassoc LOWER_THAN_ELSE` / `%nonassoc ELSETK` 处理 dangling else。

### 2) 词法桥接

- `yylex()` 从 `Lexer::next_token()` 拉取 token。
- 使用 `switch` 将 `TokenType` 映射到 Bison token 常量。
- `END_OF_FILE` 映射为 `0`，表示输入结束。

### 3) AST 构建策略

- 顶层通过 `Unit` 规则触发 `ast::push_ast_item(...)`，记录 Decl / FuncDef / MainFuncDef。
- 当前 AST 以顶层结构为核心，满足后续语义检查与 codegen 的最小需求。

### 4) 标签输出（课程兼容）

- `PRINT_TAG(...)` 宏在 `enable_parser_output` 为 true 且输出文件打开时写标签。
- 主要用于 Homework 2 期望的文法标签输出行为。

## 错误处理

- `yyerror()` 组装错误信息：语法错误描述 + 当前 token 的行列和词素。
- `ParserFrontend` 在 `yyparse()!=0` 时优先取 `take_parser_error()`，并设置回退信息。
- `parse_file_to_ast()` 使用异常捕获避免泄漏到调用层。

## 边界与限制

- 当前 AST 结构偏“轻量顶层信息”，并非完整语义树表达。
- `yylex()` 的 `default: return -1;` 属于保底分支，诊断语义仍可继续细化。

## 可扩展点

- 完整化 AST 节点类型与语义值传递（声明、表达式、语句层级）。
- 将错误恢复策略加入文法（如 `error` 产生式）以提升多错误报告能力。
- 将全局 parser 状态改造成上下文对象，减少全局变量耦合。

## 验证方式

- 执行 `build/parser` 会触发 parser + codegen 整链路。
- 单独 parser 行为可结合 `tests/test_parser.cpp` 与 `tests/test_parser_debug.cpp` 观察。
- 若解析失败，终端可见 `yyerror` 组织的位置信息。
