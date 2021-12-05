// 发送按键
class SendKeys
{
public:
    template<typename ... T>
    SendKeys(T ... keys)
    {
        std::vector <int> keys_ = { keys ... };
        for (auto & key : keys_ )
        {
            INPUT input = { 0 };
            input.type = INPUT_KEYBOARD;
            input.ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
            input.ki.wVk = key;

            // 修正鼠标消息
            switch (key)
            {
            case VK_MBUTTON:
                input.type = INPUT_MOUSE;
                input.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
                break;
            }

            inputs_.push_back(input);
        }

        SendInput((UINT)inputs_.size(), &inputs_[0], sizeof(INPUT));
    }
    ~SendKeys()
    {
        for (auto & input : inputs_)
        {
            input.ki.dwFlags |= KEYEVENTF_KEYUP;

            // 修正鼠标消息
            switch (input.ki.wVk)
            {
            case VK_MBUTTON:
                input.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
                break;
            }
        }

        SendInput((UINT)inputs_.size(), &inputs_[0], sizeof(INPUT));
    }
private:
    std::vector <INPUT> inputs_;
};

void OpenHomePage1()
{
    Sleep(20);
    SendKey(std::wstring(L"Alt+D"));
    KEY_DISABLE = TRUE;
    SendMessage(GetForegroundWindow(), WM_KEYDOWN, VK_BACK, 0);
    SendMessage(GetForegroundWindow(), WM_KEYUP, VK_BACK, 0);
    for (int i = 0; i < HomePage.length(); i++)
    {
        PostMessage(GetForegroundWindow(), WM_CHAR, HomePage[i], 0);
    }
    Sleep(120);
    KEY_DISABLE = FALSE;
    SendKey(std::wstring(L"Alt+Enter"));

}

void OpenHomePage()
{
    SendKey(std::wstring(L"Ctrl+T"));
    //KEY_DISABLE = TRUE;
    //SendMessage(GetForegroundWindow(), WM_KEYDOWN, VK_TAB, 0);
    //SendMessage(GetForegroundWindow(), WM_KEYUP, VK_TAB, 0);
    //KEY_DISABLE = FALSE;
    //SendKey(std::wstring(L"Alt+Home"));

}


/*
chrome ui tree


browser/ui/views/frame/BrowserRootView
ui/views/window/NonClientView
BrowserView
    TopContainerView
        TabStrip
            Tab
                TabCloseButton
            ImageButton
        ToolbarView
            LocationBarView
                OmniboxViewViews
    BookmarkBarView
        BookmarkButton
*/


std::map <HWND, bool> tracking_hwnd;
bool ignore_mouse_event = false;

HHOOK mouse_hook = NULL;
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{

    static bool wheel_tab_ing = false;

    if (nCode==HC_ACTION)
    {
        PMOUSEHOOKSTRUCT pmouse = (PMOUSEHOOKSTRUCT) lParam;

        if (wParam == WM_RBUTTONUP && wheel_tab_ing)
        {
            wheel_tab_ing = false;
            if (MouseGesture)
            {
                gesture_mgr.OnRButtonUp(pmouse, true);
            }
            return 1;
        }

        // 处理鼠标手势
        if(MouseGesture)
        {
            bool handled = false;

            if (wParam == WM_RBUTTONDOWN || wParam == WM_NCRBUTTONDOWN)
            {
                handled = gesture_mgr.OnRButtonDown(pmouse);
            }
            if (wParam == WM_RBUTTONUP || wParam == WM_NCRBUTTONUP)
            {
                handled = gesture_mgr.OnRButtonUp(pmouse);
            }
            if (wParam == WM_MOUSEMOVE || wParam == WM_NCMOUSEMOVE)
            {
                handled = gesture_mgr.OnMouseMove(pmouse);
            }

            if(handled)
            {
                return 1;
            }
        }

        if (RightTabSwitch)
        {
            if (wParam == WM_RBUTTONDOWN)
            {
                if (!ignore_mouse_event)
                {
                    return 1;
                }
            }
            if (wParam == WM_RBUTTONUP)
            {
                if (!ignore_mouse_event)
                {
                    ignore_mouse_event = true;
                    SendOneMouse(MOUSEEVENTF_RIGHTDOWN);
                    SendOneMouse(MOUSEEVENTF_RIGHTUP);
                    return 1;
                }
                ignore_mouse_event = false;
            }
        }

        if (HoverActivateTab && ignore_mouse_event)
        {
            if (wParam == WM_LBUTTONDBLCLK)
            {
                ignore_mouse_event = false;
                return 1;
            }
        }

        if (HoverActivateTab && wParam == WM_MOUSEMOVE)
        {
            HWND hwnd = WindowFromPoint(pmouse->pt);
            if (tracking_hwnd.find(hwnd) == tracking_hwnd.end())
            {
                TRACKMOUSEEVENT MouseEvent;
                MouseEvent.cbSize = sizeof(TRACKMOUSEEVENT);
                MouseEvent.dwFlags = TME_HOVER | TME_LEAVE;
                MouseEvent.hwndTrack = hwnd;
                MouseEvent.dwHoverTime = HoverTime;
                if (::TrackMouseEvent(&MouseEvent))
                {
                    tracking_hwnd[hwnd] = true;
                }
            }
        }

        // 忽略鼠标移动消息
        if(wParam==WM_MOUSEMOVE || wParam==WM_NCMOUSEMOVE)
        {
            return CallNextHookEx(mouse_hook, nCode, wParam, lParam );;
        }

        if(wParam==WM_MOUSEWHEEL)
        {
            PMOUSEHOOKSTRUCTEX pwheel = (PMOUSEHOOKSTRUCTEX) lParam;
            int zDelta = GET_WHEEL_DELTA_WPARAM(pwheel->mouseData);
            if( RightTabSwitch && IsPressed(VK_RBUTTON) )
            {
                // 切换标签页，发送ctrl+pagedown/pageup
                SendKeys(VK_CONTROL, zDelta>0 ? VK_PRIOR : VK_NEXT);

                wheel_tab_ing = true;
                gesture_mgr.OnRButtonUp(pmouse, true);
                return 1;
            }
            //else if (HoverTabSwitch)//加一个条件“鼠标悬停在标签栏上”
            //{
            //    // 切换标签页，发送ctrl+pagedown/pageup
            //    SendKeys(VK_CONTROL, zDelta > 0 ? VK_PRIOR : VK_NEXT);
            //}
        }

    }

    return CallNextHookEx(mouse_hook, nCode, wParam, lParam );
}


