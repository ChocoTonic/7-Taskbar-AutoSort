#include <windows.h>

extern "C"
{
#include "version_compare.h"
}

#include <gtest/gtest.h>

TEST(VersionCompare, NewerMajor)
{
    EXPECT_TRUE(VersionCompare(L"1.0.0", L"2.0.0"));
}

TEST(VersionCompare, OlderMajor)
{
    EXPECT_FALSE(VersionCompare(L"2.0.0", L"1.0.0"));
}

TEST(VersionCompare, SameMajorNewerMinor)
{
    EXPECT_TRUE(VersionCompare(L"1.0.0", L"1.1.0"));
}

TEST(VersionCompare, SameMajorOlderMinor)
{
    EXPECT_FALSE(VersionCompare(L"1.1.0", L"1.0.0"));
}

TEST(VersionCompare, NewerPatch)
{
    EXPECT_TRUE(VersionCompare(L"1.0.0", L"1.0.1"));
}

TEST(VersionCompare, OlderPatch)
{
    EXPECT_FALSE(VersionCompare(L"1.0.1", L"1.0.0"));
}

TEST(VersionCompare, SameVersion)
{
    EXPECT_FALSE(VersionCompare(L"1.0.0", L"1.0.0"));
}

TEST(VersionCompare, MultiDigitMinor)
{
    EXPECT_TRUE(VersionCompare(L"1.9.0", L"1.10.0"));
}

TEST(VersionCompare, MultiDigitPatch)
{
    EXPECT_TRUE(VersionCompare(L"2.3.9", L"2.3.10"));
}

TEST(VersionCompare, ZeroVersion)
{
    EXPECT_TRUE(VersionCompare(L"0.0.0", L"0.0.1"));
}

TEST(VersionCompare, LargeNumbers)
{
    EXPECT_TRUE(VersionCompare(L"99.99.99", L"100.0.0"));
}

TEST(VersionCompare, MajorDominatesMinor)
{
    EXPECT_TRUE(VersionCompare(L"1.9.9", L"2.0.0"));
}
