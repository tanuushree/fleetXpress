#include "fleetXpress.h"

void convertstrDateTotmDate(string strDate, time_t& tmDate)
{
    // Parse date components
    int day, month, year, hour, minute;
    char delimiter;

    istringstream dateStream(strDate);
    dateStream >> day >> delimiter >> month >> delimiter >> year;

    // Create and initialize a tm struct
    tm timeStruct = {0};
    timeStruct.tm_sec = 0;    // Seconds
    timeStruct.tm_min = 0;
    timeStruct.tm_hour = 0;
    timeStruct.tm_mday = day;
    timeStruct.tm_mon = month - 1;  // Months are 0-based
    timeStruct.tm_year = year + 100;  // Years are relative to 1900

    // Convert struct tm to time_t
    // time_t value represents date and time in seconds since the epoch (January 1, 1970)
    tmDate = mktime(&timeStruct);
}

void converttmDateTimeTostrDatestrTime(time_t dateTime, string& strDate, string& strTime)
{
    char bufferDate[12];
    tm* timeStruct = localtime(&dateTime);
    strftime(bufferDate, sizeof(bufferDate), "%d/%m/%y", timeStruct);
    strDate = bufferDate;
    
    char bufferTime[6];
    strftime(bufferTime, sizeof(bufferTime), "%H:%M", timeStruct);
    strTime = bufferTime;
}
