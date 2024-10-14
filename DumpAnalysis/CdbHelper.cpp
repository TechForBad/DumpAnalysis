#include "CdbHelper.h"

#define TEMP_BUFFER_LEN 4096

static std::vector<std::string> splitStringBySpace(const std::string& str)
{
    std::vector<std::string> tokens;
    std::istringstream iss(str);
    std::string token;
    while (iss >> token)
    {
        tokens.push_back(token);
    }
    return tokens;
}

static std::vector<std::string> splitStringByLine(const std::string& str)
{
    std::vector<std::string> lines;
    std::istringstream iss(str);
    std::string line;
    while (std::getline(iss, line, '\n'))
    {
        lines.push_back(line);
    }
    return lines;
}

bool CdbHelper::SendCommond(std::string command, std::string& result)
{
    if (!isInit_)
    {
        LOG("Not init");
        return false;
    }

    std::string outputBuffer = "";
    CHAR tempOutputBuffer[TEMP_BUFFER_LEN];
    DWORD tempBytesRead = 0;
    DWORD bytesWritten = 0;

    if (command.empty())
    {
        LOG("Command is empty");
        return false;
    }

    if (command[command.size() - 1] != '\n')
    {
        command += '\n';
    }

    // 写数据
    if (!WriteFile(hChildStd_IN_Wr_, command.data(), command.size(), &bytesWritten, NULL))
    {
        LOG("WriteFile failed");
        return false;
    }

    // 读数据
    do
    {
        if (!ReadFile(hChildStd_OUT_Rd_, tempOutputBuffer, sizeof(tempOutputBuffer) - 1, &tempBytesRead, NULL))
        {
            break;
        }
        tempOutputBuffer[tempBytesRead] = '\0';
        outputBuffer += tempOutputBuffer;

        size_t pos = outputBuffer.find_last_of('\n');
        if (pos != std::string::npos)
        {
            std::string lastLineBuffer = outputBuffer.substr(pos + 1);
            if (lastLineBuffer.size() > 5 &&
                lastLineBuffer.size() < 15 &&
                ' ' == lastLineBuffer[lastLineBuffer.size() - 1] &&
                '>' == lastLineBuffer[lastLineBuffer.size() - 2])
            {
                outputBuffer = outputBuffer.substr(0, pos);
                break;
            }
        }
    } while (true);

    result = outputBuffer;

    return true;
}

