﻿#include "iEdge.h"
#include <fstream>
#include <string>


typedef SHORT(WINAPI* GETKEYSTATE)(int vKey);
GETKEYSTATE fpGetKeyState = NULL;
SHORT WINAPI DetourGetKeyState(int vKey)
{
    if (KEY_DISABLE && (vKey == VK_SHIFT || vKey == VK_CONTROL || vKey == VK_MENU || vKey == VK_LWIN))
        return 0;
    if (KEY_SHIFT || KEY_CONTROL || KEY_MENU || KEY_LWIN)
    {
        if (((vKey == VK_SHIFT) && KEY_SHIFT) || ((vKey == VK_CONTROL) && KEY_CONTROL) || ((vKey == VK_MENU) && KEY_MENU) || ((vKey == VK_LWIN) && KEY_LWIN))
            return -128;
        else
            return 0;
    }
    else
        return fpGetKeyState(vKey);
    //SHORT State = fpGetKeyState(vKey);
    //if (((vKey == VK_SHIFT) && KEY_SHIFT) || ((vKey == VK_CONTROL) && KEY_CONTROL) || ((vKey == VK_MENU) && KEY_MENU) || ((vKey == VK_LWIN) && KEY_LWIN))
    //{
    //    if (State >= 0)
    //        State -= 128;
    //}
    //return State;
}


void GreenChrome()
{
    // exe路径
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);

    // exe所在文件夹
    wchar_t exeFolder[MAX_PATH];
    wcscpy(exeFolder, exePath);
    PathRemoveFileSpecW(exeFolder);

    // 生成默认ini文件
    wchar_t iniPath[MAX_PATH];
    ReleaseIni(exeFolder, iniPath);

    // 读取配置
    ReadConfig(iniPath);

    // 参数启动
    CheckArgs(iniPath);

    // 打造便携版chrome
    MakePortable(iniPath);

    // 标签页，书签，地址栏增强
    TabBookmark();
}


BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID pv)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        hInstance = hModule;
        DisableThreadLibraryCalls(hModule);

        // 保持系统dll原有功能
        FixLibraryImport(hModule);

        // 初始化HOOK库成功以后安装加载器
        MH_STATUS status = MH_Initialize();
        if (status == MH_OK)
        {
            MH_CreateHook(&GetKeyState, &DetourGetKeyState, reinterpret_cast<LPVOID*>(&fpGetKeyState));
            MH_EnableHook(&GetKeyState);
            GreenChrome();
        }

    }
    return TRUE;
}
