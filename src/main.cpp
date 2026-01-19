#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h" // Include ImPlot
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <numeric> // For std::accumulate
#include <algorithm> // For std::sort
#include <cmath> // For std::exp, std::pow
#include <fstream> // For file export
#include <ctime> // For timestamp
#include <windows.h> // For GetCurrentDirectory and system commands

// ====================================================================================
// 模块名称: Main Entry (应用程序入口)
// 功能描述: 
//   应用程序的主入口文件。
//   负责初始化ImGui图形界面环境，管理全局应用状态(CurrentState)，
//   并调度各个功能页面(Dashboard, DataManage, Prediction)的渲染函数。
// ====================================================================================

#include "DataModel.h"

// ------------------------------------------------------------------------------------
// [全局状态]
// 描述: 管理整个App的生命周期数据
// 作用: g_EpidemicData是单一数据源(Single Source of Truth)；g_CurrentState控制页面路由。
// ------------------------------------------------------------------------------------
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

// ------------------------------------------------------------------------------------
// [函数] InitializeData
// 描述: 初始化演示数据
// 作用: 在程序启动时，自动生成武汉、上海等城市的演示数据，包括使用SIR模型生成的模拟历史记录。
// ------------------------------------------------------------------------------------
// Function to populate our data model with some initial test data
void InitializeData() {
    // 添加一个专门用于演示的城市 - 使用真实SIR模型生成数据
    g_EpidemicData.addRegion("★ 演示城市 (Demo)", 1000000, 5000, 3000, 100);
    g_EpidemicData.addRegion("武汉 (Wuhan)", 11000000, 50340, 46464, 3869);
    g_EpidemicData.addRegion("上海 (Shanghai)", 24000000, 340, 300, 7);
    g_EpidemicData.addRegion("北京 (Beijing)", 21540000, 593, 586, 9);
    g_EpidemicData.addRegion("广州 (Guangzhou)", 15310000, 349, 348, 1);
    
    auto& regions = g_EpidemicData.getRegions();
    
    // ★ 演示城市 - 使用SIR模型生成60天的"完美"历史数据
    // 这样点击"根据历史数据计算参数"后，预测曲线会完美匹配散点
    if (regions.size() > 0) {
        Region& demo = regions[0];
        
        // SIR模型参数 (用户计算后应该得到接近这些值)
        const double demo_beta = 0.35;   // 传染率
        const double demo_gamma = 0.1;   // 恢复率
        const int N = 1000000;           // 总人口
        
        // 初始状态
        double S = N - 100;  // 易感者
        double I = 100;      // 初始感染者
        double R = 0;        // 移出者
        
        // 用于记录累计值
        int cumulative_confirmed = 100;
        int cumulative_recovered = 0;
        int cumulative_deaths = 0;
        
        // 生成61天数据 (Day 0-60)
        for (int day = 0; day <= 60; ++day) {
            // 先记录当前状态
            demo.history.push_back({
                day, 
                cumulative_confirmed, 
                cumulative_recovered, 
                cumulative_deaths
            });
            
            // 然后进行SIR模型更新（为下一天准备）
            double new_infections = (demo_beta * S * I) / N;
            double new_recoveries = demo_gamma * I;
            
            S -= new_infections;
            I += new_infections - new_recoveries;
            R += new_recoveries;
            
            // 更新累计值
            cumulative_confirmed += static_cast<int>(new_infections);
            cumulative_recovered += static_cast<int>(new_recoveries * 0.95); // 95%康复
            cumulative_deaths += static_cast<int>(new_recoveries * 0.05);    // 5%死亡
        }
        
        // 更新城市当前状态为Day 60的数据（历史最后一天）
        if (!demo.history.empty()) {
            const auto& lastDay = demo.history.back();
            demo.confirmedCases = lastDay.confirmed;
            demo.recoveredCases = lastDay.recovered;
            demo.deaths = lastDay.deaths;
        }
    }
    
    // 武汉 - 简化的真实疫情数据 (30天)
    if (regions.size() > 1) {
        Region& wuhan = regions[1];
        // 模拟真实的武汉疫情曲线
        int base_confirmed[] = {
            270, 375, 444, 549, 729, 1052, 1423, 2714, 4515, 5974,
            7711, 9692, 11791, 13522, 16678, 19558, 22112, 24953, 27100, 29631,
            31728, 33366, 34874, 36385, 37914, 39462, 41152, 42752, 44412, 46169
        };
        for (int day = 0; day < 30; ++day) {
            int confirmed = base_confirmed[day];
            int recovered = static_cast<int>(confirmed * (0.1 + 0.02 * day));
            int deaths = static_cast<int>(confirmed * 0.04);
            wuhan.history.push_back({day, confirmed, recovered, deaths});
        }
        
        // 更新城市当前状态为最后一天的数据
        if (!wuhan.history.empty()) {
            const auto& lastDay = wuhan.history.back();
            wuhan.confirmedCases = lastDay.confirmed;
            wuhan.recoveredCases = lastDay.recovered;
            wuhan.deaths = lastDay.deaths;
        }
    }
    
    // 上海 - 控制良好场景 (40天)
    if (regions.size() > 2) {
        Region& shanghai = regions[2];
        for (int day = 0; day <= 40; ++day) {
            // 线性增长后趋平
            int confirmed = static_cast<int>(50 + 8 * day - 0.08 * day * day);
            if (confirmed < 50) confirmed = 50;
            int recovered = static_cast<int>(confirmed * 0.8);
            int deaths = static_cast<int>(confirmed * 0.02);
            shanghai.history.push_back({day, confirmed, recovered, deaths});
        }
        
        // 更新城市当前状态为最后一天的数据
        if (!shanghai.history.empty()) {
            const auto& lastDay = shanghai.history.back();
            shanghai.confirmedCases = lastDay.confirmed;
            shanghai.recoveredCases = lastDay.recovered;
            shanghai.deaths = lastDay.deaths;
        }
    }
}


