#include "DataModel.h"
#include <cstring> // 为了使用 strncpy

// 1. 【真正定义】全局数据源
// 内存是在这里分配的
std::vector<CityRecord> g_Cities;

// 2. 实现构造函数
CityRecord::CityRecord(const char* n, int c, int r, int p) {
    // 安全复制字符串到 char 数组
    strncpy(name, n, sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0'; // 确保结尾有结束符
    confirmed = c;
    recovered = r;
    population = p;
}

// 3. 实现风险计算逻辑
// 规则：确诊 > 1000 高风险，> 100 中风险，否则低风险
RiskLevel CityRecord::GetRisk() const {
    if (confirmed > 1000) return Risk_High;
    if (confirmed > 100)  return Risk_Mid;
    return Risk_Low;
}

// 4. 实现初始化测试数据
void InitTestData() {
    g_Cities.clear();
    g_Cities.emplace_back("Wuhan", 50340, 46464, 11000000);
    g_Cities.emplace_back("Changsha", 242, 242, 8000000);
    g_Cities.emplace_back("Shanghai", 340, 300, 24000000);
}