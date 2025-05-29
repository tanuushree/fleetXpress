#include "fleetXpress.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <sqlite3.h>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include "rapidjson/filereadstream.h"
 
using namespace std;
using namespace rapidjson;
 
bool isDateInRange(const string &date, const string &start, const string &end)
{
    auto parse = [](const string &d)
    {
        int dd, mm, yy;
        sscanf(d.c_str(), "%d/%d/%d", &dd, &mm, &yy);
        return yy * 10000 + mm * 100 + dd;
    };
    int d = parse(date), s = parse(start), e = parse(end);
    return (d >= s && d <= e);
}
 
string getTodayDate()
{
    time_t now = time(0);
    string strDate, strTime;
    converttmDateTimeTostrDatestrTime(now, strDate, strTime);
    return strDate;
}
 
 
string addDaysToDate(const string &dateStr, int daysToAdd)
{
    time_t baseDate;
    convertstrDateTotmDate(dateStr, baseDate);
    baseDate += daysToAdd * 24 * 3600;
 
    string newDate, dummyTime;
    converttmDateTimeTostrDatestrTime(baseDate, newDate, dummyTime);
    return newDate;
}
 
 
int daysDiff(const string &date1, const string &date2)
{
    time_t t1, t2;
    convertstrDateTotmDate(date1, t1);
    convertstrDateTotmDate(date2, t2);
    double diffSeconds = difftime(t2, t1);
    return static_cast<int>(diffSeconds / (60 * 60 * 24));
}
 
Fleet::Fleet()
{
    dbase = nullptr;
}
 
