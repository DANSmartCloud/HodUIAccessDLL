# HodUIAccessDLL

HodUIAccessDLL是由禾云信创工作室·云云探索者开发的一款窗口超级置顶DLL，简称UIAccessDLL。

玩技术，来禾云技术研究中心！（QQ群号`884676312`）

适用于：Windows 7 及以上（建议 Windows 10/11），x86（32 位）进程，C/C++/易语言等调用者。

原项目参考：[killtimer0/uiaccess: 通过System令牌获取UIAccess](https://github.com/killtimer0/uiaccess)

## 概述

UIAccessDLL 将通过 System token 获取 UIAccess 权限”的逻辑封装为标准 DLL 接口，便于第三方程序调用，尝试让应用窗口获得 UI Access 层级，使 `SetWindowPos(HWND_TOPMOST)` 在更高的 Z 序生效。

> 说明  
> 
> - 真正进入 UIAccess 窗口段通常需要在“具有 UIAccess 标志的进程令牌”下重新启动进程。  本项目仅供学习研究。
> - 本 DLL 会尽力准备 UIAccess 所需的权限（设置 TokenUIAccess），若当前进程未重启，`UIAccess_CheckStatus()` 可能仍返回未启用。此时建议在拿到权限后自行重启进程或调用智能无感重启。  

## 功能特性

- ✅ **32位兼容**：专为32位环境设计
- ✅ **兼容调用约定**：使用`__stdcall`调用约定，完美兼容易语言
- ✅ **完整错误处理**：提供详细的错误码和错误信息
- ✅ **简单易用**：封装复杂的Windows API调用
- ✅ **内存安全**：自动管理资源，防止内存泄漏

## 文件结构

```
UIAccessDLL/
├── UIAccessDLL.h          # DLL头文件声明
├── UIAccessDLL.c          # DLL主要实现代码
├── UIAccessDLL.def        # DLL导出函数定义
├── UIAccessDLL.dll        # 编译生成的DLL文件
├── UIAccessDLL.lib        # 导入库文件
├── TestDLL.c              # C语言测试程序
├── TestDLL.exe            # 测试程序可执行文件
├── build_all.bat              # 编译脚本
└── README.md              # 本说明文档
```

## 前提条件

- 以管理员权限运行调用进程（否则常见返回码为“访问被拒绝”）。
- 仅 32 位进程可用（与项目配置一致）。
- 将以下文件放置在同一目录或 PATH 可达：
  - `UIAccessDLL.dll`
  - `UIAccessDLL.h`（头文件，静态编译时需要）
  - 可选：`UIAccessDLL.lib`（若选择静态链接导入库）

## 导出的 API

```c
// 错误码
#define UIACCESS_SUCCESS                0  // 操作成功
#define UIACCESS_ERROR_ALREADY_ENABLED  1  // UI Access权限已经启用
#define UIACCESS_ERROR_ACCESS_DENIED    2  // 访问被拒绝，需要管理员权限
#define UIACCESS_ERROR_SYSTEM_ERROR     3  // 系统错误
#define UIACCESS_ERROR_NOT_INITIALIZED  4  // DLL未初始化

// __stdcall 调用约定
int __stdcall UIAccess_Initialize(void);    // 初始化
int __stdcall UIAccess_GetPermission(void); // 获取UIAccess权限
int __stdcall UIAccess_CheckStatus(void);   // 判断当前应用程序是否已经具备UIAccess。返回值：1=已启用, 0=未启用, -1=失败（可用 GetLastError 获取）
int __stdcall UIAccess_Cleanup(void);       // 程序结束前清理
const char* __stdcall UIAccess_GetErrorMessage(int errorCode);//通过错误代码获取错误信息（错误代码转可读文本）
int __stdcall UIAccess_GetLastError(void);  // 获取最近的错误信息

// 新增：无感获取与句柄置顶
int __stdcall UIAccess_PrepareAndRelaunchIfNeeded(void);      // 智能无感重启获取；若未启用，尝试以 UIAccess 令牌自重启；成功时本进程会退出
int __stdcall UIAccess_SetTopmost(HWND hwnd, int enable);     // hwnd: 目标窗口句柄；enable: 1=置顶, 0=取消
const char* __stdcall AboutThisDLL(void);                     // 返回 UTF-8 介绍字符串
```

## 使用流程

- 建议在程序启动早期调用：
  1) `UIAccess_Initialize()`（初始化）
  2) 可选：`UIAccess_PrepareAndRelaunchIfNeeded()`（可能直接以 UIAccess 令牌重启本进程；若成功，当前进程会退出）
- 交互期按需调用：
  3) `UIAccess_CheckStatus()` （判断是否已具备 UIAccess）
  4) 未启用时调用 `UIAccess_GetPermission()`，必要时再次调用 `UIAccess_PrepareAndRelaunchIfNeeded()` 完成无感重启
  5) 需要置顶任意窗口时：`UIAccess_SetTopmost(hwnd, 1)`；取消置顶：`UIAccess_SetTopmost(hwnd, 0)`
  6) 程序结束调用 `UIAccess_Cleanup()`

## C++ 示例（Win32 GUI，200x400 窗口）

示例源码：`examples/CppSample/UIAccessCppSample.cpp`  
特点：动态加载 `UIAccessDLL.dll`，点击按钮尝试获取 UIAccess 并将窗口 `TOPMOST`。

构建（使用 Developer Command Prompt 或已就绪的 cl 环境）：

```cmd
cl /DUNICODE /D_UNICODE /EHsc /W3 /O2 /Zi /Fe:UIAccessCppSample.exe ^
  examples\CppSample\UIAccessCppSample.cpp user32.lib gdi32.lib
```

