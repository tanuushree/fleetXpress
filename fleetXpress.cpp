#include "fleetXpress.h"
#include <iostream>
 
 
 
int main()
{
    cout << "Welcome to FleetXpress" << endl
         << endl;
 
    UserType userRole = authenticate("UserData.json");
 
    if (userRole == invalid)
    {
        std::cout << "Invalid Role Found.\n";
        return 1;
    }
 
    // std::cout << "Authentication successful.\n";
    // std::cout << "User role: " << (userRole == admin ? "Admin" : "User") << "\n";
 
    Fleet fleet("fleetData.json", "fleet.db", "rentalData.json");
 
    while (true)
    {
        std::cout << (userRole == admin ? "\nAdmin Menu:\n" : "\nUser Menu:\n");
        std::cout << "1. Display All Fleet Data\n";
        std::cout << "2. Issue Vehicle\n";
        std::cout << "3. Return Vehicle\n";
        std::cout << "4. Check Vehicle Status\n";
 
        if (userRole == admin)
        {
            std::cout << "5. Get Vehicle Rental History\n";
            std::cout << "6. Income On Vehicle\n";
            std::cout << "7. Income on Fleet\n";
        }
 
        std::cout << "0. Exit\n";
        std::cout << "Enter your choice: ";
 
        int choice;
        std::cin >> choice;
 
        switch (choice)
        {
        case 1:
            std::cout << std::left
                      << std::setw(12) << "VID"
                      << std::setw(10) << "VMfr"
                      << std::setw(12) << "VType"
                      << std::setw(10) << "VEngine"
                      << std::setw(20) << "VLastOdometerReading"
                      << std::setw(10) << "VStatus"
                      << "\n"
                      << std::string(74, '-') << "\n";
            for (const auto &v : fleet.allVehicleAvailability())
            {
                std::cout << std::left
                          << std::setw(12) << v.VID
                          << std::setw(10) << v.VMfr
                          << std::setw(12) << v.VType
                          << std::setw(10) << v.VEngine
                          << std::setw(20) << v.VLastOdometerReading
                          << std::setw(10) << v.VStatus
                          << "\n";
            }
            break;
 
        case 2:
        {
            std::string type, license;
            int days;
            std::cout << "Enter Vehicle Type: ";
            std::cin >> type;
            std::cout << "Enter License: ";
            std::cin >> license;
            std::cout << "Number of Days: ";
            std::cin >> days;
 
            Allotment a = fleet.issueVehicle(type, license, days);
            cout<<endl;
            if (a.AID.empty())
                std::cout << "No available vehicle.\n";
            else
            {
                std::cout << "Allotment Details:\n"
                          << "Vehicle Number: " << a.AID << "\n"
                          << "License Number: " << a.ALicense << "\n"
                          << "Issue Date: " << a.AIssueDate << "\n"
                          << "Expected Return Date: " << a.AExpectedReturnDate << "\n"
                          << "Estimated Cost: " << a.AEstimatedCost << "\n"
                          << "Security Deposit: " << a.ASecurityDeposit << "\n"
                          << "Starting Odometer Reading: " << a.AStartingOdometerReading << "\n"
                          << "Allotment Successful!\n";
            }
        }
        break;
 
        case 3:
        {
            std::string vnum;
            int odo;
            std::cout << "Enter Vehicle Number: ";
            std::cin >> vnum;
            std::cout << "Enter Ending Odometer Reading: ";
            std::cin >> odo;
 
            int result = fleet.returnVehicle(vnum, odo);
 
            if (result == INT_MIN)
                std::cout << "Error: Vehicle not issued.\n";
            else if (result == INT_MAX)
                std::cout << "Error: Invalid odometer reading.\n";
            else if (result < 0)
                std::cout << "Security deposit returned: Rs. " << -result << "\n";
            else if (result > 0)
                std::cout << "Extra charges: Rs. " << result << "\n";
            else
                std::cout << "Vehicle returned successfully.\n";
        }
        break;
 
        case 4:
        {
            std::string vnum;
            std::cout << "Enter Vehicle Number: ";
            std::cin >> vnum;
            std::cout << "Status: " << fleet.checkVehicleStatus(vnum) << "\n";
        }
        break;
 
        case 5:
            if (userRole != admin)
            {
                std::cout << "Access Denied: Admin only.\n";
                break;
            }
            else
            {
                std::string vid, start, end;
                std::cout << "Enter Vehicle Number: ";
                std::cin >> vid;
                std::cout << "Enter Start Date (DD/MM/YYYY): ";
                std::cin >> start;
                std::cout << "Enter End Date (DD/MM/YYYY): ";
                std::cin >> end;
 
                auto history = fleet.getVehicleRentalHistory(vid, start, end);
                if (history.empty())
                    std::cout << "No history in given period.\n";
                else
                {
                    std::cout << "Rental History:\n";
                    for (const auto &h : history)
                    {
                        std::cout << "AID: " << h.AID << ", License: " << h.ALicense
                                  << ", Issue: " << h.AIssueDate
                                  << ", Expected: " << h.AExpectedReturnDate
                                  << ", Actual: " << h.AActualReturnDate
                                  << ", Estimated: Rs. " << h.AEstimatedCost
                                  << ", Final: Rs. " << h.AFinalCost << "\n";
                    }
                }
            }
            break;
 
        case 6:
            if (userRole != admin)
            {
                std::cout << "Access Denied: Admin only.\n";
                break;
            }
            else
            {
                std::string vnum, start, end;
                std::cout << "Enter Vehicle Number: ";
                std::cin >> vnum;
                std::cout << "Enter Start Date (DD/MM/YY): ";
                std::cin >> start;
                std::cout << "Enter End Date (DD/MM/YY): ";
                std::cin >> end;
 
                int income = fleet.incomeOnVehicle(vnum, start, end);
                std::cout << "Total Income: Rs. " << income << "\n";
            }
            break;
 
        case 7:
            if (userRole != admin)
            {
                std::cout << "Access Denied: Admin only.\n";
                break;
            }
            else
            {
                std::string start, end;
                std::cout << "Enter Start Date (DD/MM/YY): ";
                std::cin >> start;
                std::cout << "Enter End Date (DD/MM/YY): ";
                std::cin >> end;
 
                int income = fleet.incomeOnFleet(start, end);
                std::cout << "Total Income: Rs. " << income << "\n";
            }
            break;
 
        case 0:
            std::cout << "Exiting...\n";
            return 0;
 
        default:
            std::cout << "Invalid choice.\n";
            break;
        }
    }
    return 0;
}
 
 