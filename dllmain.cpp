#include "pch.h"
#include "windows.h"
#include "tchar.h"
#include "vector"

/*
�E�R���p�C�����̒��ӓ_
�]MT4�����[�h�ł���O���̃��C�u������x86�����ł��Bx86�o�C�i���t�@�C���Ƃ��ăR���p�C�����Ă��������B
�� ��L�̓v���W�F�N�g�t�@�C���ɔ��f�ς݂ł��B

�E�⑫
�]Debug�r���h�̏ꍇ�A�`��e�X�g�����˂Ă��邽�߃E�B���h�E�̐F���ʏ�Ƃ͈قȂ������̂ɂȂ�܂��B
*/

#define MT4_EXPFUNC __declspec(dllexport)
#define MODULE_NAME TEXT("UndockLibrary.dll")

MSGBOXPARAMS MsgBoxParamError;  //  ���b�Z�[�W�{�b�N�X�̎g���񂵗p
TCHAR MessageBuf[4096];

//  ���O�Œ�`�����E�B���h�E���b�Z�[�W
#define WM_DOCK_INTERNAL    0xFFF7  //  Dock�ʒm�B�g���Ă��Ȃ����b�Z�[�W�ԍ��𗘗p����
#define WM_CLOSE_INTERNAL   0xFFF8  //  �I���ʒm�B�g���Ă��Ȃ����b�Z�[�W�ԍ��𗘗p����

//  �E�B���h�E�t���[���̐F
#ifdef _DEBUG 
    //  �f�o�b�O���[�h
#define ColorWindowSurface    RGB(0xAF, 0x1F, 0x1F)
#define ColorWindowFrame    RGB(0x00, 0xAF, 0x4F)
#define COLOR_CLOSE_BUTTON_BACKGROUND RGB(0x10, 0x10, 0x7F)
#else
#define ColorWindowSurface    GetSysColor(COLOR_WINDOW)
#define ColorWindowFrame    COLOR_WINDOWFRAME
#define COLOR_CLOSE_BUTTON_BACKGROUND RGB(0xE8, 0x11, 0x43)
#endif

//  �E�B���h�E�v���p�e�B�B�`���[�g��Dock, UnDock�ŕK�v
#define WindowPropertyChartWindowDocked (WS_CHILD | WS_VISIBLE | WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR | WS_EX_CLIENTEDGE)
#define WindowPropertyChartWindowUnDocked (WS_OVERLAPPED | WS_THICKFRAME | WS_CAPTION | WS_VISIBLE | WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR | WS_EX_CLIENTEDGE)

//  �{�^���̃A�C�R��
const TCHAR ButtonIcons[4][2][2] =
{
    //  �ʏ��Ԃ̂Ƃ��ƍő剻�����Ƃ��̃A�C�R��
    {TEXT("u"), TEXT("u")}, //  Dock�{�^��
    {TEXT("0"), TEXT("0")}, //  �ŏ����{�^��
    {TEXT("1"), TEXT("2")}, //  �ő剻�{�^��
    {TEXT("r"), TEXT("r")}  //  ����{�^��
};

//  �{�^���\�ʂ̐F
const COLORREF ButtonBackgroundColors[4][2] =
{
    //  �t�H�[�J�X��ԂƔ�t�H�[�J�X���
    {GetSysColor(COLOR_BTNFACE), GetSysColor(COLOR_WINDOW)},    //  Dock�{�^��
    {GetSysColor(COLOR_BTNFACE), GetSysColor(COLOR_WINDOW)},    //  �ŏ����{�^��
    {GetSysColor(COLOR_BTNFACE), GetSysColor(COLOR_WINDOW)},    //  �ő剻�{�^��
    {COLOR_CLOSE_BUTTON_BACKGROUND, GetSysColor(COLOR_WINDOW)}        //  ����{�^��
};

//  �{�^���̕����F�i�A�N�e�B�u��ԂƔ�A�N�e�B�u��ԁj
const COLORREF ButtonCaptionColors[2] = { GetSysColor(COLOR_ACTIVECAPTION), GetSysColor(COLOR_INACTIVECAPTION) };

//  �E�B���h�E�㕔�̃{�^�����������Ƃ��ɑ��M���郁�b�Z�[�W
struct Messages
{
    char Dest;  //  ���M��̃E�B���h�E�i�h�L�������g�E�B���h�E���`���[�g�E�B���h�E���j
    UINT uMsg;
    WPARAM WParam;
    LPARAM LParam;
};

