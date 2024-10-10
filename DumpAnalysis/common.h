#pragma once

#include <windows.h>
#include <string>
#include <thread>
#include <assert.h>
#include <iostream>
#include <filesystem>
#include <vector>
#include <sstream>
#include <dbghelp.h>

#pragma warning(disable: 4996)

#pragma comment(lib, "Dbghelp.lib")

#define CONVERT_RVA(base, offset) ((PVOID)((PUCHAR)(base) + (ULONG)(offset)))

#define LOG_TAG "GameCheater"

#define __FUNC__ __func__
#define __FILENAME__ \
  (strrchr(__FILE__, '\\') ? (strrchr(__FILE__, '\\') + 1) : __FILE__)

#define LOG(...) \
  tool::log(LOG_TAG, __FILENAME__, __LINE__, __FUNC__, __VA_ARGS__)

#define WLOG(...) \
  tool::wlog(LOG_TAG, __FILENAME__, __LINE__, __FUNC__, __VA_ARGS__)

namespace tool
{

std::string Format(const char* format, ...);
std::wstring Format(const wchar_t* format, ...);

template <typename... Args>
inline void log(
    const char* tag, const char* _file, int _line, const char* _fun,
    const char* fmt, Args... args)
{
    std::string log = Format(fmt, args...);
    std::string allLog = Format("[%s]-%s(%d)::%s %s\n", tag, _file, _line, _fun, log.c_str());
    printf(allLog.c_str());
}

template <typename... Args>
inline void wlog(
    const char* tag, const char* _file, int _line, const char* _fun,
    const wchar_t* fmt, Args... args)
{
    std::wstring log = Format(fmt, args...);
    std::wstring allLog = Format(L"[%s]-%s(%d)::%s %ws\n", tag, _file, _line, _fun, log.c_str());
    wprintf(allLog.c_str());
}

// 字符串转换
std::wstring ConvertCharToWString(const char* charStr);
std::string ConvertWCharToString(const wchar_t* wcharStr);

// 提权
BOOL AdjustProcessTokenPrivilege();

// 模块获取自身所处文件夹目录
bool GetCurrentModuleDirPath(WCHAR* dirPath);

// 运行命令
bool RunAppWithCommand(
    const wchar_t* application, const wchar_t* command, HANDLE* process);
bool RunAppWithRedirection(
    const wchar_t* application, const wchar_t* command,
    HANDLE input, HANDLE output, HANDLE error, HANDLE* process);

}
