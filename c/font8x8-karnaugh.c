#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define C_OUTPUT

#ifdef NDEBUG
#define debug_printf(...) ((void)0)
#else
#define debug_printf(...) fprintf(stderr, __VA_ARGS__)
#endif

const char font8x8_basic[128][8];

//////// Trivial helpers

/// @brief  Check whether an integer is a power of two.
#define IS_POW2(x) ((x) != 0 && ((x) & ((x) - 1)) == 0)


/// @brief  Swap two elements in a uint16_t array.
static inline void swapu16(uint16_t *A, size_t i, size_t j) {
    uint16_t temp = A[i];
    A[i] = A[j];
    A[j] = temp;
}

/// @brief  Comparison function for `uint16_t` for `qsort`/`bsearch` ascending.
static int uint16_compare(const void *a, const void *b) {
    uint16_t ua = *(const uint16_t *)a;
    uint16_t ub = *(const uint16_t *)b;
    return (int)ua - (int)ub;
}

/// @brief  Print an implicant for human. The DC bits are printed as 'X'.
static void human_print_implicant(uint16_t implicant) {
    uint8_t dc_mask = implicant >> 8;
    uint8_t value = implicant & 0xff;
    for (int bit = 7; bit >= 0; --bit) {
        if ((dc_mask >> bit) & 1)
            printf("X");
        else
            printf("%d", (value >> bit) & 1);
    }
}


//////// Quine–McCluskey functions

/// @brief  A specialized version of `implicant_covers` when the child
///         implicant is a minterm.
/// @ensures \result == implicant_covers(implicant, (uint16_t)ascii)
bool covers(uint16_t implicant, uint8_t ascii) {
    uint8_t dc_mask = implicant >> 8;
    return (ascii | dc_mask) == ((implicant & 0xff) | dc_mask);
}

bool any_covers(uint16_t *implicants, size_t count, uint8_t ascii) {
    for (size_t i = 0; i < count; ++i)
        if (covers(implicants[i], ascii))
            return true;
    return false;
}

/// @brief  Check whether `parent` implicant covers `child` implicant.
bool implicant_covers(uint16_t parent, uint16_t child) {
    // An implicant covers another implicant if the following hold:
    // 1. The DC mask of the parent covers the DC mask of the child.
    //    That is, if the parent cares about a bit, then the child must also
    //    care about that bit.
    // 2. For all bits that are not DC in the parent, the values of the parent
    //    and the child are the same.
    uint8_t parent_dc_mask = parent >> 8;
    uint8_t child_dc_mask = child >> 8;
    uint8_t parent_value = (parent & 0xff) & ~parent_dc_mask;
    uint8_t child_value = (child & 0xff) & ~parent_dc_mask;
    if ((parent_dc_mask & child_dc_mask) != child_dc_mask) {
        // Truth table check:
        // P C P&C OK? P&C==C
        // 0 0  0   1    1      // Both care
        // 0 1  0   0    0      // Parent cares, child doesn't BAD
        // 1 0  0   1    1      // Child cares, parent doesn't
        // 1 1  1   1    1      // Neither cares
        return false;
    }
    return parent_value == child_value;
}

/// @brief  Explode an implicant into a list of minterms.
///         The result must be freed by the caller.
size_t explode_implicant(uint16_t implicant, uint8_t **minterms) {
    uint8_t dc_mask = implicant >> 8;
    uint8_t value = implicant & 0xff;
#ifdef __GNUC__
    size_t minterm_count = 1 << __builtin_popcount(dc_mask);
#else
    // inspired by https://stackoverflow.com/a/7813486
    uint8_t popcount = dc_mask;
    popcount = (popcount & 0x55) + ((popcount >> 1) & 0x55);
    popcount = (popcount & 0x33) + ((popcount >> 2) & 0x33);
    popcount = (popcount & 0x0f) + ((popcount >> 4) & 0x0f);
    size_t minterm_count = 1 << popcount;
#endif
    assert(minterm_count > 0);
    // Generate the minterms
    uint8_t *dest = malloc(minterm_count);
    assert(dest);
    *minterms = dest;
    for (size_t i = 0; i < minterm_count; ++i)
        dest[i] = value;
    // Generate the minterms in log2(2^D) = D steps:
    // bit 0: toggle the first half ()
    // bit 1: toggle the first quarter and the third quarter
    // ...
    // bit D-1: toggle every other item
    size_t dc_idx = 0;
    for (uint8_t bit = 0; bit < 8; ++bit) {
        assert(dc_idx < minterm_count);
        if (!((dc_mask >> bit) & 1))
            continue;
        uint8_t toggle_mask = 1 << bit;
        size_t toggle_block_size = minterm_count >> (1+dc_idx);
        assert(IS_POW2(toggle_block_size));
        size_t toggle_block_pair_count = 1 << dc_idx;
        assert(2 * toggle_block_size * toggle_block_pair_count == minterm_count);
        for (size_t block_pair = 0; block_pair < toggle_block_pair_count; ++block_pair) {
            for (size_t idx = 0; idx < toggle_block_size; ++idx) {
                // Toggle the even-numbered blocks
                size_t to_toggle = 2 * block_pair * toggle_block_size + idx;
                assert(to_toggle < minterm_count);
                dest[to_toggle] ^= toggle_mask;
            }
        }
        ++dc_idx;
    }
#ifndef NDEBUG
    debug_printf("Implicant 0x%04x generates minterms: ", implicant);
    for (size_t i = 0; i < minterm_count; ++i) {
        debug_printf("0x%02x, ", dest[i]);
        assert(covers(implicant, dest[i]));
    }
    debug_printf("\n");
    // Check that all the generated minterms are distinct
    for (size_t i = 0; i < minterm_count; ++i)
        for (size_t j = i + 1; j < minterm_count; ++j)
            assert(dest[i] != dest[j]);
#endif
    return minterm_count;
}


