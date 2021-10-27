#include "cJSON\cJSON.h"

const char extra_header[] = "Content-Type: text/html; charset=utf-8\r\nAccess-Control-Allow-Origin: http://settings.shuax.com";

void ReadList(cJSON *root, const wchar_t *iniPath, const wchar_t *name)
{
    cJSON *list = cJSON_CreateArray();

    auto contents = GetSection(name, iniPath);
    for (auto &content : contents)
    {
        cJSON_AddItemToArray(list, cJSON_CreateString(utf16to8(content.c_str()).c_str()));
    }

    cJSON_AddItemToObject(root, utf16to8(name).c_str(), list);
}

void ReadValue(cJSON *node, const wchar_t *iniPath, const wchar_t *section, const wchar_t *name)
{
    wchar_t value[256];
    GetPrivateProfileStringW(section, name, L"", value, MAX_PATH, iniPath);
    cJSON_AddItemToObject(node, utf16to8(name).c_str(), cJSON_CreateString(utf16to8(value).c_str()));
}

static bool http_post(struct mg_connection *nc, struct http_message *hm)
{
    const wchar_t *iniPath = (const wchar_t *)nc->mgr->user_data;

    if (mg_vcmp(&hm->uri, "/get_setting") == 0)
    {
        cJSON *root = cJSON_CreateObject();
        ReadList(root, iniPath, L"׷�Ӳ���");
        ReadList(root, iniPath, L"����ʱ����");
        ReadList(root, iniPath, L"�ر�ʱ����");
        ReadList(root, iniPath, L"�������");

        cJSON *node = cJSON_CreateObject();
        ReadValue(node, iniPath, L"��������", L"����Ŀ¼");
        ReadValue(node, iniPath, L"��������", L"�ϰ��");
        ReadValue(node, iniPath, L"��������", L"�±�ǩҳ��");
        ReadValue(node, iniPath, L"��������", L"�Ƴ����´���");
        ReadValue(node, iniPath, L"��������", L"�Զ��������г���");
        ReadValue(node, iniPath, L"��������", L"��Я��");
        cJSON_AddItemToObject(root, utf16to8(L"��������").c_str(), node);

        node = cJSON_CreateObject();
        ReadValue(node, iniPath, L"������ǿ", L"˫���رձ�ǩҳ");
        ReadValue(node, iniPath, L"������ǿ", L"�Ҽ��رձ�ǩҳ");
        ReadValue(node, iniPath, L"������ǿ", L"��ͣ�����ǩҳ");
        ReadValue(node, iniPath, L"������ǿ", L"��ͣʱ��");
        ReadValue(node, iniPath, L"������ǿ", L"��������ǩ");
        ReadValue(node, iniPath, L"������ǿ", L"��ͣ���ٱ�ǩ�л�");
        ReadValue(node, iniPath, L"������ǿ", L"�Ҽ����ٱ�ǩ�л�");
        ReadValue(node, iniPath, L"������ǿ", L"�±�ǩ����ǩ");
        ReadValue(node, iniPath, L"������ǿ", L"�±�ǩ����ַ");
        ReadValue(node, iniPath, L"������ǿ", L"�±�ǩҳ����Ч");
        ReadValue(node, iniPath, L"������ǿ", L"ǰ̨���±�ǩ");
        cJSON_AddItemToObject(root, utf16to8(L"������ǿ").c_str(), node);

        node = cJSON_CreateObject();
        ReadValue(node, iniPath, L"������", L"��������ַ");
        ReadValue(node, iniPath, L"������", L"���汾");
        cJSON_AddItemToObject(root, utf16to8(L"������").c_str(), node);

        node = cJSON_CreateObject();
        ReadValue(node, iniPath, L"�������", L"����");
        ReadValue(node, iniPath, L"�������", L"�켣");
        ReadValue(node, iniPath, L"�������", L"����");
        cJSON_AddItemToObject(root, utf16to8(L"������ƿ���").c_str(), node);

        cJSON_AddItemToObject(root, utf16to8(L"version").c_str(), cJSON_CreateString(RELEASE_VER_STR));

        char *str = cJSON_PrintUnformatted(root);
        int len = (int)strlen(str);
        cJSON_Delete(root);

        mg_send_head(nc, 200, len, extra_header);
        mg_send(nc, str, len);
        free(str);
    }
    else if (mg_vcmp(&hm->uri, "/set_setting") == 0)
    {
        char section[200];
        char name[200];
        char value[200];
        mg_get_http_var(&hm->body, "section", section, sizeof(section));
        mg_get_http_var(&hm->body, "name", name, sizeof(name));
        mg_get_http_var(&hm->body, "value", value, sizeof(value));
        //DebugLog(L"set_setting %s %s=%s", utf8to16(section).c_str(), utf8to16(name).c_str(), utf8to16(value).c_str());

        ::WritePrivateProfileString(utf8to16(section).c_str(), utf8to16(name).c_str(), utf8to16(value).c_str(), iniPath);
        ReadConfig(iniPath);

        mg_send_head(nc, 200, 2, extra_header);
        mg_send(nc, "{}", 2);
    }
    else if (mg_vcmp(&hm->uri, "/del_setting") == 0)
    {
        char section[200];
        char name[200];
        mg_get_http_var(&hm->body, "section", section, sizeof(section));
        mg_get_http_var(&hm->body, "name", name, sizeof(name));
        //DebugLog(L"del_setting %s %s=%s", utf8to16(section).c_str(), utf8to16(name).c_str());

        ::WritePrivateProfileString(utf8to16(section).c_str(), utf8to16(name).c_str(), NULL, iniPath);

        mg_send_head(nc, 200, 2, extra_header);
        mg_send(nc, "{}", 2);
    }
    else if (mg_vcmp(&hm->uri, "/add_section") == 0)
    {
        char section[200];
        char value[200];
        mg_get_http_var(&hm->body, "section", section, sizeof(section));
        mg_get_http_var(&hm->body, "value", value, sizeof(value));

        auto contents = GetSection(utf8to16(section).c_str(), iniPath);
        contents.push_back(utf8to16(value));
        SetSection(utf8to16(section).c_str(), contents, iniPath);

        mg_send_head(nc, 200, 2, extra_header);
        mg_send(nc, "{}", 2);
    }
    else if (mg_vcmp(&hm->uri, "/del_section") == 0)
    {
        char section[200];
        char value[200];
        mg_get_http_var(&hm->body, "section", section, sizeof(section));
        mg_get_http_var(&hm->body, "value", value, sizeof(value));

        auto contents = GetSection(utf8to16(section).c_str(), iniPath);

        int index = 0;
        for (auto &content : contents)
        {
            if (_wcsicmp(content.c_str(), utf8to16(value).c_str()) == 0)
            {
                contents.erase(contents.begin() + index);
                SetSection(utf8to16(section).c_str(), contents, iniPath);
                break;
            }
            index++;
        }

        mg_send_head(nc, 200, 2, extra_header);
        mg_send(nc, "{}", 2);
    }
    else
    {
        return false;
    }

    return true;
}

void not_found(struct mg_connection *nc)
{
    const char reason[] = "Not Found";
    mg_send_head(nc, 404, sizeof(reason) - 1, extra_header);
    mg_send(nc, reason, sizeof(reason) - 1);
}

static void http_handler(struct mg_connection *nc, int ev, void *ev_data)
{
    switch (ev)
    {
    case MG_EV_HTTP_REQUEST:
    {
        struct http_message *hm = (struct http_message *) ev_data;
        //DebugLog(L"%.*S %.*S %.*S", (int)hm->method.len, hm->method.p, (int)hm->uri.len, hm->uri.p, (int)hm->body.len, hm->body.p);

        if (mg_vcmp(&hm->method, "POST") == 0)
        {
            if (!http_post(nc, hm))
            {
                not_found(nc);
            }
        }
        else
        {
            //404
            not_found(nc);
        }
        nc->flags |= MG_F_SEND_AND_CLOSE;
    }
    break;
    default:
        break;
    }
}
