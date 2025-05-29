#include "../fleetXpress.h"
#include <gtest/gtest.h>
#include <iostream>
#include <sqlite3.h>

vector<Vehicle> populateExpectedVehicles()
{
    vector<Vehicle> vec;
    ifstream ff("giventests/vehicles.txt");
    Vehicle v;

    while (ff >> v.VID >> v.VMfr >> v.VType >> v.VEngine >> v.VLastOdometerReading >> v.VStatus)
        vec.push_back(v);

    ff.close();
    return vec;
}

TEST(TestFleetOperations, GivenFleet)
{
    Fleet f("fleetData.json", "giventests/fleet.db", "rentalData.json");

    vector<Vehicle> exp_v = populateExpectedVehicles();
    vector<Vehicle> v = f.getAllFleetData();

    ASSERT_EQ(16, v.size());
    for (int i = 0; i < exp_v.size(); ++i)
    {
        ASSERT_EQ(exp_v[i].VID, v[i].VID);
        ASSERT_EQ(exp_v[i].VMfr, v[i].VMfr);
        ASSERT_EQ(exp_v[i].VType, v[i].VType);
        ASSERT_EQ(exp_v[i].VEngine, v[i].VEngine);
        ASSERT_EQ(exp_v[i].VLastOdometerReading, v[i].VLastOdometerReading);
        ASSERT_EQ(exp_v[i].VStatus, v[i].VStatus);
    }

    Allotment a = f.issueVehicle("Sedan", "TX123", 4);
    Allotment exp_a = Allotment{};
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

    int amount = f.returnVehicle("954e0084", 50);
    ASSERT_EQ(-12000, amount);

    string vehicleStatus = f.checkVehicleStatus("5c500b88");
    ASSERT_EQ("Available", vehicleStatus);

    exp_v[0].VLastOdometerReading = 50;

    v = f.allVehicleAvailability();
    ASSERT_EQ(16, v.size());
    for (int i = 0; i < exp_v.size(); ++i)
    {
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
    vector<Allotment> vec_allot = f.getVehicleRentalHistory("954e0084", startDate, endDate);

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

    int income = f.incomeOnVehicle("954e0084", startDate, endDate);
    ASSERT_EQ(24000, income);

    int totalIncome = f.incomeOnFleet(startDate, endDate);
    ASSERT_EQ(24000, income);
}

TEST(TestFleetOperations, IssueVehicle_NoAvailability)
{
    Fleet f("fleetData.json", "giventests/fleet.db", "rentalData.json");
    Allotment a = f.issueVehicle("Convertible", "TX9999", 3);
    ASSERT_TRUE(a.AID.empty());
}

TEST(TestFleetOperations, ReturnVehicle_NotIssued)
{
    Fleet f("fleetData.json", "giventests/fleet.db", "rentalData.json");
    int retVal = f.returnVehicle("invalid_reg_no", 100);
    ASSERT_EQ(INT_MIN, retVal);
}

TEST(TestFleetOperations, ReturnVehicle_OdometerLessThanLast)
{
    Fleet f("fleetData.json", "giventests/fleet.db", "rentalData.json");
    Allotment allot = f.issueVehicle("Sedan", "TX1234", 2);
    int retVal = f.returnVehicle(allot.AID, 10);
    ASSERT_EQ(INT_MAX, retVal);
}

TEST(TestFleetOperations, CheckVehicleStatus)
{
    Fleet f("fleetData.json", "giventests/fleet.db", "rentalData.json");
    string status = f.checkVehicleStatus("954e0084");
    ASSERT_EQ("Available", status);
    f.issueVehicle("Sedan", "TX5678", 3);
    status = f.checkVehicleStatus("954e0084");
    ASSERT_EQ("Issued", status);
    status = f.checkVehicleStatus("unknown_vid");
    ASSERT_EQ("NotFound", status);
}

TEST(TestFleetOperations, IncomeOnVehicle_CompletedAndOngoing)
{
    Fleet f("fleetData.json", "giventests/fleet.db", "rentalData.json");
    Allotment allot = f.issueVehicle("Sedan", "TX1111", 2);
    f.returnVehicle(allot.AID, allot.AStartingOdometerReading + 100);
    time_t now = time(nullptr);
    string startDate, endDate, t;
    converttmDateTimeTostrDatestrTime(now - 10 * 24 * 60 * 60, startDate, t);
    converttmDateTimeTostrDatestrTime(now + 10 * 24 * 60 * 60, endDate, t);
    int incomeCompleted = f.incomeOnVehicle(allot.AID, startDate, endDate);
    ASSERT_GT(incomeCompleted, 0);
    Allotment ongoing = f.issueVehicle("Sedan", "TX2222", 3);
    int incomeOngoing = f.incomeOnVehicle(ongoing.AID, startDate, endDate);
    ASSERT_GE(incomeOngoing, incomeCompleted);
}

TEST(TestFleetOperations, VehicleRentalHistory_OnlyCompleted)
{
    Fleet f("fleetData.json", "giventests/fleet.db", "rentalData.json");
    Allotment completed = f.issueVehicle("Sedan", "TX3333", 1);
    f.returnVehicle(completed.AID, completed.AStartingOdometerReading + 50);
    Allotment ongoing = f.issueVehicle("Sedan", "TX4444", 2);
    time_t now = time(nullptr);
    string startDate, endDate, t;
    converttmDateTimeTostrDatestrTime(now - 10 * 24 * 60 * 60, startDate, t);
    converttmDateTimeTostrDatestrTime(now + 10 * 24 * 60 * 60, endDate, t);
    vector<Allotment> history = f.getVehicleRentalHistory(completed.AID, startDate, endDate);
    bool foundCompleted = false;
    bool foundOngoing = false;
    for (const auto& a : history)
    {
        if (a.AID == completed.AID) foundCompleted = true;
        if (a.AID == ongoing.AID) foundOngoing = true;
    }
    ASSERT_TRUE(foundCompleted);
    ASSERT_FALSE(foundOngoing);
}

TEST(TestFleetOperations, IncomeOnFleet_MultipleVehicles)
{
    Fleet f("fleetData.json", "giventests/fleet.db", "rentalData.json");
    Allotment a1 = f.issueVehicle("Sedan", "TX5555", 2);
    f.returnVehicle(a1.AID, a1.AStartingOdometerReading + 60);
    Allotment a2 = f.issueVehicle("Coupe", "TX6666", 1);
    f.returnVehicle(a2.AID, a2.AStartingOdometerReading + 30);
    time_t now = time(nullptr);
    string startDate, endDate, t;
    converttmDateTimeTostrDatestrTime(now - 20 * 24 * 60 * 60, startDate, t);
    converttmDateTimeTostrDatestrTime(now + 20 * 24 * 60 * 60, endDate, t);
    int income1 = f.incomeOnVehicle(a1.AID, startDate, endDate);
    int income2 = f.incomeOnVehicle(a2.AID, startDate, endDate);
    int totalIncome = f.incomeOnFleet(startDate, endDate);
    ASSERT_EQ(income1 + income2, totalIncome);
}

TEST(TestFleetOperations, IncomeOnVehicleNoRentalsInDateRange)
{
    Fleet f("fleetData.json", "giventests/fleet.db", "rentalData.json");
    string startDate = "1990-01-01";
    string endDate = "1990-12-31";
    int income = f.incomeOnVehicle("954e0084", startDate, endDate);
    ASSERT_EQ(0, income);
}

TEST(TestFleetOperations, ReturnVehicleWithExtraCharges)
{
    Fleet f("fleetData.json", "giventests/fleet.db", "rentalData.json");
    Allotment allot = f.issueVehicle("Sedan", "TX1234", 2);
    int amountDue = f.returnVehicle(allot.AID, 530);
    ASSERT_GT(amountDue, 0);
}

TEST(TestFleetOperations, IssueVehicleZeroDays)
{
    Fleet f("fleetData.json", "giventests/fleet.db", "rentalData.json");
    Allotment allot = f.issueVehicle("Sedan", "TX000", 0);
    ASSERT_TRUE(allot.AID.empty());
    ASSERT_TRUE(allot.ALicense.empty());
}

TEST(TestFleetOperations, IssueVehicleAllIssued)
{
    Fleet f("fleetData.json", "giventests/fleet.db", "rentalData.json");
    while (true) {
        Allotment a = f.issueVehicle("Sedan", "TX999", 1);
        if (a.AID.empty()) break;
    }
    Allotment allot = f.issueVehicle("Sedan", "TX000", 1);
    ASSERT_TRUE(allot.AID.empty());
}

TEST(TestFleetOperations, ReturnVehicleOnExpectedDateNoExtras)
{
    Fleet f("fleetData.json", "giventests/fleet.db", "rentalData.json");
    Allotment allot = f.issueVehicle("Sedan", "TX111", 3);
    int odometerReading = allot.AStartingOdometerReading + 3 * 200;
    int amount = f.returnVehicle(allot.AID, odometerReading);
    ASSERT_LE(amount, 0);
}

TEST(TestFleetOperations, ReturnVehicleNoMovement)
{
    Fleet f("fleetData.json", "giventests/fleet.db", "rentalData.json");
    Allotment allot = f.issueVehicle("Sedan", "TX222", 2);
    int amount = f.returnVehicle(allot.AID, allot.AStartingOdometerReading);
    ASSERT_LE(amount, 0);
}

TEST(TestFleetOperations, IncomeOnFleetOverlappingRentals)
{
    Fleet f("fleetData.json", "giventests/fleet.db", "rentalData.json");
    Allotment a1 = f.issueVehicle("Sedan", "TXA", 5);
    Allotment a2 = f.issueVehicle("Hatchback", "TXB", 3);
    f.returnVehicle(a2.AID, a2.AStartingOdometerReading + 50);
    string startDate = "2000-01-01";
    string endDate = "2050-01-01";
    int totalIncome = f.incomeOnFleet(startDate, endDate);
    ASSERT_GT(totalIncome, 0);
}

TEST(TestFleetOperations, RentalHistoryNoRecordsInRange)
{
    Fleet f("fleetData.json", "giventests/fleet.db", "rentalData.json");
    string startDate = "1990-01-01";
    string endDate = "1990-12-31";
    auto history = f.getVehicleRentalHistory("954e0084", startDate, endDate);
    ASSERT_TRUE(history.empty());
}

TEST(TestFleetOperations, ReturnVehicleExactExpectedCost)
{
    Fleet f("fleetData.json", "giventests/fleet.db", "rentalData.json");
    Allotment allot = f.issueVehicle("Sedan", "TX333", 2);
    int odometerReading = allot.AStartingOdometerReading + 2 * 200;
    int amount = f.returnVehicle(allot.AID, odometerReading);
    ASSERT_LT(amount, 0);
}

TEST(TestFleetOperations, CheckVehicleStatusAfterIssue)
{
    Fleet f("fleetData.json", "giventests/fleet.db", "rentalData.json");
    Allotment allot = f.issueVehicle("Sedan", "TX456", 3);
    ASSERT_FALSE(allot.AID.empty());
    string status = f.checkVehicleStatus(allot.AID);
    ASSERT_EQ("Issued", status);
}

TEST(TestFleetOperations, AllVehiclesAvailableInitially)
{
    Fleet f("fleetData.json", "giventests/fleet.db", "rentalData.json");
    vector<Vehicle> vehicles = f.getAllFleetData();
    for (const auto& v : vehicles) {
        ASSERT_EQ("Available", v.VStatus);
    }
}

TEST(TestFleetOperations, ReturnVehicleHugeExtraCharges)
{
    Fleet f("fleetData.json", "giventests/fleet.db", "rentalData.json");
    Allotment a = f.issueVehicle("Sports", "DL9999", 2);
    sleep(3);
    int newOdometer = a.AStartingOdometerReading + 800;
    int result = f.returnVehicle(a.AID, newOdometer);
    ASSERT_GT(result, 0);
}

TEST(TestFleetOperations, FleetDataAfterTransactions)
{
    Fleet f("fleetData.json", "giventests/fleet.db", "rentalData.json");
    Allotment a = f.issueVehicle("Coupe", "MP7788", 1);
    f.returnVehicle(a.AID, a.AStartingOdometerReading + 100);
    vector<Vehicle> fleet = f.getAllFleetData();
    for (const auto& v : fleet) {
        if (v.VID == a.AID) {
            ASSERT_EQ("Available", v.VStatus);
            ASSERT_EQ(a.AStartingOdometerReading + 100, v.VLastOdometerReading);
        }
    }
}

TEST(TestFleetOperations, ReturnVehicleTwice)
{
    Fleet f("fleetData.json", "giventests/fleet.db", "rentalData.json");
    Allotment a = f.issueVehicle("Hatchback", "DL1234", 2);
    int firstReturn = f.returnVehicle(a.AID, a.AStartingOdometerReading + 200);
    ASSERT_LE(firstReturn, 0);
    int secondReturn = f.returnVehicle(a.AID, a.AStartingOdometerReading + 300);
    ASSERT_EQ(INT_MIN, secondReturn);
}

TEST(TestFleetOperations, IssueVehicleSpecialCharLicense)
{
    Fleet f("fleetData.json", "giventests/fleet.db", "rentalData.json");
    Allotment allot = f.issueVehicle("Sedan", "@#TX_789", 2);
    ASSERT_FALSE(allot.AID.empty());
    ASSERT_EQ("@#TX_789", allot.ALicense);
}

TEST(TestFleetOperations, RentalHistoryMultipleReturns)
{
    Fleet f("fleetData.json", "giventests/fleet.db", "rentalData.json");
    Allotment a1 = f.issueVehicle("Sedan", "TX9991", 1);
    f.returnVehicle(a1.AID, a1.AStartingOdometerReading + 100);
    Allotment a2 = f.issueVehicle("Sedan", "TX9992", 1);
    f.returnVehicle(a2.AID, a2.AStartingOdometerReading + 150);
    string startDate = "2000-01-01";
    string endDate = "2050-12-31";
    vector<Allotment> history = f.getVehicleRentalHistory(a1.AID, startDate, endDate);
    ASSERT_EQ(1, history.size());
    history = f.getVehicleRentalHistory(a2.AID, startDate, endDate);
    ASSERT_EQ(1, history.size());
}

TEST(TestFleetOperations, ReturnVehicleWithVeryHighOdometer)
{
    Fleet f("fleetData.json", "giventests/fleet.db", "rentalData.json");
    Allotment a = f.issueVehicle("Sedan", "TX987", 2);
    int newOdometer = a.AStartingOdometerReading + 100000;
    int amount = f.returnVehicle(a.AID, newOdometer);
    ASSERT_GT(amount, 0);
}

TEST(TestFleetOperations, CheckVehicleStatusInvalidID)
{
    Fleet f("fleetData.json", "giventests/fleet.db", "rentalData.json");
    std::string status = f.checkVehicleStatus("invalid_vid_0000");
    ASSERT_EQ("NotFound", status);
}

TEST(TestFleetOperations, IssueInvalidVehicleType)
{
    Fleet f("fleetData.json", "giventests/fleet.db", "rentalData.json");
    Allotment a = f.issueVehicle("Truck", "TX888", 3);
    ASSERT_TRUE(a.AID.empty());
}

TEST(TestFleetOperations, ReturnVehicleNegativeOdometer)
{
    Fleet f("fleetData.json", "giventests/fleet.db", "rentalData.json");
    Allotment a = f.issueVehicle("Coupe", "XY0001", 2);
    int result = f.returnVehicle(a.AID, -100);
    ASSERT_EQ(INT_MAX, result);
}

TEST(TestFleetOperations, IssueDifferentTypesSimultaneously)
{
    Fleet f("fleetData.json", "giventests/fleet.db", "rentalData.json");
    Allotment a1 = f.issueVehicle("Sedan", "S123", 2);
    Allotment a2 = f.issueVehicle("Hatchback", "H456", 2);
    Allotment a3 = f.issueVehicle("Coupe", "C789", 2);
    ASSERT_FALSE(a1.AID.empty());
    ASSERT_FALSE(a2.AID.empty());
    ASSERT_FALSE(a3.AID.empty());
}

TEST(TestFleetOperations, RentalHistoryExcludeOngoing)
{
    Fleet f("fleetData.json", "giventests/fleet.db", "rentalData.json");
    Allotment a = f.issueVehicle("Sports", "LIVE123", 2);
    std::string start, end, dummy;
    time_t now = time(nullptr);
    converttmDateTimeTostrDatestrTime(now - 86400, start, dummy);
    converttmDateTimeTostrDatestrTime(now + 86400 * 3, end, dummy);
    vector<Allotment> history = f.getVehicleRentalHistory(a.AID, start, end);
    ASSERT_TRUE(history.empty());
}
