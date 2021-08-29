#define _CRT_SECURE_NO_WARNINGS         // �ֽ� VC++ ������ �� ��� ����
#define _WINSOCK_DEPRECATED_NO_WARNINGS // �ֽ� VC++ ������ �� ��� ����
#pragma comment(lib, "ws2_32")
#include <WinSock2.h>
#include <stdlib.h>
#include <stdio.h>
#include "resource.h"
#include "locale.h"
#include <iostream>
#include <string>
#include "rapidjson/document.h" // rapidjson's DOM-style API 
#include "rapidjson/prettywriter.h" // for stringify JSON
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <locale>
#include <codecvt>
#include <vector>

#define SERVERIP "172.30.1.59"
#define SERVERPORT 9132
#define BUFSIZE 1024
using namespace rapidjson;
using namespace std;

// ��ȭ���� ���ν���
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DlgProc2(HWND, UINT, WPARAM, LPARAM);
// ���� ��Ʈ�� ��� �Լ�
void DisplayText(char* fmt, ...);
// ���� ��� �Լ�
void err_quit(char* rec);
void err_display(char* rec);
// ���� ��� ������ �Լ�
DWORD WINAPI ClientMain(LPVOID arg);

void Update();
void Upload(HWND);
void Following(HWND hDlg);
void Friendlist(HWND hDlg);
void User_Logout();
string Create_Account();
string User_Login();
void Setting(HWND hDlg);

SOCKET sock; // ����
char txt_id[BUFSIZE + 1]; // ID �Է¿� �ۼ��� ����
char txt_pw[BUFSIZE + 1]; // PW �Է¿� �ۼ��� ����
char txt_feed[BUFSIZE + 1]; // ������ �ۼ��� ����
char buf[BUFSIZE + 1]; // ������ �ۼ��� ����
char rec[BUFSIZE];
HANDLE hReadEvent, hWriteEvent; // �̺�Ʈ
HWND hSendButton, hCreateButton, hLoginButton; // ������ ��ư, ����������ư
HWND hUpdateButton, hUploadButton, hFollowButton, // ������Ʈ, ���ε�, �ȷο��ư
hRecommendButton, hLogoutButton; // ģ����õ��ư, �α׾ƿ���ư
HWND hEdit1; // ���̾�α� 1�� ���� ��Ʈ��
HWND hID, hPWD; // ���̾�α� 1�� ���� ��Ʈ��
HWND hEdit3, hEdit4; // ���̾�α� 2�� ���� ��Ʈ��

int firstFlag = 1;
int status_code = 0;

Document d;
StringBuffer buffer;
Writer<StringBuffer> writer(buffer);
const char* output;
string jsonString;
wstring_convert < codecvt_utf8<wchar_t>, wchar_t> convertString;
wstring wideString;
wchar_t strUnicode[BUFSIZE] = { 0, };
char    strMultibyte[BUFSIZE] = { 0, };
int len;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
{
    // �̺�Ʈ ����
    hReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
    if (hReadEvent == NULL) return 1;
    hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (hWriteEvent == NULL) return 1;

    // ���� ��� ������ ����
    CreateThread(NULL, 0, ClientMain, NULL, 0, NULL);

    // ��ȭ���� ����
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

    // �̺�Ʈ ����
    CloseHandle(hReadEvent);
    CloseHandle(hWriteEvent);

    // closesocket()
    closesocket(sock);

    // ���� ����
    WSACleanup();
    return 0;
}

// ��ȭ����1 ���ν���
BOOL CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_INITDIALOG:
        hEdit1 = GetDlgItem(hDlg, IDC_EDIT1);
        hID = GetDlgItem(hDlg, IDC_EDIT2);
        hPWD = GetDlgItem(hDlg, IDC_EDIT3);
        hCreateButton = GetDlgItem(hDlg, IDC_BUTTON1);
        hLoginButton = GetDlgItem(hDlg, IDC_BUTTON2);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_BUTTON1:
            firstFlag = 1;
            Setting(hDlg);
            return TRUE;
        case IDC_BUTTON2:
            firstFlag = 2;
            Setting(hDlg);
            return TRUE;
        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}
