#pragma once
#include <vector>
#include <string>

// 1. 定义风险等级枚举
enum RiskLevel {
    Risk_Low,   // 低风险
    Risk_Mid,   // 中风险
    Risk_High   // 高风险
};

// 2. 定义城市数据结构
struct CityRecord {
    char name[64];      // 城市名 (用 char 数组方便绑定 ImGui)
    int confirmed;      // 确诊人数
    int recovered;      // 治愈人数
    int population;     // 总人口

    // 构造函数声明
    CityRecord(const char* n = "New City", int c = 0, int r = 0, int p = 100000);

    // 成员函数声明：获取当前风险等级
    RiskLevel GetRisk() const;
};

// 3. 声明全局数据源
// 这里的 extern 关键字很重要！
// 它告诉编译器："有个叫 g_Cities 的列表存在于某个 .cpp 文件里，别在这里重复创建它，去那边找。"
extern std::vector<CityRecord> g_Cities;

// 4. 声明一个初始化测试数据的函数
void InitTestData();