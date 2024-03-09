#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include <GLFW/glfw3.h>

#include <codecvt> 
#include <locale> 
#include <iostream>
#include <string>
#include <windows.h>

#include "SnapshotReader.h"

#define WINDOW_HEIGHT 712
#define WINDOW_WIDTH 1280

#define IMGUI_DRAW_HEALTH_BAR(VAL, MAX, STR) \
    { \
    float progress_saturated = VAL / MAX; \
    char buf[32]; \
    sprintf_s(buf, "%f", VAL); \
    ImGui::ProgressBar(progress_saturated, ImVec2(0.f, 0.f), buf); \
    ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x); \
    ImGui::Text(STR); \
    }

#define IMGUI_DRAW_INSIGHT_BAR(DATA, STR) \
    { \
    float progress_saturated = DATA.stat[LoadedData::iInsightSamplePoint] / DATA.fMax; \
    char buf[32]; \
    sprintf_s(buf, "%f", DATA.stat[LoadedData::iInsightSamplePoint]); \
    ImGui::ProgressBar(progress_saturated, ImVec2(0.f, 0.f), buf); \
    ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x); \
    ImGui::Text(STR); \
    }

#define IMGUI_DRAW_STAT_STRUCTURE(DATA, STR) \
    if (ImGui::CollapsingHeader(STR)) \
    { \
        ImGui::SeparatorText("Graphs"); \
        ImGui::PlotLines(STR, DATA.stat.data(), DATA.stat.size(), 0, NULL, 0.0f, DATA.fMax, ImVec2(0, 80.0f)); \
        ImGui::PlotHistogram(STR, DATA.stat.data(), DATA.stat.size(), 0, NULL, 0.0f, DATA.fMax, ImVec2(0, 80.0f)); \
        ImGui::SeparatorText("Health bars"); \
        IMGUI_DRAW_HEALTH_BAR(DATA.fAvg, DATA.fMax, "Average"); \
        IMGUI_DRAW_HEALTH_BAR(DATA.fMax, DATA.fMax, "Max Value"); \
        IMGUI_DRAW_HEALTH_BAR(DATA.fMin, DATA.fMax, "Min Value"); \
        ImGui::Separator();     \
    }

#define COLLECT_DATA(NAM) \
    SnapshotReader::QInstance().ForeachDataEntry([&](SnapshotDatum datum) \
    { \
        LoadedData::NAM.stat.push_back(datum.NAM); \
        if (datum.NAM > LoadedData::NAM.fMax) \
        { \
            LoadedData::NAM.fMax = datum.NAM; \
            LoadedData::NAM.iMaxIndex = LoadedData::NAM.stat.size() - 1; \
        } \
        if (datum.NAM < LoadedData::NAM.fMin) \
        { \
            LoadedData::NAM.fMin = datum.NAM; \
            LoadedData::NAM.iMinIndex = LoadedData::NAM.stat.size() - 1; \
        } \
    });

struct LoadedDataGroup
{
    void Reset()
    {
        stat.clear();
        fMax = 0;
        fMin = 999999;
        fAvg = 0;
        iMinIndex = 0;
        iMaxIndex = 0;
    }

    std::vector<float> stat;
    float fMax = 0;
    float fMin = 999999;
    float fAvg;
    int iMinIndex = 0;
    int iMaxIndex = 0;
};

// NOTE: Please ensure these always match the name of their respective variables in SnapshotDatum. Failure to do so will break the COLLECT_DATA macro
namespace LoadedData
{
    bool bDataLoaded = false;
    int iInsightSamplePoint = 0;
    LoadedDataGroup FPS;
    LoadedDataGroup FrameTime;
    LoadedDataGroup ParticleCount;
    LoadedDataGroup ActiveParticleCount;
    LoadedDataGroup PixelVisits;
    LoadedDataGroup ChunkVisits;
}
// NOTE: Please ensure these always match the name of their respective variables in SnapshotDatum. Failure to do so will break the COLLECT_DATA macro

namespace InsightComboSettings
{
    int iSelection = 0;
    const char* sSettings[] = 
    {
        "FPS", 
        "Frame Time (MS)", 
        "Particle Count", 
        "Active Particle Count", 
        "Pixel Visits", 
        "Chunk Visists"
    };
    const char* sPreviewString = sSettings[iSelection];
}

namespace
{
    GLFWwindow* window;
}

