#pragma once
#include <Windows.h>

namespace GuiManager {
    bool Init(const char* windowName, int width, int height);
    bool IsRunning();
    void BeginFrame();
    void EndFrame();
    void Shutdown();
}