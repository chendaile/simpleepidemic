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

void SIRModel::reset(int initialPopulation, int initialInfected, int initialRecovered) {
    population = initialPopulation;
    history.clear();

    currentData.day = 0;
    currentData.infected = static_cast<double>(initialInfected);
    currentData.recovered = static_cast<double>(initialRecovered);
    currentData.susceptible = static_cast<double>(population - initialInfected - initialRecovered);
    
    history.push_back(currentData);
}


// --- Region Struct Implementation ---

Region::Region() : population(0), confirmedCases(0), recoveredCases(0), deaths(0) {
    name[0] = '\0'; // Ensure the name is an empty string by default
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