const Messages MessagesButtonClicked[2][4][2] =
{
    //  ����Dock�{�^���A�ŏ����{�^���A�ő剻�{�^���A����{�^��
    //  ���{�^���ɂ���Ă�2�̃��b�Z�[�W�𑗐M����K�v������
    //  �ʏ�\��
    {
        {{'c', WM_DOCK_INTERNAL, NULL, NULL}, {'\0', NULL, NULL, NULL}},
        {{'c', WM_SYSCOMMAND, SC_MINIMIZE, NULL}, {'\0', NULL, NULL, NULL}},
        {{'c', WM_SYSCOMMAND, SC_MAXIMIZE, NULL}, {'\0', NULL, NULL, NULL}},
        {{'c', WM_DOCK_INTERNAL, NULL, NULL}, {'d', WM_CLOSE, NULL, NULL}}
    },
    //  �ő剻�\��
    {
        {{'c', WM_DOCK_INTERNAL, NULL, NULL}, {'\0', NULL, NULL, NULL}},
        {{'c', WM_SYSCOMMAND, SC_MINIMIZE, NULL}, {'\0', NULL, NULL, NULL}},
        {{'c', WM_SYSCOMMAND, SC_RESTORE, NULL}, {'\0', NULL, NULL, NULL}},
        {{'d', WM_DOCK_INTERNAL, NULL, NULL}, {'d', WM_CLOSE, NULL, NULL}}
    }
};

//  �E�B���h�E�t���[���̃}�[�W���B�E�B���h�E�̕`��ŕK�v
#define WindowFrameSizeVertical 10  //  �E�B���h�E���E�̃}�[�W��
#define WindowFrameSizeTop 32   //  �E�B���h�E�㕔�̃}�[�W���i���^�C�g���o�[�̍����j
#define WindowFrameSizeBottom 9 //  �E�B���h�E�����̃}�[�W��

