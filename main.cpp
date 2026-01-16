#define UNICODE
#define _UNICODE

#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <limits>
#include <optional>
#include <d3d11.h>
#include <dxgi.h>
#include <conio.h>
#include <fstream>
#include <conio.h>
#include <algorithm>
#include <iomanip>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

#include "json/json.hpp"

#include "offsets/offsets.hpp"
#include "process/utils.hpp"
#include "utils/strings.hpp"
#include "utils/getChildren.hpp"

#include "gameUtils/getWorkSpace.hpp"
#include "gameUtils/getPlayers.hpp"
#include "gameUtils/getHumanoidOfPlayer.hpp"
#include "gameUtils/getCharacterOfPlayer.hpp"
#include "gameUtils/validatePlayer.hpp"

#include "memory/memory.hpp"
//#include "structs/players.hpp"

#include "init.hpp"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dwmapi.lib")

using json = nlohmann::json;

struct Vector3 {
    float x, y, z;

    Vector3 operator*(float scalar) const {
        return { x * scalar, y * scalar, z * scalar };
    }
};

struct Vector2 {
    float x, y;
};

struct Matrix4 {
    float data[16];
};

struct Quaternion {
    float x;
    float y;
    float z;
    float w;
};

Vector2 world_to_screen(Vector3 world, Vector2 dimensions, Matrix4 viewmatrix)
{
    Quaternion q;

    q.x = world.x * viewmatrix.data[0]  + world.y * viewmatrix.data[1]  + world.z * viewmatrix.data[2]  + viewmatrix.data[3];
    q.y = world.x * viewmatrix.data[4]  + world.y * viewmatrix.data[5]  + world.z * viewmatrix.data[6]  + viewmatrix.data[7];
    q.z = world.x * viewmatrix.data[8]  + world.y * viewmatrix.data[9]  + world.z * viewmatrix.data[10] + viewmatrix.data[11];
    q.w = world.x * viewmatrix.data[12] + world.y * viewmatrix.data[13] + world.z * viewmatrix.data[14] + viewmatrix.data[15];

    if (q.w < 0.1f)
        return { -1, -1 };

    float inv_w = 1.0f / q.w;
    float ndcX = q.x * inv_w;
    float ndcY = q.y * inv_w;

    Vector2 screen;
    screen.x = (ndcX * 0.5f + 0.5f) * dimensions.x;
    screen.y = (1.0f - (ndcY * 0.5f + 0.5f)) * dimensions.y;

    return screen;
}

// std::optional<Vector2> get_screen_position(Vector3 object_pos) {
//     auto dimensions = globals::visualengine.get_dimensions(); // screen dimensions
//     auto viewmatrix = get_view_matrix(); // camera view matrix

//     Vector2 screen_pos = world_to_screen(object_pos, dimensions, viewmatrix);

//     // If the object is behind the camera, return empty
    // if (screen_pos.x == -1.0f && screen_pos.y == -1.0f) {
    //     return std::nullopt;
    // }

//     return screen_pos;
// }

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                // Optional: clear background here if needed
                EndPaint(hwnd, &ps);
            }
            return 0;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// Example DX11 initialization
bool InitD3D11(HWND hwnd)
{
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = GetSystemMetrics(SM_CXSCREEN);
    sd.BufferDesc.Height = GetSystemMetrics(SM_CYSCREEN);
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &sd,
        &g_pSwapChain,
        &g_pd3dDevice,
        nullptr,
        &g_pd3dDeviceContext
    );
    if (FAILED(hr))
        return false;

    // Create render target
    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();

    return true;
}

void DrawBoxAtScreenPosition_ImGui(float screenX, float screenY, float boxW, float boxH)
{
    ImDrawList* draw_list = ImGui::GetForegroundDrawList();

    ImVec2 topLeft(screenX - boxW/2, screenY - boxH/2);
    ImVec2 bottomRight(screenX + boxW/2, screenY + boxH/2);

    draw_list->AddRect(topLeft, bottomRight, IM_COL32(255, 0, 0, 255), 0.0f, 0, 2.0f);
}

void DrawTracerFromCenter_ImGui(float targetX, float targetY, float width, float height)
{
    ImDrawList* draw_list = ImGui::GetForegroundDrawList();

    ImVec2 start(width / 2, height / 2);
    ImVec2 end(targetX, targetY);

    draw_list->AddLine(start, end, IM_COL32(255, 0, 0, 255), 2.0f);
}

