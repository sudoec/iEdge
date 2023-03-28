#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <io.h>
#include <Windows.h>
#include <winternl.h>
#include <TlHelp32.h>


using namespace std;

static bool transparent = false;

typedef LONG(WINAPI* PROCNTQSIP)(HANDLE, UINT, PVOID, ULONG, PULONG);

typedef NTSTATUS(NTAPI* pfnNtCreateThreadEx)(
    OUT PHANDLE hThread,
    IN ACCESS_MASK DesiredAccess,
    IN PVOID ObjectAttributes,
    IN HANDLE ProcessHandle,
    IN PVOID lpStartAddress,
    IN PVOID lpParameter,
    IN ULONG Flags,
    IN SIZE_T StackZeroBits,
    IN SIZE_T SizeOfStackCommit,
    IN SIZE_T SizeOfStackReserve,
    OUT PVOID lpBytesBuffer);


//使用NtCreateThreadEx在目标进程创建线程实现注入
BOOL _Caesar_(HANDLE ProcessHandle, LPWSTR DllFullPath)
{
    if (transparent)
        return TRUE;
    // 在对方进程空间申请内存,存储Dll完整路径
    //UINT32	DllFullPathLength = (strlen(DllFullPath) + 1);
    size_t  DllFullPathLength = (wcslen(DllFullPath) + 1) * 2;
    PVOID   DllFullPathBufferData = VirtualAllocEx(ProcessHandle, NULL, DllFullPathLength, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (DllFullPathBufferData == NULL)
        return FALSE;

    // 将DllFullPath写进刚刚申请的内存中
    SIZE_T  ReturnLength;
    BOOL bOk = WriteProcessMemory(ProcessHandle, DllFullPathBufferData, DllFullPath, DllFullPathLength, &ReturnLength);

    LPTHREAD_START_ROUTINE	LoadLibraryAddress = NULL;
    HMODULE Kernel32Module = GetModuleHandle(L"Kernel32");

    char funcA_str[] = { 'L', 'o', 'a', 'd','L','i','b','r','a','r','y','W','\0' };
    LoadLibraryAddress = (LPTHREAD_START_ROUTINE)GetProcAddress(Kernel32Module, funcA_str);

    char funcB_str[] = { 'N', 't', 'C', 'r','e','a','t','e','T','h','r','e', 'a','d','E','x','\0' };
    pfnNtCreateThreadEx NtCreateThreadEx = (pfnNtCreateThreadEx)GetProcAddress(GetModuleHandle(L"ntdll.dll"), funcB_str);
    if (NtCreateThreadEx == NULL)
        return FALSE;

    HANDLE ThreadHandle = NULL;
    // 0x1FFFFF #define THREAD_ALL_ACCESS         (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xFFFF)
    NtCreateThreadEx(&ThreadHandle, 0x1FFFFF, NULL, ProcessHandle, (LPTHREAD_START_ROUTINE)LoadLibraryAddress, DllFullPathBufferData, FALSE, NULL, NULL, NULL, NULL);
    if (ThreadHandle == NULL)
        return FALSE;

    if (WaitForSingleObject(ThreadHandle, INFINITE) == WAIT_FAILED)
        return FALSE;

    if (DllFullPathBufferData != NULL)
        VirtualFreeEx(ProcessHandle, DllFullPathBufferData, 0, MEM_RELEASE);

    if (ThreadHandle != NULL)
        CloseHandle(ThreadHandle);

    return TRUE;
}

BOOL GetProcessCmdline(HANDLE procesHandle, wstring& cmdline)
{
    PROCESS_BASIC_INFORMATION pbi = { 0 };
    PEB                       peb;
    //PROCESS_PARAMETERS        ProcParam;
    DWORD64                     dwDummy;
    DWORD64                     dwSize;
    LPVOID                    lpAddress;
    RTL_USER_PROCESS_PARAMETERS para;

    PROCNTQSIP NtQueryInformationProcess = (PROCNTQSIP)GetProcAddress(GetModuleHandleW(L"ntdll"), "NtQueryInformationProcess");
    //获取信息
    if (0 != NtQueryInformationProcess(procesHandle, ProcessBasicInformation, (PVOID)&pbi, sizeof(pbi), NULL))
    {
        return false;
    }
    if (pbi.PebBaseAddress == nullptr)
    {
        //do somthing 
    }
    if (FALSE == ReadProcessMemory(procesHandle, pbi.PebBaseAddress, &peb, sizeof(peb), &dwDummy))
    {
        return false;
    }
    if (FALSE == ReadProcessMemory(procesHandle, peb.ProcessParameters, &para, sizeof(para), &dwDummy))
    {
        return false;
    }

    lpAddress = para.CommandLine.Buffer;
    dwSize = para.CommandLine.Length;

    TCHAR* cmdLineBuffer = new TCHAR[dwSize + 1];
    ZeroMemory(cmdLineBuffer, (dwSize + 1) * sizeof(WCHAR));
    if (FALSE == ReadProcessMemory(procesHandle, lpAddress, (LPVOID)cmdLineBuffer, dwSize, &dwDummy))
    {
        delete[] cmdLineBuffer;
        return false;
    }
    cmdline = cmdLineBuffer;

    delete[] cmdLineBuffer;
    return true;
}

BOOL TraverseEdge(wstring cmdStart)
{
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(pe32);

    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return false;
    }

    BOOL bResult = Process32First(hProcessSnap, &pe32);

    while (bResult)
    {
        if (wcscmp(pe32.szExeFile, L"msedge.exe") == 0 || wcscmp(pe32.szExeFile, L"chrome.exe") == 0)
        {
            std::wstring name = pe32.szExeFile;
            int id = pe32.th32ProcessID;

            auto processHaldle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);
            if (nullptr == processHaldle)
            {
                bResult = Process32Next(hProcessSnap, &pe32);
                continue;
            }

            wstring Cmdline;
            if (GetProcessCmdline(processHaldle, Cmdline))
            {
                if (Cmdline.find(cmdStart) < Cmdline.length())
                    return true;
            }

            if (nullptr != processHaldle)
                CloseHandle(processHaldle);
        }

        bResult = Process32Next(hProcessSnap, &pe32);
    }

    CloseHandle(hProcessSnap);

    return false;
}

