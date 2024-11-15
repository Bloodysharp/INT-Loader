#define IMGUI_DEFINE_MATH_OPERATORS

#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include "imgui_internal.h"
#include "imgui_settings.h"
#include <d3d9.h>
#include <tchar.h>
#include "blur.hpp"
#include <d3dx9.h>
#pragma comment (lib, "d3dx9.lib")

#include "image.h"
#include "font.h"

#include <d3d11.h>
#include <tchar.h>
#include <dwmapi.h>
#include <string>


// Data
LPDIRECT3D9              g_pD3D = nullptr;
LPDIRECT3DDEVICE9        g_pd3dDevice = nullptr;
UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
D3DPRESENT_PARAMETERS    g_d3dpp = {};

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace image
{
    IDirect3DTexture9* game = nullptr;
    IDirect3DTexture9* close_icon = nullptr;
    IDirect3DTexture9* user_icon = nullptr;
    IDirect3DTexture9* logout_icon = nullptr;
    IDirect3DTexture9* subscription_icon = nullptr;
}

namespace font
{
    ImFont* default_lexend;
    ImFont* lexand_tabs_bold;
    ImFont* lexand_name_bold;
    ImFont* lexand_info_bold;

    ImFont* lexand_notice_bold;


    ImFont* lexend_default_semibold = nullptr;
    ImFont* lexend_default_bold = nullptr;
    ImFont* lexend_child_semibold = nullptr;
}

bool login_page = true;
bool load_succes = true;
bool internet = true;

float opticaly = 0.f;
float opticaly_info = 1.f;

HWND hwnd;
RECT rc;

void TextColoredTAB(ImU32 color, const char* text, ImFont* font)
{

    ImGui::PushFont(font);
    ImGui::TextColored(ImColor(color), text);
    ImGui::PopFont();

}

void move_window() {

    ImGui::SetCursorPos(ImVec2(0, 0));
    if (ImGui::InvisibleButton("Move_detector", ImVec2(c::background::size.x, c::background::size.y)));
    if (ImGui::IsItemActive()) {

        GetWindowRect(hwnd, &rc);
        MoveWindow(hwnd, rc.left + ImGui::GetMouseDragDelta().x, rc.top + ImGui::GetMouseDragDelta().y, c::background::size.x, c::background::size.y, TRUE);
    }

}

float size = 0.f;
bool inject = false;

void RenderBlur(HWND hwnd)
{
    struct ACCENTPOLICY
    {
        int na;
        int nf;
        int nc;
        int nA;
    };
    struct WINCOMPATTRDATA
    {
        int na;
        PVOID pd;
        ULONG ul;
    };

    const HINSTANCE hm = LoadLibrary(L"user32.dll");
    if (hm)
    {
        typedef BOOL(WINAPI* pSetWindowCompositionAttribute)(HWND, WINCOMPATTRDATA*);

        const pSetWindowCompositionAttribute SetWindowCompositionAttribute = (pSetWindowCompositionAttribute)GetProcAddress(hm, "SetWindowCompositionAttribute");
        if (SetWindowCompositionAttribute)
        {
            ACCENTPOLICY policy = { 3, 0, 0, 0 }; // and even works 4,0,155,0 (Acrylic blur)
            WINCOMPATTRDATA data = { 19, &policy,sizeof(ACCENTPOLICY) };
            SetWindowCompositionAttribute(hwnd, &data);
        }
        FreeLibrary(hm);
    }
}

DWORD win_flags = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove;

namespace ImGui {

    void CustomAddText(ImDrawList* list, ImFont* font, float size, ImVec2 pos, ImU32 color, const char* text)
    {
        list->AddText(font, size, (pos - font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.f, text) / 2), color, text);
    }

    bool Spinner(const char* label, float radius, int thickness, const ImU32& color) {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems) return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);

        ImVec2 pos = window->DC.CursorPos;
        ImVec2 size((radius) * 2, (radius) * 2);

        const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        ItemSize(bb, 0.f);

        if (!ItemAdd(bb, id)) return false;

        window->DrawList->PathClear();

        int num_segments = 2048;
        int start = abs(ImSin(g.Time * 1.8f) * (num_segments - 5));

        const float a_min = IM_PI * 2.0f * ((float)start) / (float)num_segments;
        const float a_max = IM_PI * 2.0f * ((float)num_segments - 3) / (float)num_segments;

        const ImVec2 centre = ImVec2(pos.x + radius, pos.y + radius);

        for (int i = 0; i < num_segments; i++) {
            const float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
            window->DrawList->PathLineTo(ImVec2(centre.x + ImCos(a + g.Time * 8) * radius,
                centre.y + ImSin(a + g.Time * 8) * radius));
        }

        window->DrawList->PathStroke(color, false, thickness);
    }
}