HHOOK keyboard_hook = NULL;
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    static bool open_url_ing = false;
    static bool close_tab_ing = false;
    if (nCode==HC_ACTION && !(lParam & 0x80000000)) // pressed
    {
        if(wParam==VK_RETURN && OpenUrlNewTab)
        {
            if( !IsPressed(VK_MENU) && open_url_ing)
            {
                // 在新标签打开url，发送alt+enter
                open_url_ing = false;
                SendKeys(VK_MENU, VK_RETURN);
                return 1;
            }
        }

        //if (wParam == 'Z')
        //{
        //    RECT rect;
        //    HWND hq = GetForegroundWindow();
        //    GetWindowRect(hq, &rect);
        //    int window_width = (rect.right - rect.left);
        //    int window_height = (rect.bottom - rect.top);
        //    MyDebugView(std::to_string(rect.left) + " " + std::to_string(rect.top) + " " + std::to_string(window_width) + " " + std::to_string(window_height));
        //    return CallNextHookEx(keyboard_hook, nCode, wParam, lParam);
        //}

        if (wParam == 'T' && IsPressed(VK_CONTROL))
        {
            std::thread th(OpenHomePage);
            th.detach();
            return 1;
        }

        if (wParam == 'W' && close_tab_ing)
        {
            close_tab_ing = false;
            return CallNextHookEx(keyboard_hook, nCode, wParam, lParam);
        }

    }
    return CallNextHookEx(keyboard_hook, nCode, wParam, lParam );
}

void SendClick()
{
    ignore_mouse_event = true;
    SendOneMouse(MOUSEEVENTF_LEFTDOWN);
    SendOneMouse(MOUSEEVENTF_LEFTUP);
    std::thread th([]() {
        Sleep(500);
        ignore_mouse_event = false;
    });
    th.detach();
}

HHOOK message_hook = NULL;
LRESULT CALLBACK MessageProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        MSG* msg = (MSG*)lParam;
        if (msg->message == WM_MOUSEHOVER)
        {
            tracking_hwnd.erase(msg->hwnd);
        }
        else if (msg->message == WM_MOUSELEAVE)
        {
            tracking_hwnd.erase(msg->hwnd);
        }
    }
    return CallNextHookEx(message_hook, nCode, wParam, lParam);
}

HWND GetWindowHwndByPorcessID(DWORD dwProcessID)
{
    DWORD dwPID = 0;
    HWND hwndRet = NULL;
    // 取得第一个窗口句柄
    HWND hwndWindow = ::GetTopWindow(0);
    while (hwndWindow)
    {
        dwPID = 0;
        // 通过窗口句柄取得进程ID
        DWORD dwTheardID = ::GetWindowThreadProcessId(hwndWindow, &dwPID);
        if (dwTheardID != 0)
        {
            // 判断和参数传入的进程ID是否相等
            if (dwPID == dwProcessID)
            {
                // 进程ID相等，则记录窗口句柄
                hwndRet = hwndWindow;
                break;
            }
        }
        // 取得下一个窗口句柄
        hwndWindow = ::GetNextWindow(hwndWindow, GW_HWNDNEXT);
    }
    // 上面取得的窗口，不一定是最上层的窗口，需要通过GetParent获取最顶层窗口
    HWND hwndWindowParent = NULL;
    // 循环查找父窗口，以便保证返回的句柄是最顶层的窗口句柄
    while (hwndRet != NULL)
    {
        hwndWindowParent = ::GetParent(hwndRet);
        if (hwndWindowParent == NULL)
        {
            break;
        }
        hwndRet = hwndWindowParent;
    }
    // 返回窗口句柄
    return hwndRet;
}

void StartWindowsHook()
{
    HWND hd = NULL;
    while (hd == NULL)
    {
        Sleep(100);
        hd = GetWindowHwndByPorcessID(getpid());
    }
    //Sleep(400);
    mouse_hook = SetWindowsHookEx(WH_MOUSE, MouseProc, hInstance, GetWindowThreadProcessId(hd, nullptr));
    keyboard_hook = SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, hInstance, GetWindowThreadProcessId(hd, nullptr));

    if(MouseGesture)
    {
        std::thread th(StartGestureThread);
        th.detach();
    }

    // 消息循环
    WTL::CMessageLoop msgLoop;
    int ret = msgLoop.Run();
}


void TabBookmark()
{
    if(!wcsstr(GetCommandLineW(), L"--channel"))
    {
        mouse_hook = SetWindowsHookEx(WH_MOUSE, MouseProc, hInstance, GetCurrentThreadId());
        keyboard_hook = SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, hInstance, GetCurrentThreadId());
        //message_hook = SetWindowsHookEx(WH_GETMESSAGE, MessageProc, hInstance, GetCurrentThreadId());

        if(MouseGesture)
        {
            std::thread th(StartGestureThread);
            th.detach();
        }
    }
}

