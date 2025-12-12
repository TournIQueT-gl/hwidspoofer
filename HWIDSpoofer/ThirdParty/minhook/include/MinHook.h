/*
 * MinHook - The Minimalistic API Hooking Library
 * Copyright (C) 2009-2017 Tsuda Kageyu
 * 
 * 这是MinHook的头文件接口
 * 完整源码请从以下地址获取：
 * https://github.com/TsudaKageworthy/minhook
 * 
 * 或使用vcpkg安装：
 * vcpkg install minhook
 */

#ifndef MINHOOK_H
#define MINHOOK_H

#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif

// MinHook状态码
typedef enum MH_STATUS {
    // 成功
    MH_OK = 0,
    
    // 未知错误
    MH_UNKNOWN = -1,
    
    // MinHook已初始化
    MH_ERROR_ALREADY_INITIALIZED = 1,
    
    // MinHook未初始化
    MH_ERROR_NOT_INITIALIZED = 2,
    
    // Hook已存在
    MH_ERROR_ALREADY_CREATED = 3,
    
    // Hook不存在
    MH_ERROR_NOT_CREATED = 4,
    
    // Hook已启用
    MH_ERROR_ENABLED = 5,
    
    // Hook已禁用
    MH_ERROR_DISABLED = 6,
    
    // 指定的目标函数不在可执行区域
    MH_ERROR_NOT_EXECUTABLE = 7,
    
    // 指定的目标函数格式不正确
    MH_ERROR_UNSUPPORTED_FUNCTION = 8,
    
    // 内存分配失败
    MH_ERROR_MEMORY_ALLOC = 9,
    
    // 内存保护修改失败
    MH_ERROR_MEMORY_PROTECT = 10,
    
    // 指定的模块未找到
    MH_ERROR_MODULE_NOT_FOUND = 11,
    
    // 指定的函数未找到
    MH_ERROR_FUNCTION_NOT_FOUND = 12,
    
} MH_STATUS;

// 特殊目标：所有Hook
#define MH_ALL_HOOKS NULL

// 初始化MinHook库
MH_STATUS WINAPI MH_Initialize(void);

// 清理MinHook库
MH_STATUS WINAPI MH_Uninitialize(void);

// 创建Hook（禁用状态）
// pTarget    - 要Hook的目标函数
// pDetour    - 替代函数
// ppOriginal - 接收原始函数指针的变量
MH_STATUS WINAPI MH_CreateHook(
    LPVOID pTarget,
    LPVOID pDetour,
    LPVOID* ppOriginal
);

// 创建API Hook（禁用状态）
// pszModule  - 模块名
// pszProcName - 函数名
// pDetour    - 替代函数
// ppOriginal - 接收原始函数指针的变量
MH_STATUS WINAPI MH_CreateHookApi(
    LPCSTR pszModule,
    LPCSTR pszProcName,
    LPVOID pDetour,
    LPVOID* ppOriginal
);

// 创建API Hook（禁用状态）- 宽字符版本
// pszModule  - 模块名
// pszProcName - 函数名
// pDetour    - 替代函数
// ppOriginal - 接收原始函数指针的变量
// ppTarget   - 接收目标函数指针的变量
MH_STATUS WINAPI MH_CreateHookApiEx(
    LPCWSTR pszModule,
    LPCSTR pszProcName,
    LPVOID pDetour,
    LPVOID* ppOriginal,
    LPVOID* ppTarget
);

// 移除Hook
// pTarget - 目标函数，或MH_ALL_HOOKS移除所有
MH_STATUS WINAPI MH_RemoveHook(LPVOID pTarget);

// 启用Hook
// pTarget - 目标函数，或MH_ALL_HOOKS启用所有
MH_STATUS WINAPI MH_EnableHook(LPVOID pTarget);

// 禁用Hook
// pTarget - 目标函数，或MH_ALL_HOOKS禁用所有
MH_STATUS WINAPI MH_DisableHook(LPVOID pTarget);

// 在挂起所有线程的情况下启用/禁用Hook
MH_STATUS WINAPI MH_QueueEnableHook(LPVOID pTarget);
MH_STATUS WINAPI MH_QueueDisableHook(LPVOID pTarget);
MH_STATUS WINAPI MH_ApplyQueued(void);

// 获取状态码的字符串描述
const char* WINAPI MH_StatusToString(MH_STATUS status);

#ifdef __cplusplus
}
#endif

#endif // MINHOOK_H

