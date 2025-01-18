#include <windows.h>
#include <iostream>
#include <tlhelp32.h>
#include <psapi.h>
#include <dwmapi.h>
#include <d3d9.h>
#include <thread>
#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>
#include <d3dx9tex.h>
#include <functional>
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "dwmapi.lib")


static DWORD GetProcessId(LPCSTR ProcessName) {
    PROCESSENTRY32 pt;
    HANDLE hsnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    pt.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(hsnap, &pt)) {
        do {
            if (!lstrcmpi(pt.szExeFile, ProcessName)) {
                CloseHandle(hsnap);
                return pt.th32ProcessID;
            }
        } while (Process32Next(hsnap, &pt));
    }
    CloseHandle(hsnap);
    return 0;
}

static std::string RandomString(int len) {
    srand(time(NULL));
    std::string str = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::string newstr;
    int pos;
    while (newstr.size() != len) {
        pos = ((rand() % (str.size() - 1)));
        newstr += str.substr(pos, 1);
    }
    return newstr;
}


typedef struct
{
    DWORD R;
    DWORD G;
    DWORD B;
    DWORD A;
}RGBA;

struct CurrentProcess {
    DWORD ID;
    HANDLE Handle;
    HWND Hwnd;
    WNDPROC WndProc;
    int WindowWidth;
    int WindowHeight;
    int WindowLeft;
    int WindowRight;
    int WindowTop;
    int WindowBottom;
    LPCSTR Title;
    LPCSTR ClassName;
    LPCSTR Path;
} inline Process;

struct OverlayWindow {
    WNDCLASSEX WindowClass;
    HWND Hwnd;
    LPCSTR Name;
} inline Overlay;

struct DirectX9Interface {
    IDirect3D9Ex* IDirect3D9 = NULL;
    IDirect3DDevice9Ex* pDevice = NULL;
    D3DPRESENT_PARAMETERS pParameters = { NULL };
    MARGINS Margin = { -1 };
    MSG Message = { NULL };
} inline DirectX9;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK WinProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, Message, wParam, lParam))
        return true;

    switch (Message) {
    case WM_DESTROY:
        if (DirectX9.pDevice != NULL) {
            DirectX9.pDevice->EndScene();
            DirectX9.pDevice->Release();
        }
        if (DirectX9.IDirect3D9 != NULL) {
            DirectX9.IDirect3D9->Release();
        }
        PostQuitMessage(0);
        exit(4);
        break;
    case WM_SIZE:
        if (DirectX9.pDevice != NULL && wParam != SIZE_MINIMIZED) {
            ImGui_ImplDX9_InvalidateDeviceObjects();
            DirectX9.pParameters.BackBufferWidth = LOWORD(lParam);
            DirectX9.pParameters.BackBufferHeight = HIWORD(lParam);
            HRESULT hr = DirectX9.pDevice->Reset(&DirectX9.pParameters);
            if (hr == D3DERR_INVALIDCALL)
                IM_ASSERT(0);
            ImGui_ImplDX9_CreateDeviceObjects();
        }
        break;
    default:
        return DefWindowProc(hWnd, Message, wParam, lParam);
        break;
    }
    return 0;
}

class OverlayApp {
private:
    LPCSTR TargetProcess;
    bool ShowMenu;
    bool CreateConsole;

    void InputHandler() {
        for (int i = 0; i < 5; i++) {
            ImGui::GetIO().MouseDown[i] = false;
        }

        int Button = -1;
        if (GetAsyncKeyState(VK_LBUTTON)) {
            Button = 0;
        }

        if (Button != -1) {
            ImGui::GetIO().MouseDown[Button] = true;
        }
    }

    void Draw() {
        char FpsInfo[64];
        sprintf(FpsInfo, "Overlay FPS: %0.f", ImGui::GetIO().Framerate);
        //RGBA White = { 255,255,255,255 };
        //DrawStrokeText(30, 44, &White, FpsInfo);
    }

    void Render() {
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        //Draw();
        ImGui::GetIO().MouseDrawCursor = ShowMenu;

        static bool isMenuVisible = false;

        if (ShowMenu != isMenuVisible) {
            if (ShowMenu) {
                // Set window to non-transparent
                SetWindowLong(Overlay.Hwnd, GWL_EXSTYLE, WS_EX_TOOLWINDOW);
                SetFocus(Overlay.Hwnd);
            }
            else {
                // Set window to transparent
                SetWindowLong(Overlay.Hwnd, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW);
            }

            UpdateWindow(Overlay.Hwnd); // Only update when the style changes
            isMenuVisible = ShowMenu;  // Update the tracking flag
        }

        if (ShowMenu) {
            InputHandler();

            if (_renderFunction)
                _renderFunction(this);

            if (_drawFunction)
                _drawFunction(this);
        }
        ImGui::EndFrame();

        DirectX9.pDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
        if (DirectX9.pDevice->BeginScene() >= 0) {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            DirectX9.pDevice->EndScene();
        }

        HRESULT result = DirectX9.pDevice->Present(NULL, NULL, NULL, NULL);
        if (result == D3DERR_DEVICELOST && DirectX9.pDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) {
            ImGui_ImplDX9_InvalidateDeviceObjects();
            DirectX9.pDevice->Reset(&DirectX9.pParameters);
            ImGui_ImplDX9_CreateDeviceObjects();
        }
    }