/// @brief  We check whether the current list of implicants fully covers
///         another implicant. Note that this is a very expensive operation,
///         because it performs N*2^D checks, where N is the number of
///         implicants and D is the number of DC bits in the target.
bool fully_covered(uint16_t *implicants, size_t count, uint16_t target) {
    // We check for this by converting the target implicant into a list of
    // minterms and checking whether all of them are covered.
    uint8_t *minterms;
    size_t minterm_count = explode_implicant(target, &minterms);
    for (size_t i = 0; i < minterm_count; ++i) {
        if (!any_covers(implicants, count, minterms[i])) {
            free(minterms);
            return false;
        }
    }
    free(minterms);
    return true;
}


/// @brief  Mark a bit in an implicant as Don't Care.
static inline uint16_t mark_dc(uint16_t implicant, uint8_t dc_mask) {
    uint16_t result = implicant | ((uint16_t)dc_mask << 8);
    // We set the bit to 1, as discussed above in `fontmap_to_minterms`
    result |= dc_mask;
    // Post-conditions
#ifndef NDEBUG
    debug_printf("0x%04x covers 0x%04x\n", result, implicant);
    assert(implicant_covers(result, implicant));
    assert(covers(result, implicant & 0xff));
#endif
    return result;
}

/// @brief  Merge two implicants if they can be merged.
///         If they cannot be merged, this function returns 0.
///         Otherwise, it returns the merged implicant.
static uint16_t implicant_merge(uint16_t left, uint16_t right) {
    // Trivial: if one implicant covers the other, return the more general one
    if (implicant_covers(left, right))
        return left;
    if (implicant_covers(right, left))
        return right;
    // Now, two implicants can be merged if the following hold:
    // 1. They have the same DC mask.
    // 2. They differ in exactly one cared bit.
    uint8_t left_dc_mask = left >> 8;
    uint8_t right_dc_mask = right >> 8;
    if (left_dc_mask != right_dc_mask)
        return 0;
    uint8_t diff = (left ^ right) & 0xff;
    if (!IS_POW2(diff))
        return 0;
    left = mark_dc(left, diff);
    return left;
}

/// @brief  Perform a modified Quine–McCluskey algorithm in place.
/// @param implicants An array of implicants, likely all minterms initially.
/// @param implicant_count On input, the number of implicants in the array.
///                        On output, the number of prime implicants found.
/// This function will never increase the number of implicants, so it is safe
/// to pass a statically allocated array with sufficient size.
// This function differs from the standard Quine–McCluskey algorithm in that
// we do not try to find all prime implicants and then select essential
// prime implicants. Instead, we first merge adjacent or covering implicants
// until no more merges can be performed, and then we try to expand some
// implicants by marking one more bit as DC, which doubles their coverage.
// We verify the validity of the expansion by checking that all the newly
// covered minterms are not maxterms.
void quine_mccluskey(
    uint16_t *implicants,
    size_t *implicant_count
) {
    size_t merges = 0;
    for (size_t i = 0; i < *implicant_count; ++i) {
        for (size_t j = i + 1; j < *implicant_count; ++j) {
            uint16_t merged = implicant_merge(implicants[i], implicants[j]);
            if (merged) {
                debug_printf("Merged 0x%04x and 0x%04x into 0x%04x\n", implicants[i],
                             implicants[j], merged);
                // We swap the junk to the end of the array
                implicants[i] = merged;
                swapu16(implicants, j, --(*implicant_count));
                merges++;
            }
        }
    }
    debug_printf("Merges performed: %zu\n", merges);
    if (merges == 0) {
        // Here we do something interesting:
        // for each implicant, we try to expand it by marking some cared bits as DC,
        // and we check if all of the expansion part actually covers something
        // already covered by the current list of implicants.
        // If so, we perform the expansion.
        // This is the slow step, so we only do it when no more merges can be performed.
        size_t expansions = 0;
        for (size_t i = 0; i < *implicant_count; ++i) {
            for (uint8_t this_bit_mask = 1; this_bit_mask; this_bit_mask <<= 1) {
                // We must load the implicant inside the loop, since it may be
                // modified by previous iterations of this loop.
                uint16_t implicant = implicants[i];
                uint8_t dc_mask = implicant >> 8;
                if (dc_mask & this_bit_mask)
                    // Already DC, so not a candidate for expansion
                    continue;
                // We need to make sure all the newly covered minterms are
                // actually covered by some implicant already in the list,
                // otherwise the expansion is not valid.
                // Toggle the bit to mark, so we make sure that by marking it
                // as DC, we are not gaining bogus coverage.
                uint16_t to_check = implicant ^ this_bit_mask;
                // Condition: the new implicant has the same DC mask
                assert((to_check >> 8) == (implicant >> 8));
                // Condition: the modified bit is not DC
                assert(((to_check >> 8) & this_bit_mask) == 0);
                // Condition: the new implicant and the old implicant together form
                // a bigger implicant
                assert(implicant_merge(implicant, to_check) == mark_dc(implicant, this_bit_mask));
                if (fully_covered(implicants, *implicant_count, to_check)) {
                    implicants[i] = mark_dc(implicants[i], this_bit_mask);
                    debug_printf("Expanded 0x%04x into 0x%04x\n", implicant, implicants[i]);
                    expansions++;
                }
            }
        }
        debug_printf("Expansions performed: %zu\n", expansions);
        if (expansions == 0)
            // No merges or expansions can be performed, we are done
            return;
    }
    // Tail recurse until no more optimization can be performed
    quine_mccluskey(implicants, implicant_count);
}

