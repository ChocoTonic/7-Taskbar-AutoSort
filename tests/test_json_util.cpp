#include <gtest/gtest.h>
#include <windows.h>
#include "json_util.h"

static std::wstring Parse(const char *pJson)
{
    WCHAR buf[64] = {0};
    ParseVersionFromJson(pJson, buf, 64);
    return std::wstring(buf);
}

TEST(ParseVersionFromJson, ValidTagWithPrefix)
{
    const char *json = R"({"tag_name":"v1.2.3","name":"Release 1.2.3"})";
    EXPECT_EQ(Parse(json), L"1.2.3");
}

TEST(ParseVersionFromJson, ValidTagWithoutPrefix)
{
    const char *json = R"({"tag_name":"2.0.0"})";
    EXPECT_EQ(Parse(json), L"2.0.0");
}

TEST(ParseVersionFromJson, MissingTagName)
{
    const char *json = R"({"name":"Release","body":"notes"})";
    WCHAR buf[64] = {0};
    BOOL result = ParseVersionFromJson(json, buf, 64);
    EXPECT_FALSE(result);
}

TEST(ParseVersionFromJson, InvalidJson)
{
    WCHAR buf[64] = {0};
    BOOL result = ParseVersionFromJson("not json at all", buf, 64);
    EXPECT_FALSE(result);
}

TEST(ParseVersionFromJson, NullJson)
{
    WCHAR buf[64] = {0};
    BOOL result = ParseVersionFromJson(nullptr, buf, 64);
    EXPECT_FALSE(result);
}

TEST(ParseVersionFromJson, NullOutput)
{
    BOOL result = ParseVersionFromJson(R"({"tag_name":"v1.0.0"})", nullptr, 64);
    EXPECT_FALSE(result);
}

TEST(ParseVersionFromJson, ZeroOutputSize)
{
    WCHAR buf[64] = {0};
    BOOL result = ParseVersionFromJson(R"({"tag_name":"v1.0.0"})", buf, 0);
    EXPECT_FALSE(result);
}

TEST(ParseVersionFromJson, EmptyJson)
{
    WCHAR buf[64] = {0};
    BOOL result = ParseVersionFromJson("{}", buf, 64);
    EXPECT_FALSE(result);
}

TEST(ParseVersionFromJson, TagNameNotString)
{
    const char *json = R"({"tag_name":42})";
    WCHAR buf[64] = {0};
    BOOL result = ParseVersionFromJson(json, buf, 64);
    EXPECT_FALSE(result);
}

TEST(ParseVersionFromJson, LargeReleaseJson)
{
    // Simulate a realistic GitHub releases/latest payload
    const char *json = R"({"url":"https://api.github.com/repos/X/Y/releases/1","tag_name":"v0.9.1",)"
                       R"("name":"Release 0.9.1","draft":false,"prerelease":false,"body":"Changelog here."})";
    EXPECT_EQ(Parse(json), L"0.9.1");
}
