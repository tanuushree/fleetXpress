#include "../utilityFunctions.h"
#include <gtest/gtest.h>
#include <vector>
#include <x86intrin.h>

static unsigned long long total_cycles = 0;
static unsigned long long min_cycles = UINT64_MAX;
static unsigned long long max_cycles = 0;
static int test_case_count = 0;

TEST(TestAuthentication, RegularUser)
{
    vector<string> creds = {"kara", "shiva", "arya", "kanchen"};
    for (auto cred : creds)
    {
        string str = cred + "\nTrajenta500!\n";
        istringstream input(str);
        streambuf *origCinBuf = cin.rdbuf(input.rdbuf());
        testing::internal::CaptureStdout();
        unsigned long long start_cycles = __rdtsc();
        UserType actualUserType = authenticate("UserData.json");
        unsigned long long end_cycles = __rdtsc();
        unsigned long long cycle_diff = end_cycles - start_cycles;

        total_cycles += cycle_diff;
        if (cycle_diff < min_cycles)
            min_cycles = cycle_diff;
        if (cycle_diff > max_cycles)
            max_cycles = cycle_diff;
        test_case_count++;
        cin.rdbuf(origCinBuf);
        string output = testing::internal::GetCapturedStdout();
        ASSERT_EQ(user, actualUserType);
    }
}

TEST(TestAuthentication, AdminUser)
{
    vector<string> creds = {"koram", "lik", "bhatta", "junga"};
    for (auto cred : creds)
    {
        string str = cred + "\nVolix250!\n";
        istringstream input(str);
        streambuf *origCinBuf = cin.rdbuf(input.rdbuf());
        testing::internal::CaptureStdout();
        unsigned long long start_cycles = __rdtsc();
        UserType actualUserType = authenticate("UserData.json");
        unsigned long long end_cycles = __rdtsc();
        unsigned long long cycle_diff = end_cycles - start_cycles;

        total_cycles += cycle_diff;
        if (cycle_diff < min_cycles)
            min_cycles = cycle_diff;
        if (cycle_diff > max_cycles)
            max_cycles = cycle_diff;
        test_case_count++;
        cin.rdbuf(origCinBuf);
        string output = testing::internal::GetCapturedStdout();
        ASSERT_EQ(admin, actualUserType);
    }
}

TEST(TestAuthentication, InvalidUser)
{
    istringstream input("david\nEdwin100!\n");
    streambuf *origCinBuf = cin.rdbuf(input.rdbuf());
    testing::internal::CaptureStdout();
    unsigned long long start_cycles = __rdtsc();
    UserType actualUserType = authenticate("UserData.json");
    unsigned long long end_cycles = __rdtsc();
    unsigned long long cycle_diff = end_cycles - start_cycles;

    total_cycles += cycle_diff;
    if (cycle_diff < min_cycles)
        min_cycles = cycle_diff;
    if (cycle_diff > max_cycles)
        max_cycles = cycle_diff;
    test_case_count++;
    cin.rdbuf(origCinBuf);
    string output = testing::internal::GetCapturedStdout();
    ASSERT_EQ(invalid, actualUserType);
}

class StatsReporter : public ::testing::Environment
{
public:
    void TearDown() override
    {
        if (test_case_count > 0)
        {
            double average_cycles = (double)total_cycles / test_case_count;
            printf("\n=== CPU Cycle Stats for Authenticate ===\n");
            printf("Total Tests: %d\n", test_case_count);
            printf("Minimum Cycles: %llu\n", min_cycles);
            printf("Maximum Cycles: %llu\n", max_cycles);
            printf("Average Cycles: %.2f\n", average_cycles);
            printf("=============================================\n\n");
        }
    }
};

::testing::Environment *const stats_env = ::testing::AddGlobalTestEnvironment(new StatsReporter);