wstring CombineCmdLine(wstring CmdA, wstring CmdB)
{
    //WCHAR buffer[2048];
    //swprintf_s(buffer, 2048, L"[%s] %llu\n[%s] %llu\n", &CmdA[0], CmdA.length(), &CmdB[0], CmdB.length());
    //OutputDebugString(buffer);

    vector<wstring> SubA, SubB;
    for (int i = 0; i < CmdA.length(); i++)
    {
        static wstring cmd;
        static bool quote = false;
        if (CmdA[i] == L'"')
        {
            quote = !quote;
        }
        if (!quote && !cmd.empty() && (CmdA[i] == L' ' || CmdA.length() == i + 1))
        {
            if (CmdA.length() == i + 1)
                cmd.push_back(CmdA[i]);
            SubA.push_back(cmd);
            cmd.clear();
        }
        if (quote || CmdA[i] != L' ')
        {
            cmd.push_back(CmdA[i]);
        }
    }
    for (int i = 0; i < CmdB.length(); i++)
    {
        static wstring cmd;
        static bool quote = false;
        if (CmdB[i] == L'"')
        {
            quote = !quote;
        }
        if (!quote && !cmd.empty() && (CmdB[i] == L' ' || CmdB.length() == i + 1))
        {
            if (CmdB.length() == i + 1)
                cmd.push_back(CmdB[i]);
            SubB.push_back(cmd);
            cmd.clear();
        }
        if (quote || CmdB[i] != L' ')
        {
            cmd.push_back(CmdB[i]);
        }
    }
    
    for (int i = 0; i < SubA.size(); i++)
    {
        for (int j = 0; j < SubB.size(); j++)
        {
            if (SubA[i].find(L"--user-data-dir=") < SubA[i].length() && SubB[j].find(L"--user-data-dir=") < SubB[j].length())
            {
                SubA[i] = SubB[j];
                break;
            }
        }

    }

    for (int i = 0; i < SubB.size(); i++)
    {
        bool dup = false;
        for (int j = 0; j < SubA.size(); j++)
        {
            if (SubA[j] == SubB[i])
            {
                dup = true;
            }
        }
        if (!dup)
            SubA.push_back(SubB[i]);
    }

    wstring Combine;
    for (int i = 0; i < SubA.size(); i++)
    {
        Combine.append(SubA[i]);
        Combine.append(L" ");
    }
    Combine.pop_back();

    return Combine;
}

wstring GetAppFolder(LPWSTR Dir)
{
    //文件句柄
    //注意：我发现有些文章代码此处是long类型，实测运行中会报错访问异常
    intptr_t hFile = 0;
    //文件信息
    struct _wfinddata_t fileinfo;
    wstring p;
    if ((hFile = _wfindfirst(p.assign(Dir).append(L"\\*").c_str(), &fileinfo)) != -1)
    {
        do
        {
            if ((fileinfo.attrib & _A_SUBDIR))
            {
                wstring filename = fileinfo.name;
                if (count(filename.begin(), filename.end(), '.') == 3)
                    return filename;
            }
        } while (_wfindnext(hFile, &fileinfo) == 0);
        _findclose(hFile);
    }
    return wstring(L"");
}

