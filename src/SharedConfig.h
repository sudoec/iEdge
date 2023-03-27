#pragma data_seg(".SHARED")
#include <map>
#include <string>
#include <fstream>
#include <codecvt>

bool EnableSetAppId = false;
bool BlockSingleAlt = true;
bool NewTabHomePage = false;
bool OpenUrlNewTab = false;
bool RightTabSwitch = false;
bool HoverActivateTab = false;
int HoverTime = HOVER_DEFAULT;

bool MouseGesture = false;
bool MouseGestureTrack = false;
bool MouseGestureAction = false;
int MouseGestureSize = 3;
TCHAR MouseGestureColor[MAX_PATH] = L"98CC00";
std::map<std::wstring, std::pair<std::wstring, std::wstring>> MouseGestureMap;
std::string HomePage;

#pragma data_seg()
#pragma comment(linker, "/section:.SHARED,RWS")

void ReadConfig(const wchar_t *iniPath)
{
    EnableSetAppId = GetPrivateProfileInt(L"界面增强", L"快捷方式", 0, iniPath) == 1;
    BlockSingleAlt = GetPrivateProfileInt(L"界面增强", L"屏蔽转换键", 1, iniPath) == 1;
    NewTabHomePage = GetPrivateProfileInt(L"界面增强", L"新标签打开主页", 0, iniPath) == 1;
    OpenUrlNewTab = GetPrivateProfileInt(L"界面增强", L"新标签打开网址", 0, iniPath) == 1;
    RightTabSwitch = GetPrivateProfileInt(L"界面增强", L"右键快速标签切换", 1, iniPath) == 1;
    HoverActivateTab = GetPrivateProfileInt(L"界面增强", L"悬停激活标签页", 0, iniPath) == 1;
    HoverTime = GetPrivateProfileInt(L"界面增强", L"悬停时间", HOVER_DEFAULT, iniPath);

    MouseGesture = GetPrivateProfileInt(L"鼠标手势", L"启用", 1, iniPath) == 1;
    MouseGestureSize = GetPrivateProfileInt(L"鼠标手势", L"轨迹粗细", 3, iniPath);
    MouseGestureTrack = GetPrivateProfileInt(L"鼠标手势", L"轨迹", 1, iniPath) == 1;
    MouseGestureAction = GetPrivateProfileInt(L"鼠标手势", L"动作", 1, iniPath) == 1;
    GetPrivateProfileString(L"鼠标手势", L"轨迹颜色", L"98CC00", MouseGestureColor, MAX_PATH, iniPath);

    std::wifstream infile(iniPath, std::ios::binary);
    infile.imbue(std::locale(infile.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::consume_header>));
    std::wstring line;
    while (std::getline(infile, line))
    {
        if (line[0] == L'↑' || line[0] == L'↓' || line[0] == L'←' || line[0] == L'→')
        {
            for (int i = line.length() - 1; i >= 0; i--)
                if (line[i] == 10 || line[i] == 13) line.erase(i);

            MouseGestureMap[line.substr(0, line.find(L"="))] = std::make_pair(line.substr(line.find(L"=") + 1, line.find(L"|") - line.find(L"=") - 1), line.substr(line.find(L"|") + 1));


            //OutputDebugString(line.substr(0, line.find(L"=")).c_str());
            //OutputDebugString(line.substr(line.find(L"=") + 1, line.find(L"|") - line.find(L"=") - 1).c_str());
            //OutputDebugString(line.substr(line.find(L"|") + 1).c_str());
        }
            
    }
    infile.close();

    //MessageBox(NULL, std::to_wstring(MouseGesture).c_str(), L"", MB_OK);
    
}

std::wstring CheckArgs(const wchar_t* iniPath)
{
    wchar_t Params[2048];
    GetPrivateProfileSection(L"启动参数", Params, 2048, iniPath);
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
