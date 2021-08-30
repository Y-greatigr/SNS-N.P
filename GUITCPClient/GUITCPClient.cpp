#define _CRT_SECURE_NO_WARNINGS         // 최신 VC++ 컴파일 시 경고 방지
#define _WINSOCK_DEPRECATED_NO_WARNINGS // 최신 VC++ 컴파일 시 경고 방지
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

#define SERVERIP "..."
#define SERVERPORT 9999
#define BUFSIZE 1024
using namespace rapidjson;
using namespace std;

// 대화상자 프로시저
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DlgProc2(HWND, UINT, WPARAM, LPARAM);
// 편집 컨트롤 출력 함수
void DisplayText(char* fmt, ...);
// 오류 출력 함수
void err_quit(char* rec);
void err_display(char* rec);
// 소켓 통신 스레드 함수
DWORD WINAPI ClientMain(LPVOID arg);

void Update();
void Upload(HWND);
void Following(HWND hDlg);
void Friendlist(HWND hDlg);
void User_Logout();
string Create_Account();
string User_Login();
void Setting(HWND hDlg);

SOCKET sock; // 소켓
char txt_id[BUFSIZE + 1]; // ID 입력용 송수신 버퍼
char txt_pw[BUFSIZE + 1]; // PW 입력용 송수신 버퍼
char txt_feed[BUFSIZE + 1]; // 데이터 송수신 버퍼
char buf[BUFSIZE + 1]; // 데이터 송수신 버퍼
char rec[BUFSIZE];
HANDLE hReadEvent, hWriteEvent; // 이벤트
HWND hSendButton, hCreateButton, hLoginButton; // 보내기 버튼, 계정생성버튼
HWND hUpdateButton, hUploadButton, hFollowButton, // 업데이트, 업로드, 팔로우버튼
hRecommendButton, hLogoutButton; // 친구추천버튼, 로그아웃버튼
HWND hEdit1; // 다이얼로그 1의 편집 컨트롤
HWND hID, hPWD; // 다이얼로그 1의 편집 컨트롤
HWND hEdit3, hEdit4; // 다이얼로그 2의 편집 컨트롤

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
    // 이벤트 생성
    hReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
    if (hReadEvent == NULL) return 1;
    hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (hWriteEvent == NULL) return 1;

    // 소켓 통신 스레드 생성
    CreateThread(NULL, 0, ClientMain, NULL, 0, NULL);

    // 대화상자 생성
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

    // 이벤트 제거
    CloseHandle(hReadEvent);
    CloseHandle(hWriteEvent);

    // closesocket()
    closesocket(sock);

    // 윈속 종료
    WSACleanup();
    return 0;
}

// 대화상자1 프로시저
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
// 대화상자2 프로시저
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
            DisplayText("로그아웃\n");
            EnableWindow(hCreateButton, TRUE);
            EnableWindow(hLoginButton, TRUE);
            EndDialog(hDlg, IDCANCEL);        
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

// 편집 컨트롤 출력 함수
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

// 소켓 함수 오류 출력 후 종료
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

// 소켓 함수 오류 출력
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

