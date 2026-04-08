#include <climits>
#include <gtest/gtest.h>

extern "C"
{
#include "ai_interface.h"
#include "game_types.h"
}

static Param
make_param (const char *key, const char *value)
{
  return { key, value };
}

// Checks that UINT_32 overflow is properly handled in check_param.
TEST (CheckParamUINT32, OverflowAboveUINT32_MAX)
{
  uint32_t target = 0;
  Param p = make_param ("max_iterations", "9999999999"); // > UINT32_MAX

  check_param (p, "max_iterations", &target);

  EXPECT_EQ (target, UINT32_MAX)
      << "BUG: clamping to UINT32_MAX never happens.";
}

// Checks that UINT_32 underflow is properly handled in check_param.
TEST (CheckParamUINT32, UnderflowBelowUINT32_MIN)
{
  uint32_t target = 0;
  Param p = make_param (
      "max_iterations",
      "-1"); // strtoul() will interpret this as UINT32_MAX due to underflow

  check_param (p, "max_iterations", &target);

  EXPECT_EQ (target, UINT32_MAX)
      << "BUG: clamping to UINT32_MIN never happens.";
}

// Checks that int overflow is properly handled in check_param.
TEST (CheckParamInt, OverflowAboveINT_MAX)
{
  int target = 0;

  // INT_MAX + 1
  Param p = make_param ("depth", "2147483648");

  check_param (p, "depth", &target);

  EXPECT_EQ (target, INT_MAX)
      << "BUG: int overflow lead to undefined behavior.";
}

// Checks that int underflow is properly handled in check_param.
TEST (CheckParamInt, UnderflowBelowINT_MIN)
{
  int target = 0;

  // INT_MIN - 1
  Param p = make_param ("depth", "-2147483649");

  check_param (p, "depth", &target);

  EXPECT_EQ (target, INT_MIN)
      << "BUG: int underflow lead to undefined behavior.";
}

// Checks that if the value is non numeric, the target variable is set to 0.
TEST (CheckParamUINT32, NonNumericValue)
{
  uint32_t target = 42;
  Param p = make_param ("max_iterations", "not_a_number");

  check_param (p, "max_iterations", &target);

  EXPECT_EQ (target, 0)
      << "BUG: non-numeric value should lead to target being set to 0.";
}

// Checks that if the key does not match, the target variable is not modified.
TEST (CheckParamUINT32, MatchingKey)
{
  uint32_t t = 0;
  Param p = make_param ("k", "7");
  check_param (p, "k", &t);
  EXPECT_EQ (t, 7);
}

// Checks that if the key does not match, the target variable is not modified.
TEST (CheckParamUINT32, NonMatching)
{
  uint32_t t = 42;
  Param p = make_param ("x", "7");
  check_param (p, "k", &t);
  EXPECT_EQ (t, 42);
}

TEST (CheckParamUint32, ClampToMax)
{
  uint32_t t = 0;
  // Passing 5 billion (greater than UINT32_MAX 4,294,967,295)
  Param p = make_param ("k", "5000000000");
  check_param (p, "k", &t);
  EXPECT_EQ (t, UINT32_MAX);
}

TEST (CheckParamUint32, InvalidTextToZero)
{
  uint32_t t = 42;
  // Non-numeric text should hit the final 'else'
  Param p = make_param ("k", "123abc");
  check_param (p, "k", &t);
  EXPECT_EQ (t, 0);
}

TEST (CheckParamUint32, EmptyValueToZero)
{
  uint32_t t = 42;
  // Empty string should hit the final 'else'
  Param p = make_param ("k", "");
  check_param (p, "k", &t);
  EXPECT_EQ (t, 0);
}

// Checks that if the value is non numeric, the target variable is set to 0.
TEST (CheckParamInt, NonNumericValue)
{
  int target = 42;
  Param p = make_param ("depth", "not_a_number");

  check_param (p, "depth", &target);

  EXPECT_EQ (target, 0)
      << "BUG: non-numeric value should lead to target being set to 0.";
}

// Checks that if the key does not match, the target variable is not modified.
TEST (CheckParamInt, MatchingKey)
{
  int t = 0;
  Param p = make_param ("k", "7");
  check_param (p, "k", &t);
  EXPECT_EQ (t, 7);
}

// Checks that if the key does not match, the target variable is not modified.
TEST (CheckParamInt, NonMatching)
{
  int t = 42;
  Param p = make_param ("x", "7");
  check_param (p, "k", &t);
  EXPECT_EQ (t, 42);
}