// --- UI Component Functions (responsible for drawing only) ---

// ------------------------------------------------------------------------------------
// [UI组件] Dashboard (总览仪表盘)
// 描述: 首页统计显示
// 作用: 计算并显示全地区汇总数据(总确诊、总治愈等)，并绘制柱状图对比各城市数据。
// ------------------------------------------------------------------------------------
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
    static bool fit_axes = true;
    if (ImGui::Button("重置视图##Overview")) {
        fit_axes = true;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("如果图表范围不合适，请点击“重置视图”按钮来自动缩放。");
    }

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
        
        if (fit_axes) {
            ImPlot::SetNextAxisToFit(ImAxis_Y1);
            fit_axes = false;
        }
        if (ImPlot::BeginPlot("##BarChart", ImVec2(-1, -1))) {
            ImPlot::SetupAxes("地区", "人数", ImPlotAxisFlags_None, ImPlotAxisFlags_None);
            ImPlot::SetupAxisTicks(ImAxis_X1, positions.data(), names.size(), names.data());
            ImPlot::SetupAxisLimits(ImAxis_X1, -0.5, regions.size() - 0.5);

            ImPlot::PlotBars("累计确诊", confirmed_data.data(), names.size(), 0.2, -0.22);
            ImPlot::PlotBars("累计治愈", recovered_data.data(), names.size(), 0.2, 0);
            ImPlot::PlotBars("现存活跃", active_data.data(), names.size(), 0.2, 0.22);
            
            ImPlot::EndPlot();
        }
    }
}