//////// Self-tests

// 8-bit binary literals
#define B8(b7, b6, b5, b4, b3, b2, b1, b0) \
    ((uint8_t)(b7) << 7 | (uint8_t)(b6) << 6 | (uint8_t)(b5) << 5 | \
     (uint8_t)(b4) << 4 | (uint8_t)(b3) << 3 | (uint8_t)(b2) << 2 | \
     (uint8_t)(b1) << 1 | (uint8_t)(b0))

static void test_implicant_covers(void) {
    assert(!implicant_covers(0x0000 | 0, 0x0000 | 1));
    assert(!covers(0x0000 | 0, 1));
    assert(!implicant_covers(0x0000 | B8(1, 0, 0, 0, 0, 0, 0, 0), 0x0000 | 0));
    assert(!covers(0x0000 | B8(1, 0, 0, 0, 0, 0, 0, 0), 0));
    assert(implicant_covers(0x0000 | B8(1, 0, 0, 0, 0, 0, 0, 0), 0x0000 | B8(1, 0, 0, 0, 0, 0, 0, 0)));
    assert(covers(0x0000 | B8(1, 0, 0, 0, 0, 0, 0, 0), B8(1, 0, 0, 0, 0, 0, 0, 0)));
    assert(implicant_covers(0x8000 | B8(1, 0, 0, 0, 0, 0, 0, 0), 0x8000 | B8(1, 0, 0, 0, 0, 0, 0, 0)));
    assert(implicant_covers(0x0300 | B8(1, 0, 0, 0, 0, 0, 1, 1), 0x0100 | B8(1, 0, 0, 0, 0, 0, 0, 0)));
    assert(!implicant_covers(0x0300 | B8(1, 0, 0, 0, 0, 0, 1, 1), 0x0100 | B8(1, 1, 0, 0, 0, 0, 0, 0)));
    assert(covers(0x0300 | B8(1, 0, 0, 0, 0, 0, 1, 1), B8(1, 0, 0, 0, 0, 0, 0, 0)));
    assert(covers(0x0300 | B8(1, 0, 0, 0, 0, 0, 1, 1), B8(1, 0, 0, 0, 0, 0, 0, 1)));
    assert(covers(0x0300 | B8(1, 0, 0, 0, 0, 0, 1, 1), B8(1, 0, 0, 0, 0, 0, 1, 0)));
    assert(covers(0x0300 | B8(1, 0, 0, 0, 0, 0, 1, 1), B8(1, 0, 0, 0, 0, 0, 1, 1)));
    assert(!covers(0x0300 | B8(1, 0, 0, 0, 0, 0, 1, 1), B8(1, 0, 0, 0, 0, 1, 0, 0)));
}

static void test_explode_implicant(void) {
    uint8_t *minterms;
    size_t minterm_count;

    minterm_count = explode_implicant(0x0000, &minterms);
    assert(minterm_count == 1);
    assert(minterms[0] == 0);
    free(minterms);

    minterm_count = explode_implicant(0x8000, &minterms);
    assert(minterm_count == 2);
    assert(minterms[0] == 0x80);
    assert(minterms[1] == 0x00);
    free(minterms);

    minterm_count = explode_implicant(0x8aaa, &minterms);
    assert(minterm_count == 8);
    assert(minterms[7] == 0xaa);
    assert(minterms[6] == 0x2a);
    assert(minterms[5] == 0xa2);
    assert(minterms[4] == 0x22);
    assert(minterms[3] == 0xa8);
    assert(minterms[2] == 0x28);
    assert(minterms[1] == 0xa0);
    assert(minterms[0] == 0x20);
    free(minterms);
}

