#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h" // Include ImPlot
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <numeric> // For std::accumulate

#include "DataModel.h"

// --- Global Application State ---

// The single source of truth for all epidemic data
EpidemicData g_EpidemicData;

// Enum for managing which page is currently visible
enum AppState {
    State_Dashboard,    // Homepage/Dashboard
    State_DataManage,   // Data Management
    State_Prediction    // Prediction Model
};

// Global variable to control the currently displayed page
static AppState g_CurrentState = State_Dashboard;

// --- Data Initialization ---

// Function to populate our data model with some initial test data
void InitializeData() {
    g_EpidemicData.addRegion("武汉 (Wuhan)", 11000000, 50340, 46464, 3869);
    g_EpidemicData.addRegion("长沙 (Changsha)", 8000000, 242, 242, 0);
    g_EpidemicData.addRegion("上海 (Shanghai)", 24000000, 340, 300, 7);
    g_EpidemicData.addRegion("北京 (Beijing)", 21540000, 593, 586, 9);
    g_EpidemicData.addRegion("广州 (Guangzhou)", 15310000, 349, 348, 1);
}


// --- UI Component Functions (responsible for drawing only) ---

// 0. Dashboard / Statistics Page
void ShowDashboardPage() {
    ImGui::Text(">> 全局疫情数据总览");
    ImGui::Separator();

    auto& regions = g_EpidemicData.getRegions();

    // Calculate totals
    long long total_pop = 0;
    long long total_confirmed = 0;
    long long total_recovered = 0;
    long long total_deaths = 0;
    for(const auto& r : regions) {
        total_pop += r.population;
        total_confirmed += r.confirmedCases;
        total_recovered += r.recoveredCases;
        total_deaths += r.deaths;
    }
    long long total_active = total_confirmed - total_recovered - total_deaths;

    // Display totals
    ImGui::Text("核心数据统计");
    ImGui::BulletText("地区总数: %d", (int)regions.size());
    ImGui::BulletText("覆盖总人口: %lld", total_pop);
    ImGui::BulletText("累计确诊病例: %lld", total_confirmed);
    ImGui::BulletText("累计治愈病例: %lld", total_recovered);
    ImGui::BulletText("累计死亡病例: %lld", total_deaths);
    ImGui::BulletText("现存活跃病例: %lld", total_active);
    
    ImGui::Separator();
    ImGui::Text("各地区确诊数条形图");

    // Bar Chart
    if (!regions.empty()) {
        std::vector<const char*> names;
        std::vector<double> confirmed_data;
        std::vector<double> recovered_data;
        std::vector<double> active_data;
        std::vector<double> positions;

        for (int i = 0; i < regions.size(); ++i) {
            names.push_back(regions[i].name);
            positions.push_back(static_cast<double>(i));
            confirmed_data.push_back(static_cast<double>(regions[i].confirmedCases));
            recovered_data.push_back(static_cast<double>(regions[i].recoveredCases));
            active_data.push_back(static_cast<double>(regions[i].confirmedCases - regions[i].recoveredCases - regions[i].deaths));
        }
        
        if (ImPlot::BeginPlot("##BarChart", ImVec2(-1, -1))) {
            ImPlot::SetupAxes("地区", "人数", ImPlotAxisFlags_None, ImPlotAxisFlags_None);
            ImPlot::SetupAxisTicks(ImAxis_X1, positions.data(), names.size(), names.data());
            ImPlot::SetupAxisLimits(ImAxis_X1, -0.5, regions.size() - 0.5);

            ImPlot::PlotBars("累计确诊", positions.data(), confirmed_data.data(), names.size(), 0.2, -0.22);
            ImPlot::PlotBars("累计治愈", positions.data(), recovered_data.data(), names.size(), 0.2, 0);
            ImPlot::PlotBars("现存活跃", positions.data(), active_data.data(), names.size(), 0.2, 0.22);
            
            ImPlot::EndPlot();
        }
    }
}


