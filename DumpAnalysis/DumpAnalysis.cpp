#include "common.h"

#include "CdbHelper.h"
#include "PeParser.h"

// 分析的dump文件路径
constexpr LPCWSTR g_dumpPath = L"D:\\analyze\\PUBG\\parent\\TslGame.dmp";

// 需要输出为PE文件的模块名
constexpr LPCSTR g_moduleName = "TslGame";

// 输出的PE文件路径
constexpr LPCWSTR g_outputPEPath = L"D:\\analyze\\PUBG\\parent\\TslGame_dump.exe";

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
                break;
            }
        }
        if (NULL == moduleBase)
        {
            LOG("can't find");
            break;
        }

        // 解析并输出PE文件
        PeParser peParser((DWORD_PTR)moduleBase, true);
        /*
        if (!peParser.isValidPeFile())
        {
            LOG("isValidPeFile failed");
            break;
        }
        */
        DWORD_PTR entryPoint = (ULONG_PTR)moduleBase + peParser.getEntryPoint();
        if (!peParser.dumpProcess((DWORD_PTR)moduleBase, entryPoint, g_outputPEPath))
        {
            LOG("dumpProcess failed");
            break;
        }
    } while (false);

    if (!cdbHelper->FinishCdb())
    {
        LOG("FinishCdb failed");
    }

    return 0;
}
