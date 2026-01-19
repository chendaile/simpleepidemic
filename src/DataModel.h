// ====================================================================================
// 模块名称: DataModel (数据模型定义)
// 功能描述: 
//   定义了系统核心的数据结构，包括SIR传染病模型、地区数据、历史记录以及风险等级。
//   这里充当了MVC架构中的"Model"层，只包含数据和核心算法，不涉及任何UI显示代码。
// ====================================================================================

#pragma once

#include <vector>
#include <string>

// Forward-declare ImVec4 from imgui.h to avoid including the full header
struct ImVec4;

// ------------------------------------------------------------------------------------
// [结构体] SIRDataPoint
// 描述: SIR模型中单日的数据快照
// 作用: 用于存储模拟过程中每一天的S(易感)、I(感染)、R(移出)的具体数值。
// ------------------------------------------------------------------------------------
struct SIRDataPoint {
    int day = 0; // Day number
    double susceptible = 0;
    double infected = 0;
    double recovered = 0;
};

// ------------------------------------------------------------------------------------
// [类] SIRModel
// 描述: 传染病动力学模拟核心类 (Susceptible-Infected-Removed)
// 作用: 
//   封装了SIR微分方程的数值解法。
//   负责管理传染率Byta、恢复率Gamma等参数，并执行随时间步进的模拟计算。
// ------------------------------------------------------------------------------------
class SIRModel {
public:
    SIRModel();

    // Getters
    const std::vector<SIRDataPoint>& getHistory() const;
    const SIRDataPoint& getCurrentData() const;
    double getBeta() const;
    double getGamma() const;
    int getPopulation() const;

    // Setters
    void setBeta(double beta);
    void setGamma(double gamma);

    // Simulation control
    void run_single_step();
    void run(int days);
    void reset(int initialPopulation, int initialInfected, int initialRecovered, int startDay = 0);

private:
    std::vector<SIRDataPoint> history;
    SIRDataPoint currentData;
    double beta;  // Transmission rate
    double gamma; // Recovery rate
    int population;
};

// ------------------------------------------------------------------------------------
// [枚举] RiskLevel
// 描述: 疫情风险等级
// 作用: 根据活跃病例占比划分数据等级，用于UI颜色区分和分级管理。
// ------------------------------------------------------------------------------------
enum class RiskLevel { Low, Medium, High };

// ------------------------------------------------------------------------------------
// [结构体] HistoricalRecord
// 描述: 真实世界的历史数据记录
// 作用: 存储用户录入的每一天的真实确诊、治愈、死亡数据，用于与预测曲线对比或参数拟合。
// ------------------------------------------------------------------------------------
struct HistoricalRecord {
    int day; // Relative day (0, 1, 2...)
    int confirmed;
    int recovered;
    int deaths;
};

// ------------------------------------------------------------------------------------
// [结构体] Region
// 描述: 地区/城市实体
// 作用: 
//   表示一个具体的地理区域（如武汉、上海）。
//   包含该地区的基础人口信息、当前疫情状态、历史数据列表以及对应的SIR预测模型。
// ------------------------------------------------------------------------------------
struct Region {
    char name[128];
    int population;
    
    // Manually entered data (current state)
    int confirmedCases;
    int recoveredCases;
    int deaths;

    // Historical data for prediction calibration
    std::vector<HistoricalRecord> history;

    // Simulation model for this region
    SIRModel simulation;

    // Default constructor
    Region();

    // Calibration methods
    double calculateAverageBeta() const;
    double calculateAverageGamma() const;
};

// ------------------------------------------------------------------------------------
// [类] EpidemicData
// 描述: 全局疫情数据管理器 (Data Center)
// 作用: 
//   整个应用程序的数据仓库，管理所有地区(Region)的列表。
//   提供增删改查(CRUD)接口，以及通用的工具函数（如风险等级计算、颜色获取）。
// ------------------------------------------------------------------------------------
class EpidemicData {
public:
    EpidemicData();

    // Region management
    void addRegion(const char* name, int population, int confirmed, int recovered, int deaths);
    void deleteRegion(int index);
    Region* getRegion(int index);
    std::vector<Region>& getRegions();

    // Utility
    static const char* getRiskLevelString(RiskLevel level);
    static ImVec4 getRiskLevelColor(RiskLevel level);
    static RiskLevel calculateRiskLevel(const Region& region);

private:
    std::vector<Region> regions;
};