wstring GetDataFolder(LPWSTR Dir)
{
    //文件句柄
    //注意：我发现有些文章代码此处是long类型，实测运行中会报错访问异常
    intptr_t hFile = 0;
    //文件信息
    struct _wfinddata_t fileinfo;
    wstring p;
    if ((hFile = _wfindfirst(p.assign(Dir).append(L"\\*").c_str(), &fileinfo)) != -1)
    {
        do
        {
            if ((fileinfo.attrib & _A_SUBDIR))
            {
                wstring filename = fileinfo.name;
                if (filename.length() > 1 && count(filename.begin(), filename.end(), '.') == 1)
                    return filename;
            }
        } while (_wfindnext(hFile, &fileinfo) == 0);
        _findclose(hFile);
    }
    return wstring(L"");
}

void plog(const std::string str)
{
    char time[4096];
    SYSTEMTIME sys;
    GetLocalTime(&sys);
    sprintf_s(time, "[%4d/%02d/%02d %02d:%02d:%02d.%03d]\n",
        sys.wYear, sys.wMonth, sys.wDay, sys.wHour, sys.wMinute,
        sys.wSecond, sys.wMilliseconds);
    std::ofstream	OsWrite("plogs.txt", std::ofstream::app);
    OsWrite << time;
    OsWrite << str;
    OsWrite << std::endl;
    OsWrite.close();
}


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{

    LPWSTR sWinDir = new TCHAR[MAX_PATH];
    GetModuleFileNameW(NULL, sWinDir, MAX_PATH);
    (wcsrchr(sWinDir, '\\'))[1] = 0;

    wstring sConLin = wstring(sWinDir) + GetAppFolder(sWinDir) + L"\\msedge.exe";
    wstring sDllPath = wstring(sWinDir) + L"iEdge.dll";
    wstring lpConLin = wstring(lpCmdLine);

    DWORD dwAttrib = GetFileAttributes(sConLin.c_str());
    if(INVALID_FILE_ATTRIBUTES == dwAttrib || 0 != (dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
        sConLin = wstring(sWinDir) + GetAppFolder(sWinDir) + L"\\chrome.exe";

    wchar_t Params[2048];
    GetPrivateProfileSectionW(L"启动参数", Params, 2048, wstring(sWinDir).append(L"iEdge.ini").c_str());
    wstring sParams = Params;
    if (sParams.empty())
        sParams = L"--user-data-dir=\"..\\USER.\" --disable-background-networking --disable-backing-store-limit";

    size_t sIndex = sParams.find(L"--user-data-dir=") + 16;
    wstring dPath = sParams.substr(sIndex, sParams.find(L'"', sIndex + 1) - 15);
    if (sIndex < sParams.length() && dPath[dPath.length()-2] == L'.')
    {
        std::hash<std::wstring> hasher;
        size_t hWinDir = hasher(sWinDir);
        wstring hashStr = to_wstring(hWinDir).substr(0, 4);
        sParams.insert(sIndex + dPath.length() - 1, hashStr);
        dPath.insert(dPath.length() - 1, hashStr);

        wstring sData = wstring(sWinDir) + GetDataFolder(sWinDir);
        wstring nData = wstring(sWinDir) + dPath.substr(dPath.rfind('\\') + 1, dPath.length() - dPath.rfind('\\') - 2);
        
        if (sData != nData)
            _wrename(sData.c_str(), nData.c_str());
    }

    sConLin.append(L" ").append(CombineCmdLine(sParams, lpCmdLine));

    if (TraverseEdge(dPath))
        transparent = true;

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    //创建一个新进程  
    if (CreateProcessW(
        NULL,   //  指向一个NULL结尾的、用来指定可执行模块的宽字节字符串  
        &sConLin[0], // 命令行字符串  
        NULL, //    指向一个SECURITY_ATTRIBUTES结构体，这个结构体决定是否返回的句柄可以被子进程继承。  
        NULL, //    如果lpProcessAttributes参数为空（NULL），那么句柄不能被继承。<同上>  
        false,//    指示新进程是否从调用进程处继承了句柄。   
        CREATE_SUSPENDED,  //  指定附加的、用来控制优先类和进程的创建的标  
            //  CREATE_NEW_CONSOLE  新控制台打开子进程  
            //  CREATE_SUSPENDED    子进程创建后挂起，直到调用ResumeThread函数  
        NULL, //    指向一个新进程的环境块。如果此参数为空，新进程使用调用进程的环境  
        NULL, //    指定子进程的工作路径  
        &si, // 决定新进程的主窗体如何显示的STARTUPINFO结构体  
        &pi  // 接收新进程的识别信息的PROCESS_INFORMATION结构体  
    ))
    {

        _Caesar_(pi.hProcess, &sDllPath[0]);

        ResumeThread(pi.hThread);

        //下面两行关闭句柄，解除本进程和新进程的关系，不然有可能不小心调用TerminateProcess函数关掉子进程
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }

    //终止子进程
    //TerminateProcess(pi.hProcess, 300);

    //终止本进程，状态码
    //ExitProcess(1001);

    return 0;
}