/// <summary>
/// Used to configure the colour scheme for ImGui
/// </summary>
void SetImGuiStyle()
{
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
    colors[ImGuiCol_Border] = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
    colors[ImGuiCol_Button] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
    colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.50f, 0.50f, 0.50f, 0.52f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.6f, 0.6f, 0.9f, 0.36f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.6f, 0.6f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.12f, 0.33f, 0.67f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.47f, 0.72f, 0.92f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
    colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(8.00f, 8.00f);
    style.FramePadding = ImVec2(5.00f, 2.00f);
    style.CellPadding = ImVec2(6.00f, 6.00f);
    style.ItemSpacing = ImVec2(6.00f, 6.00f);
    style.ItemInnerSpacing = ImVec2(6.00f, 6.00f);
    style.TouchExtraPadding = ImVec2(0.00f, 0.00f);
    style.IndentSpacing = 25;
    style.ScrollbarSize = 15;
    style.GrabMinSize = 10;
    style.WindowBorderSize = 1;
    style.ChildBorderSize = 1;
    style.PopupBorderSize = 1;
    style.FrameBorderSize = 1;
    style.TabBorderSize = 1;
    style.WindowRounding = 7;
    style.ChildRounding = 4;
    style.FrameRounding = 3;
    style.PopupRounding = 4;
    style.ScrollbarRounding = 9;
    style.GrabRounding = 3;
    style.LogSliderDeadzone = 4;
    style.TabRounding = 4;
    style.AntiAliasedFill = true;
}

/// <summary>
/// Given a directory, try to parse that into the necessary format for display
/// </summary>
/// <param name="asDirectory">The directory to attempt to load data from</param>
void LoadData(std::string asDirectory)
{
    bool bReadOkay = SnapshotReader::QInstance().LoadSnapshot(asDirectory);
    if (bReadOkay)
    {
        // File was read okay, parse the loaded data into our itnernal structs
        // Reset data structs
        LoadedData::FPS.Reset();
        LoadedData::FrameTime.Reset();
        LoadedData::ParticleCount.Reset();
        LoadedData::ActiveParticleCount.Reset();
        LoadedData::PixelVisits.Reset();
        LoadedData::ChunkVisits.Reset();

        // Collect parsed data for each data entry
        COLLECT_DATA(FPS);
        COLLECT_DATA(FrameTime);
        COLLECT_DATA(ParticleCount);
        COLLECT_DATA(ActiveParticleCount);
        COLLECT_DATA(PixelVisits);
        COLLECT_DATA(ChunkVisits);

        // Calculate averages for each metric
        auto DataGroupAveragerFunctor = [&](LoadedDataGroup& dataGroup)
        {
            if (dataGroup.stat.size() > 0)
            {
                float fTotal = 0.f;
                for (float fStat : dataGroup.stat)
                {
                    fTotal += fStat;
                }
                dataGroup.fAvg = fTotal / (float)dataGroup.stat.size();
            }
        };

        DataGroupAveragerFunctor(LoadedData::FPS);
        DataGroupAveragerFunctor(LoadedData::FrameTime);
        DataGroupAveragerFunctor(LoadedData::ParticleCount);
        DataGroupAveragerFunctor(LoadedData::ActiveParticleCount);
        DataGroupAveragerFunctor(LoadedData::PixelVisits);
        DataGroupAveragerFunctor(LoadedData::ChunkVisits);

        // Set Window Name
        if (window)
        {
            std::string sTitle = "Perf Report Inspector (" + asDirectory + ")";
            glfwSetWindowTitle(window, sTitle.c_str());
        }
    }
    else
    {
        std::cout << "Unable to read file";
    }
    LoadedData::bDataLoaded = bReadOkay;
}

/// <summary>
/// Helper function to open the File Selection prompt on Windows
/// </summary>
void SelectFile()
{
    OPENFILENAME ofn;
    TCHAR szFile[260] = { 0 };
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    // If the user selected a file, convert from lpstr to a std::string, and pass to "LoadData"
    if (GetOpenFileName(&ofn) == TRUE)
    {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t> > converter;
        std::string sFilepath = converter.to_bytes(std::wstring(ofn.lpstrFile));
        LoadData(sFilepath);
    }
}

