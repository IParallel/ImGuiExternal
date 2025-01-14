#include <windows.h>
#include <iostream>
#include <tlhelp32.h>
#include <psapi.h>
#include <dwmapi.h>
#include <d3d9.h>
#include <thread>
#include "src/Overlay.h"
#include <imgui.h>
#include "file.hpp"
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "dwmapi.lib")

bool CreateConsole = true;

ImTextureID image;

void Render(Overlay* overlay) {
	ImGui::Begin("Tetas");
	ImGui::Text("Tetas");
	ImGui::End();
}

void Draw(Overlay* overlay) {
	char FpsInfo[64];
	sprintf(FpsInfo, "Overlay FPS: %0.f", ImGui::GetIO().Framerate);
	auto dl = ImGui::GetForegroundDrawList();
	RGBA White = { 255,255,255,255 };
	overlay->DrawStrokeText(30, 44, &White, FpsInfo);
	dl->AddImage(image, ImVec2{ 200, 200 }, ImVec2{ 260, 260 });
}


void PreInit(Overlay* overlay) {
	image = overlay->LoadTextureFromMemory((void*)icon, sizeof(icon));
}


int main() {
	if (CreateConsole == false)
		ShowWindow(GetConsoleWindow(), SW_HIDE);

	Overlay overlay("FiveM_GTAProcess.exe");
	overlay.SetRenderFunction(Render);
	overlay.SetDrawFunction(Draw);
	overlay.SetPreInitFunction(PreInit);
	overlay.Start();

}