// 1. Left Navigation Sidebar
void ShowSidebar() {
    ImGui::BeginChild("Sidebar", ImVec2(200, 0), true); 
    
    ImGui::TextDisabled("功能菜单");
    ImGui::Separator();

    if (ImGui::Selectable("  总览仪表盘", g_CurrentState == State_Dashboard)) {
        g_CurrentState = State_Dashboard;
    }
    if (ImGui::Selectable("  数据管理中心", g_CurrentState == State_DataManage)) {
        g_CurrentState = State_DataManage;
    }
    if (ImGui::Selectable("  疫情预测模型", g_CurrentState == State_Prediction)) {
        g_CurrentState = State_Prediction;
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("系统状态: 正常");
    ImGui::Text("用户: Admin");
    
    ImGui::EndChild();
}

// 2. Data Management Page
void ShowDataPage() {
    ImGui::Text(">> 城市疫情分级数据管理");
    ImGui::Separator();
    
    // --- Toolbar ---
    if (ImGui::Button("录入新城市 (+)")) { ImGui::OpenPopup("Add New Region"); }
    ImGui::SameLine();
    ImGui::Button("导出 Excel");
    ImGui::SameLine();
    static char searchBuffer[128] = "";
    ImGui::InputTextWithHint("##Search", "输入城市名查询...", searchBuffer, IM_ARRAYSIZE(searchBuffer));

    // --- Popup for "Add New Region" ---
    if (ImGui::BeginPopupModal("Add New Region", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        static char name[128] = "";
        static int pop = 0, confirmed = 0, recovered = 0, deaths = 0;
        static const char* error_text = "";

        ImGui::InputText("城市名称", name, 128); 
        ImGui::InputInt("总人口", &pop);
        ImGui::InputInt("累计确诊", &confirmed);
        ImGui::InputInt("累计治愈", &recovered);
        ImGui::InputInt("累计死亡", &deaths);
        
        if (pop < 0) pop = 0;
        if (confirmed < 0) confirmed = 0;
        if (recovered < 0) recovered = 0;
        if (deaths < 0) deaths = 0;

        if (ImGui::Button("保存")) {
            if (strlen(name) == 0) {
                error_text = "城市名称不能为空。";
            } else if (pop < confirmed) {
                error_text = "总人口不能少于累计确诊数。";
            } else if (confirmed < (recovered + deaths)) {
                error_text = "累计确诊数不能少于治愈与死亡数之和。";
            } else {
                error_text = "";
                g_EpidemicData.addRegion(name, pop, confirmed, recovered, deaths);
                memset(name, 0, sizeof(name)); pop = confirmed = recovered = deaths = 0;
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("取消")) { 
            error_text = "";
            memset(name, 0, sizeof(name)); pop = confirmed = recovered = deaths = 0;
            ImGui::CloseCurrentPopup(); 
        }

        if (strlen(error_text) > 0) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", error_text);
        }

        ImGui::EndPopup();
    }

    // --- Popup for "Edit Region" ---
    static int edit_index = -1;
    if (edit_index != -1) { ImGui::OpenPopup("Edit Region"); }
    if (ImGui::BeginPopupModal("Edit Region", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        static char name[128]; static int pop, confirmed, recovered, deaths;
        static const char* error_text = "";

        if (ImGui::IsWindowAppearing()) {
            error_text = "";
            if (Region* region = g_EpidemicData.getRegion(edit_index)) {
                strncpy(name, region->name, 128); pop = region->population;
                confirmed = region->confirmedCases; recovered = region->recoveredCases; deaths = region->deaths;
            }
        }
        ImGui::InputText("城市名称", name, 128); ImGui::InputInt("总人口", &pop);
        ImGui::InputInt("累计确诊", &confirmed); ImGui::InputInt("累计治愈", &recovered);
        ImGui::InputInt("累计死亡", &deaths);

        if (pop < 0) pop = 0;
        if (confirmed < 0) confirmed = 0;
        if (recovered < 0) recovered = 0;
        if (deaths < 0) deaths = 0;

        if (ImGui::Button("保存修改")) {
            if (strlen(name) == 0) {
                error_text = "城市名称不能为空。";
            } else if (pop < confirmed) {
                error_text = "总人口不能少于累计确诊数。";
            } else if (confirmed < (recovered + deaths)) {
                error_text = "累计确诊数不能少于治愈与死亡数之和。";
            } else {
                error_text = "";
                if (Region* region = g_EpidemicData.getRegion(edit_index)) {
                    strncpy(region->name, name, 128); region->population = pop;
                    region->confirmedCases = confirmed; region->recoveredCases = recovered; region->deaths = deaths;
                }
                edit_index = -1; ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("取消")) { 
            error_text = "";
            edit_index = -1; 
            ImGui::CloseCurrentPopup(); 
        }

        if (strlen(error_text) > 0) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", error_text);
        }
        ImGui::EndPopup();
    }
    ImGui::Spacing();

    // --- Data Table ---
    if (ImGui::BeginTable("TableData", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("城市名称");
        ImGui::TableSetupColumn("总人口");
        ImGui::TableSetupColumn("累计确诊");
        ImGui::TableSetupColumn("累计治愈");
        ImGui::TableSetupColumn("累计死亡");
        ImGui::TableSetupColumn("当前风险等级");
        ImGui::TableSetupColumn("操作");
        ImGui::TableHeadersRow();

        auto& regions = g_EpidemicData.getRegions();
        int region_to_delete = -1;

        for (int i = 0; i < regions.size(); ++i) {
            if (searchBuffer[0] != '\0' && strstr(regions[i].name, searchBuffer) == nullptr) continue;
            
            ImGui::PushID(i);
            ImGui::TableNextRow();
            Region& region = regions[i];

            ImGui::TableSetColumnIndex(0); ImGui::Text("%s", region.name);
            ImGui::TableSetColumnIndex(1); ImGui::Text("%d", region.population);
            ImGui::TableSetColumnIndex(2); ImGui::Text("%d", region.confirmedCases);
            ImGui::TableSetColumnIndex(3); ImGui::Text("%d", region.recoveredCases);
            ImGui::TableSetColumnIndex(4); ImGui::Text("%d", region.deaths);

            ImGui::TableSetColumnIndex(5);
            RiskLevel level = EpidemicData::calculateRiskLevel(region);
            ImGui::TextColored(EpidemicData::getRiskLevelColor(level), "%s", EpidemicData::getRiskLevelString(level));

            ImGui::TableSetColumnIndex(6);
            if (ImGui::Button("修改")) { edit_index = i; }
            ImGui::SameLine();
            if (ImGui::Button("删除")) { region_to_delete = i; }
            
            ImGui::PopID();
        }

        if (region_to_delete != -1) {
            g_EpidemicData.deleteRegion(region_to_delete);
        }

        ImGui::EndTable();
    }
}

// Helper struct for ImPlot's PlotLine function
struct PlotData {
    std::vector<double> days, s, i, r;
    void FromHistory(const std::vector<SIRDataPoint>& history) {
        days.clear(); s.clear(); i.clear(); r.clear();
        for (const auto& p : history) {
            days.push_back(p.day); s.push_back(p.susceptible);
            i.push_back(p.infected); r.push_back(p.recovered);
        }
    }
};

// 3. Prediction Model Page
void ShowPredictionPage() {
    ImGui::Text(">> 传染病动力学预测 (SIR Model)");
    ImGui::Separator();
    static int selected_region_idx = 0;
    auto& regions = g_EpidemicData.getRegions();
    ImGui::Columns(2, "PredCols", false); ImGui::SetColumnWidth(0, 320);

    // --- Left side: Controls ---
    {
        ImGui::BeginChild("Controls");
        ImGui::Text("模型参数设置"); ImGui::Dummy(ImVec2(0, 10));
        const char* current_name = (selected_region_idx < regions.size()) ? regions[selected_region_idx].name : "无";
        if (ImGui::BeginCombo("选择城市", current_name)) {
            for (int i = 0; i < regions.size(); ++i) {
                if (ImGui::Selectable(regions[i].name, selected_region_idx == i)) { selected_region_idx = i; }
                if (selected_region_idx == i) { ImGui::SetItemDefaultFocus(); }
            }
            ImGui::EndCombo();
        }
        static double beta = 0.3, gamma = 0.1; static int days = 90;
        ImGui::SliderFloat("传染率 (Beta)", (float*)&beta, 0.0, 2.0, "%.3f");
        ImGui::SliderFloat("恢复率 (Gamma)", (float*)&gamma, 0.0, 1.0, "%.3f");
        ImGui::SliderInt("预测天数", &days, 10, 365);
        if (ImGui::Button("运行预测", ImVec2(300, 40))) {
            if (selected_region_idx < regions.size()) {
                Region& r = regions[selected_region_idx];
                r.simulation.setBeta(beta); r.simulation.setGamma(gamma);
                int active = r.confirmedCases - r.recoveredCases - r.deaths;
                r.simulation.reset(r.population, active > 0 ? active : 1);
                for (int d = 0; d < days; ++d) { r.simulation.step(); }
            }
        }
        ImGui::EndChild();
    }
    ImGui::NextColumn();

    // --- Right side: Plot ---
    {
        ImGui::Text("数据可视化结果");
        static PlotData plot_data;
        if (selected_region_idx < regions.size()) {
            plot_data.FromHistory(regions[selected_region_idx].simulation.getHistory());
        }
        if (ImPlot::BeginPlot("SIR Model", ImVec2(-1,-1))) {
            ImPlot::SetupAxes("天 (Days)", "人数 (Population)");
            if (!plot_data.days.empty()) {
                ImPlot::PlotLine("易感者 (S)", plot_data.days.data(), plot_data.s.data(), plot_data.days.size());
                ImPlot::PlotLine("感染者 (I)", plot_data.days.data(), plot_data.i.data(), plot_data.days.size());
                ImPlot::PlotLine("康复者 (R)", plot_data.days.data(), plot_data.r.data(), plot_data.days.size());
            }
            ImPlot::EndPlot();
        }
    }
    ImGui::Columns(1);
}

// --- Main Program ---
int main(int, char**)
{
    glfwInit();
    const char* glsl_version = "#version 330";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "疫情信息管理与预测系统", NULL, NULL);
    if (window == NULL) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("c:/windows/fonts/msyh.ttc", 20.0f, nullptr, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    InitializeData();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("MainRoot", nullptr, 
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        ShowSidebar();

        ImGui::SameLine();

        ImGui::BeginChild("ContentRegion", ImVec2(0, 0), true);
        if (g_CurrentState == State_Dashboard) {
            ShowDashboardPage();
        } else if (g_CurrentState == State_DataManage) {
            ShowDataPage();
        } else if (g_CurrentState == State_Prediction) {
            ShowPredictionPage();
        }
        ImGui::EndChild();
        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
