#pragma once

#include <vector>
#include <string>

// Forward-declare ImVec4 from imgui.h to avoid including the full header
struct ImVec4;

// Struct to hold data for a single day for the SIR model
struct SIRDataPoint {
    int day = 0; // Day number
    double susceptible = 0;
    double infected = 0;
    double recovered = 0;
};

// Represents the SIR simulation model and its state for a single region
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
    void reset(int initialPopulation, int initialInfected, int initialRecovered);

private:
    std::vector<SIRDataPoint> history;
    SIRDataPoint currentData;
    double beta;  // Transmission rate
    double gamma; // Recovery rate
    int population;
};

// Enum for risk level classification
enum class RiskLevel { Low, Medium, High };

// Struct to hold all data for a single region/city
struct Region {
    char name[128];
    int population;
    
    // Manually entered data
    int confirmedCases;
    int recoveredCases;
    int deaths;

    // Simulation model for this region
    SIRModel simulation;

    // Default constructor
    Region();
};

// Main class to manage all epidemic data across multiple regions
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
