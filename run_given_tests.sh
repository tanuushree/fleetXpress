g++ giventests/utilityFunctionsGivenTests.cpp utilityFunctions.cpp -o utilityFunctionsGivenTests -lgtest -lgtest_main -pthread -std=c++11 -lsqlite3 -lssl -lcrypto
./utilityFunctionsGivenTests

g++ giventests/fleetGivenTestsNew.cpp fleet.cpp utilityFunctions.cpp dateTimeFunctions.cpp -o fleetGivenTests -lgtest -lgtest_main -pthread -std=c++11 -lsqlite3 -lssl -lcrypto
./fleetGivenTests

rm ./utilityFunctionsGivenTests ./fleetGivenTests ./giventests/fleet.db