inline void DrawCornerBox_ImGui(float screenX, float screenY, float boxW, float boxH,
                               ImU32 color = IM_COL32(255,255,255,255),
                               float thickness = 1.6f)
{
    ImDrawList* draw_list = ImGui::GetForegroundDrawList();
    if (!draw_list) return;

    ImGuiIO& io = ImGui::GetIO();
    const float scale = io.DisplayFramebufferScale.x;
    if (scale <= 0.0f) { return; }

    const float sx = screenX * scale;
    const float sy = screenY * scale;
    const float w  = boxW * scale;
    const float h  = boxH * scale;
    const float thick = std::max(0.5f, thickness * scale);

    const float lineLen = std::clamp(w * 0.28f, 6.0f * scale, std::max(12.0f * scale, w * 0.5f));

    ImVec2 tl(sx - w/2.0f, sy - h/2.0f);
    ImVec2 tr(sx + w/2.0f, sy - h/2.0f);
    ImVec2 bl(sx - w/2.0f, sy + h/2.0f);
    ImVec2 br(sx + w/2.0f, sy + h/2.0f);

    ImU32 shadow = IM_COL32(0, 0, 0, 180);
    float shadow_thick = thick + std::max(1.0f, scale);


    draw_list->AddLine(tl, ImVec2(tl.x + lineLen, tl.y), shadow, shadow_thick);
    draw_list->AddLine(tl, ImVec2(tl.x, tl.y + lineLen), shadow, shadow_thick);

    draw_list->AddLine(tr, ImVec2(tr.x - lineLen, tr.y), shadow, shadow_thick);
    draw_list->AddLine(tr, ImVec2(tr.x, tr.y + lineLen), shadow, shadow_thick);

    draw_list->AddLine(bl, ImVec2(bl.x + lineLen, bl.y), shadow, shadow_thick);
    draw_list->AddLine(bl, ImVec2(bl.x, bl.y - lineLen), shadow, shadow_thick);

    draw_list->AddLine(br, ImVec2(br.x - lineLen, br.y), shadow, shadow_thick);
    draw_list->AddLine(br, ImVec2(br.x, br.y - lineLen), shadow, shadow_thick);

    draw_list->AddLine(tl, ImVec2(tl.x + lineLen, tl.y), color, thick);
    draw_list->AddLine(tl, ImVec2(tl.x, tl.y + lineLen), color, thick);

    draw_list->AddLine(tr, ImVec2(tr.x - lineLen, tr.y), color, thick);
    draw_list->AddLine(tr, ImVec2(tr.x, tr.y + lineLen), color, thick);

    draw_list->AddLine(bl, ImVec2(bl.x + lineLen, bl.y), color, thick);
    draw_list->AddLine(bl, ImVec2(bl.x, bl.y - lineLen), color, thick);

    draw_list->AddLine(br, ImVec2(br.x - lineLen, br.y), color, thick);
    draw_list->AddLine(br, ImVec2(br.x, br.y - lineLen), color, thick);
}

struct Vec2 { float x, y; };
Vec2 GetAimDelta(Vector2 screenPos, Vector2 screenSize, float sensitivity = 0.05f)
{
    float centerX = screenSize.x * 0.5f;
    float centerY = screenSize.y * 0.5f;

    float dx = (screenPos.x - centerX) * sensitivity;
    float dy = (screenPos.y - centerY) * sensitivity;

    return { dx, dy };
}

void MoveMouseRelativeSmall(float dx, float dy)
{
    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_MOVE;

    input.mi.dx = (LONG)dx;
    input.mi.dy = (LONG)dy;

    SendInput(1, &input, sizeof(INPUT));
}

void AimAtWorld(Vector3 world, Vector2 screenSize, Matrix4 view)
{
    Vector2 sp = world_to_screen(world, screenSize, view);

    if (sp.x == -1 && sp.y == -1)
        return;

    Vec2 delta = GetAimDelta(sp, screenSize, 1.0f);

    MoveMouseRelativeSmall(delta.x, delta.y);
}

bool WriteVector3(HANDLE hProcess, uintptr_t address, const Vector3& value)
{
    if (hProcess == nullptr || hProcess == INVALID_HANDLE_VALUE)
        return false;

    SIZE_T bytesWritten = 0;

    BOOL result = WriteProcessMemory(
        hProcess,
        (LPVOID)address,
        &value,
        sizeof(Vector3),
        &bytesWritten
    );

    return (result && bytesWritten == sizeof(Vector3));
}