Fleet::Fleet(string fleetJsonFilename, string fleetDbFilename, string rentalDataJsonFilename)
{
    // Open DB
    if (sqlite3_open(fleetDbFilename.c_str(), &dbase) != SQLITE_OK)
    {
        cerr << "Cannot open database: " << sqlite3_errmsg(dbase) << endl;
        sqlite3_close(dbase);
        dbase = nullptr;
        return;
    }
 
    // Create vehicles table
    const char *createVehiclesSQL = R"(
        CREATE TABLE IF NOT EXISTS vehicles (
            VID TEXT PRIMARY KEY,
            VMfr TEXT,
            VType TEXT,
            VEngine TEXT,
            VLastOdometerReading INTEGER,
            VStatus TEXT
        );
    )";
    if (sqlite3_exec(dbase, createVehiclesSQL, nullptr, nullptr, nullptr) != SQLITE_OK)
    {
        cerr << "Error creating vehicles table: " << sqlite3_errmsg(dbase) << endl;
    }
 
    // Create allotments table
    const char *createAllotmentsSQL = R"(
        CREATE TABLE IF NOT EXISTS allotments (
            AID TEXT,
            ALicense TEXT,
            AIssueDate TEXT,
            AExpectedReturnDate TEXT,
            AActualReturnDate TEXT,
            AEstimatedCost INTEGER,
            ASecurityDeposit INTEGER,
            AFinalCost INTEGER,
            AStartingOdometerReading INTEGER,
            AEndingOdometerReading INTEGER,
            FOREIGN KEY (AID) REFERENCES vehicles(VID)
        );
    )";
    if (sqlite3_exec(dbase, createAllotmentsSQL, nullptr, nullptr, nullptr) != SQLITE_OK)
    {
        cerr << "Error creating allotments table: " << sqlite3_errmsg(dbase) << endl;
    }
 
    const char *clearAllotmentsSQL = "DELETE FROM allotments;";
    char *errMsg = nullptr;
    if (sqlite3_exec(dbase, clearAllotmentsSQL, nullptr, nullptr, &errMsg) != SQLITE_OK)
    {
        cerr << "Error clearing allotments table: " << errMsg << endl;
        sqlite3_free(errMsg);
    }
 
    // Read fleetData.json
    ifstream fleetFile(fleetJsonFilename);
    if (!fleetFile.is_open())
    {
        cerr << "Cannot open fleet data file: " << fleetJsonFilename << endl;
        return;
    }
    IStreamWrapper fleetWrapper(fleetFile);
    Document fleetDoc;
    fleetDoc.ParseStream(fleetWrapper);
 
    if (!fleetDoc.IsObject())
    {
        cerr << "fleetData.json should be a JSON object\n";
        return;
    }
 
    if (!fleetDoc.HasMember("VehicleData") || !fleetDoc["VehicleData"].IsArray())
    {
        cerr << "'VehicleData' array missing in fleetData.json\n";
        return;
    }
 
    const Value &vehicleArray = fleetDoc["VehicleData"];
 
    // Insert fleet data into vehicles table
    sqlite3_exec(dbase, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
    for (SizeType i = 0; i < vehicleArray.Size(); i++)
    {
        const Value &v = vehicleArray[i];
        if (!(v.HasMember("VID") && v["VID"].IsString() &&
              v.HasMember("VMfr") && v["VMfr"].IsString() &&
              v.HasMember("VType") && v["VType"].IsString() &&
              v.HasMember("VEngine") && v["VEngine"].IsString() &&
              v.HasMember("VLastOdometerReading") && v["VLastOdometerReading"].IsInt() &&
              v.HasMember("VStatus") && v["VStatus"].IsString()))
        {
            cerr << "Invalid vehicle record at index " << i << endl;
            continue;
        }
 
        string vid = v["VID"].GetString();
        string vmfr = v["VMfr"].GetString();
        string vtype = v["VType"].GetString();
        string vengine = v["VEngine"].GetString();
        int odo = v["VLastOdometerReading"].GetInt();
        string vstatus = v["VStatus"].GetString();
 
        string sql = "INSERT OR REPLACE INTO vehicles VALUES (?, ?, ?, ?, ?, ?);";
        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(dbase, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_text(stmt, 1, vid.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, vmfr.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3, vtype.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 4, vengine.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 5, odo);
            sqlite3_bind_text(stmt, 6, vstatus.c_str(), -1, SQLITE_TRANSIENT);
 
            if (sqlite3_step(stmt) != SQLITE_DONE)
            {
                cerr << "Failed to insert vehicle " << vid << ": " << sqlite3_errmsg(dbase) << endl;
            }
            sqlite3_finalize(stmt);
        }
        else
        {
            cerr << "Failed to prepare insert statement: " << sqlite3_errmsg(dbase) << endl;
        }
    }
    sqlite3_exec(dbase, "COMMIT;", nullptr, nullptr, nullptr);
 
    // Read rentalData.json
    ifstream rentalFile(rentalDataJsonFilename);
    if (!rentalFile.is_open())
    {
        cerr << "Cannot open rental data file: " << rentalDataJsonFilename << endl;
        return;
    }
    IStreamWrapper rentalWrapper(rentalFile);
    Document rentalDoc;
    rentalDoc.ParseStream(rentalWrapper);
 
    if (!rentalDoc.IsObject())
    {
        cerr << "rentalData.json should be a JSON object\n";
        return;
    }
 
    if (!rentalDoc.HasMember("RentalData") || !rentalDoc["RentalData"].IsArray())
    {
        cerr << "'RentalData' array missing in rentalData.json\n";
        return;
    }
 
    const Value &rentalArray = rentalDoc["RentalData"];
    vRentalData.clear();
 
    for (SizeType i = 0; i < rentalArray.Size(); i++)
    {
        const Value &r = rentalArray[i];
        if (!(r.HasMember("vehicleType") && r["vehicleType"].IsString() &&
              r.HasMember("perDayRent") && r["perDayRent"].IsInt() &&
              r.HasMember("perKmRent") && r["perKmRent"].IsInt() &&
              r.HasMember("kmLimitPerDay") && r["kmLimitPerDay"].IsInt()))
        {
            cerr << "Invalid rental record at index " << i << endl;
            continue;
        }
 
        RentalData rd;
        rd.m_vehicleType = r["vehicleType"].GetString();
        rd.m_perDayRent = r["perDayRent"].GetInt();
        rd.m_perKmRent = r["perKmRent"].GetInt();
        rd.m_kmLimitPerDay = r["kmLimitPerDay"].GetInt();
        vRentalData.push_back(rd);
    }
}
 
