# iEdge

超好用的Edge浏览器增强软件

## 简介

* iEdge具有基础的标签页增强，追加Chrome运行参数，启动、关闭时运行程序，鼠标手势，支持特殊页面使用，支持任意形状，支持自定义功能，灵敏度可调节，轨迹颜色粗细均可设置，便携化绿色版。  

* 具体功能可打开iEdge.ini进行配置。

## 安装

**更新：新增原生Chrome扩展与DLL Hook结合的方式运行，扩展实现了超级拖拽(简易版)，标签页顺序管理(新标签打开Google，新标签在当前右侧，关闭当前标签跳转到左侧标签)，任意鼠标手势**

本软件并非第三方独立浏览器，首先把iEdge.exe、iEdge.dll、iEdge.ini(没有可以自动生成)和浏览器版本文件夹(如：96.0.1054.43)放到同一个目录下，启动Edge.exe，安装iEdge Helper扩展即可使用。  

本软件默认为便携版设计，默会保存数据到当前目录(./User.xxxx)。升级简单，直接替换Edge版本文件夹即可。  

数据目录支持固定名称和动态生成，如在目录名后加"."就会额外生成四个数字，方便区分。  

**目录结构**

+-- iEdge  
|   +-- xx.x.xxx.xx  
|   +-- User.xxxx  
|   +-- iEdge.exe  
|   +-- iEdge.dll  
|   +-- iEdge.ini  

**鼠标手势**

↑=打开主页|Alt+T  
↓=页面底部|End  
←=后退|Alt+←  
→=前进|Alt+→  
↓↑=刷新|F5  
↑↓=强制刷新|Ctrl+F5  
↓→=关闭标签|Alt+W  
↓←=撤销关闭|Ctrl+Shift+T  
→↑=上翻页|PageUp  
→↓=下翻页|PageDown  
↑←=切换到右侧标签|Ctrl+PageUp  
↑→=切换到左侧标签|Ctrl+PageDown  

## 免责申明

**本软件是免费软件，不收一分钱，只是分享给朋友们使用，不负责售后服务。使用/更新前务必备份用户数据，软件造成任何数据丢失，本人概不负责。**

## 鸣谢

https://shuax.com/  
https://github.com/shuax/GreenChrome  
https://github.com/zzm-note/SuperDrag  