// ��ȭ����2 ���ν���
BOOL CALLBACK DlgProc2(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_INITDIALOG:
        hEdit3 = GetDlgItem(hDlg, IDC_EDIT1);
        hEdit4 = GetDlgItem(hDlg, IDC_EDIT2);
        hUpdateButton = GetDlgItem(hDlg, IDC_BUTTON1);
        hUploadButton = GetDlgItem(hDlg, IDC_BUTTON2);
        hFollowButton = GetDlgItem(hDlg, IDC_BUTTON3);
        hRecommendButton = GetDlgItem(hDlg, IDC_BUTTON4);
        hLogoutButton = GetDlgItem(hDlg, IDC_BUTTON5);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_BUTTON1:
            memset(rec, 0, sizeof(rec));
            Update();
            return TRUE;
        case IDC_BUTTON2:
            memset(rec, 0, sizeof(rec));
            Upload(hDlg);
            return TRUE;

        case IDC_BUTTON3:
            memset(rec, 0, sizeof(rec));
            Following(hDlg);
            return TRUE;

        case IDC_BUTTON4:
            memset(rec, 0, sizeof(rec));
            Friendlist(hDlg);
            return TRUE;

        case IDC_BUTTON5:
            User_Logout();
            status_code = 0;
            firstFlag = 3;
            DisplayText("�α׾ƿ�\n");
            EnableWindow(hCreateButton, TRUE);
            EnableWindow(hLoginButton, TRUE);
            EndDialog(hDlg, IDCANCEL);        
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

// ���� ��Ʈ�� ��� �Լ�
void DisplayText(char* fmt, ...)
{
    va_list arg;
    va_start(arg, fmt);

    char cbuf[BUFSIZE + 256];
    vsprintf(cbuf, fmt, arg);

    HWND a = (status_code == 0) ? hEdit1 : hEdit3;
    int nLength = GetWindowTextLength(a);
    SendMessage(a, EM_SETSEL, nLength, nLength);
    SendMessage(a, EM_REPLACESEL, FALSE, (LPARAM)cbuf);

    va_end(arg);
}

// ���� �Լ� ���� ��� �� ����
void err_quit(char* rec)
{
    LPVOID lpMsgBuf;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, NULL);
    MessageBox(NULL, (LPCTSTR)lpMsgBuf, rec, MB_ICONERROR);
    LocalFree(lpMsgBuf);
    exit(1);
}

// ���� �Լ� ���� ���
void err_display(char* rec)
{
    LPVOID lpMsgBuf;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, NULL);
    DisplayText("[%s] %s", rec, (char*)lpMsgBuf);
    LocalFree(lpMsgBuf);
}