// ------------------------------------------------------------------------------------
// [UI组件] Sidebar (侧边导航栏)
// 描述: 左侧固定菜单
// 作用: 提供页面切换导航按钮，以及夜间模式/白天模式的切换开关。
// ------------------------------------------------------------------------------------
// 1. Left Navigation Sidebar
void ShowSidebar() {
    static bool useDarkTheme = true; // Track current theme
    
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
    
    // Theme toggle button
    ImGui::Text("界面主题:");
    if (useDarkTheme) {
        if (ImGui::Button("[夜间] -> 切换至白天", ImVec2(-1, 0))) {
            ImGui::StyleColorsLight();
            useDarkTheme = false;
        }
    } else {
        if (ImGui::Button("[白天] -> 切换至夜间", ImVec2(-1, 0))) {
            ImGui::StyleColorsDark();
            useDarkTheme = true;
        }
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("系统状态: 正常");
    ImGui::Text("用户: Admin");
    
    ImGui::EndChild();
}

// ------------------------------------------------------------------------------------
// [UI组件] Data Management (数据管理)
// 描述: 城市数据增删改查
// 作用: 以表格形式展示所有城市数据，提供搜索、筛选、导出CSV功能，并允许用户编辑详细历史记录。
// ------------------------------------------------------------------------------------
// 2. Data Management Page
void ShowDataPage() {
    ImGui::Text(">> 城市疫情分级数据管理");
    ImGui::Separator();

    if (ImGui::CollapsingHeader("关于风险等级判定标准"))
    {
        ImGui::TextWrapped("系统根据当前活跃病例数(每10万人)来判定风险等级：");
        ImGui::BulletText("高风险 (HIGH): > 50 例活跃病例 / 10万人");
        ImGui::BulletText("中风险 (MID):  > 10 例活跃病例 / 10万人");
        ImGui::BulletText("低风险 (LOW):  <= 10 例活跃病例 / 10万人");
        ImGui::Dummy(ImVec2(0.0f, 5.0f));
        ImGui::TextDisabled("活跃病例 = 累计确诊 - 累计治愈 - 累计死亡");
        ImGui::Separator();
    }
    
    // --- Toolbar ---
    static bool exportSuccess = false;
    static std::string exportedFilePath;
    
    if (ImGui::Button("录入新城市 (+)")) { ImGui::OpenPopup("Add New Region"); }
    ImGui::SameLine();
    
    // Export to CSV button
    if (ImGui::Button("导出 Excel (CSV)")) {
        auto& regions = g_EpidemicData.getRegions();
        
        // Generate filename with timestamp
        std::time_t now = std::time(nullptr);
        char timestamp[32];
        std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", std::localtime(&now));
        std::string filename = std::string("epidemic_data_") + timestamp + ".csv";
        
        // Get full path (Windows)
        char fullPath[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, fullPath);
        exportedFilePath = std::string(fullPath) + "\\" + filename;
        
        // Open file for writing
        std::ofstream file(filename);
        if (file.is_open()) {
            // Write CSV header with BOM for Excel UTF-8 support
            file << "\xEF\xBB\xBF"; // UTF-8 BOM
            file << "城市名称,总人口,累计确诊,累计治愈,累计死亡,活跃病例,风险等级\n";
            
            // Write data rows
            for (const auto& r : regions) {
                int active = r.confirmedCases - r.recoveredCases - r.deaths;
                RiskLevel level = EpidemicData::calculateRiskLevel(r);
                const char* riskStr = EpidemicData::getRiskLevelString(level);
                
                file << r.name << ","
                     << r.population << ","
                     << r.confirmedCases << ","
                     << r.recoveredCases << ","
                     << r.deaths << ","
                     << active << ","
                     << riskStr << "\n";
            }
            
            file.close();
            exportSuccess = true;
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("导出城市数据为CSV文件（可用Excel打开）");
    }
    
    // Show export success message
    if (exportSuccess) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[导出成功!]");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", exportedFilePath.c_str());
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("打开文件夹")) {
            // Open folder in Windows Explorer
            std::string cmd = "explorer /select,\"" + exportedFilePath + "\"";
            system(cmd.c_str());
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("X")) {
            exportSuccess = false;
        }
    }
    
    ImGui::SameLine();
    static char searchBuffer[128] = "";
    ImGui::InputTextWithHint("##Search", "输入城市名查询...", searchBuffer, IM_ARRAYSIZE(searchBuffer));
    
    // 风险等级筛选
    ImGui::SameLine();
    ImGui::Text("风险等级:");
    ImGui::SameLine();
    static int riskFilter = 0; // 0=全部, 1=高风险, 2=中风险, 3=低风险
    const char* riskItems[] = { "全部", "高风险", "中风险", "低风险" };
    ImGui::SetNextItemWidth(120);
    ImGui::Combo("##RiskFilter", &riskFilter, riskItems, IM_ARRAYSIZE(riskItems));

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

    // --- Popup for "Edit Region" (Merged with History Management) ---
    static int edit_index = -1;
    if (edit_index != -1) { ImGui::OpenPopup("Edit Region"); }
    
    if (ImGui::BeginPopupModal("Edit Region", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        static char name[128];
        static int pop;
        static const char* error_text = "";
        
        Region* region = g_EpidemicData.getRegion(edit_index);
        
        if (ImGui::IsWindowAppearing() && region) {
            error_text = "";
            strncpy(name, region->name, 128);
            pop = region->population;
        }
        
        if (region) {
            // Tab Bar
            if (ImGui::BeginTabBar("EditTabs")) {
                // Tab 1: Basic Information
                if (ImGui::BeginTabItem("基本信息")) {
                    ImGui::Spacing();
                    ImGui::InputText("城市名称", name, 128);
                    ImGui::InputInt("总人口", &pop);
                    if (pop < 0) pop = 0;
                    
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.0f, 1.0f), "当前状态（自动从历史数据获取）:");
                    
                    if (!region->history.empty()) {
                        const auto& lastDay = region->history.back();
                        ImGui::Text("Day %d 的数据:", lastDay.day);
                        ImGui::BulletText("累计确诊: %d", lastDay.confirmed);
                        ImGui::BulletText("累计治愈: %d", lastDay.recovered);
                        ImGui::BulletText("累计死亡: %d", lastDay.deaths);
                        ImGui::BulletText("活跃病例: %d", lastDay.confirmed - lastDay.recovered - lastDay.deaths);
                    } else {
                        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "暂无历史数据");
                        ImGui::Text("请切换到 [历史数据] 标签添加数据");
                    }
                    
                    ImGui::EndTabItem();
                }
                
                // Tab 2: Historical Data
                if (ImGui::BeginTabItem("历史数据")) {
                    static int day = 0, h_confirmed = 0, h_recovered = 0, h_deaths = 0;
                    
                    ImGui::Spacing();
                    ImGui::Text("添加/修改记录:");
                    ImGui::InputInt("第几天 (Day)", &day);
                    ImGui::InputInt("确诊数", &h_confirmed);
                    ImGui::InputInt("治愈数", &h_recovered);
                    ImGui::InputInt("死亡数", &h_deaths);
                    
                    if (ImGui::Button("添加/更新记录")) {
                        bool found = false;
                        for (auto& rec : region->history) {
                            if (rec.day == day) {
                                rec.confirmed = h_confirmed;
                                rec.recovered = h_recovered;
                                rec.deaths = h_deaths;
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            region->history.push_back({day, h_confirmed, h_recovered, h_deaths});
                            std::sort(region->history.begin(), region->history.end(), 
                                [](const HistoricalRecord& a, const HistoricalRecord& b){ return a.day < b.day; });
                        }
                        
                        // Update current state from last history record
                        if (!region->history.empty()) {
                            const auto& lastDay = region->history.back();
                            region->confirmedCases = lastDay.confirmed;
                            region->recoveredCases = lastDay.recovered;
                            region->deaths = lastDay.deaths;
                        }
                    }
                    
                    ImGui::Dummy(ImVec2(0, 10));
                    ImGui::Text("已有历史记录 (%zu 条):", region->history.size());
                    if (ImGui::BeginChild("HistoryList", ImVec2(500, 250), true)) {
                        for (int i = 0; i < region->history.size(); ++i) {
                            auto& rec = region->history[i];
                            ImGui::Text("Day %d: 确诊:%d 治愈:%d 死亡:%d", 
                                rec.day, rec.confirmed, rec.recovered, rec.deaths);
                            ImGui::SameLine();
                            if (ImGui::SmallButton((std::string("删除##") + std::to_string(i)).c_str())) {
                                region->history.erase(region->history.begin() + i);
                                
                                // Update current state from new last record
                                if (!region->history.empty()) {
                                    const auto& lastDay = region->history.back();
                                    region->confirmedCases = lastDay.confirmed;
                                    region->recoveredCases = lastDay.recovered;
                                    region->deaths = lastDay.deaths;
                                }
                                i--;
                            }
                        }
                        ImGui::EndChild();
                    }
                    
                    ImGui::EndTabItem();
                }
                
                ImGui::EndTabBar();
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            
            // Bottom buttons
            if (ImGui::Button("保存修改", ImVec2(120, 0))) {
                if (strlen(name) == 0) {
                    error_text = "城市名称不能为空。";
                } else if (pop <= 0) {
                    error_text = "总人口必须大于0。";
                } else {
                    error_text = "";
                    strncpy(region->name, name, 128);
                    region->population = pop;
                    edit_index = -1;
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("取消", ImVec2(120, 0))) {
                error_text = "";
                edit_index = -1;
                ImGui::CloseCurrentPopup();
            }
            
            if (strlen(error_text) > 0) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", error_text);
            }
        }
        
        ImGui::EndPopup();
    }
    ImGui::Spacing();

    // --- 历史记录查询弹窗 ---
    static int history_view_index = -1;
    if (history_view_index != -1) { 
        ImGui::OpenPopup("View History"); 
    }
    
    if (ImGui::BeginPopupModal("View History", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        auto& regions = g_EpidemicData.getRegions();
        if (history_view_index >= 0 && history_view_index < regions.size()) {
            const Region& region = regions[history_view_index];
            
            ImGui::Text("城市: %s", region.name);
            ImGui::Separator();
            
            if (!region.history.empty()) {
                // 显示历史数据趋势图
                std::vector<double> hist_days, hist_confirmed, hist_recovered, hist_deaths, hist_active;
                for (const auto& rec : region.history) {
                    hist_days.push_back(static_cast<double>(rec.day));
                    hist_confirmed.push_back(static_cast<double>(rec.confirmed));
                    hist_recovered.push_back(static_cast<double>(rec.recovered));
                    hist_deaths.push_back(static_cast<double>(rec.deaths));
                    hist_active.push_back(static_cast<double>(rec.confirmed - rec.recovered - rec.deaths));
                }
                
                static bool fit_hist_axes = false;
                
                ImGui::Text("历史数据趋势图:");
                ImGui::SameLine();
                if (ImGui::Button("重置视图", ImVec2(120, 0))) {
                    fit_hist_axes = true;
                }
                ImGui::SameLine();
                ImGui::TextDisabled("(提示: 可以用鼠标拖拽和滚轮缩放图表)");
                
                if (fit_hist_axes) {
                    ImPlot::SetNextAxesToFit();
                    fit_hist_axes = false;
                }
                
                if (ImPlot::BeginPlot("##HistoryPlot", ImVec2(800, 400))) {
                    ImPlot::SetupAxes("天数", "病例数");
                    // 使用散点图显示历史数据点
                    ImPlot::PlotScatter("累计确诊", hist_days.data(), hist_confirmed.data(), hist_days.size());
                    ImPlot::PlotScatter("累计治愈", hist_days.data(), hist_recovered.data(), hist_days.size());
                    ImPlot::PlotScatter("累计死亡", hist_days.data(), hist_deaths.data(), hist_days.size());
                    ImPlot::PlotScatter("活跃病例", hist_days.data(), hist_active.data(), hist_days.size());
                    ImPlot::EndPlot();
                }
                
                ImGui::Spacing();
                ImGui::Text("历史记录详情 (共 %zu 条):", region.history.size());
                if (ImGui::BeginChild("HistoryDetails", ImVec2(800, 150), true)) {
                    if (ImGui::BeginTable("HistTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                        ImGui::TableSetupColumn("天数");
                        ImGui::TableSetupColumn("确诊");
                        ImGui::TableSetupColumn("治愈");
                        ImGui::TableSetupColumn("死亡");
                        ImGui::TableSetupColumn("活跃");
                        ImGui::TableHeadersRow();
                        
                        for (const auto& rec : region.history) {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0); ImGui::Text("%d", rec.day);
                            ImGui::TableSetColumnIndex(1); ImGui::Text("%d", rec.confirmed);
                            ImGui::TableSetColumnIndex(2); ImGui::Text("%d", rec.recovered);
                            ImGui::TableSetColumnIndex(3); ImGui::Text("%d", rec.deaths);
                            ImGui::TableSetColumnIndex(4); ImGui::Text("%d", rec.confirmed - rec.recovered - rec.deaths);
                        }
                        
                        ImGui::EndTable();
                    }
                    ImGui::EndChild();
                }
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "该地区暂无历史数据");
                ImGui::TextWrapped("提示: 您可以点击\"修改\"按钮，在\"历史数据\"标签中添加历史记录。");
            }
        }
        
        ImGui::Spacing();
        if (ImGui::Button("关闭", ImVec2(120, 0))) {
            history_view_index = -1;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }

    // --- Data Table ---
    if (ImGui::BeginTable("TableData", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("城市名称");
        ImGui::TableSetupColumn("总人口");
        ImGui::TableSetupColumn("累计确诊");
        ImGui::TableSetupColumn("累计治愈");
        ImGui::TableSetupColumn("累计死亡");
        ImGui::TableSetupColumn("当前风险等级");
        ImGui::TableSetupColumn("历史记录");
        ImGui::TableSetupColumn("操作");
        ImGui::TableHeadersRow();

        auto& regions = g_EpidemicData.getRegions();
        int region_to_delete = -1;

        for (int i = 0; i < regions.size(); ++i) {
            // 应用搜索筛选
            if (searchBuffer[0] != '\0' && strstr(regions[i].name, searchBuffer) == nullptr) continue;
            
            // 应用风险等级筛选
            RiskLevel level = EpidemicData::calculateRiskLevel(regions[i]);
            if (riskFilter == 1 && level != RiskLevel::High) continue;
            if (riskFilter == 2 && level != RiskLevel::Medium) continue;
            if (riskFilter == 3 && level != RiskLevel::Low) continue;
            
            ImGui::PushID(i);
            ImGui::TableNextRow();
            Region& region = regions[i];

            ImGui::TableSetColumnIndex(0); ImGui::Text("%s", region.name);
            ImGui::TableSetColumnIndex(1); ImGui::Text("%d", region.population);
            ImGui::TableSetColumnIndex(2); ImGui::Text("%d", region.confirmedCases);
            ImGui::TableSetColumnIndex(3); ImGui::Text("%d", region.recoveredCases);
            ImGui::TableSetColumnIndex(4); ImGui::Text("%d", region.deaths);

            ImGui::TableSetColumnIndex(5);
            ImGui::TextColored(EpidemicData::getRiskLevelColor(level), "%s", EpidemicData::getRiskLevelString(level));

            ImGui::TableSetColumnIndex(6);
            if (!region.history.empty()) {
                ImGui::Text("%zu 条", region.history.size());
                ImGui::SameLine();
                if (ImGui::SmallButton("查看")) { 
                    history_view_index = i; 
                }
            } else {
                ImGui::TextDisabled("无");
            }

            ImGui::TableSetColumnIndex(7);
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

// ------------------------------------------------------------------------------------
// [UI组件] Prediction Model (预测模型)
// 描述: SIR模型交互界面
// 作用: 
//   允许用户调整传染率(Beta)、恢复率(Gamma)等参数，实时运行SIR微分方程模拟，
//   并将预测曲线与历史真实数据绘制在同一张图表中进行对比。
// ------------------------------------------------------------------------------------
// 3. Prediction Model Page
void ShowPredictionPage() {
    ImGui::Text(">> 传染病动力学预测 (SIR Model)");
    ImGui::Separator();

    if (ImGui::CollapsingHeader("关于SIR模型说明", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::TextWrapped("来源: 这是一个数学预测模型,用来模拟和预测疫情未来的可能发展趋势。它不是基于个案统计,而是将人群分为几个大的群体来估算。");
        ImGui::BulletText("易感者 (Susceptible, S): 指的是理论上有可能被感染的健康人群。在模型开始时,这个数值通常是总人口减去已经感染和康复的人。");
        ImGui::BulletText("感染者 (Infected, I): 模型中代表当前时刻具有传染性的人群。");
        ImGui::BulletText("移出者 (Removed, R): 模型中代表已经康复并获得免疫(或因病去世)的人群。");
        ImGui::Dummy(ImVec2(0.0f, 5.0f));
        ImGui::TextUnformatted("--- 模型如何初始化 ---");
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 1.0f, 1.0f));
        ImGui::TextWrapped("当您选择一个城市或调整参数时，模型将使用所选城市的当前数据作为第0天的初始状态:");
        ImGui::BulletText("I (感染者) = 当前活跃病例 (累计确诊 - 累计治愈 - 累计死亡)");
        ImGui::BulletText("R (移出者) = 累计治愈 + 累计死亡");
        ImGui::BulletText("S (易感者) = 总人口 - I - R");
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0.0f, 5.0f));
        ImGui::TextWrapped("作用: 用于预测未来的感染高峰、疫情规模等。");
        ImGui::Separator();
        ImGui::TextUnformatted(u8"核心数学公式:");
        ImGui::BulletText(u8"dS/dt = - (β * S * I) / N");
        ImGui::BulletText(u8"dI/dt = (β * S * I) / N - γ * I");
        ImGui::BulletText(u8"dR/dt = γ * I");
        ImGui::Dummy(ImVec2(0.0f, 5.0f)); 
        ImGui::TextUnformatted(u8"其中:");
        ImGui::BulletText(u8"S: 易感者 (Susceptible)");
        ImGui::BulletText(u8"I: 感染者 (Infected)");
        ImGui::BulletText(u8"R: 移出者 (Removed)");
        ImGui::BulletText(u8"N: 总人口 (S+I+R)");
        ImGui::BulletText(u8"β (Beta): 传染率, 即一个感染者在单位时间内有效接触并传染给易感者的平均人数");
        ImGui::BulletText(u8"γ (Gamma): 恢复率, 即一个感染者在单位时间内恢复(或死亡)的比例");

        ImGui::Separator();
    }

    static int selected_region_idx = 0;
    static bool auto_fit_plot = true; // Control axis fitting
    auto& regions = g_EpidemicData.getRegions();
    ImGui::Columns(2, "PredCols", false); ImGui::SetColumnWidth(0, 320);

    // --- Left side: Controls ---
    {
        ImGui::BeginChild("Controls");
        ImGui::Text("模型参数设置"); ImGui::Dummy(ImVec2(0, 10));
        const char* current_name = (selected_region_idx < regions.size()) ? regions[selected_region_idx].name : "无";
        
        static bool first_run = true;
        bool should_run_sim = false;

        if (ImGui::BeginCombo("选择城市", current_name)) {
            for (int i = 0; i < regions.size(); ++i) {
                if (ImGui::Selectable(regions[i].name, selected_region_idx == i)) { 
                    if (selected_region_idx != i) {
                        selected_region_idx = i;
                        should_run_sim = true; 
                        auto_fit_plot = true; // City changed, so fit the plot
                    }
                }
                if (selected_region_idx == i) { ImGui::SetItemDefaultFocus(); }
            }
            ImGui::EndCombo();
        }

        static float beta = 0.3f, gamma = 0.1f; static int days = 90;
        bool params_changed = false;
        
        if (selected_region_idx < regions.size()) {
            if (ImGui::Button("根据历史数据计算参数")) {
                const auto& r = regions[selected_region_idx];
                beta = (float)r.calculateAverageBeta();
                gamma = (float)r.calculateAverageGamma();
                should_run_sim = true;
            }
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("根据已录入的历史确诊/治愈/死亡数据，自动估算平均传染率(Beta)和恢复率(Gamma)。\n至少需要2天的历史记录。");
            }
        }
        
        params_changed |= ImGui::SliderFloat("传染率 (Beta)", &beta, 0.0f, 2.0f, "%.3f");
        params_changed |= ImGui::SliderFloat("恢复率 (Gamma)", &gamma, 0.0f, 1.0f, "%.3f");
        params_changed |= ImGui::SliderInt("预测天数", &days, 10, 365);

        if (params_changed) {
            should_run_sim = true;
            // auto_fit_plot = true; // Parameters changed, so fit the plot. Let user click "Reset View" instead.
        }

        ImGui::Spacing();
        if (ImGui::Button("重置视图", ImVec2(-1, 30))) {
            auto_fit_plot = true;
        }

        if (first_run || should_run_sim) {
            if (selected_region_idx < regions.size()) {
                Region& r = regions[selected_region_idx];
                r.simulation.setBeta(beta);
                r.simulation.setGamma(gamma);
                
                // 如果有历史数据，从历史末端继续预测
                if (!r.history.empty()) {
                    const auto& lastHistory = r.history.back();
                    int startDay = lastHistory.day + 1;  // 从历史最后一天的下一天开始
                    int active = lastHistory.confirmed - lastHistory.recovered - lastHistory.deaths;
                    int removed = lastHistory.recovered + lastHistory.deaths;
                    
                    r.simulation.reset(r.population, active > 0 ? active : 1, removed, startDay);
                } else {
                    // 没有历史数据，从当前状态开始（Day 0）
                    int active = r.confirmedCases - r.recoveredCases - r.deaths;
                    r.simulation.reset(r.population, active > 0 ? active : 1, r.recoveredCases + r.deaths, 0);
                }
                
                r.simulation.run(days);
            }
            if (first_run) { auto_fit_plot = true; } // Also auto-fit on the very first run
            first_run = false;
        }

        // --- Debug Info ---
        ImGui::Separator();
        ImGui::Text("Debug Info:");
        if (selected_region_idx < regions.size()) {
            Region& r = regions[selected_region_idx];
            ImGui::Text("Model Beta: %.3f", r.simulation.getBeta());
            ImGui::Text("Model Gamma: %.3f", r.simulation.getGamma());
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
        
        // Conditionally fit the plot to the data, then give control to the user.
        if (auto_fit_plot) {
            ImPlot::SetNextAxesToFit();
            auto_fit_plot = false; // Consume the flag, handing control to user until next reset.
        }

        if (ImPlot::BeginPlot("SIR Model", ImVec2(-1,-1))) {
            ImPlot::SetupAxes("天 (Days)", "人数 (Population)");
            if (!plot_data.days.empty()) {
                ImPlot::PlotLine("易感者 (S)", plot_data.days.data(), plot_data.s.data(), plot_data.days.size());
                ImPlot::PlotLine("感染者 (I)", plot_data.days.data(), plot_data.i.data(), plot_data.days.size());
                ImPlot::PlotLine("移出者 (R)", plot_data.days.data(), plot_data.r.data(), plot_data.days.size());
            }

            // Draw historical data scatter points
            if (selected_region_idx < regions.size()) {
                Region& r = regions[selected_region_idx];
                if (!r.history.empty()) {
                    std::vector<double> h_days, h_I, h_R;
                    h_days.reserve(r.history.size());
                    h_I.reserve(r.history.size());
                    h_R.reserve(r.history.size());

                    for(const auto& rec : r.history) {
                        h_days.push_back(static_cast<double>(rec.day));
                        // Historical Active Infections = Confirmed - Recovered - Deaths
                        double active = static_cast<double>(rec.confirmed - rec.recovered - rec.deaths);
                        // Historical Removed = Recovered + Deaths
                        double removed = static_cast<double>(rec.recovered + rec.deaths);
                        
                        h_I.push_back(active);
                        h_R.push_back(removed);
                    }

                    ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle);
                    ImPlot::PlotScatter("历史-活跃 (I)", h_days.data(), h_I.data(), h_days.size());
                    
                    ImPlot::SetNextMarkerStyle(ImPlotMarker_Square);
                    ImPlot::PlotScatter("历史-移出 (R)", h_days.data(), h_R.data(), h_days.size());
                }
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

    GLFWwindow* window = glfwCreateWindow(1280, 720, "疫情信息管理与预测系统v0.1.0", NULL, NULL);
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