    void checkIfMenu() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            if (GetAsyncKeyState(VK_INSERT)) {
                ShowMenu = !ShowMenu;
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        }
    }

    void MainLoop() {
        static RECT OldRect;
        ZeroMemory(&DirectX9.Message, sizeof(MSG));
        std::thread(&OverlayApp::checkIfMenu, this).detach();
        ImGui::GetIO().IniFilename = NULL;
        while (DirectX9.Message.message != WM_QUIT) {
            if (PeekMessage(&DirectX9.Message, Overlay.Hwnd, 0, 0, PM_REMOVE)) {
                TranslateMessage(&DirectX9.Message);
                DispatchMessage(&DirectX9.Message);
            }
            HWND ForegroundWindow = GetForegroundWindow();
            if (ForegroundWindow == Process.Hwnd) {
                HWND TempProcessHwnd = GetWindow(ForegroundWindow, GW_HWNDPREV);
                SetWindowPos(Overlay.Hwnd, TempProcessHwnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            }

            RECT TempRect;
            POINT TempPoint;
            ZeroMemory(&TempRect, sizeof(RECT));
            ZeroMemory(&TempPoint, sizeof(POINT));

            GetClientRect(Process.Hwnd, &TempRect);
            ClientToScreen(Process.Hwnd, &TempPoint);

            TempRect.left = TempPoint.x;
            TempRect.top = TempPoint.y;
            ImGuiIO& io = ImGui::GetIO();

            if (TempRect.left != OldRect.left || TempRect.right != OldRect.right || TempRect.top != OldRect.top || TempRect.bottom != OldRect.bottom) {
                OldRect = TempRect;
                Process.WindowWidth = TempRect.right;
                Process.WindowHeight = TempRect.bottom;
                windowSize = { Process.WindowWidth, Process.WindowHeight };
                DirectX9.pParameters.BackBufferWidth = Process.WindowWidth;
                DirectX9.pParameters.BackBufferHeight = Process.WindowHeight;
                SetWindowPos(Overlay.Hwnd, (HWND)0, TempPoint.x, TempPoint.y, Process.WindowWidth, Process.WindowHeight, SWP_NOREDRAW);
                DirectX9.pDevice->Reset(&DirectX9.pParameters);
            }
            Render();
        }
        ImGui_ImplDX9_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        if (DirectX9.pDevice != NULL) {
            DirectX9.pDevice->EndScene();
            DirectX9.pDevice->Release();
        }
        if (DirectX9.IDirect3D9 != NULL) {
            DirectX9.IDirect3D9->Release();
        }
        DestroyWindow(Overlay.Hwnd);
        UnregisterClass(Overlay.WindowClass.lpszClassName, Overlay.WindowClass.hInstance);
    }

    bool DirectXInit() {
        if (FAILED(Direct3DCreate9Ex(D3D_SDK_VERSION, &DirectX9.IDirect3D9))) {
            return false;
        }

        D3DPRESENT_PARAMETERS Params = { 0 };
        Params.Windowed = TRUE;
        Params.SwapEffect = D3DSWAPEFFECT_DISCARD;
        Params.hDeviceWindow = Overlay.Hwnd;
        Params.MultiSampleQuality = D3DMULTISAMPLE_NONE;
        Params.BackBufferFormat = D3DFMT_A8R8G8B8;
        Params.BackBufferWidth = Process.WindowWidth;
        Params.BackBufferHeight = Process.WindowHeight;
        Params.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
        Params.EnableAutoDepthStencil = TRUE;
        Params.AutoDepthStencilFormat = D3DFMT_D16;
        Params.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
        Params.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
        if (FAILED(DirectX9.IDirect3D9->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, Overlay.Hwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &Params, 0, &DirectX9.pDevice))) {
            DirectX9.IDirect3D9->Release();
            return false;
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGui_ImplWin32_Init(Overlay.Hwnd);
        ImGui_ImplDX9_Init(DirectX9.pDevice);
        DirectX9.IDirect3D9->Release();
        return true;
    }

    void SetupWindow() {
        Overlay.WindowClass = {
            sizeof(WNDCLASSEX), 0, WinProc, 0, 0, nullptr, LoadIcon(nullptr, IDI_APPLICATION), LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr, Overlay.Name, LoadIcon(nullptr, IDI_APPLICATION)
        };

        RegisterClassEx(&Overlay.WindowClass);
        if (Process.Hwnd) {
            static RECT TempRect = { NULL };
            static POINT TempPoint;
            GetClientRect(Process.Hwnd, &TempRect);
            ClientToScreen(Process.Hwnd, &TempPoint);
            TempRect.left = TempPoint.x;
            TempRect.top = TempPoint.y;
            Process.WindowWidth = TempRect.right;
            Process.WindowHeight = TempRect.bottom;
            windowSize = { Process.WindowWidth, Process.WindowHeight };
        }

        Overlay.Hwnd = CreateWindowEx(NULL, Overlay.Name, Overlay.Name, WS_POPUP | WS_VISIBLE, Process.WindowLeft, Process.WindowTop, Process.WindowWidth, Process.WindowHeight, NULL, NULL, 0, NULL);
        DwmExtendFrameIntoClientArea(Overlay.Hwnd, &DirectX9.Margin);
        SetWindowLong(Overlay.Hwnd, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW);
        ShowWindow(Overlay.Hwnd, SW_SHOW);
        UpdateWindow(Overlay.Hwnd);
    }

    DWORD WINAPI ProcessCheck() const {
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            if (Process.Hwnd != NULL) {
                if (GetProcessId(TargetProcess) == 0) {
                    exit(0);
                }
            }
        }
    }

    std::function<void(OverlayApp*)> _renderFunction;
    std::function<void(OverlayApp*)> _drawFunction;
    std::function<void(OverlayApp*)> _preInitFunction;