void clearScreen() {
    std::cout << "\033[2J\033[H";
}

void outputMenu(const std::vector<std::string>& items, int selected) {
    for (int i = 0; i < (int)items.size(); ++i) {
        if (i == selected) {
            std::cout << " > " << items[i] << "\n";
        } else {
            std::cout << "   " << items[i] << "\n";
        }
    }
}

int wrapIndex(int idx, int delta, int size) {
    if (size <= 0) return 0;
    int next = (idx + delta) % size;
    if (next < 0) next += size;
    return next;
}

std::string boolToOnOff(bool v) {
    return v ? "On" : "Off";
}

bool saveConfig(const std::string& path, const json& j) {
    std::ofstream o(path);
    if (!o.is_open()) return false;
    o << j.dump(4);
    return true;
}

std::string asci =
    "\x1b[36m"
    R"(  .---.        ____    ,---.    ,---. _______    ______        ____     
  | ,_|      .'  __ `. |    \  /    |\  ____  \ |    _ `''.  .'  __ `.  
,-./  )     /   '  \  \|  ,  \/  ,  || |    \ | | _ | ) _  \/   '  \  \ 
\  '_ '`)   |___|  /  ||  |\_   /|  || |____/ / |( ''_'  ) ||___|  /  | 
 > (_)  )      _.-`   ||  _( )_/ |  ||   _ _ '. | . (_) `. |   _.-`   | 
(  .  .-'   .'   _    || (_ o _) |  ||  ( ' )  \|(_    ._) '.'   _    | 
 `-'`-'|___ |  _( )_  ||  (_,_)  |  || (_{;}_) ||  (_.\.' / |  _( )_  | 
  |        \\ (_ o _) /|  |      |  ||  (_,_)  /|       .'  \ (_ o _) / 
  `--------` '.(_,_).' '--'      '--'/_______.' '-----'`     '.(_,_).'  
                                                                         )"
    "\x1b[0m";