运行要求：

- 将 `UIAccessDLL.dll` 与生成的 `UIAccessCppSample.exe` 放在同一目录
- 右键“以管理员身份运行”示例程序
- 按按钮执行权限获取与置顶操作（可手动打开任务管理器或屏幕键盘做相互覆盖测试）

> 注意  
> 若 `UIAccess_CheckStatus()` 仍为未启用，属于预期情况：需要在 UIAccess 令牌下重启进程方可进入 UIAccess 段。您可在业务程序中在 `UIAccess_GetPermission()` 成功后安排一次自我重启或调用智能无感重启。

## 易语言调用说明

- ### 全部DLL命令声明（x86 进程，HWND/BOOL 用整数型即可）：
  
  ```E
  .版本 2
  
  .DLL命令 超级置顶检查状态, 整数型, "UIAccessDLL.dll", "UIAccess_CheckStatus"
  
  .DLL命令 超级置顶错误码转错误消息, 文本型, "UIAccessDLL.dll", "UIAccess_GetErrorMessage"
      .参数 错误码, 整数型
  
  .DLL命令 获取最后一个超级置顶错误, 整数型, "UIAccessDLL.dll", "UIAccess_GetLastError"
  
  .DLL命令 关于本DLL, 文本型, "UIAccessDLL.dll", "AboutThisDLL"
  
  .DLL命令 设置窗口超级置顶, 整数型, "UIAccessDLL.dll", "UIAccess_SetTopmost"
      .参数 窗口句柄, 整数型
      .参数 置顶, 整数型, , 0为取消置顶，1为置顶
  
  .DLL命令 智能应用超级置顶, 整数型, "UIAccessDLL.dll", "UIAccess_PrepareAndRelaunchIfNeeded"
  
  .DLL命令 获取超级置顶权限, 整数型, "UIAccessDLL.dll", "UIAccess_GetPermission"
  
  .DLL命令 清理超级置顶, 整数型, "UIAccessDLL.dll", "UIAccess_Cleanup", , 请务必在退出时调用！
  
  .DLL命令 初始化超级置顶, 整数型, "UIAccessDLL.dll", "UIAccess_Initialize", , 切记不能调试运行，只能编译后运行，否则会出错！
  ```

- ### 易语言快速上手示例
  
  下面示例展示初始化→无感获取→检查状态→置顶→清理的完整流程（x86 进程；以管理员运行；DLL 与 EXE 同目录）：
  
  **DLL命令定义表**

```E
.版本 2

.DLL命令 超级置顶检查状态, 整数型, "UIAccessDLL.dll", "UIAccess_CheckStatus"

.DLL命令 设置窗口超级置顶, 整数型, "UIAccessDLL.dll", "UIAccess_SetTopmost"
    .参数 窗口句柄, 整数型
    .参数 置顶, 整数型, , 0为取消置顶，1为置顶

.DLL命令 重启并应用超级置顶, 整数型, "UIAccessDLL.dll", "UIAccess_PrepareAndRelaunchIfNeeded"

.DLL命令 获取超级置顶权限, 整数型, "UIAccessDLL.dll", "UIAccess_GetPermission"

.DLL命令 清理超级置顶, 整数型, "UIAccessDLL.dll", "UIAccess_Cleanup", , 请务必在退出时调用！

.DLL命令 初始化超级置顶, 整数型, "UIAccessDLL.dll", "UIAccess_Initialize", , 切记不能调试运行，只能编译后运行，否则会出错！

.DLL命令 关于DLL, 文本型, "UIAccessDLL.dll", "AboutThisDLL"
```

    **示例源码**

```E
.版本 2 
.程序集 窗口程序集_启动窗口 
.子程序 __启动窗口_创建完毕 
初始化超级置顶 ()
超级置顶检查状态 ()
获取超级置顶权限 ()
智能应用超级置顶 ()
超级置顶检查状态 ()
设置窗口超级置顶 (取窗口句柄 (), 1)
调试输出 (UTF8到文本 (到字节集 (关于本DLL ())))
.子程序 __启动窗口_将被销毁 
清理超级置顶 () 
.子程序 _时钟1_周期事件 
_启动窗口.总在最前 ＝ 真
```

## 以导入库方式静态链接（可选）

若不想动态加载，可在项目中：

- 包含头文件：`#include "UIAccessDLL.h"`
- 链接导入库：在链接器附加依赖项中添加 `UIAccessDLL.lib`
- 运行时确保 `UIAccessDLL.dll` 可被找到

## 以易语言模块方式导入（可选）

若不想动脑思考，可在易语言项目中：

- 加载易模块：`HodUIAccess.ec`
- 调用子程序：直接用中文调用易模块中的子程序实现超级置顶，方式与示例源码中一致，但是不用导入DLL
- 无需导入：运行时 `UIAccessDLL.dll` 将被易模块自动释放
- 管理员权限：请务必勾选易语言中`工具>系统配置>存根>运行前是否请求管理员权限（UAC）`
- 易语言模块开发中

## 技术支持

如果遇到问题，请到禾云技术研究中心（QQ群号`884676312`）并提供以下信息：

- Windows版本和架构
- 易语言版本
- 错误码和错误信息
- 是否以管理员身份运行
- 运行效果截图或录屏

## 版本历史

**v1.0** (2025-09-04)

- 初始版本发布
- 支持基本的UI Access权限获取
- 提供完整的易语言接口
- 包含详细的错误处理机制

## 许可证与安全

- 项目使用HSLC v1.0开源协议开源，请遵守开源协议
- 本项目仅供学习研究，不包含恶意代码；请勿用于违反安全策略的场景
- 不建议在生产环境使用，生产环境建议走官方流程获取UI Access