public:
    POINT windowSize;

    OverlayApp(LPCSTR targetProcess, bool showMenu = true, bool createConsole = true) :
        TargetProcess(targetProcess), ShowMenu(showMenu), CreateConsole(createConsole), windowSize() {
    }

    void SetRenderFunction(const std::function<void(OverlayApp*)>& renderFunction) {
        _renderFunction = renderFunction;
    }

    void SetDrawFunction(const std::function<void(OverlayApp*)>& drawFunction) {
        _drawFunction = drawFunction;
    }

    void SetPreInitFunction(const std::function<void(OverlayApp*)> preInit) {
        _preInitFunction = preInit;
    }

    ImTextureID LoadTextureFromMemory(const void* imageData, size_t imageSize) {
        if (!DirectX9.pDevice || !imageData || imageSize == 0) {
            throw std::invalid_argument("Invalid parameters passed to LoadTextureFromMemory.");
        }

        // Create a DirectX9 texture from the image data in memory
        LPDIRECT3DTEXTURE9 texture = nullptr;
        HRESULT result = D3DXCreateTextureFromFileInMemory(DirectX9.pDevice, imageData, static_cast<UINT>(imageSize), &texture);

        if (FAILED(result)) {
            throw std::runtime_error("Failed to create texture from memory.");
        }

        // Return the texture as an ImTextureID
        return reinterpret_cast<ImTextureID>(texture);
    }

    void Run() {
        if (CreateConsole == false)
            ShowWindow(GetConsoleWindow(), SW_HIDE);

        bool WindowFocus = false;
        while (WindowFocus == false) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            DWORD ForegroundWindowProcessID;
            GetWindowThreadProcessId(GetForegroundWindow(), &ForegroundWindowProcessID);
            if (GetProcessId(TargetProcess) == ForegroundWindowProcessID) {
                Process.ID = GetCurrentProcessId();
                Process.Handle = GetCurrentProcess();
                Process.Hwnd = GetForegroundWindow();

                RECT TempRect;
                GetWindowRect(Process.Hwnd, &TempRect);
                Process.WindowWidth = TempRect.right - TempRect.left;
                Process.WindowHeight = TempRect.bottom - TempRect.top;
                Process.WindowLeft = TempRect.left;
                Process.WindowRight = TempRect.right;
                Process.WindowTop = TempRect.top;
                Process.WindowBottom = TempRect.bottom;

                char TempTitle[MAX_PATH];
                GetWindowText(Process.Hwnd, TempTitle, sizeof(TempTitle));
                Process.Title = TempTitle;

                char TempClassName[MAX_PATH];
                GetClassName(Process.Hwnd, TempClassName, sizeof(TempClassName));
                Process.ClassName = TempClassName;

                char TempPath[MAX_PATH];
                GetModuleFileNameEx(Process.Handle, NULL, TempPath, sizeof(TempPath));
                Process.Path = TempPath;

                WindowFocus = true;
            }
        }

        Overlay.Name = RandomString(10).c_str();
        SetupWindow();
        DirectXInit();

        if (_preInitFunction)
            _preInitFunction(this);

        std::thread(&OverlayApp::ProcessCheck, this).detach();
        while (TRUE) {
            MainLoop();
        }
    }

    std::string string_To_UTF8(const std::string& str) {
        int nwLen = ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);
        wchar_t* pwBuf = new wchar_t[nwLen + 1];
        ZeroMemory(pwBuf, nwLen * 2 + 2);
        ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.length(), pwBuf, nwLen);
        int nLen = ::WideCharToMultiByte(CP_UTF8, 0, pwBuf, -1, NULL, NULL, NULL, NULL);
        char* pBuf = new char[nLen + 1];
        ZeroMemory(pBuf, nLen + 1);
        ::WideCharToMultiByte(CP_UTF8, 0, pwBuf, nwLen, pBuf, nLen, NULL, NULL);
        std::string retStr(pBuf);
        delete[]pwBuf;
        delete[]pBuf;
        pwBuf = NULL;
        pBuf = NULL;
        return retStr;
    }

    void DrawStrokeText(int x, int y, RGBA* color, const char* str) {
        ImFont a;
        std::string utf_8_1 = std::string(str);
        std::string utf_8_2 = string_To_UTF8(utf_8_1);
        ImGui::GetForegroundDrawList()->AddText(ImVec2(x, y - 1), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), utf_8_2.c_str());
        ImGui::GetForegroundDrawList()->AddText(ImVec2(x, y + 1), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), utf_8_2.c_str());
        ImGui::GetForegroundDrawList()->AddText(ImVec2(x - 1, y), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), utf_8_2.c_str());
        ImGui::GetForegroundDrawList()->AddText(ImVec2(x + 1, y), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), utf_8_2.c_str());
        ImGui::GetForegroundDrawList()->AddText(ImVec2(x, y), ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)), utf_8_2.c_str());
    }

    void DrawNewText(int x, int y, RGBA* color, const char* str) {
        ImFont a;
        std::string utf_8_1 = std::string(str);
        std::string utf_8_2 = string_To_UTF8(utf_8_1);
        ImGui::GetForegroundDrawList()->AddText(ImVec2(x, y), ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)), utf_8_2.c_str());
    }

    void DrawRect(int x, int y, int w, int h, RGBA* color, int thickness) {
        ImGui::GetForegroundDrawList()->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)), 0, 0, thickness);
    }

    void DrawFilledRect(int x, int y, int w, int h, RGBA* color) {
        ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)), 0, 0);
    }

    void DrawCircleFilled(int x, int y, int radius, RGBA* color) {
        ImGui::GetForegroundDrawList()->AddCircleFilled(ImVec2(x, y), radius, ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)));
    }

    void DrawCircle(int x, int y, int radius, RGBA* color, int segments) {
        ImGui::GetForegroundDrawList()->AddCircle(ImVec2(x, y), radius, ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)), segments);
    }

    void DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, RGBA* color, float thickne) {
        ImGui::GetForegroundDrawList()->AddTriangle(ImVec2(x1, y1), ImVec2(x2, y2), ImVec2(x3, y3), ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)), thickne);
    }

    void DrawTriangleFilled(int x1, int y1, int x2, int y2, int x3, int y3, RGBA* color) {
        ImGui::GetForegroundDrawList()->AddTriangleFilled(ImVec2(x1, y1), ImVec2(x2, y2), ImVec2(x3, y3), ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)));
    }

    void DrawLine(int x1, int y1, int x2, int y2, RGBA* color, int thickness) {
        ImGui::GetForegroundDrawList()->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)), thickness);
    }

    void DrawCornerBox(int x, int y, int w, int h, int borderPx, RGBA* color) {
        DrawFilledRect(x + borderPx, y, w / 3, borderPx, color);
        DrawFilledRect(x + w - w / 3 + borderPx, y, w / 3, borderPx, color);
        DrawFilledRect(x, y, borderPx, h / 3, color);
        DrawFilledRect(x, y + h - h / 3 + borderPx * 2, borderPx, h / 3, color);
        DrawFilledRect(x + borderPx, y + h + borderPx, w / 3, borderPx, color);
        DrawFilledRect(x + w - w / 3 + borderPx, y + h + borderPx, w / 3, borderPx, color);
        DrawFilledRect(x + w + borderPx, y, borderPx, h / 3, color);
        DrawFilledRect(x + w + borderPx, y + h - h / 3 + borderPx * 2, borderPx, h / 3, color);
    }
};
