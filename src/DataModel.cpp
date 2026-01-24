// ====================================================================================
// 模块名称: DataModel Implementation
// 功能描述: 
//   实现DataModel.h中定义的类与方法。
//   包含SIR模型的具体数学计算逻辑，以及从历史数据反推参数的算法实现。
// ====================================================================================

#include "DataModel.h"
#include "imgui.h" // For ImVec4
#include <cstring>  // For strncpy
#include <algorithm> // For std::max

// --- SIRModel Class Implementation ---

SIRModel::SIRModel() : beta(0.2), gamma(0.1), population(0) {
    history.reserve(200); // Pre-allocate some memory
}

const std::vector<SIRDataPoint>& SIRModel::getHistory() const {
    return history;
}

const SIRDataPoint& SIRModel::getCurrentData() const {
    return currentData;
}

double SIRModel::getBeta() const { return beta; }
double SIRModel::getGamma() const { return gamma; }
int SIRModel::getPopulation() const { return population; }

void SIRModel::setBeta(double b) { beta = b; }
void SIRModel::setGamma(double g) { gamma = g; }

// [算法] 单步模拟 (Run Single Step)
// 核心逻辑:
//   基于当前状态(S, I, R)，利用SIR微分方程计算下一天的变化量。
//   NewInfections = (beta * S * I) / N
//   NewRecoveries = gamma * I
void SIRModel::run_single_step() {
    // Placeholder: In the future, this will calculate the next day's S, I, R values
    if (population == 0) return;

    // This is the core SIR model mathematical formula
    double S = currentData.susceptible;
    double I = currentData.infected;
    double R = currentData.recovered;
    
    double newInfections = (beta * S * I) / population;
    double newRecoveries = gamma * I;

    // Update the numbers, ensuring they don't go below zero
    S = std::max(0.0, S - newInfections);
    I = std::max(0.0, I + newInfections - newRecoveries);
    R = std::max(0.0, R + newRecoveries);

    // Update current data for the next step
    currentData.day += 1;
    currentData.susceptible = S;
    currentData.infected = I;
    currentData.recovered = R;

    // Store this step in history
    history.push_back(currentData);
}

void SIRModel::run(int days) {
    // The reset function already clears history and adds day 0.
    // This loop will add day 1 through `days`.
    for (int d = 0; d < days; ++d) {
        run_single_step();
    }
}

void SIRModel::reset(int initialPopulation, int initialInfected, int initialRecovered, int startDay) {
    population = initialPopulation;
    history.clear();

    currentData.day = startDay;
    currentData.infected = static_cast<double>(initialInfected);
    currentData.recovered = static_cast<double>(initialRecovered);
    currentData.susceptible = static_cast<double>(population - initialInfected - initialRecovered);
    
    history.push_back(currentData);
}


// --- Region Struct Implementation ---

Region::Region() : population(0), confirmedCases(0), recoveredCases(0), deaths(0) {
    name[0] = '\0'; // Ensure the name is an empty string by default
}

// [算法] 估算传染率 (Calculate Average Beta)
// 逻辑:
//   遍历历史数据，利用SIR微分方程反推每一天的Beta值。
//   最后取平均值作为该地区的估算传染率。
double Region::calculateAverageBeta() const {
    if (history.size() < 2) return 0.2; // Default fallback if not enough data

    double sumBeta = 0.0;
    int count = 0;

    for (size_t t = 0; t < history.size() - 1; ++t) {
        const auto& today = history[t];
        const auto& nextDay = history[t + 1];

        // SIR model derivation:
        // dS/dt = - beta * S * I / N
        // dI/dt = beta * S * I / N - gamma * I
        // beta calculation
        // new infections approximation

        double activeToday = (double)(today.confirmed - today.recovered - today.deaths);
        double removedToday = (double)(today.recovered + today.deaths);
        double S_today = (double)(population - activeToday - removedToday);
        
        if (activeToday <= 0 || S_today <= 0) continue;

        double newInfections = std::max(0.0, (double)(nextDay.confirmed - today.confirmed));
        
        // Avoid division by zero
        double dailyBeta = (population * newInfections) / (S_today * activeToday);
        
        // Filter out unreasonable values (noise in data)
        if (dailyBeta > 0 && dailyBeta < 5.0) {
            sumBeta += dailyBeta;
            count++;
        }
    }

    return (count > 0) ? (sumBeta / count) : 0.2;
}