int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{

    WNDCLASSEXW wc;
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = NULL;
    wc.cbWndExtra = NULL;
    wc.hInstance = nullptr;
    wc.hIcon = LoadIcon(0, IDI_APPLICATION);
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszMenuName = L"ImGui";
    wc.lpszClassName = L"Example";
    wc.hIconSm = LoadIcon(0, IDI_APPLICATION);

    RegisterClassExW(&wc);
    hwnd = CreateWindowExW(NULL, wc.lpszClassName, L"Example", WS_POPUP, (GetSystemMetrics(SM_CXSCREEN) / 2) - (c::background::size.x / 2), (GetSystemMetrics(SM_CYSCREEN) / 2) - (c::background::size.y / 2), c::background::size.x, c::background::size.y, 0, 0, 0, 0);

    MARGINS margins = { -1 };
    DwmExtendFrameIntoClientArea(hwnd, &margins);

    POINT mouse;
    rc = { 0 };
    GetWindowRect(hwnd, &rc);

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    font::lexend_default_semibold = io.Fonts->AddFontFromMemoryTTF(lexand_semibold, sizeof(lexand_semibold), 15.f, NULL, io.Fonts->GetGlyphRangesCyrillic());
    font::lexend_child_semibold = io.Fonts->AddFontFromMemoryTTF(lexand_semibold, sizeof(lexand_semibold), 13.f, NULL, io.Fonts->GetGlyphRangesCyrillic());
    font::lexand_notice_bold = io.Fonts->AddFontFromMemoryTTF(lexand_black, sizeof(lexand_black), 40.f, NULL, io.Fonts->GetGlyphRangesCyrillic());
    font::lexand_name_bold = io.Fonts->AddFontFromMemoryTTF(lexand_black, sizeof(lexand_black), 20.f, NULL, io.Fonts->GetGlyphRangesCyrillic());

    font::lexand_info_bold = io.Fonts->AddFontFromMemoryTTF(lexand_bold, sizeof(lexand_bold), 15.f, NULL, io.Fonts->GetGlyphRangesCyrillic());
    font::lexand_tabs_bold = io.Fonts->AddFontFromMemoryTTF(lexand_bold, sizeof(lexand_bold), 18.f, NULL, io.Fonts->GetGlyphRangesCyrillic());
    font::default_lexend = io.Fonts->AddFontFromMemoryTTF(lexand_semibold, sizeof(lexand_semibold), 20.f, NULL, io.Fonts->GetGlyphRangesCyrillic());

    if (image::game == nullptr) D3DXCreateTextureFromFileInMemoryEx(g_pd3dDevice, &game_image, sizeof(game_image), 510, 90, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &image::game);
    if (image::user_icon == nullptr) D3DXCreateTextureFromFileInMemoryEx(g_pd3dDevice, &user_ico, sizeof(user_ico), 35, 35, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &image::user_icon);
    if (image::close_icon == nullptr) D3DXCreateTextureFromFileInMemoryEx(g_pd3dDevice, &close_icon, sizeof(close_icon), 20, 20, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &image::close_icon);
    if (image::logout_icon == nullptr) D3DXCreateTextureFromFileInMemoryEx(g_pd3dDevice, &logout_ico, sizeof(logout_ico), 20, 20, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &image::logout_icon);
    if (image::subscription_icon == nullptr) D3DXCreateTextureFromFileInMemoryEx(g_pd3dDevice, &subscription_ico, sizeof(subscription_ico), 20, 20, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &image::subscription_icon);

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);

    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            g_d3dpp.BackBufferWidth = g_ResizeWidth;
            g_d3dpp.BackBufferHeight = g_ResizeHeight;
            g_ResizeWidth = g_ResizeHeight = 0;
            ResetDevice();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGuiStyle* style = &ImGui::GetStyle();

        style->WindowBorderSize = 0.f;
        style->WindowPadding = ImVec2(0.f, 0.f);
        style->ItemSpacing = ImVec2(20.f, 20.f);

        ImGui::SetNextWindowSize(ImVec2(c::background::size));

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::Begin("IMGUI", nullptr, win_flags);
        {
            const ImVec2& pos = ImGui::GetWindowPos();
            const ImVec2& region = ImGui::GetContentRegionMax();
            const ImVec2& spacing = ImGui::GetStyle().ItemSpacing;

            ImGui::GetBackgroundDrawList()->AddRectFilled(pos + ImVec2(0, 0), pos + ImVec2(150, region.y), ImGui::GetColorU32(c::background::tab_list), c::background::rounding, ImDrawCornerFlags_Left);
            ImGui::GetBackgroundDrawList()->AddRectFilled(pos + ImVec2(150, 0), pos + ImVec2(region.x, region.y), ImGui::GetColorU32(c::background::background), c::background::rounding, ImDrawCornerFlags_Right);

            opticaly = ImLerp(opticaly, login_page ? 0.1f : 1.0f, ImGui::GetIO().DeltaTime * 6.f);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, opticaly);

            ImGui::GetBackgroundDrawList()->AddLine(pos + ImVec2(150, 0), pos + ImVec2(150, region.y), ImGui::GetColorU32(c::background::stoke), 1.f);
            ImGui::GetBackgroundDrawList()->AddLine(pos + ImVec2(0, 60), pos + ImVec2(region.x, 60), ImGui::GetColorU32(c::background::stoke), 1.f);
            ImGui::GetBackgroundDrawList()->AddLine(pos + ImVec2(region.x - 60, 0), pos + ImVec2(region.x - 60, 60), ImGui::GetColorU32(c::background::stoke), 1.f);

            ImGui::CustomAddText(ImGui::GetBackgroundDrawList(), font::lexand_name_bold, 20.f, pos + ImVec2(150, 60) / 2, ImGui::GetColorU32(c::accent_color), "INT PROJECT");

            ImGui::GetBackgroundDrawList()->AddImage(image::close_icon, pos + ImVec2(region.x - 40, 20), pos + ImVec2(region.x - 20, 40), ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(c::text::text));
            if (!login_page && GetAsyncKeyState(VK_LBUTTON) && ImGui::IsMouseHoveringRect(pos + ImVec2(region.x - 60, 0), pos + ImVec2(region.x, 60), true)) PostQuitMessage(0);

            ImGui::GetBackgroundDrawList()->AddImageRounded(image::user_icon, pos + ImVec2(region.x - (73 + 35), 13), pos + ImVec2(region.x - 73, 47), ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(c::text::text_active), 30.f);
            ImGui::GetBackgroundDrawList()->AddText(pos + ImVec2(region.x - (73 + 47) - ImGui::CalcTextSize("EAX").x, (60 - ImGui::CalcTextSize("EAX").y) / 2), ImGui::GetColorU32(c::accent_color), "EAX");

            ImGui::GetBackgroundDrawList()->AddImage(image::subscription_icon, pos + ImVec2(150 + spacing.x, 20), pos + ImVec2(170 + spacing.x, 40), ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(c::accent_color));

            ImGui::PushFont(font::lexand_tabs_bold);
            ImGui::GetBackgroundDrawList()->AddText(pos + ImVec2(183 + (spacing.x), (60 - ImGui::CalcTextSize("SUBSCRIPTIONS").y) / 2), ImGui::GetColorU32(c::text::text), "SUBSCRIPTIONS");
            ImGui::PopFont();

            ImGui::BeginDisabled(login_page);
            {
                static int tabs;
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(c::tabs::spacing));

                ImGui::SetCursorPos(ImVec2(10, 70));
                ImGui::BeginGroup();
                {

                    TextColoredTAB(ImGui::GetColorU32(c::accent_color), "MAIN", font::lexand_tabs_bold);

                    if (ImGui::Tab(0 == tabs, "Subscriptins", ImVec2(122, 20))) tabs = 0;

                    TextColoredTAB(ImGui::GetColorU32(c::accent_color), "LINKS", font::lexand_tabs_bold);

                    if (ImGui::Tab(1 == tabs, "Personal area", ImVec2(122, 20))) tabs = 1;

                    if (ImGui::Tab(2 == tabs, "Webside", ImVec2(122, 20))) tabs = 2;

                    if (ImGui::Tab(3 == tabs, "FAQ", ImVec2(122, 20))) tabs = 3;

                }
                ImGui::EndGroup();
                ImGui::PopStyleVar();

                opticaly_info = ImLerp(opticaly_info, size < 0.99f ? login_page ? 0.1f : 1.0f : 0.0f, ImGui::GetIO().DeltaTime * 6.f);

                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, opticaly_info);

                ImGui::SetCursorPos(ImVec2(150, 60) + spacing);
                ImGui::BeginGroup();
                {

                    static float tab_alpha = 0.f; /* */ static float tab_add; /* */ static int active_tab = 0;

                    tab_alpha = ImClamp(tab_alpha + (4.f * ImGui::GetIO().DeltaTime * (tabs == active_tab ? 1.f : -1.f)), 0.f, 1.f);

                    if (tab_alpha == 0.f && tab_add == 0.f) active_tab = tabs;

                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, tab_alpha * style->Alpha);

                    if (active_tab == 0) {

                        ImGui::GetBackgroundDrawList()->AddImage(image::logout_icon, pos + ImVec2((150 - 20) / 2 - 35, region.y - (80 - 20) / 2), pos + ImVec2((150 + 20) / 2 - 35, region.y - (80 + 20) / 2), ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(c::accent_color));
                        if (GetAsyncKeyState(VK_LBUTTON) && ImGui::IsMouseHoveringRect(pos + ImVec2(0, region.y - 50), pos + ImVec2(150, region.y), true)) login_page = true;

                        ImGui::CustomAddText(ImGui::GetBackgroundDrawList(), font::lexand_info_bold, 15.f, pos + ImVec2((150 / 2) + 20, region.y - 40), ImGui::GetColorU32(c::text::text), "LOG OUT");

                        ImGui::GetWindowDrawList()->AddImageRounded(image::game, ImGui::GetCursorScreenPos(), ImGui::GetCursorScreenPos() + ImVec2(region.x - (spacing.x * 2 + 150), 90), ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(c::text::text_active), c::child::rounding);

                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 90 + spacing.y);

                        ImGui::CustomBeginChild("INFO", "Information about our product", ImVec2(region.x - (spacing.x * 2 + 150), 150), false, NULL);
                        {

                            ImGui::GetWindowDrawList()->AddText(font::lexand_info_bold, 15.f, ImGui::GetCursorScreenPos() + ImVec2(0, 0), ImGui::GetColorU32(c::text::text_hov), "GAME");
                            ImGui::GetWindowDrawList()->AddText(font::lexand_info_bold, 15.f, ImGui::GetCursorScreenPos() + ImVec2(0, 30), ImGui::GetColorU32(c::text::text_hov), "STATUS");
                            ImGui::GetWindowDrawList()->AddText(font::lexand_info_bold, 15.f, ImGui::GetCursorScreenPos() + ImVec2(0, 60), ImGui::GetColorU32(c::text::text_hov), "LAST UPD");

                            ImGui::GetWindowDrawList()->AddText(font::lexand_info_bold, 15.f, ImGui::GetCursorScreenPos() + ImVec2(ImGui::GetContentRegionMax().x - (style->WindowPadding.x + ImGui::CalcTextSize("Counter-Strike: Global offensive").x), 0), ImGui::GetColorU32(c::text::text_hov), "Counter-Strike: Global offensive");
                            ImGui::GetWindowDrawList()->AddText(font::lexand_info_bold, 15.f, ImGui::GetCursorScreenPos() + ImVec2(ImGui::GetContentRegionMax().x - (style->WindowPadding.x + ImGui::CalcTextSize("UNDETECTED").x), 30), ImGui::GetColorU32(c::text::text_hov), "UNDETECTED");
                            ImGui::GetWindowDrawList()->AddText(font::lexand_info_bold, 15.f, ImGui::GetCursorScreenPos() + ImVec2(ImGui::GetContentRegionMax().x - (style->WindowPadding.x + ImGui::CalcTextSize("01.01.2024").x), 60), ImGui::GetColorU32(c::text::text_hov), "01.01.2024");

                                if (login_page || !internet)
                                 DrawBackgroundBlur(ImGui::GetWindowDrawList(), g_pd3dDevice);

                        }
                        ImGui::CustomEndChild();

                        if (ImGui::Button("INJECT", ImVec2(80, 30))) inject = true;

                        ImGui::SameLine();

                        size = ImLerp(size, inject ? 1.f : 0.f, ImGui::GetIO().DeltaTime * 6.f);
                        ImGui::ProgressBar(1, size, ImVec2(region.x - (spacing.x * 3 + 150) - 80, 30));

                    }
                    else if (active_tab == 3)
                    {
                        ImGui::CustomBeginChild("FAQ", "Information about our cheat and other", ImVec2(region.x - (spacing.x * 2 + 150), region.y - (spacing.y * 2 + 60)), false, NULL);
                        {
                            ImGui::PushTextWrapPos(ImGui::GetContentRegionMax().x);

                            ImGui::TextColored(ImColor(ImGui::GetColorU32(c::text::text_hov)), "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ac ut consequat semper viverra nam libero. Molestie a iaculis at erat. Nullam ac tortor vitae purus faucibus ornare suspendisse. Commodo viverra maecenas accumsan lacus vel. Nec ullamcorper sit amet risus nullam eget felis eget nunc. Volutpat consequat mauris nunc congue nisi vitae suscipit. Magnis dis parturient montes nascetur ridiculus mus mauris. Potenti nullam ac tortor vitae purus faucibus ornare. Tristique senectus et netus et malesuada fames ac. Netus et malesuada fames ac turpis egestas maecenas. Ut placerat orci nulla pellentesque. Volutpat odio facilisis mauris sit amet massa. A lacus vestibulum sed arcu non. Nibh tortor id aliquet lectus proin nibh nisl condimentum. Nunc scelerisque viverra mauris in aliquam sem fringilla ut. Et egestas quis ipsum suspendisse ultrices. Tempus imperdiet nulla malesuada pellentesque. Vivamus at augue eget arcu dictum varius duis at consectetur.Nibh praesent tristique magna sit amet purus gravida quis.Sit amet dictum sit amet justo.Quam elementum pulvinar etiam non quam.Sed felis eget velit aliquet sagittis id consectetur purus ut.Vitae elementum curabitur vitae nunc.Donec ultrices tincidunt arcu non sodales neque sodales ut.Purus faucibus ornare suspendisse sed.Venenatis a condimentum vitae sapien pellentesque habitant morbi.Nec ultrices dui sapien eget mi proin.Ipsum dolor sit amet consectetur adipiscing.Vitae ultricies leo integer malesuada nunc vel risus.Mauris commodo quis imperdiet massa tincidunt nunc.Nisl nunc mi ipsum faucibus vitae aliquet. Quis viverra nibh cras pulvinar mattis nunc sed.Semper feugiat nibh sed pulvinar proin gravida hendrerit.Eget magna fermentum iaculis eu non diam phasellus vestibulum.Arcu dictum varius duis at consectetur lorem donec massa sapien.Vel risus commodo viverra maecenas.Elementum eu facilisis sed odio morbi quis commodo odio.Eget gravida cum sociis natoque penatibus et magnis.Ac ut consequat semper viverra nam libero justo laoreet sit.Diam ut venenatis tellus in metus vulputate eu.Eu nisl nunc mi ipsum faucibus vitae. Amet nisl purus in mollis nunc sed.Turpis egestas maecenas pharetra convallis posuere morbi leo.Ultrices mi tempus imperdiet nulla malesuada pellentesque.Orci dapibus ultrices in iaculis nunc sed augue.Magna fermentum iaculis eu non diam phasellus vestibulum lorem sed.Nibh sit amet commodo nulla facilisi nullam vehicula ipsum a.Velit euismod in pellentesque massa placerat.Sit amet aliquam id diam.Vel quam elementum pulvinar etiam non quam lacus suspendisse faucibus.Non quam lacus suspendisse faucibus interdum posuere lorem ipsum dolor.Amet luctus venenatis lectus magna.Maecenas volutpat blandit aliquam etiam erat velit scelerisque in.Nec ultrices dui sapien eget mi. Purus sit amet volutpat consequat mauris nunc congue nisi vitae.Et netus et malesuada fames ac.Nibh ipsum consequat nisl vel pretium lectus quam id.Tellus molestie nunc non blandit massa.Proin nibh nisl condimentum id.Vel pretium lectus quam id leo in.A condimentum vitae sapien pellentesque habitant morbi tristique senectus.Nulla facilisi cras fermentum odio eu feugiat pretium.Eleifend donec pretium vulputate sapien nec.Tellus cras adipiscing enim eu turpis egestas pretium aenean.Non pulvinar neque laoreet suspendisse interdum consectetur.Malesuada bibendum arcu vitae elementum curabitur.Turpis massa sed elementum tempus egestas sed sed risus. Condimentum id venenatis a condimentum vitae.Amet dictum sit amet justo.Vel eros donec ac odio.Dignissim cras tincidunt lobortis feugiat vivamus at augue.Mauris a diam maecenas sed enim ut sem.Mi eget mauris pharetra et ultrices neque.Odio pellentesque diam volutpat commodo sed egestas egestas fringilla phasellus.Vel pretium lectus quam id leo in vitae turpis.Eget mauris pharetra et ultrices.Egestas sed sed risus pretium quam.Ipsum dolor sit amet consectetur adipiscing elit ut aliquam.Pretium quam vulputate dignissim suspendisse in est.Laoreet suspendisse interdum consectetur libero id.Nunc mattis enim ut tellus elementum.Risus ultricies tristique nulla aliquet enim tortor at auctor. Mattis vulputate enim nulla aliquet porttitor.Semper feugiat nibh sed pulvinar proin gravida hendrerit.In egestas erat imperdiet sed.Varius sit amet mattis vulputate enim nulla aliquet porttitor lacus.Condimentum id venenatis a condimentum.Urna condimentum mattis pellentesque id nibh tortor.Amet risus nullam eget felis eget.Sit amet mauris commodo quis imperdiet massa tincidunt nunc pulvinar.Consequat ac felis donec et odio pellentesque diam volutpat commodo.Eget sit amet tellus cras adipiscing enim eu.Diam volutpat commodo sed egestas egestas fringilla.At consectetur lorem donec massa sapien faucibus.Urna id volutpat lacus laoreet non curabitur gravida arcu.Velit dignissim sodales ut eu sem integer vitae justo eget.Eu tincidunt tortor aliquam nulla facilisi cras.Diam sit amet nisl suscipit adipiscing bibendum est ultricies. Maecenas sed enim ut sem.Quam viverra orci sagittis eu volutpat odio facilisis mauris.Facilisi etiam dignissim diam quis enim lobortis.Feugiat vivamus at augue eget arcu dictum.Tortor consequat id porta nibh venenatis cras sed.Maecenas pharetra convallis posuere morbi leo urna molestie at elementum.Odio morbi quis commodo odio aenean sed adipiscing diam.Id cursus metus aliquam eleifend mi.Interdum consectetur libero id faucibus nisl tincidunt eget nullam non.Ridiculus mus mauris vitae ultricies. Pharetra sit amet aliquam id diam maecenas.Pellentesque elit eget gravida cum sociis natoque penatibus et.Bibendum ut tristique et egestas.Proin gravida hendrerit lectus a.Ipsum faucibus vitae aliquet nec ullamcorper sit amet risus nullam.Iaculis eu non diam phasellus vestibulum lorem sed risus.Commodo sed egestas egestas fringilla phasellus faucibus.Tincidunt lobortis feugiat vivamus at.Vulputate enim nulla aliquet porttitor lacus.Non sodales neque sodales ut etiam sit amet.Etiam dignissim diam quis enim lobortis scelerisque fermentum.Ut tellus elementum sagittis vitae et leo duis.Id eu nisl nunc mi.Amet consectetur adipiscing elit duis tristique sollicitudin.Magna ac placerat vestibulum lectus mauris ultrices eros in cursus.Et egestas quis ipsum suspendisse ultrices.Feugiat in fermentum posuere urna nec tincidunt praesent.Consectetur a erat nam at lectus urna duis.Purus non enim praesent elementum facilisis leo vel fringilla est.Pellentesque id nibh tortor id aliquet lectus proin nibh nisl. Libero volutpat sed cras ornare arcu dui vivamus arcu.Eget mi proin sed libero enim sed faucibus turpis.Faucibus a pellentesque sit amet porttitor eget dolor morbi.Euismod elementum nisi quis eleifend quam adipiscing vitae proin.Sed ullamcorper morbi tincidunt ornare massa eget egestas.Massa enim nec dui nunc mattis enim.Morbi enim nunc faucibus a pellentesque sit amet porttitor eget.Justo eget magna fermentum iaculis eu non.Sed sed risus pretium quam.Platea dictumst quisque sagittis purus.");

                            ImGui::PopTextWrapPos();

                        }
                        ImGui::CustomEndChild();
                    }



                    ImGui::EndGroup();
                }
                ImGui::PopStyleVar();

            }
            ImGui::EndDisabled();

            ImGui::PopStyleVar(2);

            if (size > 0.99f) {
                ImGui::CustomAddText(ImGui::GetForegroundDrawList(), font::lexand_notice_bold, 40.f, pos + ImVec2((c::background::size.x + 150) / 2, ((c::background::size.y + 60) / 2) - 25.f), ImGui::GetColorU32(c::text::text_hint, 1.f - opticaly_info), "SUCCESFULLY");
                ImGui::CustomAddText(ImGui::GetForegroundDrawList(), font::lexand_notice_bold, 40.f, pos + ImVec2((c::background::size.x + 150) / 2, ((c::background::size.y + 60) / 2) + 25.f), ImGui::GetColorU32(c::text::text_hint, 1.f - opticaly_info), "INJECTED");
            }

            if (login_page && internet)
            {

                DrawBackgroundBlur(ImGui::GetWindowDrawList(), g_pd3dDevice);

                ImGui::SetCursorPos(ImVec2(c::background::size / 4) + ImVec2(0, 15));
                ImGui::CustomBeginChild("LOG IN", "Enter your username and password", ImVec2(c::background::size / 2) - ImVec2(0, 15), true, NULL);
                {

                    std::string login_succes = "user";
                    static char login[24] = { "" };
                    ImGui::InputTextEx("##LOGIN", "Enter your login here", login, 24, ImVec2(ImGui::GetContentRegionMax().x - style->WindowPadding.x, 25), NULL);

                    std::string password_succes = "123";
                    static char password[24] = { "" };
                    ImGui::InputTextEx("##PASSWORD", "Enter your password here", password, 24, ImVec2(ImGui::GetContentRegionMax().x - style->WindowPadding.x, 25), NULL);

                    ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetCursorScreenPos() + ImVec2(50, 0), ImGui::GetCursorScreenPos() + ImVec2((ImGui::GetContentRegionMax().x - style->WindowPadding.x) - 50, 4), ImGui::GetColorU32(c::input::outline), 30.f);

                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15);

                    if (ImGui::Button("AUTHORIZATION", ImVec2(ImGui::GetContentRegionMax().x - style->WindowPadding.x, 25)) && login == login_succes && password == password_succes) login_page = false;


                }
                ImGui::CustomEndChild();
            }

            if (!internet) {
                DrawBackgroundBlur(ImGui::GetWindowDrawList(), g_pd3dDevice);

                ImGui::SetCursorPos(ImVec2(c::background::size / 2) - ImVec2(50, 50));
                ImGui::Spinner("##SPIN", 50.f, 4.f, ImGui::GetColorU32(c::accent_color));

                ImGui::CustomAddText(ImGui::GetWindowDrawList(), font::lexand_info_bold, 15.f, pos + ImVec2(c::background::size.x / 2, region.y - 40), ImGui::GetColorU32(c::text::text), "Check your internet connection");
            }

            move_window();
        }
        ImGui::End();

        ImGui::EndFrame();
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        D3DCOLOR clear_col_dx = 0;
        g_pd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
        if (g_pd3dDevice->BeginScene() >= 0)
        {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }
        HRESULT result = g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);

        // Handle loss of D3D9 device
        if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
            ResetDevice();
    }

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == nullptr)
        return false;

    // Create the D3DDevice
    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN; // Need to use an explicit format with alpha if needing per-pixel alpha composition.
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
    //g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
        return false;

    return true;
}

void CleanupDeviceD3D()
{
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
    if (g_pD3D) { g_pD3D->Release(); g_pD3D = nullptr; }
}

void ResetDevice()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
    if (hr == D3DERR_INVALIDCALL)
        IM_ASSERT(0);
    ImGui_ImplDX9_CreateDeviceObjects();
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
