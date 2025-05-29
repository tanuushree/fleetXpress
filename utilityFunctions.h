#ifndef UTILITYFUNCTIONS_H
#define UTILITYFUNCTIONS_H
#include <string>
#include "fleetXpress.h"
using namespace std;
enum UserType { user, admin, invalid } ;

enum UserType authenticate(string dataFile);
string sha256(const string& input);

#endif