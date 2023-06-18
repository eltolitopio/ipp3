#include "main.h"
#include "resource.h"
#include "stdafx.h"
#pragma comment(lib, "ws2_32.lib")

#define MYSRV_ADDR "192.168.190.129"
#define MYSRV_PORT htons(1050)
#define MYSRV_COMMDOMAIN AF_INET
#define MYSRV_PROTOCOL IPPROTO_TCP
#define IDT_REDRAW 6000

enum GRAPH_TYPE {
    GRAPH_EMPTY = 0,
    GRAPH_SINE = 1,
    GRAPH_SQR = 2,
} graphType = GRAPH_EMPTY;

SOCKET sock_listen = INVALID_SOCKET, sock_client = INVALID_SOCKET;

DWORD CALLBACK DlgThread(LPVOID lpParam) {
    DWORD dwRet = (DWORD)DialogBox(
        GetModuleHandle(NULL),
        MAKEINTRESOURCE(IDD_FORMVIEW),
        HWND_DESKTOP,
        DlgProc
    );
    return dwRet;
}

LRESULT CALLBACK DlgProc(
    HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam
) {
    switch (uMsg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hDC = BeginPaint(hWnd, &ps);
        RECT rt;
        LPTSTR lpszBuf = (LPTSTR)LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR) * 16);
        GetClientRect(hWnd, &rt);

        _stprintf_s(
            lpszBuf, 16,
            _T("Graph: %u"),
            graphType
        );
        TextOut(hDC, 20, 20, lpszBuf, lstrlen(lpszBuf));
        LocalFree(lpszBuf);

        SetWindowOrgEx(hDC, -(rt.right / 2), -(rt.bottom / 2), NULL);
        SetGraphicsMode(hDC, GM_ADVANCED);

        XFORM xf = { 0 };
        xf.eM11 = (FLOAT)1.0;
        xf.eM12 = (FLOAT)0.0;
        xf.eM21 = (FLOAT)0.0;
        xf.eM22 = (FLOAT)-1.0;
        xf.eDx = (FLOAT)0.0;
        xf.eDy = (FLOAT)0.0;
        SetWorldTransform(hDC, &xf);

        MoveToEx(hDC, 0, -rt.bottom / 2, NULL);
        LineTo(hDC, 0, rt.bottom / 2);
        MoveToEx(hDC, -rt.right / 2, 0, NULL);
        LineTo(hDC, rt.right / 2, 0);
        MoveToEx(hDC, -rt.right / 2, 0, NULL);

        enum GRAPH_TYPE selectedGraphType = graphType;

        for (double i = (double)(-rt.right) / 2; i < (double)(rt.right) / 2; ++i) {
            switch (selectedGraphType) {
            case GRAPH_SINE:
                LineTo(hDC, (int)(i), (int)(sin(i / 10) * 20));
                MoveToEx(hDC, (int)(i), (int)(sin(i / 10) * 20), NULL);
                break;
            case GRAPH_SQR:
                LineTo(hDC, (int)(i), (int)(i / 10 * i / 10 * 20));
                MoveToEx(hDC, (int)(i), (int)(i / 10 * i / 10 * 20), NULL);
                break;
            default:
                break;
            }
        }

        EndPaint(hWnd, &ps);
    }
    break;

    case WM_TIMER:
        switch (wParam) {
        case IDT_REDRAW:
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        DEFAULT_UNREACHABLE;
        }
        break;

    case WM_CLOSE:
        SendMessage(hWnd, WM_DESTROY, 0, 0);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_INITDIALOG:
        SetTimer(hWnd, IDT_REDRAW, 1000, NULL);
        return TRUE;

    DEFAULT_UNREACHABLE;
    }

    return FALSE;
}

INT APIENTRY _tWinMain(
    HINSTANCE hInst, HINSTANCE hPrevInst, LPTSTR lpszCmd, INT nCmdShow
) {
    WSADATA wsaData = { 0 };
    LPTSTR lpszError = NULL;
    LPHOSTENT hst = NULL;
    SOCKADDR_IN sa_in = { 0 };
    INT iRet = 0;
    HANDLE hThread = NULL;

    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(hPrevInst);
    UNREFERENCED_PARAMETER(lpszCmd);
    UNREFERENCED_PARAMETER(nCmdShow);

    if ((iRet = WSAStartup(MAKEWORD(1, 1), &wsaData)) != EXIT_SUCCESS) {
        FormatWSAMessage(lpszError);
        MessageBox(
            HWND_DESKTOP, lpszError,
            _T("WSAStartup: "), MB_ICONSTOP | MB_OK
        );
        goto done;
    }

    if ((sock_listen = socket(MYSRV_COMMDOMAIN, SOCK_STREAM, MYSRV_PROTOCOL)) == INVALID_SOCKET) {
        FormatWSAMessage(lpszError);
        MessageBox(
            HWND_DESKTOP, lpszError,
            _T("Socket creation failed: "), MB_ICONSTOP | MB_OK
        );
        goto done;
    }

    hst = (LPHOSTENT)gethostbyname(MYSRV_ADDR);
    sa_in.sin_family = hst->h_addrtype;
    sa_in.sin_port = MYSRV_PORT;
    memcpy((CHAR*)&sa_in.sin_addr, hst->h_addr, hst->h_length);

    if ((iRet = bind(sock_listen, (LPSOCKADDR)&sa_in, sizeof(sa_in))) == SOCKET_ERROR) {
        FormatWSAMessage(lpszError);
        MessageBox(
            HWND_DESKTOP, lpszError,
            _T("Bind failed: "), MB_ICONSTOP | MB_OK
        );
        goto done;
    }

    if ((iRet = listen(sock_listen, 2)) == SOCKET_ERROR) {
        FormatWSAMessage(lpszError);
        MessageBox(
            HWND_DESKTOP, lpszError,
            _T("Listen failed: "), MB_ICONSTOP | MB_OK
        );
        goto done;
    }

    hThread = CreateThread(NULL, 0, DlgThread, NULL, 0, NULL);

    if ((sock_client = accept(sock_listen, NULL, NULL)) == INVALID_SOCKET) {
        FormatWSAMessage(lpszError);
        MessageBox(
            HWND_DESKTOP, lpszError,
            _T("Failed accepting connection: "), MB_ICONSTOP | MB_OK
        );
        goto done;
    }

    while (1) {
        if (recv(sock_client, (char*)&graphType, sizeof(graphType), 0) != SOCKET_ERROR) {
            if (graphType == GRAPH_EMPTY) {
                shutdown(sock_client, 0);
                send(sock_client, "bye\n", 5, 0);
                break;
            }
        }
    }

done:
    shutdown(sock_listen, 2);
    shutdown(sock_client, 2);

    if ((iRet = closesocket(sock_client)) == SOCKET_ERROR) {
        FormatWSAMessage(lpszError);
        MessageBox(
            HWND_DESKTOP, lpszError,
            _T("Failed closing client socket: "), MB_ICONSTOP | MB_OK
        );
    }

    if ((iRet = closesocket(sock_listen)) == SOCKET_ERROR) {
        FormatWSAMessage(lpszError);
        MessageBox(
            HWND_DESKTOP, lpszError,
            _T("Failed closing listen socket: "), MB_ICONSTOP | MB_OK
        );
    }

    WSACleanup();

    if (lpszError != NULL)
        LocalFree(lpszError);

    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);

    return iRet;
}