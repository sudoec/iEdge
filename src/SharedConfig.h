#pragma data_seg(".SHARED")
#include <map>
#include <string>
#include <fstream>
#include <codecvt>

bool EnableSetAppId = false;
bool BlockSingleAlt = true;
bool NewTabHomePage = false;
bool OpenUrlNewTab = false;
bool RightTabSwitch = true;
bool HoverActivateTab = false;
int HoverTime = HOVER_DEFAULT;

bool MouseGesture = true;
bool MouseGestureTrack = true;
bool MouseGestureAction = false;
int MouseGestureSize = 5;
TCHAR MouseGestureColor[MAX_PATH] = L"337AB7";
std::map<std::wstring, std::pair<std::wstring, std::wstring>> MouseGestureMap = {
    {L"↑", std::make_pair(L"打开主页", L"Ctrl+T")},
    {L"↓", std::make_pair(L"页面顶部", L"Home")},
    {L"←", std::make_pair(L"后退", L"Alt+←")},
    {L"→", std::make_pair(L"前进", L"Alt+→")},
    {L"↑↓", std::make_pair(L"刷新", L"F5")},
    {L"↓↑", std::make_pair(L"强制刷新", L"Ctrl+F5")},
    {L"↓→", std::make_pair(L"关闭标签", L"Ctrl+W")},
    {L"↓←", std::make_pair(L"撤销关闭", L"Ctrl+Shift+T")},
    {L"→↑", std::make_pair(L"上翻页", L"PageUp")},
    {L"→↓", std::make_pair(L"下翻页", L"PageDown")},
    {L"↑←", std::make_pair(L"切换到右侧标签", L"Ctrl+PageUp")},
    {L"↑→", std::make_pair(L"切换到左侧标签", L"Ctrl+PageDown")}
};


#pragma data_seg()
#pragma comment(linker, "/section:.SHARED,RWS")

void ReadConfig(const wchar_t* iniPath)
{
    EnableSetAppId = GetPrivateProfileInt(L"界面增强", L"快捷方式", 0, iniPath) == 1;
    BlockSingleAlt = GetPrivateProfileInt(L"界面增强", L"屏蔽转换键", 1, iniPath) == 1;
    NewTabHomePage = GetPrivateProfileInt(L"界面增强", L"新标签打开主页", 0, iniPath) == 1;
    OpenUrlNewTab = GetPrivateProfileInt(L"界面增强", L"新标签打开网址", 0, iniPath) == 1;
    RightTabSwitch = GetPrivateProfileInt(L"界面增强", L"右键快速标签切换", 1, iniPath) == 1;
    HoverActivateTab = GetPrivateProfileInt(L"界面增强", L"悬停激活标签页", 0, iniPath) == 1;
    HoverTime = GetPrivateProfileInt(L"界面增强", L"悬停时间", HOVER_DEFAULT, iniPath);

    MouseGesture = GetPrivateProfileInt(L"鼠标手势", L"启用", 1, iniPath) == 1;
    MouseGestureSize = GetPrivateProfileInt(L"鼠标手势", L"轨迹粗细", 5, iniPath);
    MouseGestureTrack = GetPrivateProfileInt(L"鼠标手势", L"轨迹", 1, iniPath) == 1;
    MouseGestureAction = GetPrivateProfileInt(L"鼠标手势", L"动作", 0, iniPath) == 1;
    GetPrivateProfileString(L"鼠标手势", L"轨迹颜色", L"337AB7", MouseGestureColor, MAX_PATH, iniPath);

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

        }

    }
    infile.close();

}