bool CdbHelper::GetData(PVOID pStartAddress, SIZE_T length, PBYTE pByte)
{
    if (!isInit_)
    {
        LOG("Not init");
        return false;
    }

    if (length <= 1)
    {
        std::string command_prefix = tool::Format(".for(r $t0 = 0x%llx; @$t0 < 0x%llx; r $t0 = @$t0 + 1)", (ULONG_PTR)pStartAddress, (ULONG_PTR)pStartAddress + length);
        std::string command_suffix = R"({.if($vvalid(@$t0, 1)){.printf @"%02x ", by(@$t0)}.else{.printf "?? "}};.printf "\n";)";
        std::string command = command_prefix + command_suffix;
        std::string result;
        if (!SendCommond(command, result))
        {
            LOG("SendCommond failed");
            return false;
        }

        std::vector<std::string> splitStringlist = splitStringBySpace(result);
        for (size_t i = 0; i < splitStringlist.size(); ++i)
        {
            std::string splitString = splitStringlist[i];
            if ("??" == splitString)
            {
                return false;
            }
            else
            {
                pByte[i] = std::stoi(splitString, nullptr, 16);
            }
        }
    }
    else
    {
        // 拼接得到临时文件路径
        CHAR tempFilePath[MAX_PATH] = { 0 };
        if (!tool::GetCurrentModuleDirPathA(tempFilePath))
        {
            LOG("GetCurrentModuleDirPathA failed");
            return false;
        }
        strcpy(tempFilePath, "temp.txt");

        // 如果文件存在，则删除
        if (tool::FilePathIsExist(tempFilePath, false))
        {
            if (!DeleteFileA(tempFilePath))
            {
                LOG("DeleteFileA failed");
                return false;
            }
        }

        // 打开临时文件
        HANDLE hFile = CreateFileA(
            tempFilePath,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
        if (INVALID_HANDLE_VALUE == hFile)
        {
            LOG("CreateFileA failed, last error: %d", GetLastError());
            DeleteFileA(tempFilePath);
            return false;
        }

        std::string command = tool::Format(".writemem %s 0x%llx l0x%llx", tempFilePath, (ULONG_PTR)pStartAddress, length);
        std::string result;
        if (!SendCommond(command, result))
        {
            LOG("SendCommond failed");
            DeleteFileA(tempFilePath);
            return false;
        }

        DWORD fileSize = GetFileSize(hFile, NULL);
        if (fileSize != length)
        {
            LOG("GetFileSize failed, read size: %d, need size: %d", fileSize, length);
            DeleteFileA(tempFilePath);
            return false;
        }

        SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

        DWORD bytesRead = 0;
        if (!ReadFile(hFile, pByte, length, &bytesRead, NULL))
        {
            LOG("ReadFile failed, last error: %d", GetLastError());
            CloseHandle(hFile);
            DeleteFileA(tempFilePath);
            return false;
        }

        if (bytesRead != length)
        {
            LOG("need read: %d, but read: %d", length, bytesRead);
            CloseHandle(hFile);
            DeleteFileA(tempFilePath);
            return false;
        }

        CloseHandle(hFile);
        DeleteFileA(tempFilePath);
    }

    return true;
}

bool CdbHelper::GetModuleList(std::vector<CdbHelper::ModuleInfo>& moduleInfoList)
{
    if (!isInit_)
    {
        LOG("Not init");
        return false;
    }

    std::string command = "!for_each_module .echo @#Base @#End @#ModuleName @#ImageName";
    std::string result;
    if (!SendCommond(command, result))
    {
        LOG("SendCommond failed");
        return false;
    }

    std::vector<std::string> lineStringList = splitStringByLine(result);
    moduleInfoList.clear();
    moduleInfoList.reserve(lineStringList.size());
    for (const std::string& lineString : lineStringList)
    {
        size_t pos_1 = lineString.find(' ', 0);
        if (std::string::npos == pos_1)
        {
            continue;
        }
        std::string strBaseAddress = lineString.substr(0, pos_1);
        PVOID baseAddress = (PVOID)std::stoull(strBaseAddress.c_str(), nullptr, 16);

        size_t pos_2 = lineString.find(' ', pos_1 + 1);
        if (std::string::npos == pos_2)
        {
            continue;
        }
        std::string strEndAddress = lineString.substr(pos_1 + 1, pos_2 - pos_1 - 1);
        PVOID endAddress = (PVOID)std::stoull(strEndAddress.c_str(), nullptr, 16);

        size_t pos_3 = lineString.find(' ', pos_2 + 1);
        if (std::string::npos == pos_3)
        {
            continue;
        }
        std::string moduleName = lineString.substr(pos_2 + 1, pos_3 - pos_2 - 1);

        std::string modulePath = lineString.substr(pos_3 + 1);

        ModuleInfo moduleInfo;
        moduleInfo.baseAddress = baseAddress;
        moduleInfo.endAddress = endAddress;
        moduleInfo.moduleName = moduleName;
        moduleInfo.modulePath = modulePath;
        moduleInfoList.emplace_back(moduleInfo);
    }

    return true;
}

bool CdbHelper::InitCdb(std::wstring dump_path)
{
    HANDLE hChildStd_IN_Rd = NULL;
    HANDLE hChildStd_OUT_Wr = NULL;
    HANDLE hChildStd_ERR_Wr = NULL;

    std::string outputBuffer = "";
    CHAR tempOutputBuffer[1024];
    DWORD tempBytesRead = 0;

    // 设置安全属性，以便子进程可以继承句柄
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES) };
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    // 创建管道用于标准输入
    if (!CreatePipe(&hChildStd_IN_Rd, &hChildStd_IN_Wr_, &sa, 0))
    {
        LOG("StdIn Rd CreatePipe\n");
        return false;
    }

    // 确保写句柄在子进程中不可见
    if (!SetHandleInformation(hChildStd_IN_Wr_, HANDLE_FLAG_INHERIT, 0))
    {
        LOG("StdIn SetHandleInformation\n");
        return false;
    }

    // 创建管道用于标准输出
    if (!CreatePipe(&hChildStd_OUT_Rd_, &hChildStd_OUT_Wr, &sa, 0))
    {
        LOG("StdOut Rd CreatePipe\n");
        return false;
    }

    // 确保读句柄在子进程中不可见
    if (!SetHandleInformation(hChildStd_OUT_Rd_, HANDLE_FLAG_INHERIT, 0))
    {
        LOG("StdOut SetHandleInformation\n");
        return false;
    }

    // 创建管道用于标准错误
    if (!CreatePipe(&hChildStd_ERR_Rd_, &hChildStd_ERR_Wr, &sa, 0))
    {
        LOG("StdErr Rd CreatePipe\n");
        return false;
    }

    // 确保读句柄在子进程中不可见
    if (!SetHandleInformation(hChildStd_ERR_Rd_, HANDLE_FLAG_INHERIT, 0))
    {
        LOG("StdErr SetHandleInformation\n");
        return false;
    }

    // 创建进程
    std::wstring app_path = L"C:\\Program Files (x86)\\Windows Kits\\10\\Debuggers\\x64\\cdb.exe";
    std::wstring cmd_line = tool::Format(L"\"%ws\" -z \"%ws\"", app_path.c_str(), dump_path.c_str());
    if (!tool::RunAppWithRedirection(
        app_path.c_str(), cmd_line.c_str(),
        hChildStd_IN_Rd, hChildStd_OUT_Wr, hChildStd_ERR_Wr, &hCdbProcess_))
    {
        LOG("RunAppWithCommand failed");
        return false;
    }

    // 关闭不需要的句柄
    CloseHandle(hChildStd_IN_Rd);
    CloseHandle(hChildStd_OUT_Wr);
    CloseHandle(hChildStd_ERR_Wr);

    // 读数据
    do
    {
        if (!ReadFile(hChildStd_OUT_Rd_, tempOutputBuffer, sizeof(tempOutputBuffer) - 1, &tempBytesRead, NULL))
        {
            break;
        }
        tempOutputBuffer[tempBytesRead] = '\0';
        outputBuffer += tempOutputBuffer;

        size_t pos = outputBuffer.find_last_of('\n');
        if (pos != std::string::npos)
        {
            std::string lastLineBuffer = outputBuffer.substr(pos + 1);
            if (lastLineBuffer.size() > 5 &&
                lastLineBuffer.size() < 15 &&
                ' ' == lastLineBuffer[lastLineBuffer.size() - 1] &&
                '>' == lastLineBuffer[lastLineBuffer.size() - 2])
            {
                break;
            }
        }
    } while (true);

    isInit_ = true;

    return true;
}

bool CdbHelper::FinishCdb()
{
    if (!isInit_)
    {
        return true;
    }

    std::string command = "q\n";
    if (!WriteFile(hChildStd_IN_Wr_, command.data(), command.size(), NULL, NULL))
    {
        LOG("WriteFile failed");
        return false;
    }

    if (hChildStd_IN_Wr_)
    {
        CloseHandle(hChildStd_IN_Wr_);
        hChildStd_IN_Wr_ = NULL;
    }
    if (hChildStd_OUT_Rd_)
    {
        CloseHandle(hChildStd_OUT_Rd_);
        hChildStd_OUT_Rd_ = NULL;
    }
    if (hChildStd_ERR_Rd_)
    {
        CloseHandle(hChildStd_ERR_Rd_);
        hChildStd_ERR_Rd_ = NULL;
    }

    if (hCdbProcess_)
    {
        CloseHandle(hCdbProcess_);
        hCdbProcess_ = NULL;
    }

    isInit_ = false;

    return true;
}
