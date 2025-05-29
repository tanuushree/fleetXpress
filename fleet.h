#ifndef FLEET_H
#define FLEET_H
#include <string>
#include <vector>
#include <sqlite3.h>
#include "dateTimeFunctions.h"
using namespace std;

struct RentalData
{
	string m_vehicleType;
	int m_perDayRent;
	int m_perKmRent;
	int m_kmLimitPerDay;
};

struct Vehicle
{
	string VID;
	string VMfr;
	string VType;
	string VEngine;
	int VLastOdometerReading;
	string VStatus;
};

struct Allotment
{
	string AID;
	string ALicense;
	string AIssueDate;
	string AExpectedReturnDate;
	string AActualReturnDate;
	int AEstimatedCost;
	int ASecurityDeposit;
	int AFinalCost;
	int AStartingOdometerReading;
	int AEndingOdometerReading;
};

class Fleet
{
	private:
		sqlite3* dbase;
		vector<RentalData> vRentalData;

	public:
		Fleet();
		Fleet(string fleetJsonFilename, string fleetDbFilename, string rentalDataJsonFilename);
		vector<Vehicle> getAllFleetData();
		void printAllotmentsTable();
		Allotment issueVehicle(string vehicleNumber, string licenseNumber, int numOfDays);
		int returnVehicle(string vehicleNumber, int odometerReading);
		string checkVehicleStatus(string vehicleNumber);
		vector<Vehicle> allVehicleAvailability();
		vector<Allotment> getVehicleRentalHistory(string vehicleNumber, string strStartDate, string strEndDate);
		int incomeOnVehicle(string vehicleNumber, string strStartDate, string strEndDate);
		int incomeOnFleet(string strStartDate, string strEndDate);
		~Fleet();  
};

#endif