// TCP Ŭ���̾�Ʈ ���� �κ�
DWORD WINAPI ClientMain(LPVOID arg)
{
    int retval;

    // ���� �ʱ�ȭ
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return 1;

    // socket()
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) err_quit("socket()");

    // connect()
    SOCKADDR_IN serveraddr;
    ZeroMemory(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);
    serveraddr.sin_port = htons(SERVERPORT);
    retval = connect(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) err_quit("connect()");

    // ������ ������ ���
    while (1) {

        memset(rec, 0, sizeof(rec));
        WaitForSingleObject(hWriteEvent, INFINITE); // ���� �Ϸ� ��ٸ���

        Document d;
        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        const char* output;
        string a;

        if (strlen(txt_id) == 0 || strlen(txt_pw) == 0) {
            SetEvent(hReadEvent); // �б� �Ϸ� �˸���
            continue;
        }

        if (firstFlag == 1)
            a = Create_Account();
        else if (firstFlag == 2)
            a = User_Login();
        else
            continue;
        d.Parse(a.c_str());
        d.Accept(writer);
        output = buffer.GetString();

        // ������ ������
        retval = send(sock, output, strlen(output), 0);
        if (retval == SOCKET_ERROR) {
            err_display("send()");
            break;
        }

        // ������ �ޱ�
        if (recv(sock, rec, BUFSIZE, 0) == SOCKET_ERROR) {   // ���� �ܼ� ���
            cout << "error" << endl;
            break;
        }

        d.Parse(rec);
        assert(d.IsObject());
        SetWindowText(hEdit1, 0);
        if (firstFlag == 1) {
            if (d["status_code"].GetInt() == 1)
                DisplayText("ȸ������ ����\n");
            else if (d["status_code"].GetInt() != 1)
                DisplayText("ȸ������ ����\n");
        }
        else if (firstFlag == 2) {
            if (d["status_code"].GetInt() == 1) {
                DisplayText("�α��� ����\n");
                EnableWindow(hCreateButton, FALSE);
                EnableWindow(hLoginButton, FALSE);
                status_code = 1;
                DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG2), NULL, DlgProc2);
            }
            else
                DisplayText("�α��� ����\n");
        }
        EnableWindow(hCreateButton, TRUE);
        EnableWindow(hLoginButton, TRUE);

        SetEvent(hReadEvent); // �б� �Ϸ� �˸���
    }
    return 0;
}
void Setting(HWND hDlg) {
    WaitForSingleObject(hReadEvent, INFINITE); // �б� �Ϸ� ��ٸ���
    GetDlgItemText(hDlg, IDC_EDIT2, txt_id, BUFSIZE + 1);
    GetDlgItemText(hDlg, IDC_EDIT3, txt_pw, BUFSIZE + 1);
    SetEvent(hWriteEvent); // ���� �Ϸ� �˸���
    SetFocus(hID);
    SetFocus(hPWD);
    SendMessage(hID, EM_SETSEL, 0, -1);
    SendMessage(hPWD, EM_SETSEL, 0, -1);
}
string Create_Account() {
    string a = " { \"function_id\" : ";
    a += "1, \"user_id\" : \"";
    a += txt_id;
    a += "\", \"user_pw\" : \"";
    a += txt_pw;
    a += "\"}";
    return a;
}
string User_Login() {
    string a = " { \"function_id\" : ";
    a += "2, \"user_id\" : \"";
    a += txt_id;
    a += "\", \"user_pw\" : \"";
    a += txt_pw;
    a += "\"}";
    return a;
}
void Update() {
    buffer.Clear();
    Writer<StringBuffer> writer(buffer);
    vector<string> writers;
    vector<string> text;
    string a = " { \"function_id\" : ";
    a += "3, \"user_id\" : \"";
    a += txt_id;
    a += "\"}";
    d.Parse(a.c_str());
    d.Accept(writer);
    output = buffer.GetString();

    // ������ ������
    if (send(sock, output, strlen(output), 0) == SOCKET_ERROR) {
        err_display("send()");
        return;
    }
    if (recv(sock, rec, BUFSIZE, 0) == SOCKET_ERROR) {   // ���� �ܼ� ���
        cout << "error" << endl;
        return;
    }
    d.Parse(rec);
    SetWindowText(hEdit3, 0);
    if (d["status_code"].GetInt() == 1) {
        for (Value::ConstValueIterator itr = d["texts"].Begin(); itr != d["texts"].End(); ++itr)
            text.push_back(itr->GetString());
        for (Value::ConstValueIterator itr = d["writers"].Begin(); itr != d["writers"].End(); ++itr)
            writers.push_back(itr->GetString());
        for (int i = 0; i < text.size(); i++) {
            wideString = convertString.from_bytes(writers[i]);
            wcscpy_s(strUnicode, BUFSIZE, wideString.c_str());
            len = WideCharToMultiByte(CP_ACP, 0, strUnicode, -1, NULL, 0, NULL, NULL);
            WideCharToMultiByte(CP_ACP, 0, strUnicode, -1, strMultibyte, len, NULL, NULL);
            DisplayText("ID : %s\n", strMultibyte);

            wideString = convertString.from_bytes(text[i]);
            wcscpy_s(strUnicode, BUFSIZE, wideString.c_str());
            len = WideCharToMultiByte(CP_ACP, 0, strUnicode, -1, NULL, 0, NULL, NULL);
            WideCharToMultiByte(CP_ACP, 0, strUnicode, -1, strMultibyte, len, NULL, NULL);
            DisplayText("%s\n", strMultibyte);
            DisplayText("\n");
            
        }
    }
    else
        DisplayText("�ڽ� Ȥ�� ģ���� �ۼ��� ���� �����ϴ�\n");
    text.clear();
    writers.clear();
}
void Upload(HWND hDlg) {
    buffer.Clear();
    Writer<StringBuffer> writer(buffer);
    GetDlgItemText(hDlg, IDC_EDIT2, txt_feed, BUFSIZE + 1);
    SetEvent(hWriteEvent); // ���� �Ϸ� �˸���
    SetFocus(hEdit4);
    if (strlen(txt_feed) == 0) {
        SetEvent(hReadEvent); // �б� �Ϸ� �˸���
        return;
    }
    string a = " { \"function_id\" : ";
    a += "4, \"user_id\" : \"";
    a += txt_id;
    a += "\", \"text\" : \"";
    a += txt_feed;
    a += "\"}";
    d.Parse(a.c_str());
    d.Accept(writer);
    output = buffer.GetString();
    // ������ ������
    if (send(sock, output, strlen(output), 0) == SOCKET_ERROR) {
        err_display("send()");
        return;
    }
    if (recv(sock, rec, BUFSIZE, 0) == SOCKET_ERROR) {   // ���� �ܼ� ���
        cout << "error" << endl;
        return;
    }
    d.Parse(rec);
    SetWindowText(hEdit3, 0);
    if (d["status_code"].GetInt() == 1)
        DisplayText("���ε� ����\n");
    else if (d["status_code"].GetInt() != 1)
        DisplayText("���ε� ����\n");
}
void Following(HWND hDlg) {
    buffer.Clear();
    Writer<StringBuffer> writer(buffer);
    GetDlgItemText(hDlg, IDC_EDIT2, txt_feed, BUFSIZE + 1);
    SetEvent(hWriteEvent); // ���� �Ϸ� �˸���
    SetFocus(hEdit4);
    if (strlen(txt_feed) == 0) {
        SetEvent(hReadEvent); // �б� �Ϸ� �˸���
        return;
    }
    string a = " { \"function_id\" : ";
    a += "5, \"user_id\" : \"";
    a += txt_id;
    a += "\", \"follow_id\" : \"";
    a += txt_feed;
    a += "\"}";
    d.Parse(a.c_str());
    d.Accept(writer);
    output = buffer.GetString();
    // ������ ������
    if (send(sock, output, strlen(output), 0) == SOCKET_ERROR) {
        err_display("send()");
        return;
    }
    if (recv(sock, rec, BUFSIZE, 0) == SOCKET_ERROR) {   // ���� �ܼ� ���
        cout << "error" << endl;
        return;
    }
    SetWindowText(hEdit3, 0);
    d.Parse(rec);
    jsonString = d["text"].GetString();
    wideString = convertString.from_bytes(jsonString);

    wcscpy_s(strUnicode, BUFSIZE, wideString.c_str());
    len = WideCharToMultiByte(CP_ACP, 0, strUnicode, -1, NULL, 0, NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, strUnicode, -1, strMultibyte, len, NULL, NULL);

    DisplayText("%s\n", strMultibyte);
}
void Friendlist(HWND hDlg) {
    buffer.Clear();
    Writer<StringBuffer> writer(buffer);
    string a = " { \"function_id\" : ";
    a += "6, \"user_id\" : \"";
    a += txt_id;
    a += "\"}";
    d.Parse(a.c_str());
    d.Accept(writer);
    output = buffer.GetString();
    // ������ ������
    if (send(sock, output, strlen(output), 0) == SOCKET_ERROR) {
        err_display("send()");
        return;
    }
    if (recv(sock, rec, BUFSIZE, 0) == SOCKET_ERROR) {   // ���� �ܼ� ���
        cout << "error" << endl;
        return;
    }
    d.Parse(rec);
    SetWindowText(hEdit3, 0);
    if (d["status_code"].GetInt() == 1) {
        DisplayText("\n�ȷο� ����Ʈ : \n");
        for (Value::ConstValueIterator itr = d["friends"].Begin(); itr != d["friends"].End(); ++itr) {
            DisplayText("%s \n", itr->GetString());
        }
    }
    else {
        jsonString = d["error"].GetString();
        wideString = convertString.from_bytes(jsonString);

        wcscpy_s(strUnicode, BUFSIZE, wideString.c_str());
        len = WideCharToMultiByte(CP_ACP, 0, strUnicode, -1, NULL, 0, NULL, NULL);
        WideCharToMultiByte(CP_ACP, 0, strUnicode, -1, strMultibyte, len, NULL, NULL);

        DisplayText("%s\n", strMultibyte);
    }
}
void User_Logout() {
    buffer.Clear();
    Writer<StringBuffer> writer(buffer);
    string a = " { \"function_id\" : ";
    a += "7, \"user_id\" : \"";
    a += txt_id;
    a += "\"}";
    d.Parse(a.c_str());
    d.Accept(writer);
    output = buffer.GetString();
    // ������ ������
    if (send(sock, output, strlen(output), 0) == SOCKET_ERROR) {
        err_display("send()");
        return;
    }
}