# Lexer 实现说明

## 职责

词法模块将源代码字符流转换为 token 流，提供给 Bison 语法分析器或独立词法程序使用。

## 核心文件

- `include/Stream.h`：通用流模板，提供 `next()` / `peek()` / `unget()` 与行列追踪
- `include/Token.h`、`src/Token.cpp`：`TokenType`、`Token`、关键字映射与字符串化
- `include/Lexer.h`、`src/Lexer.cpp`：词法状态判断与 token 构造
- `src/lexer_main.cpp`：Homework 1 主程序，输出 `output.txt`

## 调用链

1. 程序创建 `Lexer`（来自文件路径或 `Stream<char>`）。
2. 上层循环调用 `Lexer::next_token()`。
3. `next_token()` 调用 `next_token_impl()` 并缓存 `m_current_token`。
4. `next_token_impl()` 读取字符，分派到各类读取函数。

## 关键实现

### 1) 字符流与位置追踪

- `Stream<char>::next()` 在读取时维护 `line` / `column`。
- 对 Windows 换行 `\r\n` 做了统一处理（归一为 `\n` 语义）。
- `current_location()` 供 lexer 在 token 起始处记录位置。

### 2) 标识符与关键字

- `read_identifier()` 识别 `[a-zA-Z_][a-zA-Z0-9_]*`。
- 通过 `TokenUtils::is_keyword()` 判断是否映射到关键字 token。

### 3) 整数常量

- `read_integer()` 支持：
  - 十进制：`[0-9]+`
  - 十六进制：`0x...` / `0X...`

### 4) 字符串常量

- `read_string()` 从起始双引号读到结束双引号。
- 当前实现以“读取直到下一个 `"` 或 EOF”为主，未在 lexer 层做复杂转义校验。

### 5) 运算符与界符

- `read_operator_or_delimiter()` 区分单双字符符号。
- 覆盖 `>= <= == != && ||` 及单字符运算符/界符。

### 6) 注释跳过

- `skip_comment()` 支持：
  - `//...` 单行注释
  - `/*...*/` 多行注释
- 在 `next_token_impl()` 中，若读到 `/` 且判定为注释则继续扫描，不输出 token。

## 错误处理

- 无法识别的单字符或非法 `&` / `|` 组合返回 `TokenType::INVALID`。
- 文件打开失败由 `Stream` 构造函数抛异常，上层捕获并输出错误。

## 边界与限制

- `unget()` 注释声明“仅支持单次回退”，调用方应避免连续回退设计。
- 字符串内容合法性（如非法转义、未闭合引号的精细诊断）目前较简化。

## 可扩展点

- 扩展 token 类型：在 `TokenType`、`TokenUtils` 与 `Lexer` 分派逻辑同步添加。
- 增强字符串/字符字面量校验：可在 `read_string()` 中增加状态机。
- 强化错误恢复：可让 lexer 输出更丰富诊断而非仅 `INVALID`。

## 验证方式

- 运行：`build/lexer <input.sy>`（或默认读取 `testfile.txt`）
- 输出：`output.txt`
- 观测：每行 `TokenType lexeme`，可用于对照 Homework 1 评测格式
