#include "iEdge.h"
#include <fstream>
#include <string>
#include <vector>
#include <tlhelp32.h>


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

typedef LSTATUS(WINAPI* REGSETVALUEEXW)(HKEY hKey, LPCWSTR lpValueName, DWORD Reserved, DWORD dwType, CONST BYTE* lpData, DWORD cbData);
REGSETVALUEEXW fpRegSetValueExW = NULL;
LSTATUS WINAPI DetourRegSetValueExW(HKEY hKey, LPCWSTR lpValueName, DWORD Reserved, DWORD dwType, CONST BYTE* lpData, DWORD cbData)
{
    WCHAR* lpStr = new WCHAR[cbData + 1];
    memcpy(lpStr, lpData, cbData);
    lpStr[cbData] = 0;
    
    std::wstring npString;
    std::wstring lpString(lpStr);
    if (lpString.find(L"\\msedge.exe") < lpString.length())
    {
        npString = lpString.substr(0, lpString.rfind(L"\\", lpString.rfind(L"\\") - 1) + 1) + L"iEdge.exe" + lpString.substr(lpString.rfind(L"\\") + 11);
    }
    delete[] lpStr;

    return fpRegSetValueExW(hKey, lpValueName, Reserved, dwType, (BYTE*)&npString[0], 2*(npString.length()+1));
}

void iEdge()
{
    // 配置设置
    ConfigIni();

    // 快捷方式
    SetAppId();

    // 打造便携版
    MakePortable();

    // 修补资源文件
    //PakPatch();

    // 标签页，书签，地址栏增强
    std::thread thook(StartWindowsHook);
    thook.detach();
}

BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID pv)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        hInstance = hModule;
        //DisableThreadLibraryCalls(hModule);

        // 保持系统dll原有功能
        //FixLibraryImport(hModule);

        // 初始化HOOK库成功以后安装加载器
        MH_STATUS status = MH_Initialize();
        if (status == MH_OK)
        {
            MH_CreateHook(&GetKeyState, &DetourGetKeyState, reinterpret_cast<LPVOID*>(&fpGetKeyState));
            MH_EnableHook(&GetKeyState);

            MH_CreateHook(&RegSetValueExW, &DetourRegSetValueExW, reinterpret_cast<LPVOID*>(&fpRegSetValueExW));
            MH_EnableHook(&RegSetValueExW);

            iEdge();

        }

    }
    return TRUE;
}
