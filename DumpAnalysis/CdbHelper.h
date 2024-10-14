#pragma once

#include "common.h"

class CdbHelper
{
public:
    static CdbHelper* GetInstance()
    {
        static CdbHelper instance;
        return &instance;
    }

    bool InitCdb(std::wstring dump_path);

    bool FinishCdb();

    bool SendCommond(std::string command, std::string& result);

    bool GetData2(PVOID pStartAddress, SIZE_T length, PBYTE pByte);
    bool GetData(PVOID pStartAddress, SIZE_T length, PBYTE pByte);

    typedef struct _ModuleInfo
    {
        PVOID baseAddress;
        PVOID endAddress;
        std::string moduleName;
        std::string modulePath;
    } ModuleInfo, * PModuleInfo;
    bool GetModuleList(std::vector<ModuleInfo>& moduleInfoList);

private:
    CdbHelper() = default;
    ~CdbHelper() = default;
    CdbHelper(const CdbHelper&) = delete;
    CdbHelper& operator=(const CdbHelper&) = delete;

private:
    bool isInit_{ false };

    HANDLE hChildStd_IN_Wr_{ NULL };
    HANDLE hChildStd_OUT_Rd_{ NULL };
    HANDLE hChildStd_ERR_Rd_{ NULL };

    HANDLE hCdbProcess_{ NULL };
};
