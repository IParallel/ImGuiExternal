# Pretty simple overlay for games or apps

### How to use
- Clone the repo
- You can instantiate the class and call some helper functions to start the overlay
  ```c
  Overlay::SetRenderFunction;
  Overlay::SetDrawFunction;
  Overlay::SetPreInitFunction;
  Overlay::Start;
  ```
- An example:
  ```c
  RGBA White = { 255,255,255,255 };
  ImTextureID image;
  // you can render here the imgui things
  void Render(Overlay* overlay) {
	ImGui::Begin("Example title");
	ImGui::Text("Example text");
	ImGui::End();
  }
  
  // this is like the Render one but this will be executed after render so you can draw things like in the bg or fg
  void Draw(Overlay* overlay) {
    char FpsInfo[64];
	sprintf(FpsInfo, "Overlay FPS: %0.f", ImGui::GetIO().Framerate);
	auto dl = ImGui::GetForegroundDrawList();
	overlay->DrawStrokeText(30, 44, &White, FpsInfo);
	dl->AddImage(image, ImVec2{ 200, 200 }, ImVec2{ 260, 260 });
  }

  // this will be called before the first render so you can instantitate your assets or do other things
  void PreInit(Overlay* overlay) {
  // helper function to load textures from in memory byte array
	image = overlay->LoadTextureFromMemory((void*)icon, sizeof(icon));
  }

  int main() {
	if (CreateConsole == false)
		ShowWindow(GetConsoleWindow(), SW_HIDE);

	Overlay overlay("your_process_name.exe");
	overlay.SetRenderFunction(Render); // helper functions
	overlay.SetDrawFunction(Draw);
	overlay.SetPreInitFunction(PreInit);
	overlay.Start(); // this is a blocking call

  }
  ```
