#include "common.h"

#include "CdbHelper.h"
#include "PeParser.h"

constexpr LPCWSTR g_dumpPath = L"D:\\analyze\\CalculatorApp.dmp";
constexpr LPCSTR g_moduleName = "ntdll";
constexpr LPCWSTR g_dumpPEPath = L"D:\\analyze\\ntdll_mydump.dll";

int main()
{
    CdbHelper* cdbHelper = CdbHelper::GetInstance();

    do
    {
        // 初始化
        if (!cdbHelper->InitCdb(g_dumpPath))
        {
            LOG("InitCdb failed");
            break;
        }

        // 获取模块列表
        std::vector<CdbHelper::ModuleInfo> moduleInfoList{};
        if (!cdbHelper->GetModuleList(moduleInfoList))
        {
            LOG("GetModuleList failed");
            break;
        }

        // 遍历模块列表，找到想要的模块
        PVOID moduleBase = NULL;
        SIZE_T moduleLength = 0;
        for (const CdbHelper::ModuleInfo& moduleInfo : moduleInfoList)
        {
            if (0 == stricmp(moduleInfo.moduleName.c_str(), g_moduleName))
            {
                moduleBase = moduleInfo.baseAddress;
                moduleLength = (ULONG_PTR)moduleInfo.endAddress - (ULONG_PTR)moduleInfo.baseAddress;
            }
        }
        if (NULL == moduleBase)
        {
            LOG("can't find");
            break;
        }

        // 解析并输出PE文件
        PeParser peParser((DWORD_PTR)moduleBase, true);
        if (!peParser.isValidPeFile())
        {
            LOG("isValidPeFile failed");
            break;
        }
        DWORD_PTR entryPoint = (ULONG_PTR)moduleBase + peParser.getEntryPoint();
        peParser.dumpProcess((DWORD_PTR)moduleBase, entryPoint, g_dumpPEPath);
    } while (false);

    if (!cdbHelper->FinishCdb())
    {
        LOG("FinishCdb failed");
    }

    return 0;
}
