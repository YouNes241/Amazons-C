#include "bitboard.h"
#include <stdint.h>
#include <stdio.h>

#define TEST_PASS(msg) printf ("\033[0;32m[PASS]\033[0m %s\n", msg)
#define TEST_FAIL(msg) printf ("\033[0;31m[FAIL]\033[0m %s\n", msg)

void
unit_access_test ()
{
  printf ("--- UNIT ACCESS TESTS ---\n");
  Bitboard bb = bitboard_create_empty ();

  // Test 1.1: Extremes
  bitboard_set_bit (&bb, 0);
  bitboard_set_bit (&bb, 127);
  if (bitboard_get_bit (bb, 0) == 1 && bitboard_get_bit (bb, 127) == 1)
    {
      TEST_PASS ("Test 1.1 - Extremes (0 and 127) set correctly.");
    }
  else
    {
      TEST_FAIL ("Test 1.1 - Error setting the extremes.");
    }

  // Test 1.2: Boundary
  bb = bitboard_create_empty ();
  bitboard_set_bit (&bb, 63);
  bitboard_set_bit (&bb, 64);
  if (bitboard_get_bit (bb, 63) == 1 && bitboard_get_bit (bb, 64) == 1)
    {
      TEST_PASS ("Test 1.2 - Boundary (63 and 64) respected.");
    }
  else
    {
      TEST_FAIL ("Test 1.2 - Overflow error at boundary.");
    }

  // Test 1.3: Isolation
  bb = bitboard_create_empty ();
  bitboard_set_bit (&bb, 40);
  if (bitboard_get_bit (bb, 40) == 1 && bitboard_get_bit (bb, 104) == 0)
    {
      TEST_PASS (
          "Test 1.3 - Isolation: Modifying 'low' does not affect 'high'.");
    }
  else
    {
      TEST_FAIL ("Test 1.3 - Isolation: Incorrect mask (forgot 1ULL?).");
    }

  // Test 1.4: Clearing
  bb = bitboard_create_empty ();
  bitboard_set_bit (&bb, 10);
  bitboard_set_bit (&bb, 80);
  bitboard_clear_bit (&bb, 80); // Clears only the 80
  if (bitboard_get_bit (bb, 10) == 1 && bitboard_get_bit (bb, 80) == 0)
    {
      TEST_PASS ("Test 1.4 - Clear: The bit was selectively cleared.");
    }
  else
    {
      TEST_FAIL ("Test 1.4 - Clear: Error clearing the bit.");
    }

  // Test 1.5: Inversion (Toggle)
  bb = bitboard_create_empty ();
  bitboard_toggle_bit (&bb, 50); // Turns on
  int etat1 = bitboard_get_bit (bb, 50);
  bitboard_toggle_bit (&bb, 50); // Turns off
  int etat2 = bitboard_get_bit (bb, 50);
  if (etat1 == 1 && etat2 == 0)
    {
      TEST_PASS ("Test 1.5 - Toggle: Inversion (1 then 0) works.");
    }
  else
    {
      TEST_FAIL ("Test 1.5 - Toggle: Error toggling the bit.");
    }
}