// Checks that if the value is negative, the target variable is set to the
// negative value.
TEST (CheckParamInt, Negative)
{
  int t = 0;
  Param p = make_param ("k", "-3");
  check_param (p, "k", &t);
  EXPECT_EQ (t, -3);
}

TEST (CheckParamInt, ClampToMax)
{
  int t = 0;
  Param p = make_param ("k", "3000000000");
  check_param (p, "k", &t);
  EXPECT_EQ (t, INT_MAX);
}

TEST (CheckParamInt, ClampToMin)
{
  int t = 0;
  Param p = make_param ("k", "-3000000000");
  check_param (p, "k", &t);
  EXPECT_EQ (t, INT_MIN);
}

TEST (CheckParamInt, InvalidTextToZero)
{
  int t = 42;
  Param p = make_param ("k", "abc");
  check_param (p, "k", &t);
  EXPECT_EQ (t, 0);
}

// Checks that if the key matches, the target variable is updated to the
// parameter's value.
TEST (CheckParamUl, MatchingKey)
{
  unsigned long t = 0;
  Param p = make_param ("k", "5000");
  check_param (p, "k", &t);
  EXPECT_EQ (t, 5000UL);
}

// Checks that if the key does not match, the target variable is not modified.
TEST (CheckParamUl, NonMatching)
{
  unsigned long t = 3;
  Param p = make_param ("x", "5000");
  check_param (p, "k", &t);
  EXPECT_EQ (t, 3UL);
}

TEST (CheckParamUl, InvalidTextToZero)
{
  unsigned long t = 42;
  Param p = make_param ("k", "123abc");
  check_param (p, "k", &t);
  EXPECT_EQ (t, 0UL);
}

TEST (CheckParamUl, EmptyValueToZero)
{
  unsigned long t = 42;
  Param p = make_param ("k", "");
  check_param (p, "k", &t);
  EXPECT_EQ (t, 0UL);
}

// Checks that if the value is True, the target variable is set to true.
TEST (CheckParamBool, TrueValue)
{
  bool t = false;
  Param p = make_param ("k", "1");
  check_param (p, "k", &t);
  EXPECT_TRUE (t);
}

// Checks that if the value is False, the target variable is set to false.
TEST (CheckParamBool, FalseValue)
{
  bool t = true;
  Param p = make_param ("k", "0");
  check_param (p, "k", &t);
  EXPECT_FALSE (t);
}

// Checks that if the key does not match, the target variable is not modified.
TEST (CheckParamBool, NonMatching)
{
  bool t = true;
  Param p = make_param ("x", "0");
  check_param (p, "k", &t);
  EXPECT_TRUE (t);
}

// Checks that if the key is not a boolean, any non-zero value leads to target
// being set to true.
TEST (CheckParamBool, NonBoolTrue)
{
  bool t = false;
  Param p = make_param ("k", "42");
  check_param (p, "k", &t);
  EXPECT_TRUE (t);
}

// Checks that if the key is not a boolean, any non-zero negative value leads
// to target being set to true.
TEST (CheckParamBool, NonBoolNegativeTrue)
{
  bool t = false;
  Param p = make_param ("k", "-42");
  check_param (p, "k", &t);
  EXPECT_TRUE (t);
}

// Checks that if the key does not match, the target variable is not modified.
TEST (CheckParamBool, KeyNonMatching)
{
  bool t = false;
  Param p = make_param ("x", "42");
  check_param (p, "k", &t);
  EXPECT_FALSE (t);
}

// Checks that if the key matches, the target variable is updated to the
// parameter's value.
TEST (CheckParamChar, MatchingKey)
{
  const char *t = nullptr;
  Param p = make_param ("k", "rth");
  check_param (p, "k", &t);
  EXPECT_STREQ (t, "rth");
}

// Checks that if the key does not match, the target variable is not modified.
TEST (CheckParamChar, NonMatching)
{
  const char *s = "orig";
  const char *t = s;
  Param p = make_param ("x", "rth");
  check_param (p, "k", &t);
  EXPECT_EQ (t, s);
}

// Checks that if the value is an empty string, the target variable is set to
// an empty string.
TEST (CheckParamChar, EmptyString)
{
  const char *t = nullptr;
  Param p = make_param ("k", "");
  check_param (p, "k", &t);
  EXPECT_STREQ (t, "");
}
