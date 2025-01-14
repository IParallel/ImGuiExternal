#pragma once
#include <Windows.h>
#include <string>
#include <d3d9.h>
#include <functional>
#include <imgui.h>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "dwmapi.lib")

typedef struct {
    DWORD R;
    DWORD G;
    DWORD B;
    DWORD A;
} RGBA;

class Overlay
{
public:
    Overlay(const char* processName) : TargetProcess(processName) {}

    DWORD GetProcessId(LPCSTR ProcessName);

    void SetRenderFunction(const std::function<void(Overlay*)>& renderFunction) {
        _renderFunction = renderFunction;
    }

    void SetDrawFunction(const std::function<void(Overlay*)>& drawFunction) {
        _drawFunction = drawFunction;
    }

    void SetPreInitFunction(const std::function<void(Overlay*)> preInit) {
        _preInitFunction = preInit;
    }

    std::string RandomString(int len);
    // Converts a string to UTF-8 encoding
    std::string string_To_UTF8(const std::string& str);

    // Draws stroke text at a specific position
    void DrawStrokeText(int x, int y, RGBA* color, const char* str);

    // Draws plain text at a specific position
    void DrawNewText(int x, int y, RGBA* color, const char* str);

    // Draws a rectangle outline
    void DrawRect(int x, int y, int w, int h, RGBA* color, int thickness);

    // Draws a filled rectangle
    void DrawFilledRect(int x, int y, int w, int h, RGBA* color);

    // Draws a filled circle
    void DrawCircleFilled(int x, int y, int radius, RGBA* color);

    // Draws a circle outline
    void DrawCircle(int x, int y, int radius, RGBA* color, int segments);

    // Draws a triangle outline
    void DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, RGBA* color, float thickne);

    // Draws a filled triangle
    void DrawTriangleFilled(int x1, int y1, int x2, int y2, int x3, int y3, RGBA* color);

    // Draws a line
    void DrawLine(int x1, int y1, int x2, int y2, RGBA* color, int thickness);

    // Draws a cornered box
    void DrawCornerBox(int x, int y, int w, int h, int borderPx, RGBA* color);
    // Loads an image from memory and returns an ImTextureID
    ImTextureID LoadTextureFromMemory(const void* imageData, size_t imageSize);

    void Start();

    bool ShowMenu = true;
private:
    std::function<void(Overlay*)> _renderFunction;
    std::function<void(Overlay*)> _drawFunction;
    std::function<void(Overlay*)> _preInitFunction;
    bool DirectXInit();
    void SetupWindow();
    void InputHandler();
    void Render();
    DWORD __stdcall ProcessCheck(LPVOID lpParameter);
    void MainLoop();
    LPCSTR TargetProcess = "processname.exe";
};

