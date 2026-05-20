# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目简介

Xbtn2PgScroll 是一个 Windows 系统托盘工具，全局拦截鼠标侧键（XBUTTON1/XBUTTON2）并映射为键盘 PageUp/PageDown。

## 构建

使用 Visual Studio 2026 (v145 工具集) 打开 `Xbtn2PgScroll.slnx` 构建。

使用 `PowerShell` 执行以下命令进行 **Release x64** 构建，**不要**使用 `Bash`。

```PowerShell
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" Xbtn2PgScroll.slnx /p:Configuration=Release /p:Platform=x64 /v:minimal
```

输出路径: `x64\Release\Xbtn2PgScroll.exe`

## 编译选项

- 语言标准: C++23，C17
- 额外编译选项: `/utf-8 /Zc:__cplusplus`
- 静态链接 CRT (`/MT`)，无 MSVC 运行时 DLL 依赖
- 仅 x64 配置为目标，Win32 配置为未使用的默认值

## 架构

整个应用仅一个源文件 `Xbtn2PgScroll/src/main.cpp`，无外部依赖，仅使用 Windows SDK。

核心流程:
1. **消息窗口** — `CreateWindowExW` 创建 `HWND_MESSAGE` 类型的不可见窗口，作为托盘图标的宿主
2. **鼠标钩子** — `SetWindowsHookExW(WH_MOUSE_LL, ...)` 安装系统级低级鼠标钩子，拦截所有鼠标事件
3. **按键映射** — `mouse_proc` 中检测 `WM_XBUTTONDOWN`，XBUTTON1→PageDown，XBUTTON2→PageUp，通过 `SendInput` 注入合成按键；XBUTTONDOWN 和 XBUTTONUP 均返回 1（吞掉原始事件）
4. **系统托盘** — `Shell_NotifyIconW` 注册托盘图标，右键菜单提供"退出"选项
5. **退出清理** — `WM_DESTROY` 中卸载钩子、删除托盘图标、`PostQuitMessage`

## 注意事项

- 安装 `WH_MOUSE_LL` 钩子需要**管理员权限**，否则会弹窗提示并退出
- 应用程序清单设置了 `PerMonitorV2` DPI 感知
- 退出方式: 托盘图标右键菜单 → "退出"