static void test_fully_covered(void) {
    uint16_t implicants1[] = {
        0x0300 | B8(0, 0, 0, 0, 0, 0, 1, 1), // A'B'
        0x0300 | B8(0, 0, 0, 0, 0, 1, 1, 1), // A'B
    };
    // They cover A'D, although neither of them covers A'D alone
    assert(fully_covered(implicants1, 2, 0x0600 | B8(0, 0, 0, 0, 0, 1, 1, 1)));
    assert(!fully_covered(implicants1, 1, 0x0600 | B8(0, 0, 0, 0, 0, 1, 1, 1)));
    assert(!fully_covered(&implicants1[1], 1, 0x0600 | B8(0, 0, 0, 0, 0, 1, 1, 1)));
    assert(!fully_covered(NULL, 0, 0x0600 | B8(0, 0, 0, 0, 0, 1, 1, 1)));
    // Same for A'C
    assert(fully_covered(implicants1, 2, 0x0500 | B8(0, 0, 0, 0, 0, 1, 1, 1)));
    assert(!fully_covered(implicants1, 1, 0x0500 | B8(0, 0, 0, 0, 0, 1, 1, 1)));
    assert(!fully_covered(&implicants1[1], 1, 0x0500 | B8(0, 0, 0, 0, 0, 1, 1, 1)));
    assert(!fully_covered(NULL, 0, 0x0500 | B8(0, 0, 0, 0, 0, 1, 1, 1)));
    // A'BCD is covered by [1] but not by [0]
    assert(fully_covered(implicants1, 2, 0x0000 | B8(0, 0, 0, 0, 0, 1, 1, 1)));
    assert(!fully_covered(implicants1, 1, 0x0000 | B8(0, 0, 0, 0, 0, 1, 1, 1)));
    assert(fully_covered(&implicants1[1], 1, 0x0000 | B8(0, 0, 0, 0, 0, 1, 1, 1)));
    assert(!fully_covered(NULL, 0, 0x0000 | B8(0, 0, 0, 0, 0, 1, 1, 1)));
    // But they do not cover AB anyhow
    assert(!fully_covered(implicants1, 2, 0x0300 | B8(0, 0, 0, 0, 1, 1, 0, 0)));
    assert(!fully_covered(NULL, 0, 0x0300 | B8(0, 0, 0, 0, 1, 1, 0, 0)));
    // Nor do they cover ACD
    assert(!fully_covered(implicants1, 2, 0x0400 | B8(0, 0, 0, 0, 1, 1, 1, 1)));
    assert(!fully_covered(NULL, 0, 0x0400 | B8(0, 0, 0, 0, 1, 1, 1, 1)));
}

static bool is_in_array(uint16_t *arr, size_t count, uint16_t target) {
    for (size_t i = 0; i < count; ++i) {
        if (arr[i] == target)
            return true;
    }
    return false;
}

static void test_quine_mccluskey(void) {
    // Some four-variable examples that I can check by hand with a Karnaugh map
    uint16_t implicants1[] = {
        0x0000 | B8(0, 0, 0, 0, 0, 0, 0, 1),
        0x0000 | B8(0, 0, 0, 0, 0, 0, 1, 1),
        0x0000 | B8(0, 0, 0, 0, 1, 1, 1, 1),
    };
    size_t implicant_count1 = sizeof(implicants1) / sizeof(uint16_t);
    quine_mccluskey(implicants1, &implicant_count1);
    assert(implicant_count1 == 2);
    assert(is_in_array(implicants1, implicant_count1,
        (B8(0, 0, 0, 0, 0, 0, 1, 0) << 8) | B8(0, 0, 0, 0, 0, 0, 1, 1)));
    assert(is_in_array(implicants1, implicant_count1,
        0x0000 | B8(0, 0, 0, 0, 1, 1, 1, 1)));
    uint16_t implicants2[] = {0, 2, 3, 5, 6, 7, 9, 12, 13, 15};
    size_t implicant_count2 = sizeof(implicants2) / sizeof(uint16_t);
    quine_mccluskey(implicants2, &implicant_count2);
    assert(implicant_count2 == 5);
    assert(is_in_array(implicants2, implicant_count2,
        (B8(0, 0, 0, 0, 0, 1, 0, 1) << 8) | B8(0, 0, 0, 0, 0, 1, 1, 1)));
    assert(is_in_array(implicants2, implicant_count2,
        (B8(0, 0, 0, 0, 1, 0, 1, 0) << 8) | B8(0, 0, 0, 0, 1, 1, 1, 1)));
    assert(is_in_array(implicants2, implicant_count2,
        (B8(0, 0, 0, 0, 0, 0, 1, 0) << 8) | B8(0, 0, 0, 0, 0, 0, 1, 0)));
    assert(is_in_array(implicants2, implicant_count2,
        (B8(0, 0, 0, 0, 0, 1, 0, 0) << 8) | B8(0, 0, 0, 0, 1, 1, 0, 1)));
    assert(is_in_array(implicants2, implicant_count2,
        (B8(0, 0, 0, 0, 0, 0, 0, 1) << 8) | B8(0, 0, 0, 0, 1, 1, 0, 1)));
}

//////// For this module only

/// @brief Convert a font table from an array of eight 8-bit integers to a single 64-bit integer.
/// @param font8x8_in The input font table ([char_count][8]). Each char is a row
/// @param font8x8_uint64 The output font table ([char_count]). MSB is the top row, LSB is the bottom row.
/// @param char_count The number of characters in the font table.
/// @requires \length(font8x8_in) >= char_count;
/// @requires \length(font8x8_uint64) >= char_count;
static void convert_chartable_uint64(
    const char font8x8_in[][8],
    uint64_t *font8x8_uint64,
    const size_t char_count
) {
    for (size_t c = 0; c < char_count; ++c) {
        uint64_t payload = (uint64_t)font8x8_in[c][0];
        for (size_t row = 1; row < 8; ++row) {
            // loop invariants: check for overflow
            assert((payload & 0xff00000000000000) == 0);
            payload <<= 8;
            // Avoid sign extension by casting to unsigned char first
            payload |= (uint64_t)(unsigned char)font8x8_in[c][row];
        }
        font8x8_uint64[c] = payload;
        debug_printf("Char 0x%02zx: 0x%016llx\n", c, payload);
    }

    // Post-conditions
    for (size_t c = 0; c < char_count; ++c) {
        for (size_t row = 0; row < 8; ++row) {
            uint8_t byte = (font8x8_uint64[c] >> (64 - 8 - row * 8)) & 0xff;
            assert(byte == (uint8_t)font8x8_in[c][row]);
        }
    }
}

