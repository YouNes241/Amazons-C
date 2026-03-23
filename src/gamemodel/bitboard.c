#include "../../include/bitboard.h"
#include <stdio.h>

static Bitboard MASK_NOT_A[12];
static Bitboard MASK_NOT_H[12];
static Bitboard BOARD_MASKS[12];
static bool slide_masks_init = false;
static bool masks_initialized = false;

static void
init_slide_masks ()
{
  if (slide_masks_init)
    return;
  for (int w = 4; w <= 11; w++)
    {
      MASK_NOT_A[w] = bitboard_create_full ();
      MASK_NOT_H[w] = bitboard_create_full ();
      for (int r = 0; r < w; r++)
        {
          bitboard_clear_bit (&MASK_NOT_A[w], r * w);
          bitboard_clear_bit (&MASK_NOT_H[w], r * w + (w - 1));
        }
    }
  slide_masks_init = true;
}

void
bitboard_print (Bitboard bb, uint8_t width)
{
  if (width < 4 || width > 11)
    width = 10;

  printf ("\n   ");
  for (int col = 0; col < width; col++)
    printf ("%d ", col);
  printf ("\n");

  for (int row = 0; row < width; row++)
    {
      printf ("%d: ", row);
      for (int col = 0; col < width; col++)
        {
          // Calculate the bit index for the current cell (row, col)
          uint8_t index = row * width + col;

          if (bitboard_get_bit (bb, index))
            {
              printf ("X "); // Occupied
            }
          else
            {
              printf (". "); // Empty
            }
        }
      printf ("\n");
    }
  printf ("\n");
}

/**
 * @brief Generates all squares reachable by a "King" step from the 'sources'
 * bitboard.
 *
 * @param sources Bitboard with the starting positions (e.g., queens).
 * @param empty Bitboard with all empty squares (not occupied).
 * @param width The width of the board.
 * @return A Bitboard representing all valid reachable squares.
 */
Bitboard
expand_king_step (Bitboard sources, Bitboard empty, uint8_t width)
{
  if (!slide_masks_init)
    init_slide_masks ();

  Bitboard result = bitboard_create_empty ();

  result = bitboard_or (result, bitboard_shift_right (sources, width)); // N
  result = bitboard_or (result, bitboard_shift_left (sources, width));  // S

  // Fetch pre-computed boundary masks
  Bitboard mask_not_a = MASK_NOT_A[width];
  Bitboard mask_not_h = MASK_NOT_H[width];
  Bitboard west_moves = bitboard_and (sources, mask_not_a);
  Bitboard east_moves = bitboard_and (sources, mask_not_h);

  // Apply shifts for East, West, and Diagonals
  result = bitboard_or (result, bitboard_shift_right (west_moves, 1)); // W
  result = bitboard_or (result,
                        bitboard_shift_right (west_moves, width + 1)); // NW
  result = bitboard_or (result,
                        bitboard_shift_left (west_moves, width - 1)); // SW

  result = bitboard_or (result, bitboard_shift_left (east_moves, 1)); // E
  result = bitboard_or (result,
                        bitboard_shift_right (east_moves, width - 1)); // NE
  result = bitboard_or (result,
                        bitboard_shift_left (east_moves, width + 1)); // SE

  return bitboard_and (result, empty);
}

/**
 * @brief Generates all squares reachable by "Queen" slides from the 'sources'
 * bitboard.
 *
 * @param sources Bitboard with the starting positions (e.g., queens).
 * @param empty Bitboard with all empty squares (not occupied).
 * @param width The width of the board.
 * @return A Bitboard representing all valid reachable squares.
 */
Bitboard
expand_queen_slide (Bitboard sources, Bitboard empty, uint8_t width)
{
  if (!slide_masks_init)
    init_slide_masks ();

  Bitboard not_a = MASK_NOT_A[width];
  Bitboard not_h = MASK_NOT_H[width];
  Bitboard full = bitboard_create_full ();

  Bitboard attacks = bitboard_create_empty ();

  // N/S
  attacks
      = bitboard_or (attacks, kogge_stone_right (sources, empty, full, width));
  attacks
      = bitboard_or (attacks, kogge_stone_left (sources, empty, full, width));

  // E/W
  attacks = bitboard_or (attacks, kogge_stone_left (sources, empty, not_a, 1));
  attacks
      = bitboard_or (attacks, kogge_stone_right (sources, empty, not_h, 1));

  // NE/NW/SE/SW
  attacks = bitboard_or (attacks,
                         kogge_stone_right (sources, empty, not_a, width - 1));
  attacks = bitboard_or (attacks,
                         kogge_stone_right (sources, empty, not_h, width + 1));
  attacks = bitboard_or (attacks,
                         kogge_stone_left (sources, empty, not_a, width + 1));
  attacks = bitboard_or (attacks,
                         kogge_stone_left (sources, empty, not_h, width - 1));

  return attacks;
}

/**
 * @brief Pre-compute masks for valid squares based on board size.
 *
 * Initializes board masks used to correct uninitialized squares.
 * The masks are pre-computed to avoid repeated calculations.
 *
 * @return void
 */
void
init_board_masks ()
{
  for (uint8_t s = 4; s <= 11; s++)
    {
      int total_squares = s * s;
      Bitboard m = { 0, 0 };

      if (total_squares <= 64)
        {
          m.low = (total_squares == 64) ? ~0ULL : (1ULL << total_squares) - 1;
          m.high = 0;
        }
      else
        {
          m.low = ~0ULL;
          m.high = (1ULL << (total_squares - 64)) - 1;
        }
      BOARD_MASKS[s] = m;
    }
  masks_initialized = true;
}

/**
 * @brief Retrieves the pre-computed board mask for a given board size.
 *
 * The mask filters out-of-bounds squares for boards sized 4 to 11.
 * Automatically initializes masks on the first call if necessary.
 *
 * @param size The size of the board (4 to 11).
 * @return A Bitboard with bits set for valid squares on the board of the given
 * size.
 */
Bitboard
get_board_mask (uint8_t size)
{
  if (!masks_initialized)
    {
      init_board_masks ();
    }

  uint8_t limit = (size > 11) ? 10 : (size < 4 ? 4 : size);
  return BOARD_MASKS[limit];
}
