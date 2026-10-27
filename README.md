# 实验1 带键盘事件的计算器

Qt Widgets 实现的四则运算计算器，**鼠标点击与键盘按键走同一套处理逻辑**。

- 课程：Qt 应用程序开发（2026 年秋季）
- 姓名：罗汝佳　学号：2024414290424　班级：24 级软件工程 4 班

## 一、开发环境

| 项目 | 版本 |
| --- | --- |
| 操作系统 | Windows 11 (10.0.26200) |
| Qt | 6.11.2 mingw_64（实验指导书要求 6.9.2，本机为 6.11.2，API 完全兼容） |
| 编译器 | MinGW-w64 g++ 13.1.0（`C:\Qt\Tools\mingw1310_64`） |
| 构建工具 | qmake + mingw32-make |
| 版本管理 | Git 2.54 |

## 二、编译运行

```bash
# 命令行构建（也可以直接用 Qt Creator 打开 Calculator.pro）
mkdir build-desktop && cd build-desktop
C:/Qt/6.11.2/mingw_64/bin/qmake.exe ../Calculator.pro
mingw32-make -j8
./release/Calculator.exe
```

## 三、目录结构

```
Exp1_Calculator/
├── Calculator.pro          工程文件
├── main.cpp                程序入口
├── mainwindow.h            主窗口声明（状态机成员变量在此声明）
├── mainwindow.cpp          全部业务逻辑
├── mainwindow.ui           Qt Designer 界面（布局与控件属性）
├── resources.qrc           资源文件
├── style.qss               QSS 样式表
├── screenshots/            实验报告用的运行结果截图
└── tools/screenshot/       截图驱动工具（以真实键盘/鼠标事件驱动界面后抓图）
```

## 四、界面结构

主窗口使用 `QVBoxLayout` 垂直排布三部分，按键区使用 `QGridLayout`（5 行 4 列）：

```
┌─────────────────────────────┐
│  expressionLabel  表达式行   │  ← QLabel，右对齐，显示 "12.5 + 3.75 ="
├─────────────────────────────┤
│  displayEdit      主显示区   │  ← QLineEdit，readOnly，右对齐，22pt
├──────┬──────┬──────┬────────┤
│  C   │  ←   │  ÷   │   ×    │
│  7   │  8   │  9   │   -    │
│  4   │  5   │  6   │   +    │
│  1   │  2   │  3   │   =    │  ← 等号跨 2 行
│    0（跨 2 列）│  .   │        │
└──────┴──────┴──────┴────────┘
```

控件命名规则：`btnDigit0`~`btnDigit9`、`btnPoint`、`btnAdd` / `btnSubtract` / `btnMultiply` / `btnDivide`、`btnEqual`、`btnBackspace`、`btnClear`；显示区为 `displayEdit`、表达式行为 `expressionLabel`。

所有按钮 `focusPolicy = NoFocus`，保证键盘事件不会被按钮抢走，而是稳定送到 `MainWindow::keyPressEvent()`。

## 五、状态机设计

计算器全部状态由 5 个成员变量描述：

| 变量 | 含义 |
| --- | --- |
| `m_currentInput` | 当前正在输入的操作数（**字符串**，可保留 `12.` 这类中间态） |
| `m_accumulated` | 左操作数；连续运算时保存累积结果 |
| `m_pendingOperator` | 待执行运算符；为空表示当前无待执行运算 |
| `m_waitingForOperand` | true 表示下一个数字要开启新操作数（刚按运算符或刚出结果） |
| `m_hasError` | 错误态（除零 / 溢出），只能由 C 或新的数字输入解除 |

处理流程：

```
鼠标点击按钮 ──clicked()──┐
                          ├─> handleInput(token) ─> inputDigit / inputDecimalPoint
键盘按键 ──keyPressEvent()─┘                        / applyOperator / calculateResult
                                                    / backspace / clearAll
```

`handleInput()` 是**唯一的输入入口**，这也是"鼠标与键盘复用同一套逻辑"的实现方式。

## 六、键盘映射

| 按键 | 功能 |
| --- | --- |
| `0`~`9`（含小键盘） | 输入数字 |
| `.` / `,` | 小数点 |
| `+` `-` `*` `/` | 四则运算 |
| `Enter` / 小键盘 `Enter` / `=` | 计算结果 |
| `Backspace` | 退格 |
| `Esc` / `Delete` | 清除（同 C） |

键盘输入时对应按钮会闪烁 120 ms，使两种输入方式的视觉反馈一致。

## 七、边界情况处理

| 输入序列 | 期望结果 | 处理方式 |
| --- | --- | --- |
| `12..3` | `12.3` | 追加 `.` 前检查 `m_currentInput.contains('.')` |
| `5 ÷ 0 =` | 提示"除数不能为 0" | `qFuzzyIsNull(rhs)` 在除法前拦截，进入错误态 |
| `2 + 3 = 7` | `7` | 等号后置 `m_waitingForOperand = true` |
| `2 + 3 +` | 显示 `5 +` | 连续运算：先结算再挂新运算符 |
| `2 + 3 × 4 =` | `20` | 按输入顺序逐次结算（无优先级，与手机计算器一致） |
| `8 + =` | `16` | 第二个操作数缺省取左操作数 |
| `0.0000001 × 0.0000001` 等溢出 | 提示"结果超出范围" | `std::isfinite()` 兜底 |
| 退格删空 | 回落为 `0` | `backspace()` 中判空 |

## 八、推送到 GitHub

本机未安装 GitHub CLI，仓库已在本地初始化并完成完整迭代提交。推送到远程：

```bash
# 1. 在 GitHub 网页上新建空仓库（不要勾选 README / .gitignore）
# 2. 添加远程并推送
git remote add origin https://github.com/<你的用户名>/Exp1_Calculator.git
git branch -M main
git push -u origin main
```