int main()
{
    // GLFW boilerplate

    if (!glfwInit())
        return -1;

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Perf Report Inspector", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // ImGui boilerplate
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    SetImGuiStyle();

    bool bShowImGuiDemo = false;
    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);

        // ImGui - setup
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();


        // Fullscreen window setup
        bool pOpen = true;
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);

        // Primary ImGui window draw
        if (ImGui::Begin("ImGui Window", &pOpen, flags))
        {
            // Window Menu
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    if (ImGui::MenuItem("Open"))
                    {
                        // Open the file loader dialogue
                        SelectFile();
                    }

                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Debug"))
                {
                    ImGui::MenuItem("Show ImGui Demo", NULL, &bShowImGuiDemo);
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }

            if (LoadedData::bDataLoaded)
            {
                ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
                if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags))
                {
                    if (ImGui::BeginTabItem("Graphs"))
                    {
                        IMGUI_DRAW_STAT_STRUCTURE(LoadedData::FPS, "FPS");
                        IMGUI_DRAW_STAT_STRUCTURE(LoadedData::FrameTime, "Frame Time (MS)");
                        IMGUI_DRAW_STAT_STRUCTURE(LoadedData::ParticleCount, "Particle Count");
                        IMGUI_DRAW_STAT_STRUCTURE(LoadedData::ActiveParticleCount, "Active Particle Count");
                        IMGUI_DRAW_STAT_STRUCTURE(LoadedData::PixelVisits, "Pixel Visits");
                        IMGUI_DRAW_STAT_STRUCTURE(LoadedData::ChunkVisits, "Chunk Visits");
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Insights"))
                    {
                        auto previewData = LoadedData::FPS;
                        switch (InsightComboSettings::iSelection)
                        {
                        case 1:
                            previewData = LoadedData::FrameTime;
                            break;
                        case 2:
                            previewData = LoadedData::ParticleCount;
                            break;
                        case 3:
                            previewData = LoadedData::ActiveParticleCount;
                            break;
                        case 4:
                            previewData = LoadedData::PixelVisits;
                            break;
                        case 5:
                            previewData = LoadedData::ChunkVisits;
                            break;
                        default:
                            break;
                        }
                        ImGui::PlotHistogram("", previewData.stat.data(), previewData.stat.size(), 0, NULL, 0.0f, previewData.fMax, ImVec2(0, 80.0f));
                        ImGui::SameLine();
                        ImGui::PushItemWidth(256);
                        {
                            if (ImGui::BeginCombo("Preview Graph", InsightComboSettings::sPreviewString))
                            {
                                for (int i = 0; i < IM_ARRAYSIZE(InsightComboSettings::sSettings); ++i)
                                {
                                    const bool bIsSelected = (InsightComboSettings::iSelection == i);
                                    if (ImGui::Selectable(InsightComboSettings::sSettings[i], bIsSelected))
                                    {
                                        InsightComboSettings::iSelection = i;
                                        InsightComboSettings::sPreviewString = InsightComboSettings::sSettings[i];
                                    }
                                }
                                ImGui::EndCombo();
                            }
                        }
                        ImGui::PopItemWidth();

                        ImGui::SliderInt("Sample Point", &LoadedData::iInsightSamplePoint, 0, LoadedData::FPS.stat.size() - 1);
                        ImGui::SameLine();
                        ImGui::PushItemWidth(128);
                        {
                            ImGui::InputInt("##InputInt", &LoadedData::iInsightSamplePoint, 1, 100, ImGuiInputTextFlags_::ImGuiInputTextFlags_CharsDecimal);
                        }
                        ImGui::PopItemWidth();
                        ImGui::SameLine();
                        if (ImGui::Button("Jump to Min")) { LoadedData::iInsightSamplePoint = previewData.iMinIndex; }
                        ImGui::SameLine();
                        if (ImGui::Button("Jump to Max")) { LoadedData::iInsightSamplePoint = previewData.iMaxIndex; }
                        LoadedData::iInsightSamplePoint = min(max(0, LoadedData::iInsightSamplePoint), LoadedData::FPS.stat.size() - 1);

                        ImGui::Separator();
                        IMGUI_DRAW_INSIGHT_BAR(LoadedData::FPS,                 "FPS");
                        IMGUI_DRAW_INSIGHT_BAR(LoadedData::FrameTime,           "Frame Time (MS)");
                        IMGUI_DRAW_INSIGHT_BAR(LoadedData::ParticleCount,       "Particle Count");
                        IMGUI_DRAW_INSIGHT_BAR(LoadedData::ActiveParticleCount, "Active Particle Count");
                        IMGUI_DRAW_INSIGHT_BAR(LoadedData::PixelVisits,         "Pixel Visits");
                        IMGUI_DRAW_INSIGHT_BAR(LoadedData::ChunkVisits,         "Chunk Visits");
                        ImGui::EndTabItem();
                    }
                    ImGui::EndTabBar();
                }
                ImGui::Separator();

            }

            ImGui::End();
        }

        if (bShowImGuiDemo)
        {
            ImGui::ShowDemoWindow();
        }

        // Render imgui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();

}