vector<Vehicle> Fleet::getAllFleetData()
{
    vector<Vehicle> vehicles;
    if (!dbase)
        return vehicles;
 
    const char *sql = "SELECT * FROM vehicles;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(dbase, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        cerr << "Failed to prepare statement: " << sqlite3_errmsg(dbase) << endl;
        return vehicles;
    }
 
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        Vehicle v;
        v.VID = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        v.VMfr = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        v.VType = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        v.VEngine = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
        v.VLastOdometerReading = sqlite3_column_int(stmt, 4);
        v.VStatus = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));
        vehicles.push_back(v);
    }
    sqlite3_finalize(stmt);
    return vehicles;
}
 
string Fleet::checkVehicleStatus(string vehicleNumber)
{
    if (!dbase)
        return "Unknown";
 
    const char *sql = "SELECT VStatus FROM vehicles WHERE VID = ?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(dbase, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        cerr << "Failed to prepare statement: " << sqlite3_errmsg(dbase) << endl;
        return "Unknown";
    }
    sqlite3_bind_text(stmt, 1, vehicleNumber.c_str(), -1, SQLITE_TRANSIENT);
 
    string status = "Unknown";
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        status = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    }
    sqlite3_finalize(stmt);
    return status;
}
 
vector<Vehicle> Fleet::allVehicleAvailability()
{
    vector<Vehicle> availableVehicles;
    if (!dbase)
        return availableVehicles;
 
    const char *sql = "SELECT * FROM vehicles WHERE VStatus = 'Available';";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(dbase, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        cerr << "Failed to prepare statement: " << sqlite3_errmsg(dbase) << endl;
        return availableVehicles;
    }
 
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        Vehicle v;
        v.VID = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        v.VMfr = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        v.VType = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        v.VEngine = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
        v.VLastOdometerReading = sqlite3_column_int(stmt, 4);
        v.VStatus = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));
        availableVehicles.push_back(v);
    }
    sqlite3_finalize(stmt);
    return availableVehicles;
}
 
string to_lowercase_string(string str) {
    for(int i=0;i<str.length();i++) {
        if(str[i]>=65 && str[i]<=90){
            str[i] += 32;
        }
    }
    return str;
}
 
