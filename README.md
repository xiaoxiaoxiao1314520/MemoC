# MemoC


一个零第三方依赖、离线运行的 C 语言错题温习助手

\# MemoC - C语言错题温习助手



!\[Language](https://img.shields.io/badge/Language-C99-blue.svg)

!\[Dependencies](https://img.shields.io/badge/Dependencies-Zero-brightgreen.svg)

!\[Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey.svg)

!\[License](https://img.shields.io/badge/License-MIT-yellow.svg)



> 本项目为《软件工程导论》课程考查作业。一个零第三方依赖、离线运行的标准 C 语言控制台程序，旨在解决 C 语言初学者“错题散落各处、不知道每天该优先复习哪一条”的痛点。



\## 🌟 核心特色



与传统的“学生信息管理系统”模板不同，MemoC 侧重于\*\*调度决策\*\*与\*\*数据安全\*\*，具有以下三大独立设计的核心机制：



1\. \*\*基于遗忘规律的优先级调度\*\*

&#x20;  \* 算法公式：`P = F × (D + 1) × 60 / M`

&#x20;  \* 综合了\*\*错误频次 (F)\*\*、\*\*搁置天数 (D)\*\* 和 \*\*掌握度 (M)\*\*，将三者合成为一个整数分数。分数越高，越优先复习。

2\. \*\*指纹去重机制\*\*

&#x20;  \* 自动去除代码中的空格、换行并统一转为小写，通过自定义哈希函数生成“指纹”。指纹相同且前 64 字符比对一致，视为同一错题，自动增加频次并降低掌握度，避免账本冗余。

3\. \*\*原子化安全落盘\*\*

&#x20;  \* 拒绝直接覆盖原文件。采用「写入临时文件 -> 改名备份 -> 正式转正 -> 删除备份」的流程。若中途断电，可自动回滚，保证任意时刻磁盘上至少有一份完整账本。每行包含独立校验和，防范手工篡改导致的数据损坏。



\## 🛠️ 编译与运行



本项目仅依赖标准 C 库（`stdio.h`, `string.h`, `stdlib.h`, `ctype.h`, `time.h`, `limits.h`），支持 Windows / Linux / macOS。



\### 环境要求

\* GCC (MinGW) 或 Dev-C++ 等任何支持 ISO C99 的编译器。



\### 编译命令

```bash

gcc -std=c99 -Wall -o memoc memoc.c

```



运行



```bash

./memoc

```



(注：首次运行会自动在同目录生成 memoc.dat 账本文件。程序内置了 Windows 控制台 UTF-8 代码页切换，避免中文乱码。)



📖 账本格式



memoc.dat 为纯文本文件，一行代表一条记录，字段以 | 分隔：

fp | code | hint | tag | freq | mastery | last | next | sum



🧑‍💻 作者声明



· 开发者：\[xiaoxiao1314]

· 开发背景：大一《软件工程导论》课程作业。

· AI 工具使用声明：核心算法、数据结构和代码逻辑均由本人独立设计与编写。AI 工具仅用于辅助排版课程报告、绘制流程图及润色文本。

