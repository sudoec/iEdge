#pragma data_seg(".SHARED")

bool DoubleClickCloseTab = false;
bool RightClickCloseTab = false;
bool HoverActivateTab = false;
int HoverTime = HOVER_DEFAULT;
bool KeepLastTab = false;
bool HoverTabSwitch = false;
bool RightTabSwitch = false;
bool BookMarkNewTab = false;
bool OpenUrlNewTab = false;
bool FrontNewTab = false;
bool MouseGesture = false;
std::string HomePage;

#pragma data_seg()
#pragma comment(linker, "/section:.SHARED,RWS")

void GetHomePage(const wchar_t* iniPath)
{
    wchar_t buffer[256];
    GetPrivateProfileStringW(L"��������", L"��ҳ", L"", buffer, 256, iniPath);
    std::wstring wPage(buffer);
    HomePage = std::string(wPage.begin(),wPage.end());
}

void ReadConfig(const wchar_t *iniPath)
{
    DoubleClickCloseTab = GetPrivateProfileInt(L"������ǿ", L"˫���رձ�ǩҳ", 1, iniPath) == 1;
    RightClickCloseTab = GetPrivateProfileInt(L"������ǿ", L"�Ҽ��رձ�ǩҳ", 0, iniPath) == 1;
    HoverActivateTab = GetPrivateProfileInt(L"������ǿ", L"��ͣ�����ǩҳ", 0, iniPath) == 1;
    HoverTime = GetPrivateProfileInt(L"������ǿ", L"��ͣʱ��", HOVER_DEFAULT, iniPath);
    KeepLastTab = GetPrivateProfileInt(L"������ǿ", L"��������ǩ", 1, iniPath) == 1;
    HoverTabSwitch = GetPrivateProfileInt(L"������ǿ", L"��ͣ���ٱ�ǩ�л�", 1, iniPath) == 1;
    RightTabSwitch = GetPrivateProfileInt(L"������ǿ", L"�Ҽ����ٱ�ǩ�л�", 1, iniPath) == 1;
    BookMarkNewTab = GetPrivateProfileInt(L"������ǿ", L"�±�ǩ����ǩ", 1, iniPath) == 1;
    OpenUrlNewTab = GetPrivateProfileInt(L"������ǿ", L"�±�ǩ����ַ", 0, iniPath) == 1;
    FrontNewTab = GetPrivateProfileInt(L"������ǿ", L"ǰ̨���±�ǩ", 1, iniPath) == 1;

    MouseGesture = GetPrivateProfileInt(L"�������", L"����", 1, iniPath) == 1;

    GetHomePage(iniPath);
}

std::wstring CheckArgs(const wchar_t* iniPath)
{
    wchar_t Params[2048];
    GetPrivateProfileSection(L"��������", Params, 2048, iniPath);
    std::wstring Args = GetCommandLineW();
    int index = Args.find(L".exe") + 6;
    std::wstring Path = Args.substr(0, index);
    Args = Args.substr(min(Args.length(), index), std::string::npos);
    if (Args.length() < 5)
    {
        std::wstring exec = Path + Params;
        std::string str(exec.begin(), exec.end());
        WinExec(str.c_str(), SW_SHOW);
        //ShellExecuteW(NULL, L"open", exec.c_str(), L"", L"", SW_HIDE);
        //RunExecute(exec.c_str());
        ExitProcess(0);
    }
    return Args;
}