// [算法] 估算恢复率 (Calculate Average Gamma)
// 逻辑:
//   利用公式 Gamma = dR / I 反推。
//   dR = 新增康复 + 新增死亡
double Region::calculateAverageGamma() const {
    if (history.size() < 2) return 0.1; // Default fallback

    double sumGamma = 0.0;
    int count = 0;

    for (size_t t = 0; t < history.size() - 1; ++t) {
        const auto& today = history[t];
        const auto& nextDay = history[t + 1];

        // dR/dt = gamma * I
        // gamma calculation

        double activeToday = (double)(today.confirmed - today.recovered - today.deaths);
        
        if (activeToday <= 0) continue;

        double newRemoved = std::max(0.0, (double)((nextDay.recovered + nextDay.deaths) - (today.recovered + today.deaths)));

        double dailyGamma = newRemoved / activeToday;

        if (dailyGamma > 0 && dailyGamma < 1.0) {
            sumGamma += dailyGamma;
            count++;
        }
    }

    return (count > 0) ? (sumGamma / count) : 0.1;
}


// --- EpidemicData Class Implementation ---

EpidemicData::EpidemicData() {
    // The vector is already initialized by its own default constructor
}

void EpidemicData::addRegion(const char* name, int population, int confirmed, int recovered, int deaths) {
    regions.emplace_back(); // Create a new default-constructed Region at the end
    Region& newRegion = regions.back();

    strncpy(newRegion.name, name, sizeof(newRegion.name) - 1);
    newRegion.name[sizeof(newRegion.name) - 1] = '\0';

    newRegion.population = population;
    newRegion.confirmedCases = confirmed;
    newRegion.recoveredCases = recovered;
    newRegion.deaths = deaths;

    // Also initialize its simulation model
    newRegion.simulation.reset(population, confirmed - recovered - deaths, recovered + deaths);
}

void EpidemicData::deleteRegion(int index) {
    if (index >= 0 && index < regions.size()) {
        regions.erase(regions.begin() + index);
    }
}

Region* EpidemicData::getRegion(int index) {
    if (index >= 0 && index < regions.size()) {
        return &regions[index];
    }
    return nullptr;
}

std::vector<Region>& EpidemicData::getRegions() {
    return regions;
}

// --- Static Utility Functions ---

const char* EpidemicData::getRiskLevelString(RiskLevel level) {
    switch (level) {
        case RiskLevel::High:   return "高风险 (HIGH)";
        case RiskLevel::Medium: return "中风险 (MID)";
        case RiskLevel::Low:
        default:                return "低风险 (LOW)";
    }
}

ImVec4 EpidemicData::getRiskLevelColor(RiskLevel level) {
    switch (level) {
        case RiskLevel::High:   return ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
        case RiskLevel::Medium: return ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
        case RiskLevel::Low:
        default:                return ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green
    }
}

RiskLevel EpidemicData::calculateRiskLevel(const Region& region) {
    int activeCases = region.confirmedCases - region.recoveredCases - region.deaths;
    // Basic logic: risk is based on active cases per 100k people
    if (region.population == 0) return RiskLevel::Low;
    
    double activePer100k = (static_cast<double>(activeCases) / region.population) * 100000.0;

    if (activePer100k > 50) return RiskLevel::High;
    if (activePer100k > 10) return RiskLevel::Medium;
    return RiskLevel::Low;
}
