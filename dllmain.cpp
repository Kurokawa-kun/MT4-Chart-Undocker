#include "pch.h"
#include "windows.h"
#include "tchar.h"
#include "vector"

/*
・コンパイル時の注意点
‐MT4がロードできる外部のライブラリはx86だけです。x86バイナリファイルとしてコンパイルしてください。
※ 上記はプロジェクトファイルに反映済みです。

・補足
‐Debugビルドの場合、描画テストも兼ねているためウィンドウの色が通常とは異なったものになります。
*/

#define MT4_EXPFUNC __declspec(dllexport)
#define MODULE_NAME TEXT("UndockLibrary.dll")

MSGBOXPARAMS MsgBoxParamError;  //  メッセージボックスの使い回し用
TCHAR MessageBuf[4096];

//  自前で定義したウィンドウメッセージ
#define WM_DOCK_INTERNAL    0xFFF7  //  Dock通知。使われていないメッセージ番号を利用する
#define WM_CLOSE_INTERNAL   0xFFF8  //  終了通知。使われていないメッセージ番号を利用する

//  ウィンドウフレームの色
#ifdef _DEBUG 
    //  デバッグモード
#define ColorWindowSurface    RGB(0xAF, 0x1F, 0x1F)
#define ColorWindowFrame    RGB(0x00, 0xAF, 0x4F)
#define COLOR_CLOSE_BUTTON_BACKGROUND RGB(0x10, 0x10, 0x7F)
#else
#define ColorWindowSurface    GetSysColor(COLOR_WINDOW)
#define ColorWindowFrame    COLOR_WINDOWFRAME
#define COLOR_CLOSE_BUTTON_BACKGROUND RGB(0xE8, 0x11, 0x43)
#endif

//  ウィンドウプロパティ。チャートのDock, UnDockで必要
#define WindowPropertyChartWindowDocked (WS_CHILD | WS_VISIBLE | WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR | WS_EX_CLIENTEDGE)
#define WindowPropertyChartWindowUnDocked (WS_OVERLAPPED | WS_THICKFRAME | WS_CAPTION | WS_VISIBLE | WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR | WS_EX_CLIENTEDGE)

//  ボタンのアイコン
const TCHAR ButtonIcons[4][2][2] =
{
    //  通常状態のときと最大化したときのアイコン
    {TEXT("u"), TEXT("u")}, //  Dockボタン
    {TEXT("0"), TEXT("0")}, //  最小化ボタン
    {TEXT("1"), TEXT("2")}, //  最大化ボタン
    {TEXT("r"), TEXT("r")}  //  閉じるボタン
};

//  ボタン表面の色
const COLORREF ButtonBackgroundColors[4][2] =
{
    //  フォーカス状態と非フォーカス状態
    {GetSysColor(COLOR_BTNFACE), GetSysColor(COLOR_WINDOW)},    //  Dockボタン
    {GetSysColor(COLOR_BTNFACE), GetSysColor(COLOR_WINDOW)},    //  最小化ボタン
    {GetSysColor(COLOR_BTNFACE), GetSysColor(COLOR_WINDOW)},    //  最大化ボタン
    {COLOR_CLOSE_BUTTON_BACKGROUND, GetSysColor(COLOR_WINDOW)}        //  閉じるボタン
};

//  ボタンの文字色（アクティブ状態と非アクティブ状態）
const COLORREF ButtonCaptionColors[2] = { GetSysColor(COLOR_ACTIVECAPTION), GetSysColor(COLOR_INACTIVECAPTION) };

//  ウィンドウ上部のボタンを押したときに送信するメッセージ
struct Messages
{
    char Dest;  //  送信先のウィンドウ（ドキュメントウィンドウかチャートウィンドウか）
    UINT uMsg;
    WPARAM WParam;
    LPARAM LParam;
};