Allotment Fleet::issueVehicle(string vehicleType, string licenseNumber, int numOfDays)
{
    Allotment newAllotment = {};
 
    if (!dbase)
    {
        cerr << "Database not initialized.\n";
        return newAllotment; // empty allotment
    }
 
    sqlite3_stmt *stmt;
 
    const char *sqlQuery = "SELECT VID, VLastOdometerReading FROM vehicles WHERE VStatus = 'Available' AND VType = ? COLLATE NOCASE LIMIT 1;";
 
    if (sqlite3_prepare_v2(dbase, sqlQuery, -1, &stmt, nullptr) != SQLITE_OK)
    {
        cerr << "Failed to prepare vehicle select statement: " << sqlite3_errmsg(dbase) << endl;
        return newAllotment;
    }
 
    sqlite3_bind_text(stmt, 1, vehicleType.c_str(), -1, SQLITE_TRANSIENT);
 
    string selectedVID;
    int startingOdometer = -1;
 
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        selectedVID = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        startingOdometer = sqlite3_column_int(stmt, 1);
    }
    else
    {
        // No vehicle available
        sqlite3_finalize(stmt);
        return newAllotment;
    }
    sqlite3_finalize(stmt);
 
    RentalData rental;
    bool rentalFound = false;
    for (const auto &data : vRentalData)
    {
        if (to_lowercase_string(data.m_vehicleType) == to_lowercase_string(vehicleType))
        {
            rental = data;
            rentalFound = true;
            break;
        }
    }
    if (!rentalFound)
    {
        cerr << "Rental data not found for vehicle type: " << vehicleType << endl;
        return newAllotment;
    }
 
    // Step 3: Calculate estimated cost and security deposit
    int estimatedCost = rental.m_perDayRent * numOfDays;
    int securityDeposit = 2 * rental.m_perDayRent;
    string issueDate = getTodayDate();
    string expectedReturnDate = addDaysToDate(issueDate, numOfDays);
 
    // Step 4: Insert allotment record into allotments table
    sqlite3_stmt *insertStmt;
    const char *insertSQL = R"(
        INSERT INTO allotments (AID, ALicense, AIssueDate, AExpectedReturnDate,
                               AEstimatedCost, ASecurityDeposit, AFinalCost, AStartingOdometerReading, AEndingOdometerReading)
        VALUES (?, ?, ?, ?, ?, ?, 0,?, 0);
    )";
 
    if (sqlite3_prepare_v2(dbase, insertSQL, -1, &insertStmt, nullptr) != SQLITE_OK)
    {
        cerr << "Failed to prepare allotment insert statement: " << sqlite3_errmsg(dbase) << endl;
        return newAllotment;
    }
 
    sqlite3_bind_text(insertStmt, 1, selectedVID.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(insertStmt, 2, licenseNumber.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(insertStmt, 3, issueDate.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(insertStmt, 4, expectedReturnDate.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(insertStmt, 5, estimatedCost);
    sqlite3_bind_int(insertStmt, 6, securityDeposit);
    sqlite3_bind_int(insertStmt, 7, startingOdometer);
 
    if (sqlite3_step(insertStmt) != SQLITE_DONE)
    {
        cerr << "Failed to insert allotment record: " << sqlite3_errmsg(dbase) << endl;
        sqlite3_finalize(insertStmt);
        return newAllotment;
    }
    sqlite3_finalize(insertStmt);
 
    // Step 5: Update vehicle status to 'Issued'
    sqlite3_stmt *updateStmt;
    const char *updateSQL = "UPDATE vehicles SET VStatus = 'Issued' WHERE VID = ?;";
    if (sqlite3_prepare_v2(dbase, updateSQL, -1, &updateStmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_text(updateStmt, 1, selectedVID.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(updateStmt) != SQLITE_DONE)
        {
            cerr << "Failed to update vehicle status: " << sqlite3_errmsg(dbase) << endl;
        }
        sqlite3_finalize(updateStmt);
    }
    else
    {
        cerr << "Failed to prepare vehicle status update: " << sqlite3_errmsg(dbase) << endl;
    }
 
    // Step 6: Populate and return allotment object
    newAllotment.AID = selectedVID;
    newAllotment.ALicense = licenseNumber;
    newAllotment.AIssueDate = issueDate;
    newAllotment.AExpectedReturnDate = expectedReturnDate;
    newAllotment.AEstimatedCost = estimatedCost;
    newAllotment.ASecurityDeposit = securityDeposit;
    newAllotment.AStartingOdometerReading = startingOdometer;
    newAllotment.AEndingOdometerReading = 0;
    newAllotment.AFinalCost = 0;
 
    return newAllotment;
}
 
int Fleet::returnVehicle(string vehicleNumber, int odometerReading)
{
    if (!dbase)
        return -1;
 
    const char *sqlAllotment = R"(
        SELECT AID, ALicense, AIssueDate, AExpectedReturnDate, AActualReturnDate,
               AEstimatedCost, ASecurityDeposit, AFinalCost,
               AStartingOdometerReading, AEndingOdometerReading
        FROM allotments
        WHERE AID = ? AND (AActualReturnDate IS NULL OR AActualReturnDate = '');
    )";
 
    sqlite3_stmt *stmtAllot;
    if (sqlite3_prepare_v2(dbase, sqlAllotment, -1, &stmtAllot, nullptr) != SQLITE_OK)
    {
        cerr << "Failed to prepare allotment query: " << sqlite3_errmsg(dbase) << endl;
        return -1;
    }
    sqlite3_bind_text(stmtAllot, 1, vehicleNumber.c_str(), -1, SQLITE_TRANSIENT);
 
    int rc = sqlite3_step(stmtAllot);
    if (rc != SQLITE_ROW)
    {
        // Vehicle not issued or no active allotment
        sqlite3_finalize(stmtAllot);
        return INT_MIN;
    }
 
    string allotmentID = reinterpret_cast<const char *>(sqlite3_column_text(stmtAllot, 0));
    string issueDate = reinterpret_cast<const char *>(sqlite3_column_text(stmtAllot, 2));
    string expectedReturnDate = reinterpret_cast<const char *>(sqlite3_column_text(stmtAllot, 3));
    int estimatedCost = sqlite3_column_int(stmtAllot, 5);
    int securityDeposit = sqlite3_column_int(stmtAllot, 6);
    int startingOdometer = sqlite3_column_int(stmtAllot, 8);
 
    sqlite3_finalize(stmtAllot);
 
    if (odometerReading < startingOdometer)
    {
        return INT_MAX;
    }
 
    const char *sqlVehicle = "SELECT VLastOdometerReading, VType FROM vehicles WHERE VID = ?;";
    sqlite3_stmt *stmtVeh;
    if (sqlite3_prepare_v2(dbase, sqlVehicle, -1, &stmtVeh, nullptr) != SQLITE_OK)
    {
        cerr << "Failed to prepare vehicle query: " << sqlite3_errmsg(dbase) << endl;
        return -1;
    }
    sqlite3_bind_text(stmtVeh, 1, vehicleNumber.c_str(), -1, SQLITE_TRANSIENT);
 
    rc = sqlite3_step(stmtVeh);
    if (rc != SQLITE_ROW)
    {
        sqlite3_finalize(stmtVeh);
        return INT_MIN; // Vehicle not found
    }
 
    int lastOdometer = sqlite3_column_int(stmtVeh, 0);
    string vehicleType = reinterpret_cast<const char *>(sqlite3_column_text(stmtVeh, 1));
    sqlite3_finalize(stmtVeh);
 
    // Odometer reading must be >= last recorded odometer
    if (odometerReading < lastOdometer)
    {
        return INT_MAX;
    }
 
    int kmsDriven = odometerReading - startingOdometer;
 
    RentalData *rentData = nullptr;
    for (auto &rd : vRentalData)
    {
        if (rd.m_vehicleType == vehicleType)
        {
            rentData = &rd;
            break;
        }
    }
    if (!rentData)
    {
        cerr << "No rental data for vehicle type " << vehicleType << endl;
        return -1;
    }
    int bookedDays = daysDiff(issueDate, expectedReturnDate);
 
    if (bookedDays <= 0)
        bookedDays = 1; // Minimum 1 day
 
    int allowedKms = bookedDays * rentData->m_kmLimitPerDay;
 
    // 6. Get current date as actual return date string YYYY-MM-DD
    time_t now = time(nullptr);
    struct tm *ltm = localtime(&now);
    char actualReturnDate[9];
    snprintf(actualReturnDate, sizeof(actualReturnDate), "%02d/%02d/%02d",
             ltm->tm_mday, ltm->tm_mon + 1, (ltm->tm_year + 1900) % 100);
 
    // 7. Calculate extra days and extra kms
    int extraDays = daysDiff(expectedReturnDate, actualReturnDate);
 
    if (extraDays < 0)
        extraDays = 0;
 
    int extraKms = kmsDriven - allowedKms;
 
    if (extraKms < 0)
        extraKms = 0;
 
    // 8. Calculate final cost
    int finalCost = estimatedCost + (extraDays * rentData->m_perDayRent) + (extraKms * rentData->m_perKmRent);
    int totalCover = estimatedCost + securityDeposit;
 
    // 9. Update allotments table with actual return date, final cost, ending odometer
    const char *updateAllotSQL = R"(
        UPDATE allotments
        SET AActualReturnDate = ?,
            AFinalCost = ?,
            AEndingOdometerReading = ?
        WHERE AID = ?;
    )";
 
    sqlite3_stmt *stmtUpdateAllot;
    if (sqlite3_prepare_v2(dbase, updateAllotSQL, -1, &stmtUpdateAllot, nullptr) != SQLITE_OK)
    {
        cerr << "Failed to prepare allotment update: " << sqlite3_errmsg(dbase) << endl;
        return -1;
    }
    sqlite3_bind_text(stmtUpdateAllot, 1, actualReturnDate, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmtUpdateAllot, 2, finalCost);
    sqlite3_bind_int(stmtUpdateAllot, 3, odometerReading);
    sqlite3_bind_text(stmtUpdateAllot, 4, allotmentID.c_str(), -1, SQLITE_TRANSIENT);
 
    if (sqlite3_step(stmtUpdateAllot) != SQLITE_DONE)
    {
        cerr << "Failed to update allotment: " << sqlite3_errmsg(dbase) << endl;
        sqlite3_finalize(stmtUpdateAllot);
        return -1;
    }
    sqlite3_finalize(stmtUpdateAllot);
 
    // 10. Update vehicle last odometer reading and status to 'Available'
    const char *updateVehSQL = R"(
        UPDATE vehicles
        SET VLastOdometerReading = ?, VStatus = 'Available'
        WHERE VID = ?;
    )";
 
    sqlite3_stmt *stmtUpdateVeh;
    if (sqlite3_prepare_v2(dbase, updateVehSQL, -1, &stmtUpdateVeh, nullptr) != SQLITE_OK)
    {
        cerr << "Failed to prepare vehicle update: " << sqlite3_errmsg(dbase) << endl;
        return -1;
    }
    sqlite3_bind_int(stmtUpdateVeh, 1, odometerReading);
    sqlite3_bind_text(stmtUpdateVeh, 2, vehicleNumber.c_str(), -1, SQLITE_TRANSIENT);
 
    if (sqlite3_step(stmtUpdateVeh) != SQLITE_DONE)
    {
        cerr << "Failed to update vehicle: " << sqlite3_errmsg(dbase) << endl;
        sqlite3_finalize(stmtUpdateVeh);
        return -1;
    }
    sqlite3_finalize(stmtUpdateVeh);
 
    // 11. Return amount to be returned or collected:
    if (finalCost == estimatedCost)
    {
        // Return full security deposit
        return -securityDeposit;
    }
    else if (finalCost < totalCover)
    {
        // Return difference from security deposit
        return -(totalCover - finalCost);
    }
    else
    {
        // Collect extra amount from customer
        return finalCost - totalCover;
    }
}
 
//
 
vector<Allotment> Fleet::getVehicleRentalHistory(string vehicleNumber, string strStartDate, string strEndDate)
{
    vector<Allotment> history;
 
    if (!dbase)
    {
        cerr << "Database not connected." << endl;
        return history;
    }
 
    const char *sql = R"(
        SELECT AID, ALicense, AIssueDate, AExpectedReturnDate, AActualReturnDate,
               AEstimatedCost, ASecurityDeposit, AFinalCost,
               AStartingOdometerReading, AEndingOdometerReading
        FROM allotments
        WHERE AID = ? AND AActualReturnDate IS NOT NULL AND AActualReturnDate != ''
    )";
 
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(dbase, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        cerr << "Failed to prepare history query: " << sqlite3_errmsg(dbase) << endl;
        return history;
    }
 
    sqlite3_bind_text(stmt, 1, vehicleNumber.c_str(), -1, SQLITE_TRANSIENT);
 
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        string issueDate = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
 
        if (!isDateInRange(issueDate, strStartDate, strEndDate))
            continue;
 
        Allotment record;
        record.AID = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        record.ALicense = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        record.AIssueDate = issueDate;
        record.AExpectedReturnDate = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
        record.AActualReturnDate = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
        record.AEstimatedCost = sqlite3_column_int(stmt, 5);
        record.ASecurityDeposit = sqlite3_column_int(stmt, 6);
        record.AFinalCost = sqlite3_column_int(stmt, 7);
        record.AStartingOdometerReading = sqlite3_column_int(stmt, 8);
        record.AEndingOdometerReading = sqlite3_column_int(stmt, 9);
 
        history.push_back(record);
    }
 
    sqlite3_finalize(stmt);
    return history;
}
 