//  �؂藣�����`���[�g���̂��̂�\���N���X
class UndockedChart
{
    public:
        static HWND MT4TerminalWindow;  //  MT4�̎���
        static HWND MDIClientWindow;    //  MDI�N���C�A���g�E�B���h�E
        static LONG WndPrcMT4TerminalWindowOld;   //  �����ւ��O��MT4�̃E�B���h�E�v���V�[�W��
        static LONG WndPrcDocumentWindowOld;   //  �����ւ��O�̃`���[�g�E�B���h�E�̃E�B���h�E�v���V�[�W��
        static LONG WndPrcChartWindowOld;   //  �����ւ��O�̃`�h�L�������g�E�B���h�E�̃E�B���h�E�v���V�[�W��
        //  ���\�b�h��static�ł��闝�R�͓��ɂȂ��B�A�h���X���L�����Ă��������������\�o�C�g�ߖ�ł��邭�炢�B
        static COLORREF ColorToColorRef(DWORD color);
        static LRESULT CALLBACK WndPrcMT4TerminalWindow(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        static LRESULT CALLBACK WndPrcDocumentWindow(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        static LRESULT CALLBACK WndPrcChartWindow(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        static void PaintButtons(UndockedChart* CurrentUndockedChart, RECT* RectButtons);
        static void DockWindow(bool Dock, bool ChangeWndPrc, UndockedChart* CurrentUndockedChart);
    
    public:
        HWND DocumentWindow;   //  �h�L�������g�E�B���h�E�i�`���[�g�̃t���[���j
        HWND ChartWindow;   //  �`���[�g�̎���
        HWND DockButton;   //  Dock�{�^���̎���
        TCHAR WindowTitle[128];
        int CurrentMouseHitting;    //  ���݂ǂ̃{�^���̏�ɂ��邩��\���B�^�C�g���o�[�̕`��ɕK�v
        RECT RectChartWindow;    //  �؂藣�����Ƃ��̃`���[�g�̈ʒu�ƃT�C�Y
        bool IsActive;  //  ���̃E�B���h�E���A�N�e�B�u���ǂ���
        
        bool operator==(const UndockedChart& x)
        {
            return ChartWindow == x.ChartWindow;
        }
        bool operator==(UndockedChart& x)
        {
            return ChartWindow == x.ChartWindow;
        }
    public:
        //  �R���X�g���N�^
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
        //  �R�s�[�R���X�g���N�^
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
        //  �f�X�g���N�^
        ~UndockedChart()
        {
            //  ���ɓ��I���������̂͂Ȃ�
            return;
        }
        //  �`���[�g�E�B���h�E�̐؂藣�����s��
        int UndockChartWindow()
        {
            SendMessage(ChartWindow, WM_SETTEXT, 0, (LPARAM)WindowTitle);
            UndockedChart::DockWindow(FALSE, TRUE, this);
            return 0;
        }
};

//  �x�N�g�����ł̔�r���邽�߂̉��Z�q
inline bool operator==(UndockedChart& a, UndockedChart& b)
{
    return a.ChartWindow == b.ChartWindow;
}

//  �v���Z�X�I�����ɗ̈�������Ȃ��悤�ɂ��邽�߁i�����������VC++�̃����^�C��������Ă����j�B
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

//  �Ǝ���`�̊֐�
//  �F�̌^�ϊ����s��
COLORREF UndockedChart::ColorToColorRef(DWORD color)
{
    return DWORD((BYTE)color + ((BYTE)((WORD)color >> 8) + (BYTE)(color >> 16)));
}

//  �`���[�g��؂藣���E�܂��͌��ɖ߂�
void UndockedChart::DockWindow(bool Dock, bool ChangeWndPrc, UndockedChart* CurrentUndockedChart)
{
    //  �`���[�g�̃v���p�e�B��ύX����
    if (SetWindowLong(CurrentUndockedChart->ChartWindow, GWL_STYLE, Dock ? WindowPropertyChartWindowDocked : WindowPropertyChartWindowUnDocked) == NULL)
    {
        wsprintf(MessageBuf, TEXT("�`���[�g�E�B���h�E�̃v���p�e�B�ύX���ł��܂���ł����B�G���[�R�[�h:%d, �t�@�C����:%s, �֐���:%s, �s��:%d"), GetLastError(), TEXT(__FILE__), TEXT(__FUNCTION__), __LINE__);
        MessageBox(NULL, MessageBuf, TEXT("�G���["), MB_OK | MB_ICONERROR);
    }
    //  �`���[�g�E�B���h�E���ړ�����i�e��ύX����j
    if (SetParent(CurrentUndockedChart->ChartWindow, Dock ? CurrentUndockedChart->DocumentWindow : GetDesktopWindow()) == NULL)
    {
        wsprintf(MessageBuf, TEXT("�E�B���h�E�̐e��ύX�ł��܂���ł����B�G���[�R�[�h:%d, �t�@�C����:%s, �֐���:%s, �s��:%d"), GetLastError(), TEXT(__FILE__), TEXT(__FUNCTION__), __LINE__);
        MessageBox(NULL, MessageBuf, TEXT("�G���["), MB_OK | MB_ICONERROR);
    }
    //  �`���[�g�E�B���h�E�̃E�B���h�E�v���V�[�W���������ւ���
    if (SetWindowLong(CurrentUndockedChart->ChartWindow, GWL_WNDPROC, Dock ? (LONG)WndPrcChartWindowOld : (LONG)WndPrcChartWindow) == NULL)
    {
        wsprintf(MessageBuf, TEXT("�`���[�g�E�B���h�E�̃E�B���h�E�v���V�[�W�������ւ������s���܂����B�G���[�R�[�h:%d, �t�@�C����:%s, �֐���:%s, �s��:%d"), GetLastError(), TEXT(__FILE__), TEXT(__FUNCTION__), __LINE__);
        MessageBox(NULL, MessageBuf, TEXT("�G���["), MB_OK | MB_ICONERROR);
    }
    //  �h�L�������g�E�B���h�E�̃E�B���h�E�v���V�[�W���������ւ���
    if (ChangeWndPrc)
    {
        if (SetWindowLong(CurrentUndockedChart->DocumentWindow, GWL_WNDPROC, Dock ? (LONG)WndPrcDocumentWindowOld : (LONG)WndPrcDocumentWindow) == NULL)
        {
            wsprintf(MessageBuf, TEXT("�h�L�������g�E�B���h�E�̃E�B���h�E�v���V�[�W�������ւ������s���܂����B�G���[�R�[�h:%d, �t�@�C����:%s, �֐���:%s, �s��:%d"), GetLastError(), TEXT(__FILE__), TEXT(__FUNCTION__), __LINE__);
            MessageBox(NULL, MessageBuf, TEXT("�G���["), MB_OK | MB_ICONERROR);
        }
    }
    //  �h�L�������g�E�B���h�E�̏�Ԃ�ύX����
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

    //  �E�B���h�E�����ɖ߂��A���E�B���h�E�v���V�[�W����ύX����ꍇ
    //  �� ���̃E�B���h�E�̏��͂����g��Ȃ��̂Ńx�N�g������폜����
    if (Dock && ChangeWndPrc)
    {
        //  �x�N�g�����猻�݂̃`���[�g�̏����폜����
        unsigned int c = 0;
        for (; c < UndockCharts.size(); c++)
        {
            if (CurrentUndockedChart->ChartWindow == UndockCharts[c].ChartWindow) break;
        }
        CurrentUndockedChart->ChartWindow = NULL;
        UndockCharts.erase(UndockCharts.begin() + c);

        //  �x�N�g���̗v�f����0�ɂȂ����ꍇ�ł������Ń��C�u�����̃A�����[�h�͍s��Ȃ��BMT4���I������ۂɍs���B
        //if (UndockCharts.size() == 0)
        //{
        //      FreeLibrary(HandleDLLFile);
        //}
    }

    return;
}

//  Dock�{�^���A�ŏ����{�^���A�ő剻�{�^���A����{�^���̕`����܂Ƃ߂čs��
    //  �E�B���h�E�v���V�[�W���̕������œ������Ƃ�����̂ł����ɓZ�߂Ă���
//  Vista�ȍ~�̃E�B���h�E�}�l�[�W�����g�������Ȃ��̂ł����Ŏ��O�ŕ`�悷��i�ł���΂��̕��@�͂�肽���Ȃ��c�j
void UndockedChart::PaintButtons(UndockedChart* CurrentUndockedChart, RECT* RectButtons)
{
    HDC hdc = GetWindowDC(CurrentUndockedChart->ChartWindow);
    HFONT hFontButton = CreateFont(GetSystemMetrics(SM_CYMENUCHECK), 0, 0, 0, FW_NORMAL, 0, 0, 0, SYMBOL_CHARSET, 0, 0, 0, 0, _T("Marlett"));
    SelectObject(hdc, hFontButton);

    //  �{�^����`�悷��
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

//  �����ւ���̃E�B���h�E�v���V�[�W���iMT4�^�[�~�i���j
LRESULT CALLBACK UndockedChart::WndPrcMT4TerminalWindow(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //  MT4�̕���{�^���������ꂽ�ꍇ�A��ɐ؂藣�����E�B���h�E�����ɖ߂��Ă����K�v������B
    //  ����ȊO�͌��̃E�B���h�E�v���V�[�W���Ɉς˂�B

    //  ���b�Z�[�W�̏���
    switch (uMsg)
    {
        case WM_CLOSE:
        {
            //  �S�ẴE�B���h�E�̐e�����ɖ߂�
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

//  �����ւ���̃E�B���h�E�v���V�[�W���i�h�L�������g�E�B���h�E�j
LRESULT CALLBACK UndockedChart::WndPrcDocumentWindow(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //  �x�N�g���̒��ɂ��鎩�����g�̏���T��
    UndockedChart* CurrentUndockedChart = NULL;
    for (unsigned int c = 0; c < UndockCharts.size(); c++)
    {
        if (UndockCharts[c].DocumentWindow == hwnd)
        {
            CurrentUndockedChart = &UndockCharts.at(c);
            break;
        }
    }
    if (CurrentUndockedChart == NULL) return NULL;   //  �x�N�g���Ƀ`���[�g�̏�񂪂Ȃ��i���̃p�^�[���͂��蓾�Ȃ����A�����Ă��G���[�����̂��悤���Ȃ��j

    //  ���b�Z�[�W�̏���
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
            //  �`���[�g�E�B���h�E�ɏI���ʒm�𑗂�;
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
                        //  ����������W���������Ă��遁���[�U�[�ɂ���ĉ����ꂽ����������
                        return NULL;
                    }
                    else
                    {
                        //  ���W��(0, 0)���l�H�I�ɐ������ꂽ���b�Z�[�W
                        //  �����ł͉������Ȃ����f�t�H���g�E�B���h�E�v���V�[�W���ɏ���������
                    }
                    break;
                }
            }
            break;
        }
    }
    return CallWindowProc((WNDPROC)UndockedChart::WndPrcDocumentWindowOld, hwnd, uMsg, wParam, lParam);
}

//  �����ւ���̃E�B���h�E�v���V�[�W���i�`���[�g�E�B���h�E�j
LRESULT CALLBACK UndockedChart::WndPrcChartWindow(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    RECT WindowRect;
    GetWindowRect(hwnd, &WindowRect);
    POINT WindowEdge = { WindowRect.right, WindowRect.bottom };
    ScreenToClient(hwnd, &WindowEdge);

    //  �x�N�g���̒��ɂ��鎩�����g�̏���T��
    UndockedChart* CurrentUndockedChart = NULL;
    for (unsigned int c = 0; c < UndockCharts.size(); c++)
    {
        if (UndockCharts[c].ChartWindow == hwnd)
        {
            CurrentUndockedChart = &UndockCharts.at(c);
            break;
        }
    }
    if (CurrentUndockedChart == NULL) return NULL;   //  �x�N�g���Ƀ`���[�g�̏�񂪂Ȃ��i���̃p�^�[���͂��蓾�Ȃ����A�����Ă��G���[�����̂��悤���Ȃ��j

    //  ���b�Z�[�W�̏���
    switch (uMsg)
    {
        case WM_DOCK_INTERNAL:
        {
            //  Dock�{�^���������ꂽ�Ƃ��̏���
            UndockedChart::DockWindow(TRUE, TRUE, CurrentUndockedChart);
            return FALSE;
        }
        case WM_CLOSE_INTERNAL:
        case WM_CLOSE:
        {
            //  ���X�ɏI���ʒm������ or ����{�^���������ꂽ
            //  �^�C�g���o�[�͓Ǝ��ɕ`�悵�Ă���̂ŕ���{�^����������邱�Ƃ͂Ȃ��͂������A�O�̂���
            //  �X���b�g���S���󂢂Ă�����HandleFrameWindow�����ɖ߂�
            UndockedChart::DockWindow(TRUE, TRUE, CurrentUndockedChart);
            
            //  �x�N�g�����猻�݂̃`���[�g�̏����폜����
            unsigned int c = 0;
            for (; c < UndockCharts.size(); c++)
            {
                if (CurrentUndockedChart->ChartWindow == UndockCharts[c].ChartWindow) break;
            }
            CurrentUndockedChart->ChartWindow = NULL;
            UndockCharts.erase(UndockCharts.begin() + c);
            
            //  �x�N�g���̗v�f����0�ɂȂ����ꍇ�A����Undock�`���[�g�͑��݂��Ȃ����߁A���C�u�������A�����[�h����
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
            //  �{�^����`�悷��
            RECT RectButtons[4];
            for (int i = 0; i < 4; i++)
            {
                RectButtons[i] = { 0, 0, 48, 32 };
                OffsetRect(&RectButtons[i], WindowEdge.x - (4 - i) * 48, 1);
            }
        
            //  NC�̈悾��PtInRect�֐�������ɓ��삵�Ȃ����߁A���O�Ŕ��菈�����s��
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
            break;  //  �K�����E�B���h�E�v���V�[�W���ɑ��邱��
        }
        case WM_NCLBUTTONDOWN:
        {
            //  ���{�^���������ꂽ�ꍇ            
            //  �{�^���̏�ŉ����ꂽ�ꍇ�̂݃{�^���̏������s��
            //  ����ȊO�̏ꏊ�ŉ����ꂽ�ꍇ�͋��E�B���h�E�v���V�[�W���Ɉς˂�
            RECT RectButtons[4];
            for (int i = 0; i < 4; i++)
            {
                RectButtons[i] = { 0, 0, 48, 32 };
                OffsetRect(&RectButtons[i], WindowEdge.x - (4 - i) * 48, 1);
            }
            
            //  NC�̈悾��PtInRect�֐�������ɓ��삵�Ȃ����߁A���O�Ŕ��菈�����s��
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
            
            break;  //  return FALSE�ɂ����WM_ENTERSIZEMOVE�C�x���g���������Ȃ��Ȃ�̂ŃT�C�Y�ύX���ł��Ȃ��Ȃ�
        }
        case WM_NCRBUTTONDOWN:
        {
            //  �V�X�e�����j���[�͕\���������Ȃ��̂ł����Ńu���b�N����
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
            //  �f�t�H���g�E�B���h�E�v���V�[�W����1�x�ł������̃��b�Z�[�W���󂯎��ƁiVista�ȍ~�́jDWM���L���ɂȂ��Ă��܂����߂����Ńu���b�N����B
            return FALSE;
        }
        case WM_PAINT:
        {
            //  �E�B���h�E�̕`��
            RECT RectButtons[4];
            for (int i = 0; i < 4; i++)
            {
                RectButtons[i] = { 0, 0, 48, 32 };
                OffsetRect(&RectButtons[i], WindowEdge.x - (4 - i) * 48, 1);
            }
            
            HDC hdc = GetWindowDC(hwnd);
            RECT RectWindow;
            GetClientRect(CurrentUndockedChart->ChartWindow, &RectWindow);
            
            //  �E�B���h�E�̔w�i��`�悷��
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
            
            //  �E�B���h�E�̘g��`�悷��
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
            
            //  �A�v���P�[�V��������\������
            SetTextColor(hdc, GetSysColor(CurrentUndockedChart->IsActive ? COLOR_ACTIVECAPTION : COLOR_INACTIVECAPTION));
            HFONT hFont = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, (DEFAULT_PITCH | FF_DONTCARE), TEXT("�l�r Gothic"));
            SelectObject(hdc, hFont);
            
            TextOut(hdc, 18, 10, (LPCWSTR)CurrentUndockedChart->WindowTitle, _tcslen(CurrentUndockedChart->WindowTitle));
            DeleteObject(hFont);
            
            //  �E�B���h�E�̏�[�̊ۂ݂�`�悷��
            MoveToEx(hdc, 1, 2, NULL);
            LineTo(hdc, 1, 4);
            MoveToEx(hdc, 2, 1, NULL);
            LineTo(hdc, 4, 1);
            MoveToEx(hdc, RectTopWindow.right - 1, 2, NULL);
            LineTo(hdc, RectTopWindow.right - 1, 4);
            MoveToEx(hdc, RectTopWindow.right - 2, 1, NULL);
            LineTo(hdc, RectTopWindow.right - 4, 1);
            
            //  �{�^����`�悷��
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

//  �v���O�����{��
extern "C" __declspec(dllexport) INT Undock(LPCTSTR WindowTitle, INT Handle)
{
    // �\���̂��[���N���A
    ZeroMemory(&MsgBoxParamError, sizeof MsgBoxParamError);

    // �_�C�A���O�{�b�N�X�̑���
    MsgBoxParamError.cbSize = sizeof MsgBoxParamError;
    MsgBoxParamError.hInstance = NULL;
    MsgBoxParamError.lpszCaption = TEXT("�G���[");
    MsgBoxParamError.lpszText = TEXT("(not initialized yet)");
    MsgBoxParamError.dwStyle = MB_OK | MB_ICONERROR;

    //  ���ɐ؂藣���ς݂̃E�B���h�E��I�����Ă��Ȃ����`�F�b�N����
    //  �������̃��[�g�͒ʂ�Ȃ��Ǝv�����O�̂���
    for (auto i = UndockCharts.begin(); i != UndockCharts.end(); i++)
    {
        if ((HWND)Handle == i->ChartWindow)
        {
            wsprintf(MessageBuf, TEXT("���̃`���[�g�E�B���h�E�͊��ɐ؂藣�����s���Ă��܂��B"));
            MessageBox(NULL, MessageBuf, TEXT("�G���["), MB_OK | MB_ICONERROR);
            return 1;
        }
    }

    //  ���C�u�����̃��[�h
    if (UndockCharts.size() == 0)
    {
        HandleDLLFile = LoadLibrary(MODULE_NAME);   //  ���̊֐��͕�����Ăяo���Ă����Ȃ�
        if (HandleDLLFile == NULL)
        {
            //  ���̃��[�g�ɂ���̂̓R���p�C���̃I�v�V�������s�K�؂������ꍇ
            //  �������s���ł����ɂ��邱�Ƃ͂܂����蓾�Ȃ�
            wsprintf(MessageBuf, TEXT("DLL���Ăяo���܂���ł����B�G���[�R�[�h:%d, �t�@�C����:%s, �֐���:%s, �s��:%d"), GetLastError(), TEXT(__FILE__), TEXT(__FUNCTION__), __LINE__);
            MessageBox(NULL, MessageBuf, TEXT("�G���["), MB_OK | MB_ICONERROR);
            return 2;
        }
    }

    //  �`���[�g�����x�N�g���ɒǉ�����
    UndockedChart udc = UndockedChart(WindowTitle, (HWND)Handle);
    UndockCharts.push_back(udc);

    //  �`���[�g��؂藣��
    udc.UndockChartWindow();

    return 0;
}
