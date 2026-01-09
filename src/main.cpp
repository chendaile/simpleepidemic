#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <stdio.h>

// 定义当前页面状态
enum AppState {
    State_Dashboard,    // 首页/总览
    State_DataManage,   // 数据管理
    State_Prediction    // 预测模型
};

// 全局变量，用来控制当前显示哪个页面
static AppState g_CurrentState = State_DataManage;

// --- 界面组件函数 (只负责画图，不负责算数) ---

// 1. 左侧导航栏
void ShowSidebar() {
    // 这一块区域宽度固定为 200
    ImGui::BeginChild("Sidebar", ImVec2(200, 0), true); 
    
    ImGui::TextDisabled("功能菜单");
    ImGui::Separator();

    // 漂亮的按钮布局
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

// 2. 数据管理页面 (对应需求 2, 4)
void ShowDataPage() {
    ImGui::Text(">> 城市疫情分级数据管理");
    ImGui::Separator();
    
    // 模拟的工具栏
    ImGui::Button("录入新城市 (+)"); ImGui::SameLine();
    ImGui::Button("导出 Excel"); ImGui::SameLine();
    static char searchBuffer[128] = "";
    ImGui::InputTextWithHint("##Search", "输入城市名查询...", searchBuffer, IM_ARRAYSIZE(searchBuffer));

    ImGui::Spacing();

    // 表格区域
    // 5列：城市，确诊，治愈，风险等级，操作
    if (ImGui::BeginTable("TableData", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
        // 表头
        ImGui::TableSetupColumn("城市名称");
        ImGui::TableSetupColumn("确诊人数");
        ImGui::TableSetupColumn("治愈人数");
        ImGui::TableSetupColumn("当前风险等级"); // 对应需求(4)
        ImGui::TableSetupColumn("操作");
        ImGui::TableHeadersRow();

        // --- 这里是假数据，为了让你看效果 ---
        // 行 1
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0); ImGui::Text("武汉 (Wuhan)");
        ImGui::TableSetColumnIndex(1); ImGui::Text("50340");
        ImGui::TableSetColumnIndex(2); ImGui::Text("46464");
        ImGui::TableSetColumnIndex(3); ImGui::TextColored(ImVec4(1, 0, 0, 1), "高风险 (HIGH)"); // 红色
        ImGui::TableSetColumnIndex(4); ImGui::Button("修改##1"); ImGui::SameLine(); ImGui::Button("删除##1");

        // 行 2
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0); ImGui::Text("长沙 (Changsha)");
        ImGui::TableSetColumnIndex(1); ImGui::Text("242");
        ImGui::TableSetColumnIndex(2); ImGui::Text("242");
        ImGui::TableSetColumnIndex(3); ImGui::TextColored(ImVec4(0, 1, 0, 1), "低风险 (LOW)"); // 绿色
        ImGui::TableSetColumnIndex(4); ImGui::Button("修改##2"); ImGui::SameLine(); ImGui::Button("删除##2");

        // 行 3
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0); ImGui::Text("上海 (Shanghai)");
        ImGui::TableSetColumnIndex(1); ImGui::Text("340");
        ImGui::TableSetColumnIndex(2); ImGui::Text("300");
        ImGui::TableSetColumnIndex(3); ImGui::TextColored(ImVec4(1, 1, 0, 1), "中风险 (MID)"); // 黄色
        ImGui::TableSetColumnIndex(4); ImGui::Button("修改##3"); ImGui::SameLine(); ImGui::Button("删除##3");

        ImGui::EndTable();
    }
}

// 3. 预测模型页面 (对应需求 3, 5, 6)
void ShowPredictionPage() {
    ImGui::Text(">> 传染病动力学预测 (SEIR Model)");
    ImGui::Separator();

    ImGui::Columns(2, "PredCols", false); // 分两列
    ImGui::SetColumnWidth(0, 300); // 左边参数区宽度 300

    // --- 左边：参数调整 ---
    ImGui::Text("模型参数设置");
    ImGui::Dummy(ImVec2(0, 10)); // 空一行

    static float beta = 0.5f;
    static float gamma = 0.1f;
    static int days = 30;
    
    ImGui::SliderFloat("传染率 (Beta)", &beta, 0.0f, 2.0f);
    ImGui::TextColored(ImVec4(0.5f,0.5f,0.5f,1), "描述病毒传播的速度");
    
    ImGui::SliderFloat("治愈率 (Gamma)", &gamma, 0.0f, 1.0f);
    ImGui::SliderInt("预测天数", &days, 10, 180);

    ImGui::Spacing();
    if (ImGui::Button("开始运算", ImVec2(280, 40))) {
        // 这里以后写 calc_logic()
    }

    ImGui::NextColumn(); // 切换到右边

    // --- 右边：图表展示区 ---
    ImGui::Text("数据可视化结果");
    
    // 因为还没加 ImPlot，我们先画个框占位，假装这里有图
    ImVec2 contentSize = ImGui::GetContentRegionAvail();
    contentSize.y -= 20; // 留点底边距
    
    // 画一个深色的背景框代表图表区域
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    ImGui::Button("##PlotPlaceholder", contentSize);
    ImGui::PopStyleColor();

    // 在框框中间写个字
    // (这是一些复杂的定位计算，为了把字居中，看不懂没关系)
    ImVec2 textSize = ImGui::CalcTextSize("图表显示区域 (Waiting for ImPlot)");
    ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + (contentSize.x - textSize.x)/2 + 300, 
                               ImGui::GetCursorPosY() - contentSize.y/2));
    ImGui::Text("图表显示区域 (Waiting for ImPlot)");

    ImGui::Columns(1); // 意思是：恢复为 1 列（即结束分列）
}


// --- 主程序 ---
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

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.Fonts->AddFontFromFileTTF("c:/windows/fonts/msyh.ttc", 20.0f, nullptr, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
    ImGui::StyleColorsDark(); // 深色主题，比较专业

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // --- 核心 UI 布局逻辑 ---
        
        // 1. 创建一个占满全屏的根窗口
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("MainRoot", nullptr, 
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        // 2. 左边显示侧边栏
        ShowSidebar();

        ImGui::SameLine(); // 让下一个控件在右边，而不是下面

        // 3. 右边显示具体内容页面
        ImGui::BeginChild("ContentRegion", ImVec2(0, 0), true);
        
        if (g_CurrentState == State_DataManage) {
            ShowDataPage();
        } else if (g_CurrentState == State_Prediction) {
            ShowPredictionPage();
        }

        ImGui::EndChild(); // 结束右边区域
        ImGui::End();      // 结束根窗口

        // --- 渲染 ---
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
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