int Fleet::incomeOnVehicle(string vehicleNumber, string strStartDate, string strEndDate)
{
    if (!dbase)
    {
        cerr << "Database not connected." << endl;
        return -1;
    }
 
    const char *sql = R"(
        SELECT AIssueDate, AExpectedReturnDate, AActualReturnDate, AEstimatedCost, AFinalCost
        FROM allotments
        WHERE AID = ?;
    )";
 
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(dbase, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        cerr << "Failed to prepare income query: " << sqlite3_errmsg(dbase) << endl;
        return -1;
    }
 
    sqlite3_bind_text(stmt, 1, vehicleNumber.c_str(), -1, SQLITE_TRANSIENT);
 
    int totalIncome = 0;
 
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        string issueDate = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        string expectedReturnDate = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        const unsigned char *actualReturnDateText = sqlite3_column_text(stmt, 2);
        string actualReturnDate = actualReturnDateText ? reinterpret_cast<const char *>(actualReturnDateText) : "";
 
        int estimatedCost = sqlite3_column_int(stmt, 3);
        int finalCost = sqlite3_column_int(stmt, 4);
 
        // Check if this rental falls within the [strStartDate, strEndDate] range
        bool overlaps = isDateInRange(issueDate, strStartDate, strEndDate) ||
                        isDateInRange(expectedReturnDate, strStartDate, strEndDate) ||
                        isDateInRange(strStartDate, issueDate, expectedReturnDate) ||
                        isDateInRange(strEndDate, issueDate, expectedReturnDate);
 
        if (!overlaps)
            continue;
 
        if (!actualReturnDate.empty())
        {
            // Completed rental — use final cost
            totalIncome += finalCost;
        }
        else
        {
            // Ongoing rental — use estimated cost
            totalIncome += estimatedCost;
        }
    }
 
    sqlite3_finalize(stmt);
    return totalIncome;
}
 
