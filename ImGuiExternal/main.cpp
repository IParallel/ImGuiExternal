#include <windows.h>
#include <iostream>
#include <tlhelp32.h>
#include <psapi.h>
#include <dwmapi.h>
#include <d3d9.h>
#include <thread>
#include "src/Overlay.h"
#include <imgui.h>
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "dwmapi.lib")

bool CreateConsole = true;

void Render(Overlay* overlay) {
	ImGui::Begin("Tetas");
	ImGui::Text("Tetas");
	ImGui::End();
}

void Draw(Overlay* overlay) {
	char FpsInfo[64];
	sprintf(FpsInfo, "Overlay FPS: %0.f", ImGui::GetIO().Framerate);
	RGBA White = { 255,255,255,255 };
	overlay->DrawStrokeText(30, 44, &White, FpsInfo);
}

int main() {
	if (CreateConsole == false)
		ShowWindow(GetConsoleWindow(), SW_HIDE);

	Overlay overlay("FiveM_GTAProcess.exe");
	overlay.SetRenderFunction(Render);
	overlay.SetDrawFunction(Draw);
	overlay.Start();

}