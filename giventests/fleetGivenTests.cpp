#include "../fleetXpress.h"
#include <gtest/gtest.h>
#include <x86intrin.h>
#include <map>

static int test_case_count = 0;

struct CycleStats {
    unsigned long long total = 0;
    unsigned long long min = UINT64_MAX;
    unsigned long long max = 0;
    int count = 0;

    void update(unsigned long long cycles) {
        total += cycles;
        if (cycles < min) min = cycles;
        if (cycles > max) max = cycles;
        ++count;
    }

    void print(const std::string& label) const {
        if (count == 0) return;
        printf("\n=== CPU Cycle Stats for %s ===\n", label.c_str());
        printf("Total Calls: %d\n", count);
        printf("Minimum Cycles: %llu\n", min);
        printf("Maximum Cycles: %llu\n", max);
        printf("Average Cycles: %.2f\n", (double)total / count);
        printf("=============================================\n");
    }
};

static std::map<std::string, CycleStats> cycle_map;

#define MEASURE_CYCLES(label, expr, result)                    \
    do {                                                       \
        unsigned long long start = __rdtsc();                  \
        result = (expr);                                       \
        unsigned long long end = __rdtsc();                    \
        cycle_map[label].update(end - start);                  \
    } while (0)

vector<Vehicle> populateExpectedVehicles() {
    vector<Vehicle> vec;
    ifstream ff("giventests/vehicles.txt");
    Vehicle v;
    while (ff >> v.VID >> v.VMfr >> v.VType >> v.VEngine >> v.VLastOdometerReading >> v.VStatus)
        vec.push_back(v);
    ff.close();
    return vec;
}