const Messages MessagesButtonClicked[2][4][2] =
{
    //  順にDockボタン、最小化ボタン、最大化ボタン、閉じるボタン
    //  ※ボタンによっては2つのメッセージを送信する必要がある
    //  通常表示
    {
        {{'c', WM_DOCK_INTERNAL, NULL, NULL}, {'\0', NULL, NULL, NULL}},
        {{'c', WM_SYSCOMMAND, SC_MINIMIZE, NULL}, {'\0', NULL, NULL, NULL}},
        {{'c', WM_SYSCOMMAND, SC_MAXIMIZE, NULL}, {'\0', NULL, NULL, NULL}},
        {{'c', WM_DOCK_INTERNAL, NULL, NULL}, {'d', WM_CLOSE, NULL, NULL}}
    },
    //  最大化表示
    {
        {{'c', WM_DOCK_INTERNAL, NULL, NULL}, {'\0', NULL, NULL, NULL}},
        {{'c', WM_SYSCOMMAND, SC_MINIMIZE, NULL}, {'\0', NULL, NULL, NULL}},
        {{'c', WM_SYSCOMMAND, SC_RESTORE, NULL}, {'\0', NULL, NULL, NULL}},
        {{'d', WM_DOCK_INTERNAL, NULL, NULL}, {'d', WM_CLOSE, NULL, NULL}}
    }
};

//  ウィンドウフレームのマージン。ウィンドウの描画で必要
#define WindowFrameSizeVertical 10  //  ウィンドウ左右のマージン
#define WindowFrameSizeTop 32   //  ウィンドウ上部のマージン（＝タイトルバーの高さ）
#define WindowFrameSizeBottom 9 //  ウィンドウ下部のマージン