// TCP 클라이언트 시작 부분
DWORD WINAPI ClientMain(LPVOID arg)
{
    int retval;

    // 윈속 초기화
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

    // 서버와 데이터 통신
    while (1) {

        memset(rec, 0, sizeof(rec));
        WaitForSingleObject(hWriteEvent, INFINITE); // 쓰기 완료 기다리기

        Document d;
        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        const char* output;
        string a;

        if (strlen(txt_id) == 0 || strlen(txt_pw) == 0) {
            SetEvent(hReadEvent); // 읽기 완료 알리기
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

        // 데이터 보내기
        retval = send(sock, output, strlen(output), 0);
        if (retval == SOCKET_ERROR) {
            err_display("send()");
            break;
        }

        // 데이터 받기
        if (recv(sock, rec, BUFSIZE, 0) == SOCKET_ERROR) {   // 에러 콘솔 출력
            cout << "error" << endl;
            break;
        }

        d.Parse(rec);
        assert(d.IsObject());
        SetWindowText(hEdit1, 0);
        if (firstFlag == 1) {
            if (d["status_code"].GetInt() == 1)
                DisplayText("회원가입 성공\n");
            else if (d["status_code"].GetInt() != 1)
                DisplayText("회원가입 실패\n");
        }
        else if (firstFlag == 2) {
            if (d["status_code"].GetInt() == 1) {
                DisplayText("로그인 성공\n");
                EnableWindow(hCreateButton, FALSE);
                EnableWindow(hLoginButton, FALSE);
                status_code = 1;
                DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG2), NULL, DlgProc2);
            }
            else
                DisplayText("로그인 실패\n");
        }
        EnableWindow(hCreateButton, TRUE);
        EnableWindow(hLoginButton, TRUE);

        SetEvent(hReadEvent); // 읽기 완료 알리기
    }
    return 0;
}
void Setting(HWND hDlg) {
    WaitForSingleObject(hReadEvent, INFINITE); // 읽기 완료 기다리기
    GetDlgItemText(hDlg, IDC_EDIT2, txt_id, BUFSIZE + 1);
    GetDlgItemText(hDlg, IDC_EDIT3, txt_pw, BUFSIZE + 1);
    SetEvent(hWriteEvent); // 쓰기 완료 알리기
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

    // 데이터 보내기
    if (send(sock, output, strlen(output), 0) == SOCKET_ERROR) {
        err_display("send()");
        return;
    }
    if (recv(sock, rec, BUFSIZE, 0) == SOCKET_ERROR) {   // 에러 콘솔 출력
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
        DisplayText("자신 혹은 친구가 작성한 글이 없습니다\n");
    text.clear();
    writers.clear();
}
void Upload(HWND hDlg) {
    buffer.Clear();
    Writer<StringBuffer> writer(buffer);
    GetDlgItemText(hDlg, IDC_EDIT2, txt_feed, BUFSIZE + 1);
    SetEvent(hWriteEvent); // 쓰기 완료 알리기
    SetFocus(hEdit4);
    if (strlen(txt_feed) == 0) {
        SetEvent(hReadEvent); // 읽기 완료 알리기
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
    // 데이터 보내기
    if (send(sock, output, strlen(output), 0) == SOCKET_ERROR) {
        err_display("send()");
        return;
    }
    if (recv(sock, rec, BUFSIZE, 0) == SOCKET_ERROR) {   // 에러 콘솔 출력
        cout << "error" << endl;
        return;
    }
    d.Parse(rec);
    SetWindowText(hEdit3, 0);
    if (d["status_code"].GetInt() == 1)
        DisplayText("업로드 성공\n");
    else if (d["status_code"].GetInt() != 1)
        DisplayText("업로드 실패\n");
}
void Following(HWND hDlg) {
    buffer.Clear();
    Writer<StringBuffer> writer(buffer);
    GetDlgItemText(hDlg, IDC_EDIT2, txt_feed, BUFSIZE + 1);
    SetEvent(hWriteEvent); // 쓰기 완료 알리기
    SetFocus(hEdit4);
    if (strlen(txt_feed) == 0) {
        SetEvent(hReadEvent); // 읽기 완료 알리기
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
    // 데이터 보내기
    if (send(sock, output, strlen(output), 0) == SOCKET_ERROR) {
        err_display("send()");
        return;
    }
    if (recv(sock, rec, BUFSIZE, 0) == SOCKET_ERROR) {   // 에러 콘솔 출력
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
    // 데이터 보내기
    if (send(sock, output, strlen(output), 0) == SOCKET_ERROR) {
        err_display("send()");
        return;
    }
    if (recv(sock, rec, BUFSIZE, 0) == SOCKET_ERROR) {   // 에러 콘솔 출력
        cout << "error" << endl;
        return;
    }
    d.Parse(rec);
    SetWindowText(hEdit3, 0);
    if (d["status_code"].GetInt() == 1) {
        DisplayText("\n팔로우 리스트 : \n");
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
    // 데이터 보내기
    if (send(sock, output, strlen(output), 0) == SOCKET_ERROR) {
        err_display("send()");
        return;
    }
}