int main() {
    start:
    bool esp = false;
    bool tracers = false;
    bool aimbot = false;

    Vector3 espColor = {255, 0, 0};
    Vector3 tracersColor = {255, 0, 0};

    char espKeybind = 'Y';
    char tracersKeybind = 'Y';
    char aimbotKeybind = 'Y';

    float aimbotSensitivity = 0.5f;

    const std::string configPath = "config.json";

    json j;
    {
        std::ifstream configFile(configPath);
        if (configFile.is_open()) {
            try {
                configFile >> j;
            } catch (const std::exception& e) {
                std::cerr << "Warning: failed to parse " << configPath << " (" << e.what() << "). Using defaults.\n";
                j = json::object();
            }
        } else {
            j = json::object();
        }
    }

    if (j.contains("aimbot")) {
        auto ja = j["aimbot"];
        if (ja.contains("enabled")) aimbot = ja["enabled"].get<bool>();
        if (ja.contains("keyBind")) {
            std::string keystr = ja["keyBind"].get<std::string>();
            if (!keystr.empty()) aimbotKeybind = keystr[0];
        }
        if (ja.contains("sensitivity")) aimbotSensitivity = ja["sensitivity"].get<float>();
    }

    if (j.contains("esp")) {
        auto je = j["esp"];
        if (je.contains("enabled")) esp = je["enabled"].get<bool>();
        if (je.contains("keyBind")) {
            std::string keystr = je["keyBind"].get<std::string>();
            if (!keystr.empty()) espKeybind = keystr[0];
        }
    }

    if (j.contains("tracers")) {
        auto jt = j["tracers"];
        if (jt.contains("enabled")) tracers = jt["enabled"].get<bool>();
        if (jt.contains("keyBind")) {
            std::string keystr = jt["keyBind"].get<std::string>();
            if (!keystr.empty()) tracersKeybind = keystr[0];
        }
    }

    std::vector<std::string> mainOptions = {
        "Attach to roblox process",
        "Edit config",
        "Information",
        "Exit"
    };

    int mainIndex = 0;
    bool running = true;

    while (running) {
        clearScreen();
        std::cout << asci << "\n";
        outputMenu(mainOptions, mainIndex);

        while (true) {
            if (_kbhit()) {
                int ch = _getch();
                if (ch == 'w' || ch == 'W') {
                    mainIndex = wrapIndex(mainIndex, -1, (int)mainOptions.size());
                    clearScreen();
                    std::cout << asci << "\n";
                    outputMenu(mainOptions, mainIndex);
                } else if (ch == 's' || ch == 'S') {
                    mainIndex = wrapIndex(mainIndex, +1, (int)mainOptions.size());
                    clearScreen();
                    std::cout << asci << "\n";
                    outputMenu(mainOptions, mainIndex);
                } else if (ch == 13) {
                    break;
                }
            }
        }

        switch (mainIndex) {
            case 0: {
                clearScreen();
                std::cout << asci << "\n";
                goto attach;
            }
            case 1: {
                std::vector<std::string> cfgOptions = {"Aimbot", "ESP", "Tracers", "Back"};
                int cfgIndex = 0;
                bool inCfg = true;
                while (inCfg) {
                    clearScreen();
                    std::cout << asci << "\n";
                    outputMenu(cfgOptions, cfgIndex);

                    while (true) {
                        if (_kbhit()) {
                            int ch = _getch();
                            if (ch == 'w' || ch == 'W') {
                                cfgIndex = wrapIndex(cfgIndex, -1, (int)cfgOptions.size());
                                clearScreen();
                                std::cout << asci << "\n";
                                outputMenu(cfgOptions, cfgIndex);
                            } else if (ch == 's' || ch == 'S') {
                                cfgIndex = wrapIndex(cfgIndex, +1, (int)cfgOptions.size());
                                clearScreen();
                                std::cout << asci << "\n";
                                outputMenu(cfgOptions, cfgIndex);
                            } else if (ch == 13) {
                                break;
                            }
                        }
                    }

                    if (cfgIndex == 3) {
                        inCfg = false;
                        break;
                    }

                    if (cfgIndex == 0) {
                        std::vector<std::string> aimOptions = {
                            std::string("Enabled: ") + boolToOnOff(aimbot),
                            std::string("Sensitivity: ") + std::to_string(aimbotSensitivity),
                            std::string("Keybind: ") + std::string(1, aimbotKeybind),
                            "Back"
                        };
                        int aimIndex = 0;
                        bool inAim = true;
                        while (inAim) {
                            aimOptions[0] = "Enabled: " + boolToOnOff(aimbot);
                            {
                                std::ostringstream ss;
                                ss << std::fixed << std::setprecision(2) << aimbotSensitivity;
                                aimOptions[1] = "Sensitivity: " + ss.str();
                            }
                            aimOptions[2] = std::string("Keybind: ") + std::string(1, aimbotKeybind);

                            clearScreen();
                            std::cout << asci << "\n";
                            outputMenu(aimOptions, aimIndex);

                            while (true) {
                                if (_kbhit()) {
                                    int ch = _getch();
                                    if (ch == 'w' || ch == 'W') {
                                        aimIndex = wrapIndex(aimIndex, -1, (int)aimOptions.size());
                                        clearScreen();
                                        std::cout << asci << "\n";
                                        outputMenu(aimOptions, aimIndex);
                                    } else if (ch == 's' || ch == 'S') {
                                        aimIndex = wrapIndex(aimIndex, +1, (int)aimOptions.size());
                                        clearScreen();
                                        std::cout << asci << "\n";
                                        outputMenu(aimOptions, aimIndex);
                                    } else if (ch == 13) {
                                        break;
                                    }
                                }
                            }

                            if (aimIndex == 3) {
                                inAim = false;
                                break;
                            } else if (aimIndex == 0) {
                                aimbot = !aimbot;
                            } else if (aimIndex == 2) {
                                clearScreen();
                                std::cout << asci << "\n";
                                std::cout << "Press a single key to set as Aimbot keybind:\n";
                                int k = _getch();
                                if (k != 0 && k != 224) {
                                    aimbotKeybind = static_cast<char>(k);
                                }
                            } else if (aimIndex == 1) {
                                bool editing = true;
                                clearScreen();
                                std::cout << asci << "\n";
                                std::cout << "Aimbot sensitivity:\n";
                                std::cout << std::fixed << std::setprecision(2) << aimbotSensitivity << "\n";
                                std::cout << "[W] Increase  [S] Decrease  [ENTER] Back\n";
                                while (editing) {
                                    if (_kbhit()) {
                                        int ch = _getch();
                                        if (ch == 'w' || ch == 'W') {
                                            aimbotSensitivity = std::clamp(aimbotSensitivity + 0.10f, 0.0f, 1.0f);
                                            clearScreen();
                                            std::cout << asci << "\n";
                                            std::cout << "Aimbot sensitivity:\n";
                                            std::cout << std::fixed << std::setprecision(2) << aimbotSensitivity << "\n";
                                            std::cout << "[W] Increase  [S] Decrease  [ENTER] Back\n";
                                        } else if (ch == 's' || ch == 'S') {
                                            aimbotSensitivity = std::clamp(aimbotSensitivity - 0.10f, 0.0f, 1.0f);
                                            clearScreen();
                                            std::cout << asci << "\n";
                                            std::cout << "Aimbot sensitivity:\n";
                                            std::cout << std::fixed << std::setprecision(2) << aimbotSensitivity << "\n";
                                            std::cout << "[W] Increase  [S] Decrease  [ENTER] Back\n";
                                        } else if (ch == 13) {
                                            editing = false;
                                        }
                                    }
                                }
                            }
                        }
                    } else if (cfgIndex == 1) {
                        std::vector<std::string> espOptions = {
                            std::string("Enabled: ") + boolToOnOff(esp),
                            std::string("Keybind: ") + std::string(1, espKeybind),
                            "Back"
                        };
                        int espIndex = 0;
                        bool inEsp = true;
                        while (inEsp) {
                            espOptions[0] = "Enabled: " + boolToOnOff(esp);
                            espOptions[1] = "Keybind: " + std::string(1, espKeybind);
                            clearScreen();
                            std::cout << asci << "\n";
                            outputMenu(espOptions, espIndex);

                            while (true) {
                                if (_kbhit()) {
                                    int ch = _getch();
                                    if (ch == 'w' || ch == 'W') {
                                        espIndex = wrapIndex(espIndex, -1, (int)espOptions.size());
                                        clearScreen();
                                        std::cout << asci << "\n";
                                        outputMenu(espOptions, espIndex);
                                    } else if (ch == 's' || ch == 'S') {
                                        espIndex = wrapIndex(espIndex, +1, (int)espOptions.size());
                                        clearScreen();
                                        std::cout << asci << "\n";
                                        outputMenu(espOptions, espIndex);
                                    } else if (ch == 13) {
                                        break;
                                    }
                                }
                            }

                            if (espIndex == 2) {
                                inEsp = false;
                                break;
                            } else if (espIndex == 0) {
                                esp = !esp;
                            } else if (espIndex == 1) {
                                clearScreen();
                                std::cout << asci << "\n";
                                std::cout << "Press a single key to set as ESP keybind:\n";
                                int k = _getch();
                                if (k != 0 && k != 224) espKeybind = static_cast<char>(k);
                            }
                        }
                    } else if (cfgIndex == 2) {
                        std::vector<std::string> trOptions = {
                            std::string("Enabled: ") + boolToOnOff(tracers),
                            std::string("Keybind: ") + std::string(1, tracersKeybind),
                            "Back"
                        };
                        int trIndex = 0;
                        bool inTr = true;
                        while (inTr) {
                            trOptions[0] = "Enabled: " + boolToOnOff(tracers);
                            trOptions[1] = "Keybind: " + std::string(1, tracersKeybind);
                            clearScreen();
                            std::cout << asci << "\n";
                            outputMenu(trOptions, trIndex);

                            while (true) {
                                if (_kbhit()) {
                                    int ch = _getch();
                                    if (ch == 'w' || ch == 'W') {
                                        trIndex = wrapIndex(trIndex, -1, (int)trOptions.size());
                                        clearScreen();
                                        std::cout << asci << "\n";
                                        outputMenu(trOptions, trIndex);
                                    } else if (ch == 's' || ch == 'S') {
                                        trIndex = wrapIndex(trIndex, +1, (int)trOptions.size());
                                        clearScreen();
                                        std::cout << asci << "\n";
                                        outputMenu(trOptions, trIndex);
                                    } else if (ch == 13) {
                                        break;
                                    }
                                }
                            }

                            if (trIndex == 2) { inTr = false; break; }
                            else if (trIndex == 0) { tracers = !tracers; }
                            else if (trIndex == 1) {
                                clearScreen();
                                std::cout << asci << "\n";
                                std::cout << "Press a single key to set as Tracers keybind:\n";
                                int k = _getch();
                                if (k != 0 && k != 224) tracersKeybind = static_cast<char>(k);
                            }
                        }
                    }

                    j["aimbot"]["enabled"] = aimbot;
                    j["aimbot"]["keyBind"] = std::string(1, aimbotKeybind);
                    j["aimbot"]["sensitivity"] = aimbotSensitivity;

                    j["esp"]["enabled"] = esp;
                    j["esp"]["keyBind"] = std::string(1, espKeybind);

                    j["tracers"]["enabled"] = tracers;
                    j["tracers"]["keyBind"] = std::string(1, tracersKeybind);

                    saveConfig(configPath, j);
                }
                break;
            }
            case 2: {
                clearScreen();
                std::cout << asci << "\n\n";
                std::cout << "Version: beta0.0.0\nDeveloper: Benda\nDiscord: hi.im.benda\nReport any bugs to: Benda\nDeveloper note: Thank you for using matrix client.\n\nTested games\n- Base Plate\n\n[ENTER TO RETURN]\n";
                while (true) {
                    if (_kbhit() && _getch() == 13) break;
                }
                break;
            }
            case 3: {
                running = false;
                break;
            }
            default:
                break;
        }
    }

    j["aimbot"]["enabled"] = aimbot;
    j["aimbot"]["keyBind"] = std::string(1, aimbotKeybind);
    j["aimbot"]["sensitivity"] = aimbotSensitivity;
    j["esp"]["enabled"] = esp;
    j["esp"]["keyBind"] = std::string(1, espKeybind);
    j["tracers"]["enabled"] = tracers;
    j["tracers"]["keyBind"] = std::string(1, tracersKeybind);
    saveConfig(configPath, j);

attach:
    out main = init();

    if (main.success == 1) {
        goto start;
        return 1;
    }

    std::cout << "Attached to roblox" << std::endl;

    WorkSpace workSpace = getWorkSpace(main.hProcess, main.realDataModel);

    Players players = getPlayers(main.hProcess, main.realDataModel);
    auto playersChildrenn = getChildren(main.hProcess, players.playersAddress);

    HINSTANCE hInst = GetModuleHandle(NULL);

    WNDCLASS wc{};
    wc.lpfnWndProc = WndProc;
    wc.lpszClassName = TEXT("OverlayClass");
    wc.hInstance = hInst;
    RegisterClass(&wc);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    HWND hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT,
        wc.lpszClassName,
        TEXT("ESP Overlay"),
        WS_POPUP,
        0, 0, screenW, screenH,
        nullptr, nullptr, hInst, nullptr
    );

    SetLayeredWindowAttributes(hwnd, RGB(0,0,0), 0, LWA_COLORKEY);
    ShowWindow(hwnd, SW_SHOW);

    if (!InitD3D11(hwnd)) {
        goto start;
        return 1;
    }

    std::cout << "Rendering window is ready." << std::endl;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    ImGui::StyleColorsDark();

    std::cout << "Rendering client is ready." << std::endl;

    auto localPlayerAddress = playersChildrenn[0];
    auto localPlayerName = getChildName(main.hProcess, localPlayerAddress);

    auto workSpaceChildren = getChildren(main.hProcess, workSpace.workSpaceAddress);
    int cameraIndex = findFirstChildOfByName(workSpaceChildren, main.hProcess, "Camera");

    auto localPlayerCharacter = getCharacterOfPlayer(main.hProcess, workSpace.workSpaceAddress, localPlayerName);
    auto localPlayerCharacterChildren = getChildren(main.hProcess, localPlayerCharacter.characterAddress);

    auto headIndex = findFirstChildOfByName(localPlayerCharacterChildren, main.hProcess, "Head");
    auto headAddress = localPlayerCharacterChildren[headIndex];

    auto localPlayerHumanoid = getHumanoidOfPlayer(main.hProcess, workSpace.workSpaceAddress, localPlayerName);
    float localPlayerHealth = ReadMemory<float>(main.hProcess, localPlayerHumanoid.humanoidAddress + Offsets::Humanoid::Health);
    float localPlayerMaxHealth = ReadMemory<float>(main.hProcess, localPlayerHumanoid.humanoidAddress + Offsets::Humanoid::MaxHealth);

    std::cout << "\nLocal player username: " << localPlayerName << "\nIf this is not your roblox username, please contact hi.im.benda on discord." << std::endl;
    // std::cout << "Local player health: " << localPlayerHealth << "/" << localPlayerMaxHealth << std::endl;

    uintptr_t cameraAddress = workSpaceChildren[cameraIndex];

    float fieldOfView = ReadMemory<float>(main.hProcess, cameraAddress + Offsets::Camera::FieldOfView);
    std::cout << "fov: " << fieldOfView << std::endl;

    const double targetFrameTime = 1.0 / 60.0;
    auto lastTime = std::chrono::high_resolution_clock::now();

    auto playersChildren = getChildren(main.hProcess, players.playersAddress);

    bool update = false;

    std::vector<uintptr_t> validPlayers = std::vector<uintptr_t>{};

    for (int index = 1; index < playersChildren.size(); index++) {
        if (isValidPlayer(getChildName(main.hProcess, playersChildren[index]), main.hProcess, workSpaceChildren)) {
            validPlayers.push_back(playersChildren[index]);
        }
    }

    MSG msg{};
    while (true) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (GetAsyncKeyState(aimbotKeybind) & 0x1) {
            aimbot  = !aimbot;
        }

        if (GetAsyncKeyState(espKeybind) & 0x1) {
            esp     = !esp;
        }

        if (GetAsyncKeyState(tracersKeybind) & 0x1) {
            tracers     = !tracers;
        }

        if (esp || tracers) {
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
        }

        std::cout << update << std::endl;

        if (findFirstChildOfByName(workSpaceChildren, main.hProcess, localPlayerName) == -1 || update) {
            while (true) {
                workSpace = getWorkSpace(main.hProcess, main.realDataModel);
                players = getPlayers(main.hProcess, main.realDataModel);

                workSpaceChildren = getChildren(main.hProcess, workSpace.workSpaceAddress);
                playersChildren = getChildren(main.hProcess, players.playersAddress);

                if (!workSpaceChildren.empty() && !playersChildren.empty()) {
                    localPlayerAddress = playersChildren[0];
                    localPlayerName = getChildName(main.hProcess, localPlayerAddress);

                    cameraIndex = findFirstChildOfByName(workSpaceChildren, main.hProcess, "Camera");

                    std::cout << "got workspace and players children" << std::endl;

                    // std::cout << "\nLocal player username: " << localPlayerName << "\nIf this is not your roblox username, please contact hi.im.benda on discord." << std::endl;
                    // std::cout << "Local player health: " << localPlayerHealth << "/" << localPlayerMaxHealth << std::endl;

                    cameraAddress = workSpaceChildren[cameraIndex];

                    fieldOfView = ReadMemory<float>(main.hProcess, cameraAddress + Offsets::Camera::FieldOfView);
                    std::cout << "fov: " << fieldOfView << std::endl;

                    //debugFindFirstChildOfByName(playersChildren, main.hProcess, "oadpoiad");

                    validPlayers = std::vector<uintptr_t>{};

                    for (int index = 1; index < playersChildren.size(); index++) {
                        if (isValidPlayer(getChildName(main.hProcess, playersChildren[index]), main.hProcess, workSpaceChildren)) {
                            validPlayers.push_back(playersChildren[index]);
                        }
                    }

                    break;
                }

                Sleep(0);
            }

            update = false;
        }

        for (int index = 1; index < playersChildren.size(); index++) {
            if (isValidPlayer(getChildName(main.hProcess, playersChildren[index]), main.hProcess, workSpaceChildren) && std::find(validPlayers.begin(), validPlayers.end(), playersChildren[index]) != playersChildren.end()) {
                // std::cout << "player is valid" << std::endl;

                auto playerAddress = playersChildren[index];
                auto playerName = getChildName(main.hProcess, playerAddress);

                // std::cout << "got player name" << std::endl;

                if (playerName != localPlayerName) {
                    // std::cout << "player exists" << std::endl;
                    if (esp || tracers || aimbot) {

                        auto playerCharacter = getCharacterOfPlayer(main.hProcess, workSpace.workSpaceAddress, playerName);
                        auto playerCharacterChildren = getChildren(main.hProcess, playerCharacter.characterAddress);

                        // std::cout << "got here" << std::endl;

                        auto externalHeadIndex = findFirstChildOfByName(playerCharacterChildren, main.hProcess, "Head");

                        if (externalHeadIndex < 0) {
                            //update = true;
                            continue;
                        }

                        // std::cout << "got here 1" << std::endl;

                        auto externalHeadAddress = playerCharacterChildren[externalHeadIndex];

                        uintptr_t headPrimitive = ReadMemory<uintptr_t>(main.hProcess, externalHeadAddress + Offsets::BasePart::Primitive);
                        Vector3 headPos = ReadMemory<Vector3>( main.hProcess, headPrimitive + Offsets::BasePart::Position);
                        uintptr_t vePtr = ReadMemory<uintptr_t>(main.hProcess, main.base + Offsets::VisualEngine::Pointer);
                        Matrix4 viewMatrix = ReadMemory<Matrix4>(main.hProcess, vePtr + Offsets::VisualEngine::ViewMatrix);
                        Vector2 screen = world_to_screen(headPos, {(float)screenW, (float)screenH}, viewMatrix);

                        // std::cout << "got here 2" << std::endl;

                        if (screen.x-1 >= 0 && screen.x-1 <= screenW-1 && screen.y-1 >= 0 && screen.y-1 <= screenH-1) {
                            if (esp) 
                                DrawCornerBox_ImGui(screen.x-1, screen.y-1, 20, 40);
                            
                            if (tracers)
                                DrawTracerFromCenter_ImGui(screen.x-1, screen.y-1, (float)screenW-1, (float)screenH-1);
                            
                            if (aimbot)
                                AimAtWorld(headPos, {(float)screenW-1, (float)screenH-1}, viewMatrix);
                        }
                    }
                }

            } else if (std::find(validPlayers.begin(), validPlayers.end(), playersChildren[index]) != playersChildren.end()) {
                std::cout << "Player failed at validation: " << getChildName(main.hProcess, playersChildren[index]) << std::endl;

                if (trim(getChildName(main.hProcess, playersChildren[index])) == "Hello" || trim(getChildName(main.hProcess, playersChildren[index])) == "World" || trim(getChildName(main.hProcess, playersChildren[index])) == "") {
                    if (!update) {
                        std::cout << "Blacklisted player detected: " << getChildName(main.hProcess, playersChildren[index]) << std::endl;
                        update = false;
                    } else {
                        std::cout << "[Ignored] Blacklisted player detected: " << getChildName(main.hProcess, playersChildren[index]) << std::endl;
                    }
                } else {
                    update = true;
                }

                validPlayers.erase(std::remove(validPlayers.begin(), validPlayers.end(), playersChildren[index]), validPlayers.end());              
            }
        }

        if (esp || tracers) {
            ImGui::Render();
            float clear_color[4] = {0,0,0,0};
            g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
            g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);

            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

            g_pSwapChain->Present(1, 0);
        }

        Sleep(14);
    }

    // int ind = 1;

    // while (true) {
    //     auto player = playersChildren[ind];
    //     if (isValidPlayer(getChildName(main.hProcess, player), main.hProcess, workSpaceChildren)) {
    //         break;
    //     }

    //     ind++;
    // }

    // auto externalPlayer = playersChildren[ind];

    // auto playerCharacter = getCharacterOfPlayer(main.hProcess, workSpace.workSpaceAddress, getChildName(main.hProcess, externalPlayer));
    // auto playerCharacterChildren = getChildren(main.hProcess, playerCharacter.characterAddress);

    // auto externalHeadIndex = findFirstChildOfByName(playerCharacterChildren, main.hProcess, "Head");

    // // std::cout << "got here 1" << std::endl;

    // std::cout << "Resolved target player: " << getChildName(main.hProcess, externalPlayer) << std::endl;

    // auto externalHeadAddress = playerCharacterChildren[externalHeadIndex];

    // uintptr_t headPrimitive = ReadMemory<uintptr_t>(main.hProcess, externalHeadAddress + Offsets::BasePart::Primitive);
    // Vector3 headPos = ReadMemory<Vector3>( main.hProcess, headPrimitive + Offsets::BasePart::Position);

    // int localPlayerHumanoidIndex = debugFindFirstChildOfByName(localPlayerCharacterChildren, main.hProcess, "Humanoid");
    // auto localPlayerHumanoidRootPart = localPlayerCharacterChildren[localPlayerHumanoidIndex];

    // uintptr_t localPlayerHumanoidRootPartPrimitive = ReadMemory<uintptr_t>(main.hProcess, externalHeadAddress + Offsets::BasePart::Primitive);

    // std::cout << "Player humanoid roo part has been resolved, will attempt to teleport soon." << std::endl;

    // WriteVector3(main.hProcess, localPlayerHumanoidRootPart + Offsets::BasePart::Position, headPos);

    // std::cout << "Player humanoid root part has attempted to teleport." << std::endl;

    CloseHandle(main.hProcess);
    return 0;
}
