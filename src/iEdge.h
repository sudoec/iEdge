﻿#ifndef __MAIN_H__
#define __MAIN_H__

#define WINVER 0x600
#define _WIN32_IE 0x600

#include <vector>
#include <string>

#define _MMSYSTEM_H
#define _INC_MMSYSTEM
#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <shlobj.h>
#include <psapi.h>
#include <oleacc.h>
#include <process.h>
#include <thread>

#include <GdiPlus.h>
using namespace Gdiplus;

#pragma warning(disable: 4838)
#pragma warning(disable: 4302)
#include <atlbase.h>
#include <atlapp.h>
#include <atlmisc.h>
#include <atlcrack.h>
#include <atlctrls.h>

// 定义MWORD为机器字长
#include <stdint.h>
#ifdef _WIN64
typedef uint64_t MWORD;
#else
typedef uint32_t MWORD;
#endif

void iEdge();

HMODULE hInstance;

#include "..\version.h"

#include "SharedConfig.h"
#include "version.h"
#include "util.h"

#include "minhook\include\MinHook.h"
#include "mini_gzip\miniz.c"
#include "mini_gzip\mini_gzip.h"
#include "mini_gzip\mini_gzip.c"

#include "gesture\GestureRecognition.h"
#include "gesture\GestureWindow.h"
#include "gesture\GestureMgr.h"
#include "TabBookmark.h"
#include "PakPatch.h"

#include "patchs.h"
#include "Bosskey.h"
#include "Appid.h"

#endif // __MAIN_H__