// Convert the u64 font table to a list of minterms for each of the 64 bits.
// We assign the least significant bit to be bit-0
// There are at most 128 minterms for each output bit
// We use a uint16_t array here: right now at this step, every element is
// both a minterm and an implicant.
// In addition to the lower 8 bits storing the minterm index, we use the
// upper 8 bits as a "Don't Care" mask (1 means Don't Care).
// When one bit is marked as Don't Care, we always set that bit to 1 to make
// `is_match` simpler.
// `bit_implicants[]` must be preallocated to have enough space. There will
// not be more than `char_count` minterms for each bit.
static void fontmap_to_minterms(
    uint64_t *font8x8_uint64,
    size_t char_count,
    uint16_t *bit_implicants[64], // an array of 64 (pointers to uint16_t)
    size_t bit_implicant_counts[64]
) {
    // We never care about bit 7, since ASCII basic only has 7 bits
    const uint16_t DC_MASK = 0x8080;
    for (int bit = 0; bit < 64; ++bit) {
        uint16_t *implicants = bit_implicants[bit];

        size_t implicant_count = 0;
        debug_printf("Bit %2d: Minterms: ", bit);
        for (uint16_t codepoint = 0; codepoint < char_count; ++codepoint) {
            uint64_t glyph = font8x8_uint64[codepoint];
            if ((glyph >> bit) & 1) {
                implicants[implicant_count] = codepoint | DC_MASK;
                implicant_count++;
                debug_printf("%d, ", codepoint);
            }
        }
        debug_printf("count=%zu\n", implicant_count);
        bit_implicant_counts[bit] = implicant_count;
    }
    // Post-conditions
    for (uint16_t codepoint = 0; codepoint < char_count; ++codepoint) {
        // Based on 15-122 checkin 3
        uint64_t to_check = font8x8_uint64[codepoint];
        size_t bit_idx = 0;
        // debug_printf("Checking ASCII 0x%02x\n", codepoint);
        while (to_check) {
            // loop invariants
            assert(bit_idx < 64);
            assert((to_check << bit_idx) <= font8x8_uint64[codepoint]);
            // debug_printf("\tBit %2zu: expected %d\n", bit_idx, (to_check &
            // 1));
            //  Check whether the bit is in the minterms
            uint16_t check_for = codepoint | DC_MASK;
            assert((int)(to_check & 1) ^
                   (!bsearch(&check_for, bit_implicants[bit_idx],
                             bit_implicant_counts[bit_idx], sizeof(uint16_t),
                             uint16_compare)));
            to_check >>= 1;
            bit_idx++;
        }
    }
}

// linebuf is used for combining multiple lines of output into one line
// of at most 68 characters, to minimize the number of total lines
#define MAX_LINEBUF 68
// See below under `C0_OUTPUT` for why we need this macro
#define PRINT_INTO_LINEBUF_OR_FLUSH(...) do { \
    char temp_buf[MAX_LINEBUF + 1]; \
    size_t n = snprintf(temp_buf, sizeof(temp_buf), __VA_ARGS__); \
    assert(n > 0 && n < sizeof(temp_buf)); \
    if (linebuf_pointer + n >= MAX_LINEBUF) { \
        linebuf[linebuf_pointer] = '\0'; \
        puts(linebuf); \
        linebuf_pointer = 4; \
        strncpy(linebuf, "    ", 4); \
    } \
    strncpy(linebuf + linebuf_pointer, temp_buf, MAX_LINEBUF - linebuf_pointer); \
    linebuf_pointer += n; \
} while (0)