int Fleet::incomeOnFleet(string strStartDate, string strEndDate)
{
    if (!dbase)
    {
        cerr << "Database not connected." << endl;
        return -1;
    }
 
    const char *sql = R"(
        SELECT AIssueDate, AExpectedReturnDate, AActualReturnDate, AEstimatedCost, AFinalCost
        FROM allotments;
    )";
 
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(dbase, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        cerr << "Failed to prepare fleet income query: " << sqlite3_errmsg(dbase) << endl;
        return -1;
    }
 
    int totalIncome = 0;
 
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        string issueDate = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        string expectedReturnDate = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        const unsigned char *actualReturnDateText = sqlite3_column_text(stmt, 2);
        string actualReturnDate = actualReturnDateText ? reinterpret_cast<const char *>(actualReturnDateText) : "";
 
        int estimatedCost = sqlite3_column_int(stmt, 3);
        int finalCost = sqlite3_column_int(stmt, 4);
 
        // Check overlap with the specified date range
        bool overlaps = isDateInRange(issueDate, strStartDate, strEndDate) ||
                        isDateInRange(expectedReturnDate, strStartDate, strEndDate) ||
                        isDateInRange(strStartDate, issueDate, expectedReturnDate) ||
                        isDateInRange(strEndDate, issueDate, expectedReturnDate);
 
        if (!overlaps)
            continue;
 
        if (!actualReturnDate.empty())
        {
            // Completed rental
            totalIncome += finalCost;
        }
        else
        {
            // Ongoing rental
            totalIncome += estimatedCost;
        }
    }
 
    sqlite3_finalize(stmt);
    return totalIncome;
}
 
Fleet::~Fleet()
{
    if (dbase)
    {
        sqlite3_close(dbase);
    }
}
 
 