//  切り離したチャートそのものを表すクラス
class UndockedChart
{
    public:
        static HWND MT4TerminalWindow;  //  MT4の実体
        static HWND MDIClientWindow;    //  MDIクライアントウィンドウ
        static LONG WndPrcMT4TerminalWindowOld;   //  差し替え前のMT4のウィンドウプロシージャ
        static LONG WndPrcDocumentWindowOld;   //  差し替え前のチャートウィンドウのウィンドウプロシージャ
        static LONG WndPrcChartWindowOld;   //  差し替え前のチドキュメントウィンドウのウィンドウプロシージャ
        //  メソッドがstaticである理由は特にない。アドレスを記憶しておくメモリが数十バイト節約できるくらい。
        static COLORREF ColorToColorRef(DWORD color);
        static LRESULT CALLBACK WndPrcMT4TerminalWindow(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        static LRESULT CALLBACK WndPrcDocumentWindow(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        static LRESULT CALLBACK WndPrcChartWindow(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        static void PaintButtons(UndockedChart* CurrentUndockedChart, RECT* RectButtons);
        static void DockWindow(bool Dock, bool ChangeWndPrc, UndockedChart* CurrentUndockedChart);
    
    public:
        HWND DocumentWindow;   //  ドキュメントウィンドウ（チャートのフレーム）
        HWND ChartWindow;   //  チャートの実体
        HWND DockButton;   //  Dockボタンの実体
        TCHAR WindowTitle[128];
        int CurrentMouseHitting;    //  現在どのボタンの上にいるかを表す。タイトルバーの描画に必要
        RECT RectChartWindow;    //  切り離したときのチャートの位置とサイズ
        bool IsActive;  //  このウィンドウがアクティブかどうか
        
        bool operator==(const UndockedChart& x)
        {
            return ChartWindow == x.ChartWindow;
        }
        bool operator==(UndockedChart& x)
        {
            return ChartWindow == x.ChartWindow;
        }
    public:
        //  コンストラクタ
        UndockedChart(LPCTSTR pWindowTitle, HWND pChartWindow)
        {
            _tcscpy_s(WindowTitle, sizeof(WindowTitle) / sizeof(TCHAR), pWindowTitle);
            ChartWindow = pChartWindow;
            DocumentWindow = GetParent(ChartWindow);
            DockButton = NULL;
            if (MDIClientWindow == NULL)
            {
                MDIClientWindow = GetParent(DocumentWindow);
                MT4TerminalWindow = GetParent(MDIClientWindow);
                UndockedChart::WndPrcMT4TerminalWindowOld = GetWindowLong(MT4TerminalWindow, GWL_WNDPROC);
                SetWindowLong(MT4TerminalWindow, GWL_WNDPROC, (LONG)WndPrcMT4TerminalWindow);
                UndockedChart::WndPrcDocumentWindowOld = GetWindowLong(DocumentWindow, GWL_WNDPROC);
                UndockedChart::WndPrcChartWindowOld = GetWindowLong(ChartWindow, GWL_WNDPROC);
            }
            CurrentMouseHitting = 0;
            RectChartWindow = { 50, 30, 800 + 50, 640 + 30 };
            IsActive = TRUE;
            return;
        }
        //  コピーコンストラクタ
        UndockedChart(const UndockedChart& Copied)
        {
            DocumentWindow = Copied.DocumentWindow;
            ChartWindow = Copied.ChartWindow;
            DockButton = Copied.DockButton;
            for (int c = 0; c < 128; c++)
            {
                WindowTitle[c] = Copied.WindowTitle[c];
            }
            CurrentMouseHitting = Copied.CurrentMouseHitting;
            RectChartWindow = Copied.RectChartWindow;
            IsActive = Copied.IsActive;
            return;
        }
        //  デストラクタ
        ~UndockedChart()
        {
            //  特に動的解放するものはない
            return;
        }
        //  チャートウィンドウの切り離しを行う
        int UndockChartWindow()
        {
            SendMessage(ChartWindow, WM_SETTEXT, 0, (LPARAM)WindowTitle);
            UndockedChart::DockWindow(FALSE, TRUE, this);
            return 0;
        }
};

//  ベクトル内での比較するための演算子
inline bool operator==(UndockedChart& a, UndockedChart& b)
{
    return a.ChartWindow == b.ChartWindow;
}

//  プロセス終了時に領域解放されないようにするため（メモリ解放はVC++のランタイムがやってくれる）。
#pragma comment(linker, "/SECTION:.shared,RWS")
#pragma data_seg(".shared")
HMODULE HandleDLLFile = NULL;
std::vector<UndockedChart> UndockCharts;
HWND UndockedChart::MT4TerminalWindow = NULL;
HWND UndockedChart::MDIClientWindow = NULL;
LONG UndockedChart::WndPrcMT4TerminalWindowOld = NULL;
LONG UndockedChart::WndPrcDocumentWindowOld = NULL;
LONG UndockedChart::WndPrcChartWindowOld = NULL;
#pragma data_seg()

//  独自定義の関数
//  色の型変換を行う
COLORREF UndockedChart::ColorToColorRef(DWORD color)
{
    return DWORD((BYTE)color + ((BYTE)((WORD)color >> 8) + (BYTE)(color >> 16)));
}

//  チャートを切り離す・または元に戻す
void UndockedChart::DockWindow(bool Dock, bool ChangeWndPrc, UndockedChart* CurrentUndockedChart)
{
    //  チャートのプロパティを変更する
    if (SetWindowLong(CurrentUndockedChart->ChartWindow, GWL_STYLE, Dock ? WindowPropertyChartWindowDocked : WindowPropertyChartWindowUnDocked) == NULL)
    {
        wsprintf(MessageBuf, TEXT("チャートウィンドウのプロパティ変更ができませんでした。エラーコード:%d, ファイル名:%s, 関数名:%s, 行数:%d"), GetLastError(), TEXT(__FILE__), TEXT(__FUNCTION__), __LINE__);
        MessageBox(NULL, MessageBuf, TEXT("エラー"), MB_OK | MB_ICONERROR);
    }
    //  チャートウィンドウを移動する（親を変更する）
    if (SetParent(CurrentUndockedChart->ChartWindow, Dock ? CurrentUndockedChart->DocumentWindow : GetDesktopWindow()) == NULL)
    {
        wsprintf(MessageBuf, TEXT("ウィンドウの親を変更できませんでした。エラーコード:%d, ファイル名:%s, 関数名:%s, 行数:%d"), GetLastError(), TEXT(__FILE__), TEXT(__FUNCTION__), __LINE__);
        MessageBox(NULL, MessageBuf, TEXT("エラー"), MB_OK | MB_ICONERROR);
    }
    //  チャートウィンドウのウィンドウプロシージャを差し替える
    if (SetWindowLong(CurrentUndockedChart->ChartWindow, GWL_WNDPROC, Dock ? (LONG)WndPrcChartWindowOld : (LONG)WndPrcChartWindow) == NULL)
    {
        wsprintf(MessageBuf, TEXT("チャートウィンドウのウィンドウプロシージャ差し替えが失敗しました。エラーコード:%d, ファイル名:%s, 関数名:%s, 行数:%d"), GetLastError(), TEXT(__FILE__), TEXT(__FUNCTION__), __LINE__);
        MessageBox(NULL, MessageBuf, TEXT("エラー"), MB_OK | MB_ICONERROR);
    }
    //  ドキュメントウィンドウのウィンドウプロシージャを差し替える
    if (ChangeWndPrc)
    {
        if (SetWindowLong(CurrentUndockedChart->DocumentWindow, GWL_WNDPROC, Dock ? (LONG)WndPrcDocumentWindowOld : (LONG)WndPrcDocumentWindow) == NULL)
        {
            wsprintf(MessageBuf, TEXT("ドキュメントウィンドウのウィンドウプロシージャ差し替えが失敗しました。エラーコード:%d, ファイル名:%s, 関数名:%s, 行数:%d"), GetLastError(), TEXT(__FILE__), TEXT(__FUNCTION__), __LINE__);
            MessageBox(NULL, MessageBuf, TEXT("エラー"), MB_OK | MB_ICONERROR);
        }
    }
    //  ドキュメントウィンドウの状態を変更する
    SendMessage(CurrentUndockedChart->DocumentWindow, WM_SYSCOMMAND, Dock ? SC_RESTORE : SC_MINIMIZE, 0);
    if (Dock)
    {
        RECT rect;
        GetClientRect(CurrentUndockedChart->DocumentWindow, &rect);
        MoveWindow(CurrentUndockedChart->ChartWindow, rect.left, rect.top, rect.right, rect.bottom, TRUE);
    }
    else
    {
        MoveWindow(CurrentUndockedChart->ChartWindow, CurrentUndockedChart->RectChartWindow.left, CurrentUndockedChart->RectChartWindow.top, CurrentUndockedChart->RectChartWindow.right, CurrentUndockedChart->RectChartWindow.bottom, TRUE);
    }

    //  ウィンドウを元に戻す、かつウィンドウプロシージャを変更する場合
    //  → このウィンドウの情報はもう使わないのでベクトルから削除する
    if (Dock && ChangeWndPrc)
    {
        //  ベクトルから現在のチャートの情報を削除する
        unsigned int c = 0;
        for (; c < UndockCharts.size(); c++)
        {
            if (CurrentUndockedChart->ChartWindow == UndockCharts[c].ChartWindow) break;
        }
        CurrentUndockedChart->ChartWindow = NULL;
        UndockCharts.erase(UndockCharts.begin() + c);

        //  ベクトルの要素数が0になった場合でもここでライブラリのアンロードは行わない。MT4が終了する際に行う。
        //if (UndockCharts.size() == 0)
        //{
        //      FreeLibrary(HandleDLLFile);
        //}
    }

    return;
}

//  Dockボタン、最小化ボタン、最大化ボタン、閉じるボタンの描画をまとめて行う
    //  ウィンドウプロシージャの複数個所で同じことをするのでここに纏めておく
//  Vista以降のウィンドウマネージャを使いたくないのでここで自前で描画する（できればこの方法はやりたくない…）
void UndockedChart::PaintButtons(UndockedChart* CurrentUndockedChart, RECT* RectButtons)
{
    HDC hdc = GetWindowDC(CurrentUndockedChart->ChartWindow);
    HFONT hFontButton = CreateFont(GetSystemMetrics(SM_CYMENUCHECK), 0, 0, 0, FW_NORMAL, 0, 0, 0, SYMBOL_CHARSET, 0, 0, 0, 0, _T("Marlett"));
    SelectObject(hdc, hFontButton);

    //  ボタンを描画する
    for (int i = 0; i < 4; i++)
    {
        HBRUSH BrushCurrentButton = CreateSolidBrush(ButtonBackgroundColors[i][CurrentUndockedChart->CurrentMouseHitting == (i + 1) ? 0 : 1]);
        FillRect(hdc, &RectButtons[i], BrushCurrentButton);
        SetTextColor(hdc, ButtonCaptionColors[CurrentUndockedChart->IsActive ? 0 : 1]);
        SetBkColor(hdc, ButtonBackgroundColors[i][CurrentUndockedChart->CurrentMouseHitting == (i + 1) ? 0 : 1]);
        TextOut(hdc, RectButtons[i].left + 16, 8 + 2, IsZoomed(CurrentUndockedChart->ChartWindow) ? ButtonIcons[i][1] : ButtonIcons[i][0], 1);
        DeleteObject(BrushCurrentButton);
    }
    SelectObject(hdc, GetStockObject(SYSTEM_FONT));

    ReleaseDC(CurrentUndockedChart->ChartWindow, hdc);
    return;
}

//  差し替え後のウィンドウプロシージャ（MT4ターミナル）
LRESULT CALLBACK UndockedChart::WndPrcMT4TerminalWindow(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //  MT4の閉じるボタンが押された場合、先に切り離したウィンドウを元に戻しておく必要がある。
    //  それ以外は元のウィンドウプロシージャに委ねる。

    //  メッセージの処理
    switch (uMsg)
    {
        case WM_CLOSE:
        {
            //  全てのウィンドウの親を元に戻す
            UndockedChart* CurrentUndockedChart = NULL;
            for (unsigned int c = 0; c < UndockCharts.size(); c++)
            {
                DockWindow(TRUE, TRUE, CurrentUndockedChart);
            }
            FreeLibrary(HandleDLLFile);
            break;
        }
    }
    return CallWindowProc((WNDPROC)UndockedChart::WndPrcMT4TerminalWindowOld, hwnd, uMsg, wParam, lParam);
}

//  差し替え後のウィンドウプロシージャ（ドキュメントウィンドウ）
LRESULT CALLBACK UndockedChart::WndPrcDocumentWindow(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //  ベクトルの中にある自分自身の情報を探す
    UndockedChart* CurrentUndockedChart = NULL;
    for (unsigned int c = 0; c < UndockCharts.size(); c++)
    {
        if (UndockCharts[c].DocumentWindow == hwnd)
        {
            CurrentUndockedChart = &UndockCharts.at(c);
            break;
        }
    }
    if (CurrentUndockedChart == NULL) return NULL;   //  ベクトルにチャートの情報がない（このパターンはあり得ないし、あってもエラー処理のしようがない）

    //  メッセージの処理
    switch (uMsg)
    {
        case WM_LBUTTONDOWN:
        case WM_UNINITMENUPOPUP:
        {
            DockWindow(FALSE, FALSE, CurrentUndockedChart);
            return NULL;
        }
        case WM_DESTROY:
        {
            SendMessage(CurrentUndockedChart->ChartWindow, WM_DOCK_INTERNAL, 0, 0);
            break;
        }
        case WM_CLOSE:
        {
            //  チャートウィンドウに終了通知を送る;
            DockWindow(TRUE, TRUE, CurrentUndockedChart);
            SendMessage(CurrentUndockedChart->ChartWindow, WM_CLOSE_INTERNAL, 0, 0);
            break;
        }
        case WM_SYSCOMMAND:
        {
            switch (wParam)
            {
                case SC_RESTORE:
                case SC_MAXIMIZE:
                case SC_MINIMIZE:
                {
                    if (lParam != 0x00000000)
                    {
                        //  何かしら座標が代入されている＝ユーザーによって押された→無視する
                        return NULL;
                    }
                    else
                    {
                        //  座標が(0, 0)＝人工的に生成されたメッセージ
                        //  ここでは何もしない→デフォルトウィンドウプロシージャに処理させる
                    }
                    break;
                }
            }
            break;
        }
    }
    return CallWindowProc((WNDPROC)UndockedChart::WndPrcDocumentWindowOld, hwnd, uMsg, wParam, lParam);
}

//  差し替え後のウィンドウプロシージャ（チャートウィンドウ）
LRESULT CALLBACK UndockedChart::WndPrcChartWindow(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    RECT WindowRect;
    GetWindowRect(hwnd, &WindowRect);
    POINT WindowEdge = { WindowRect.right, WindowRect.bottom };
    ScreenToClient(hwnd, &WindowEdge);

    //  ベクトルの中にある自分自身の情報を探す
    UndockedChart* CurrentUndockedChart = NULL;
    for (unsigned int c = 0; c < UndockCharts.size(); c++)
    {
        if (UndockCharts[c].ChartWindow == hwnd)
        {
            CurrentUndockedChart = &UndockCharts.at(c);
            break;
        }
    }
    if (CurrentUndockedChart == NULL) return NULL;   //  ベクトルにチャートの情報がない（このパターンはあり得ないし、あってもエラー処理のしようがない）

    //  メッセージの処理
    switch (uMsg)
    {
        case WM_DOCK_INTERNAL:
        {
            //  Dockボタンが押されたときの処理
            UndockedChart::DockWindow(TRUE, TRUE, CurrentUndockedChart);
            return FALSE;
        }
        case WM_CLOSE_INTERNAL:
        case WM_CLOSE:
        {
            //  内々に終了通知がきた or 閉じるボタンが押された
            //  タイトルバーは独自に描画しているので閉じるボタンが押されることはないはずだが、念のため
            //  スロットが全部空いていたらHandleFrameWindowを元に戻す
            UndockedChart::DockWindow(TRUE, TRUE, CurrentUndockedChart);
            
            //  ベクトルから現在のチャートの情報を削除する
            unsigned int c = 0;
            for (; c < UndockCharts.size(); c++)
            {
                if (CurrentUndockedChart->ChartWindow == UndockCharts[c].ChartWindow) break;
            }
            CurrentUndockedChart->ChartWindow = NULL;
            UndockCharts.erase(UndockCharts.begin() + c);
            
            //  ベクトルの要素数が0になった場合、もうUndockチャートは存在しないため、ライブラリをアンロードする
            if (UndockCharts.size() == 0)
            {
                FreeLibrary(HandleDLLFile);
            }
            
            break;
        }
        //case WM_INITMENUPOPUP:
        case WM_RBUTTONDOWN:
        {
            UndockedChart::DockWindow(TRUE, FALSE, CurrentUndockedChart);
            SendMessage(CurrentUndockedChart->DocumentWindow, WM_RBUTTONDOWN, (WPARAM)wParam, lParam);
            break;
        }
        case WM_SETFOCUS:
        {
            CurrentUndockedChart->IsActive = TRUE;
            return FALSE;
        }
        case WM_KILLFOCUS:
        {
            CurrentUndockedChart->IsActive = FALSE;
            return FALSE;
        }
        case WM_NCMOUSEMOVE:
        case WM_MOUSEMOVE:
        {
            //  ボタンを描画する
            RECT RectButtons[4];
            for (int i = 0; i < 4; i++)
            {
                RectButtons[i] = { 0, 0, 48, 32 };
                OffsetRect(&RectButtons[i], WindowEdge.x - (4 - i) * 48, 1);
            }
        
            //  NC領域だとPtInRect関数が正常に動作しないため、自前で判定処理を行う
            POINT CurrentPos;
            GetCursorPos(&CurrentPos);
            CurrentPos.x += WindowFrameSizeVertical;
            ScreenToClient(hwnd, &CurrentPos);
            for (int i = 0; i < 5; i++)
            {
                if (i == 4)
                {
                    if (CurrentUndockedChart->CurrentMouseHitting != 0)
                    {
                        PaintButtons(CurrentUndockedChart, RectButtons);
                    }
                    CurrentUndockedChart->CurrentMouseHitting = 0;
                    break;
                }
                if (RectButtons[i].left <= CurrentPos.x && CurrentPos.x < RectButtons[i].right && CurrentPos.y < 0 && CurrentUndockedChart->CurrentMouseHitting != (i + 1))
                {
                    CurrentUndockedChart->CurrentMouseHitting = i + 1;
                    PaintButtons(CurrentUndockedChart, RectButtons);
                    break;
                }
            }
            break;  //  必ず旧ウィンドウプロシージャに送ること
        }
        case WM_NCLBUTTONDOWN:
        {
            //  左ボタンが押された場合            
            //  ボタンの上で押された場合のみボタンの処理を行う
            //  それ以外の場所で押された場合は旧ウィンドウプロシージャに委ねる
            RECT RectButtons[4];
            for (int i = 0; i < 4; i++)
            {
                RectButtons[i] = { 0, 0, 48, 32 };
                OffsetRect(&RectButtons[i], WindowEdge.x - (4 - i) * 48, 1);
            }
            
            //  NC領域だとPtInRect関数が正常に動作しないため、自前で判定処理を行う
            POINT CurrentPos = { 0 };
            CurrentPos;
            GetCursorPos(&CurrentPos);
            CurrentPos.x += WindowFrameSizeVertical;
            ScreenToClient(hwnd, &CurrentPos);
            for (int i = 0; i < 4; i++)
            {
                if (RectButtons[i].left <= CurrentPos.x && CurrentPos.x < RectButtons[i].right && CurrentPos.y < 0 && CurrentUndockedChart->CurrentMouseHitting != (i + 1))
                {
                    for (int a = 0; a < 2; a++)
                    {
                        if (MessagesButtonClicked[IsZoomed(CurrentUndockedChart->ChartWindow) ? 1 : 0][i][a].Dest == '\0') break;
                        
                        SendMessage(MessagesButtonClicked[IsZoomed(CurrentUndockedChart->ChartWindow) ? 1 : 0][i][a].Dest == 'd' ? CurrentUndockedChart->DocumentWindow : CurrentUndockedChart->ChartWindow,
                            MessagesButtonClicked[IsZoomed(CurrentUndockedChart->ChartWindow) ? 1 : 0][i][a].uMsg,
                            MessagesButtonClicked[IsZoomed(CurrentUndockedChart->ChartWindow) ? 1 : 0][i][a].WParam,
                            MessagesButtonClicked[IsZoomed(CurrentUndockedChart->ChartWindow) ? 1 : 0][i][a].LParam
                        );
                    }
                    return FALSE;
                }
            }
            
            break;  //  return FALSEにするとWM_ENTERSIZEMOVEイベントが発生しなくなるのでサイズ変更ができなくなる
        }
        case WM_NCRBUTTONDOWN:
        {
            //  システムメニューは表示したくないのでここでブロックする
            return TRUE;
        }
        case WM_NCACTIVATE:
        case WM_ACTIVATE:
        {
            switch (wParam)
            {
                case WA_ACTIVE:
                case WA_CLICKACTIVE:
                {
                    CurrentUndockedChart->IsActive = TRUE;
                }
                case WA_INACTIVE:
                {
                    CurrentUndockedChart->IsActive = FALSE;
                }
            }
            return TRUE;
        }
        case WM_NCPAINT:
        {
            //  デフォルトウィンドウプロシージャが1度でもこれらのメッセージを受け取ると（Vista以降の）DWMが有効になってしまうためここでブロックする。
            return FALSE;
        }
        case WM_PAINT:
        {
            //  ウィンドウの描画
            RECT RectButtons[4];
            for (int i = 0; i < 4; i++)
            {
                RectButtons[i] = { 0, 0, 48, 32 };
                OffsetRect(&RectButtons[i], WindowEdge.x - (4 - i) * 48, 1);
            }
            
            HDC hdc = GetWindowDC(hwnd);
            RECT RectWindow;
            GetClientRect(CurrentUndockedChart->ChartWindow, &RectWindow);
            
            //  ウィンドウの背景を描画する
            HBRUSH BrushFrame = CreateSolidBrush(ColorWindowSurface);
            RECT RectTopWindow;
            RectTopWindow.left = 1;
            RectTopWindow.top = 1;
            RectTopWindow.right = RectWindow.right + 2 * WindowFrameSizeVertical - 1;
            RectTopWindow.bottom = WindowFrameSizeTop + 1;
            FillRect(hdc, &RectTopWindow, BrushFrame);
            
            RECT RectLeftWindow;
            RectLeftWindow.left = 1;
            RectLeftWindow.top = RectTopWindow.bottom;
            RectLeftWindow.right = WindowFrameSizeVertical;
            RectLeftWindow.bottom = RectLeftWindow.top + RectWindow.bottom;
            FillRect(hdc, &RectLeftWindow, BrushFrame);
            
            RECT RectRightWindow;
            RectRightWindow.left = RectWindow.right + WindowFrameSizeVertical;
            RectRightWindow.top = RectTopWindow.bottom;
            RectRightWindow.right = RectTopWindow.right;
            RectRightWindow.bottom = RectLeftWindow.bottom;
            FillRect(hdc, &RectRightWindow, BrushFrame);
            
            RECT RectBottomWindow;
            RectBottomWindow.left = RectLeftWindow.left;
            RectBottomWindow.top = RectLeftWindow.bottom;
            RectBottomWindow.right = RectRightWindow.right;
            RectBottomWindow.bottom = RectBottomWindow.top + WindowFrameSizeBottom + 1;
            FillRect(hdc, &RectBottomWindow, BrushFrame);
            
            DeleteObject(BrushFrame);
            
            //  ウィンドウの枠を描画する
            HPEN PenWindowFrame = CreatePen(PS_SOLID, 1, ColorWindowFrame);
            SelectObject(hdc, PenWindowFrame);
            
            RECT RectWindowFrame;
            GetClientRect(CurrentUndockedChart->ChartWindow, &RectWindowFrame);
            RectWindowFrame.left = 0;
            RectWindowFrame.top = 0;
            RectWindowFrame.right += 2 * WindowFrameSizeVertical - 1;
            RectWindowFrame.bottom = RectBottomWindow.bottom - 1;
            MoveToEx(hdc, RectWindowFrame.left, RectWindowFrame.top, NULL);
            LineTo(hdc, RectWindowFrame.left, RectWindowFrame.bottom);
            LineTo(hdc, RectWindowFrame.right, RectWindowFrame.bottom);
            LineTo(hdc, RectWindowFrame.right, RectWindowFrame.top);
            LineTo(hdc, RectWindowFrame.left, RectWindowFrame.top);
            
            DeleteObject(BrushFrame);
            DeleteObject(PenWindowFrame);
            
            //  アプリケーション名を表示する
            SetTextColor(hdc, GetSysColor(CurrentUndockedChart->IsActive ? COLOR_ACTIVECAPTION : COLOR_INACTIVECAPTION));
            HFONT hFont = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, (DEFAULT_PITCH | FF_DONTCARE), TEXT("ＭＳ Gothic"));
            SelectObject(hdc, hFont);
            
            TextOut(hdc, 18, 10, (LPCWSTR)CurrentUndockedChart->WindowTitle, _tcslen(CurrentUndockedChart->WindowTitle));
            DeleteObject(hFont);
            
            //  ウィンドウの上端の丸みを描画する
            MoveToEx(hdc, 1, 2, NULL);
            LineTo(hdc, 1, 4);
            MoveToEx(hdc, 2, 1, NULL);
            LineTo(hdc, 4, 1);
            MoveToEx(hdc, RectTopWindow.right - 1, 2, NULL);
            LineTo(hdc, RectTopWindow.right - 1, 4);
            MoveToEx(hdc, RectTopWindow.right - 2, 1, NULL);
            LineTo(hdc, RectTopWindow.right - 4, 1);
            
            //  ボタンを描画する
            PaintButtons(CurrentUndockedChart, RectButtons);
            break;
        }
        default:
        {
            break;
        }
    }
    
    return CallWindowProc((WNDPROC)UndockedChart::WndPrcChartWindowOld, hwnd, uMsg, wParam, lParam);
}

//  プログラム本体
extern "C" __declspec(dllexport) INT Undock(LPCTSTR WindowTitle, INT Handle)
{
    // 構造体をゼロクリア
    ZeroMemory(&MsgBoxParamError, sizeof MsgBoxParamError);

    // ダイアログボックスの属性
    MsgBoxParamError.cbSize = sizeof MsgBoxParamError;
    MsgBoxParamError.hInstance = NULL;
    MsgBoxParamError.lpszCaption = TEXT("エラー");
    MsgBoxParamError.lpszText = TEXT("(not initialized yet)");
    MsgBoxParamError.dwStyle = MB_OK | MB_ICONERROR;

    //  既に切り離し済みのウィンドウを選択していないかチェックする
    //  多分このルートは通らないと思うが念のため
    for (auto i = UndockCharts.begin(); i != UndockCharts.end(); i++)
    {
        if ((HWND)Handle == i->ChartWindow)
        {
            wsprintf(MessageBuf, TEXT("このチャートウィンドウは既に切り離しが行われています。"));
            MessageBox(NULL, MessageBuf, TEXT("エラー"), MB_OK | MB_ICONERROR);
            return 1;
        }
    }

    //  ライブラリのロード
    if (UndockCharts.size() == 0)
    {
        HandleDLLFile = LoadLibrary(MODULE_NAME);   //  この関数は複数回呼び出しても問題ない
        if (HandleDLLFile == NULL)
        {
            //  このルートにくるのはコンパイラのオプションが不適切だった場合
            //  メモリ不足でここにくることはまずあり得ない
            wsprintf(MessageBuf, TEXT("DLLが呼び出せませんでした。エラーコード:%d, ファイル名:%s, 関数名:%s, 行数:%d"), GetLastError(), TEXT(__FILE__), TEXT(__FUNCTION__), __LINE__);
            MessageBox(NULL, MessageBuf, TEXT("エラー"), MB_OK | MB_ICONERROR);
            return 2;
        }
    }

    //  チャート情報をベクトルに追加する
    UndockedChart udc = UndockedChart(WindowTitle, (HWND)Handle);
    UndockCharts.push_back(udc);

    //  チャートを切り離す
    udc.UndockChartWindow();

    return 0;
}