int main(void) {
#define NUM_BITS 64
#define CHAR_COUNT 128
    // First run all selftests
    test_implicant_covers();
    test_explode_implicant();
    test_fully_covered();
    test_quine_mccluskey();
    // Here the index is the ASCII value.
    // We find a minimal expression using Karnaugh map/Quine–McCluskey
    // for all 64 bits
    // Mathematical formulation:
    // find a minimal boolean injective mapping from 7 bits
    // to the 64-bit letter images

    // STEP ZERO: convert the font table from char to uint64_t
    uint64_t font8x8_uint64[CHAR_COUNT];
    convert_chartable_uint64(font8x8_basic, font8x8_uint64, CHAR_COUNT);

    // STEP ONE: find the sum-of-minterm expression of each of the 64 bits
    uint16_t *bit_implicants[NUM_BITS];
    size_t bit_implicant_counts[NUM_BITS];
    for (int bit = 0; bit < NUM_BITS; ++bit) {
        // `bit_implicants[bit]` wants us to preallocate space
        bit_implicants[bit] = malloc(CHAR_COUNT * sizeof(uint16_t));
        assert(bit_implicants[bit]);
    }
    fontmap_to_minterms(font8x8_uint64, CHAR_COUNT, bit_implicants, bit_implicant_counts);

    // STEP TWO: implicant minimization
    for (int bit = 0; bit < NUM_BITS; ++bit) {
        debug_printf("Processing bit %d with %zu implicants\n", bit,
                     bit_implicant_counts[bit]);
        quine_mccluskey(bit_implicants[bit], &bit_implicant_counts[bit]);
        debug_printf("Bit %d: result ", bit);
        for (size_t i = 0; i < bit_implicant_counts[bit]; ++i)
            debug_printf("0x%04x, ", bit_implicants[bit][i]);
        debug_printf("count=%zu\n", bit_implicant_counts[bit]);
    }

    // STEP THREE: we merge every pair of implicants that can be merged,
    // so that the final result is an array of `uint32_t`.
    // If a bit has an odd number of PIs, it is fine because 0x0 only covers
    // ASCII NUL, which should not appear in any text anyway.
    uint32_t *bit_implicants32[NUM_BITS];
    size_t bit_implicant32_counts[NUM_BITS];
    for (int bit = 0; bit < NUM_BITS; ++bit) {
        size_t pi_count = bit_implicant_counts[bit];
        size_t merged_count = (pi_count + 1) / 2;
        bit_implicants32[bit] = malloc(merged_count * sizeof(uint32_t));
        assert(bit_implicants32[bit]);
        size_t out_idx = 0;
        for (size_t i = 0; i < pi_count / 2; ++i) {
            uint16_t left = bit_implicants[bit][2 * i];
            uint16_t right = bit_implicants[bit][2 * i + 1];
            uint32_t merged = ((uint32_t)left << 16) | (uint32_t)right;
            bit_implicants32[bit][out_idx++] = merged;
        }
        if (pi_count % 2 == 1)
            bit_implicants32[bit][out_idx++] = (uint32_t)bit_implicants[bit][pi_count - 1];
        bit_implicant32_counts[bit] = out_idx;
        free(bit_implicants[bit]);
        debug_printf("Bit %d: Merged PIs count=%zu\n", bit, out_idx);
    }

    // Integration test: we reconstruct each glyph and check against the original
    for (uint16_t ascii = 0; ascii < CHAR_COUNT; ++ascii) {
        uint64_t reconstructed = 0;
        for (int bit = 0; bit < NUM_BITS; ++bit) {
            uint32_t *implicants32 = bit_implicants32[bit];
            size_t implicant32_count = bit_implicant32_counts[bit];
            for (size_t i = 0; i < implicant32_count; ++i) {
                uint16_t upper = (implicants32[i] >> 16) & 0xffff;
                uint16_t lower = implicants32[i] & 0xffff;
                if (covers(lower, (uint8_t)ascii) || (upper && covers(upper, (uint8_t)ascii))) {
                    reconstructed |= ((uint64_t)1 << bit);
                    break;
                }
            }
        }
        debug_printf("Reconstructed ASCII 0x%02x: 0x%016llx\n", ascii, reconstructed);
        //debug_printf("result: %s\n", reconstructed == font8x8_uint64[ascii] ? "PASS" : "FAIL");
        assert(reconstructed == font8x8_uint64[ascii]);
    }

    // STEP THREE: output the result
#ifdef C_OUTPUT
    size_t max_implicant32_count = 0;
    for (int bit = 0; bit < NUM_BITS; ++bit)
        if (bit_implicant32_counts[bit] > max_implicant32_count)
            max_implicant32_count = bit_implicant32_counts[bit];
    printf("unsigned F[][%zu] = {\n", max_implicant32_count);
    for (int bit = 0; bit < NUM_BITS; ++bit) {
        uint32_t *arr = bit_implicants32[bit];
        size_t size = bit_implicant32_counts[bit];
        printf("    {");
        for (size_t i = 0; i < size; ++i) {
            printf("0x%x", arr[i]);
            if (i + 1 < size)
                putchar(',');
        }
        putchar('}');
        if (bit + 1 < NUM_BITS)
            putchar(',');
        putchar('\n');
    }
    printf("};\n unsigned C[] = {");
    for (int bit = 0; bit < NUM_BITS; ++bit) {
        printf("%zu", bit_implicant32_counts[bit]);
        if (bit + 1 < NUM_BITS)
            putchar(',');
    }
    puts("};");
#elif defined(C0_OUTPUT)
    // CMU 15-122 C0 style output
    char linebuf[MAX_LINEBUF + 1];
    // Initialize linebuf with indentation
    size_t linebuf_pointer = 4;
    strncpy(linebuf, "    ", 4);

    PRINT_INTO_LINEBUF_OR_FLUSH("int[][] F=alloc_array(int[],%d);", NUM_BITS);

    for (int bit = 0; bit < NUM_BITS; ++bit) {
        size_t implicant32_count = bit_implicant32_counts[bit];
        if (implicant32_count == 0)
            continue;
        PRINT_INTO_LINEBUF_OR_FLUSH("F[%d]=alloc_array(int,%zu);", bit, implicant32_count);
        for (size_t i = 0; i < implicant32_count; ++i)
            PRINT_INTO_LINEBUF_OR_FLUSH("F[%d][%zu]=0x%x;", bit, i, bit_implicants32[bit][i]);
    }
    PRINT_INTO_LINEBUF_OR_FLUSH("int[] C=alloc_array(int,%d);", NUM_BITS);
    for (int bit = 0; bit < NUM_BITS; ++bit) {
        PRINT_INTO_LINEBUF_OR_FLUSH("C[%d]=%zu;", bit, bit_implicant32_counts[bit]);
    }
    printf("\n");
#else
    // Half human-readable output for debugging:
    // useful for piping into sort/diff with another tool's output
    for (int bit = 0; bit < NUM_BITS; ++bit) {
        for (size_t i = 0; i < bit_implicant32_counts[bit]; ++i) {
            uint16_t upper = (bit_implicants32[bit][i] >> 16) & 0xffff;
            uint16_t lower = bit_implicants32[bit][i] & 0xffff;
            printf("%02d:", bit);
            human_print_implicant(lower);
            printf("\n");
            if (upper) {
                printf("%02d:", bit);
                human_print_implicant(upper);
                printf("\n");
            }
        }
    }
#endif

    for (int bit = 0; bit < NUM_BITS; ++bit)
        free(bit_implicants32[bit]);

    return 0;
}

