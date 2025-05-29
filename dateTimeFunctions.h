#ifndef DATETIMEFUNCTIONS_H
#define DATETIMEFUNCTIONS_H
#include <string>
using namespace std;

void convertstrDateTotmDate(string strDate, time_t& tmDate);
void converttmDateTimeTostrDatestrTime(time_t dateTime, string& strDate, string& strTime);

#endif