TEST(TestFleetOperations, GivenFleet) {
    Fleet f("fleetData.json", "giventests/fleet.db", "rentalData.json");

    vector<Vehicle> exp_v = populateExpectedVehicles();

    vector<Vehicle> v;
    MEASURE_CYCLES("getAllFleetData", f.getAllFleetData(), v);

    ASSERT_EQ(16, v.size());
    for (int i = 0; i < exp_v.size(); ++i) {
        ASSERT_EQ(exp_v[i].VID, v[i].VID);
        ASSERT_EQ(exp_v[i].VMfr, v[i].VMfr);
        ASSERT_EQ(exp_v[i].VType, v[i].VType);
        ASSERT_EQ(exp_v[i].VEngine, v[i].VEngine);
        ASSERT_EQ(exp_v[i].VLastOdometerReading, v[i].VLastOdometerReading);
        ASSERT_EQ(exp_v[i].VStatus, v[i].VStatus);
    }

    Allotment a;
    MEASURE_CYCLES("issueVehicle", f.issueVehicle("Sedan", "TX123", 4), a);

    Allotment exp_a{};
    string t;
    time_t curr_time = time(nullptr);

    exp_a.AID = "954e0084";
    exp_a.ALicense = "TX123";
    converttmDateTimeTostrDatestrTime(curr_time, exp_a.AIssueDate, t);
    converttmDateTimeTostrDatestrTime(curr_time + 4 * 24 * 60 * 60, exp_a.AExpectedReturnDate, t);
    exp_a.AEstimatedCost = 24000;
    exp_a.ASecurityDeposit = 12000;
    exp_a.AStartingOdometerReading = 30;

    ASSERT_EQ(exp_a.AID, a.AID);
    ASSERT_EQ(exp_a.ALicense, a.ALicense);
    ASSERT_EQ(exp_a.AIssueDate, a.AIssueDate);
    ASSERT_EQ(exp_a.AExpectedReturnDate, a.AExpectedReturnDate);
    ASSERT_EQ(exp_a.AActualReturnDate, a.AActualReturnDate);
    ASSERT_EQ(exp_a.AEstimatedCost, a.AEstimatedCost);
    ASSERT_EQ(exp_a.ASecurityDeposit, a.ASecurityDeposit);
    ASSERT_EQ(exp_a.AFinalCost, a.AFinalCost);
    ASSERT_EQ(exp_a.AStartingOdometerReading, a.AStartingOdometerReading);
    ASSERT_EQ(exp_a.AEndingOdometerReading, a.AEndingOdometerReading);

    int amount;
    MEASURE_CYCLES("returnVehicle", f.returnVehicle("954e0084", 50), amount);
    ASSERT_EQ(-12000, amount);

    string vehicleStatus;
    MEASURE_CYCLES("checkVehicleStatus", f.checkVehicleStatus("5c500b88"), vehicleStatus);
    ASSERT_EQ("Available", vehicleStatus);

    exp_v[0].VLastOdometerReading = 50;

    MEASURE_CYCLES("allVehicleAvailability", f.allVehicleAvailability(), v);
    ASSERT_EQ(16, v.size());
    for (int i = 0; i < exp_v.size(); ++i) {
        ASSERT_EQ(exp_v[i].VID, v[i].VID);
        ASSERT_EQ(exp_v[i].VMfr, v[i].VMfr);
        ASSERT_EQ(exp_v[i].VType, v[i].VType);
        ASSERT_EQ(exp_v[i].VEngine, v[i].VEngine);
        ASSERT_EQ(exp_v[i].VLastOdometerReading, v[i].VLastOdometerReading);
        ASSERT_EQ(exp_v[i].VStatus, v[i].VStatus);
    }

    curr_time = time(nullptr);
    string startDate, endDate;
    converttmDateTimeTostrDatestrTime(curr_time - 24 * 60 * 60, startDate, t);
    converttmDateTimeTostrDatestrTime(curr_time + 24 * 60 * 60, endDate, t);

    vector<Allotment> vec_allot;
    MEASURE_CYCLES("getVehicleRentalHistory", f.getVehicleRentalHistory("954e0084", startDate, endDate), vec_allot);

    exp_a.AID = "954e0084";
    exp_a.ALicense = "TX123";
    converttmDateTimeTostrDatestrTime(curr_time, exp_a.AIssueDate, t);
    converttmDateTimeTostrDatestrTime(curr_time + 4 * 24 * 60 * 60, exp_a.AExpectedReturnDate, t);
    converttmDateTimeTostrDatestrTime(curr_time, exp_a.AActualReturnDate, t);
    exp_a.AEstimatedCost = 24000;
    exp_a.ASecurityDeposit = 12000;
    exp_a.AFinalCost = 24000;
    exp_a.AStartingOdometerReading = 30;
    exp_a.AEndingOdometerReading = 50;

    ASSERT_EQ(1, vec_allot.size());
    ASSERT_EQ(exp_a.AID, vec_allot[0].AID);
    ASSERT_EQ(exp_a.ALicense, vec_allot[0].ALicense);
    ASSERT_EQ(exp_a.AIssueDate, vec_allot[0].AIssueDate);
    ASSERT_EQ(exp_a.AExpectedReturnDate, vec_allot[0].AExpectedReturnDate);
    ASSERT_EQ(exp_a.AActualReturnDate, vec_allot[0].AActualReturnDate);
    ASSERT_EQ(exp_a.AEstimatedCost, vec_allot[0].AEstimatedCost);
    ASSERT_EQ(exp_a.ASecurityDeposit, vec_allot[0].ASecurityDeposit);
    ASSERT_EQ(exp_a.AFinalCost, vec_allot[0].AFinalCost);
    ASSERT_EQ(exp_a.AStartingOdometerReading, vec_allot[0].AStartingOdometerReading);
    ASSERT_EQ(exp_a.AEndingOdometerReading, vec_allot[0].AEndingOdometerReading);

    int income, totalIncome;
    MEASURE_CYCLES("incomeOnVehicle", f.incomeOnVehicle("954e0084", startDate, endDate), income);
    ASSERT_EQ(24000, income);

    MEASURE_CYCLES("incomeOnFleet", f.incomeOnFleet(startDate, endDate), totalIncome);
    ASSERT_EQ(24000, totalIncome);

    ++test_case_count;
}

class StatsReporter : public ::testing::Environment {
public:
    void TearDown() override {
        if (test_case_count == 0) return;
        printf("\n====== Overall Function Performance Stats ======\n");
        for (const auto& entry : cycle_map) {
            entry.second.print(entry.first);
        }
        printf("================================================\n\n");
    }
};

::testing::Environment* const stats_env = ::testing::AddGlobalTestEnvironment(new StatsReporter);