// Public domain data from https://github.com/dhepper/font8x8
const char font8x8_basic[128][8] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0000 (nul)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0001
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0002
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0003
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0004
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0005
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0006
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0007
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0008
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0009
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000A
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000B
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000C
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000D
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000E
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000F
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0010
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0011
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0012
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0013
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0014
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0015
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0016
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0017
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0018
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0019
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001A
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001B
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001C
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001D
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001E
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001F
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0020 (space)
    { 0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00},   // U+0021 (!)
    { 0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0022 (")
    { 0x36, 0x36, 0x7F, 0x36, 0x7F, 0x36, 0x36, 0x00},   // U+0023 (#)
    { 0x0C, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x0C, 0x00},   // U+0024 ($)
    { 0x00, 0x63, 0x33, 0x18, 0x0C, 0x66, 0x63, 0x00},   // U+0025 (%)
    { 0x1C, 0x36, 0x1C, 0x6E, 0x3B, 0x33, 0x6E, 0x00},   // U+0026 (&)
    { 0x06, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0027 (')
    { 0x18, 0x0C, 0x06, 0x06, 0x06, 0x0C, 0x18, 0x00},   // U+0028 (()
    { 0x06, 0x0C, 0x18, 0x18, 0x18, 0x0C, 0x06, 0x00},   // U+0029 ())
    { 0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00},   // U+002A (*)
    { 0x00, 0x0C, 0x0C, 0x3F, 0x0C, 0x0C, 0x00, 0x00},   // U+002B (+)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x06},   // U+002C (,)
    { 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00},   // U+002D (-)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x00},   // U+002E (.)
    { 0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x01, 0x00},   // U+002F (/)
    { 0x3E, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x3E, 0x00},   // U+0030 (0)
    { 0x0C, 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00},   // U+0031 (1)
    { 0x1E, 0x33, 0x30, 0x1C, 0x06, 0x33, 0x3F, 0x00},   // U+0032 (2)
    { 0x1E, 0x33, 0x30, 0x1C, 0x30, 0x33, 0x1E, 0x00},   // U+0033 (3)
    { 0x38, 0x3C, 0x36, 0x33, 0x7F, 0x30, 0x78, 0x00},   // U+0034 (4)
    { 0x3F, 0x03, 0x1F, 0x30, 0x30, 0x33, 0x1E, 0x00},   // U+0035 (5)
    { 0x1C, 0x06, 0x03, 0x1F, 0x33, 0x33, 0x1E, 0x00},   // U+0036 (6)
    { 0x3F, 0x33, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x00},   // U+0037 (7)
    { 0x1E, 0x33, 0x33, 0x1E, 0x33, 0x33, 0x1E, 0x00},   // U+0038 (8)
    { 0x1E, 0x33, 0x33, 0x3E, 0x30, 0x18, 0x0E, 0x00},   // U+0039 (9)
    { 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x00},   // U+003A (:)
    { 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x06},   // U+003B (;)
    { 0x18, 0x0C, 0x06, 0x03, 0x06, 0x0C, 0x18, 0x00},   // U+003C (<)
    { 0x00, 0x00, 0x3F, 0x00, 0x00, 0x3F, 0x00, 0x00},   // U+003D (=)
    { 0x06, 0x0C, 0x18, 0x30, 0x18, 0x0C, 0x06, 0x00},   // U+003E (>)
    { 0x1E, 0x33, 0x30, 0x18, 0x0C, 0x00, 0x0C, 0x00},   // U+003F (?)
    { 0x3E, 0x63, 0x7B, 0x7B, 0x7B, 0x03, 0x1E, 0x00},   // U+0040 (@)
    { 0x0C, 0x1E, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x00},   // U+0041 (A)
    { 0x3F, 0x66, 0x66, 0x3E, 0x66, 0x66, 0x3F, 0x00},   // U+0042 (B)
    { 0x3C, 0x66, 0x03, 0x03, 0x03, 0x66, 0x3C, 0x00},   // U+0043 (C)
    { 0x1F, 0x36, 0x66, 0x66, 0x66, 0x36, 0x1F, 0x00},   // U+0044 (D)
    { 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x46, 0x7F, 0x00},   // U+0045 (E)
    { 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x06, 0x0F, 0x00},   // U+0046 (F)
    { 0x3C, 0x66, 0x03, 0x03, 0x73, 0x66, 0x7C, 0x00},   // U+0047 (G)
    { 0x33, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x33, 0x00},   // U+0048 (H)
    { 0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // U+0049 (I)
    { 0x78, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E, 0x00},   // U+004A (J)
    { 0x67, 0x66, 0x36, 0x1E, 0x36, 0x66, 0x67, 0x00},   // U+004B (K)
    { 0x0F, 0x06, 0x06, 0x06, 0x46, 0x66, 0x7F, 0x00},   // U+004C (L)
    { 0x63, 0x77, 0x7F, 0x7F, 0x6B, 0x63, 0x63, 0x00},   // U+004D (M)
    { 0x63, 0x67, 0x6F, 0x7B, 0x73, 0x63, 0x63, 0x00},   // U+004E (N)
    { 0x1C, 0x36, 0x63, 0x63, 0x63, 0x36, 0x1C, 0x00},   // U+004F (O)
    { 0x3F, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x0F, 0x00},   // U+0050 (P)
    { 0x1E, 0x33, 0x33, 0x33, 0x3B, 0x1E, 0x38, 0x00},   // U+0051 (Q)
    { 0x3F, 0x66, 0x66, 0x3E, 0x36, 0x66, 0x67, 0x00},   // U+0052 (R)
    { 0x1E, 0x33, 0x07, 0x0E, 0x38, 0x33, 0x1E, 0x00},   // U+0053 (S)
    { 0x3F, 0x2D, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // U+0054 (T)
    { 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3F, 0x00},   // U+0055 (U)
    { 0x33, 0x33, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00},   // U+0056 (V)
    { 0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00},   // U+0057 (W)
    { 0x63, 0x63, 0x36, 0x1C, 0x1C, 0x36, 0x63, 0x00},   // U+0058 (X)
    { 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x0C, 0x1E, 0x00},   // U+0059 (Y)
    { 0x7F, 0x63, 0x31, 0x18, 0x4C, 0x66, 0x7F, 0x00},   // U+005A (Z)
    { 0x1E, 0x06, 0x06, 0x06, 0x06, 0x06, 0x1E, 0x00},   // U+005B ([)
    { 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x40, 0x00},   // U+005C (\)
    { 0x1E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1E, 0x00},   // U+005D (])
    { 0x08, 0x1C, 0x36, 0x63, 0x00, 0x00, 0x00, 0x00},   // U+005E (^)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF},   // U+005F (_)
    { 0x0C, 0x0C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0060 (`)
    { 0x00, 0x00, 0x1E, 0x30, 0x3E, 0x33, 0x6E, 0x00},   // U+0061 (a)
    { 0x07, 0x06, 0x06, 0x3E, 0x66, 0x66, 0x3B, 0x00},   // U+0062 (b)
    { 0x00, 0x00, 0x1E, 0x33, 0x03, 0x33, 0x1E, 0x00},   // U+0063 (c)
    { 0x38, 0x30, 0x30, 0x3e, 0x33, 0x33, 0x6E, 0x00},   // U+0064 (d)
    { 0x00, 0x00, 0x1E, 0x33, 0x3f, 0x03, 0x1E, 0x00},   // U+0065 (e)
    { 0x1C, 0x36, 0x06, 0x0f, 0x06, 0x06, 0x0F, 0x00},   // U+0066 (f)
    { 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x1F},   // U+0067 (g)
    { 0x07, 0x06, 0x36, 0x6E, 0x66, 0x66, 0x67, 0x00},   // U+0068 (h)
    { 0x0C, 0x00, 0x0E, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // U+0069 (i)
    { 0x30, 0x00, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E},   // U+006A (j)
    { 0x07, 0x06, 0x66, 0x36, 0x1E, 0x36, 0x67, 0x00},   // U+006B (k)
    { 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // U+006C (l)
    { 0x00, 0x00, 0x33, 0x7F, 0x7F, 0x6B, 0x63, 0x00},   // U+006D (m)
    { 0x00, 0x00, 0x1F, 0x33, 0x33, 0x33, 0x33, 0x00},   // U+006E (n)
    { 0x00, 0x00, 0x1E, 0x33, 0x33, 0x33, 0x1E, 0x00},   // U+006F (o)
    { 0x00, 0x00, 0x3B, 0x66, 0x66, 0x3E, 0x06, 0x0F},   // U+0070 (p)
    { 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x78},   // U+0071 (q)
    { 0x00, 0x00, 0x3B, 0x6E, 0x66, 0x06, 0x0F, 0x00},   // U+0072 (r)
    { 0x00, 0x00, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x00},   // U+0073 (s)
    { 0x08, 0x0C, 0x3E, 0x0C, 0x0C, 0x2C, 0x18, 0x00},   // U+0074 (t)
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x33, 0x6E, 0x00},   // U+0075 (u)
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00},   // U+0076 (v)
    { 0x00, 0x00, 0x63, 0x6B, 0x7F, 0x7F, 0x36, 0x00},   // U+0077 (w)
    { 0x00, 0x00, 0x63, 0x36, 0x1C, 0x36, 0x63, 0x00},   // U+0078 (x)
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x3E, 0x30, 0x1F},   // U+0079 (y)
    { 0x00, 0x00, 0x3F, 0x19, 0x0C, 0x26, 0x3F, 0x00},   // U+007A (z)
    { 0x38, 0x0C, 0x0C, 0x07, 0x0C, 0x0C, 0x38, 0x00},   // U+007B ({)
    { 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00},   // U+007C (|)
    { 0x07, 0x0C, 0x0C, 0x38, 0x0C, 0x0C, 0x07, 0x00},   // U+007D (})
    { 0x6E, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+007E (~)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}    // U+007F
};