void
shifts_test ()
{
  printf ("\n--- SHIFT TESTS ---\n");
  Bitboard bb;

  // Test 2.1: Crossing upwards (Shift Left 63 -> 64)
  bb = bitboard_create_empty ();
  bitboard_set_bit (&bb, 63);
  bb = bitboard_shift_left (bb, 1);
  if (bitboard_get_bit (bb, 64) == 1 && bitboard_get_bit (bb, 63) == 0)
    {
      TEST_PASS (
          "Test 2.1 - Shift Left: Crossing from LOW to HIGH successful.");
    }
  else
    {
      TEST_FAIL ("Test 2.1 - Shift Left: Bit lost at boundary.");
    }

  // Test 2.2: Crossing downwards (Shift Right 64 -> 63)
  bb = bitboard_create_empty ();
  bitboard_set_bit (&bb, 64);
  bb = bitboard_shift_right (bb, 1);
  if (bitboard_get_bit (bb, 63) == 1 && bitboard_get_bit (bb, 64) == 0)
    {
      TEST_PASS (
          "Test 2.2 - Shift Right: Crossing from HIGH to LOW successful.");
    }
  else
    {
      TEST_FAIL ("Test 2.2 - Shift Right: Bit lost at boundary.");
    }

  // Test 2.3: Full switch (Shift = 64)
  bb = bitboard_create_empty ();
  bitboard_set_bit (&bb, 10);
  bb = bitboard_shift_left (bb, 64);
  if (bitboard_get_bit (bb, 74) == 1 && bb.low == 0)
    {
      TEST_PASS (
          "Test 2.3 - Shift Left (64): Full switch to HIGH successful.");
    }
  else
    {
      TEST_FAIL ("Test 2.3 - Shift Left (64): Logic error if shift == 64.");
    }

  // Test 2.4: Giant Shift (Shift > 64)
  bb = bitboard_create_empty ();
  bitboard_set_bit (&bb, 5);
  bb = bitboard_shift_left (bb, 70); // Should land on 75
  if (bitboard_get_bit (bb, 75) == 1 && bb.low == 0)
    {
      TEST_PASS (
          "Test 2.4 - Shift Left Giant (> 64): Bit lands in the right place.");
    }
  else
    {
      TEST_FAIL (
          "Test 2.4 - Shift Left Giant (> 64): Shift calculation error.");
    }

  // Test 2.5: Null Shift (0)
  bb = bitboard_create_empty ();
  bitboard_set_bit (&bb, 25);
  bb = bitboard_shift_left (bb, 0);
  if (bitboard_get_bit (bb, 25) == 1 && bb.high == 0)
    {
      TEST_PASS ("Test 2.5 - Null Shift (0): Board remains intact.");
    }
  else
    {
      TEST_FAIL ("Test 2.5 - Null Shift (0): Board was incorrectly modified.");
    }

  // Test 2.6: Invalid Shift (>= 128)
  bb = bitboard_create_empty ();
  bitboard_set_bit (&bb, 10);
  bitboard_set_bit (&bb, 100);
  bb = bitboard_shift_left (bb, 130);
  if (bb.low == 0 && bb.high == 0)
    {
      TEST_PASS ("Test 2.6 - Invalid Shift (130): Board is correctly empty.");
    }
  else
    {
      TEST_FAIL ("Test 2.6 - Invalid Shift (130): Some bits survived.");
    }
}

void
logical_operations_test ()
{
  printf ("\n--- LOGICAL OPERATIONS TESTS ---\n");
  Bitboard bb1 = bitboard_create_empty ();
  Bitboard bb2 = bitboard_create_empty ();

  // Test 3.1: Union (OR)
  bitboard_set_bit (&bb1, 15);
  bitboard_set_bit (&bb2, 85);

  Bitboard resultat_or = bitboard_or (bb1, bb2);

  if (bitboard_get_bit (resultat_or, 15) == 1
      && bitboard_get_bit (resultat_or, 85) == 1)
    {
      TEST_PASS ("Test 3.1 - OR Operation: Both bitboards merged.");
    }
  else
    {
      TEST_FAIL ("Test 3.1 - OR Operation: Error during merge.");
    }

  // Test 3.2: Collision (AND)
  bb1 = bitboard_create_empty ();
  bb2 = bitboard_create_empty ();
  bitboard_set_bit (&bb1, 42);
  bitboard_set_bit (&bb2, 42);
  bitboard_set_bit (&bb2, 90);

  Bitboard resultat_and = bitboard_and (bb1, bb2);

  if (bitboard_get_bit (resultat_and, 42) == 1
      && bitboard_get_bit (resultat_and, 90) == 0)
    {
      TEST_PASS ("Test 3.2 - AND Operation: Collision detection successful.");
    }
  else
    {
      TEST_FAIL ("Test 3.2 - AND Operation: Collision not detected.");
    }

  // Test 3.3: Total inversion (NOT)
  bb1 = bitboard_create_empty ();
  Bitboard resultat_not = bitboard_not (bb1);

  if (resultat_not.low == ~0ULL && resultat_not.high == ~0ULL)
    {
      TEST_PASS ("Test 3.3 - NOT Operation: Total inversion (128 bits to 1) "
                 "successful.");
    }
  else
    {
      TEST_FAIL ("Test 3.3 - NOT Operation: Inversion failed.");
    }
}

int
main ()
{
  printf ("========== START OF TEST SUITE ==========\n\n");

  unit_access_test ();
  shifts_test ();
  logical_operations_test ();

  printf ("\n=========== END OF TEST SUITE ===========\n");
  return 0;
}
