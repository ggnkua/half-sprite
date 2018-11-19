//
//                             `.-://++oooooo+++/:-.`
//                        .-/osssssssssssssssssssssssso/:.
//                    `:+ssssssssssssssssssssssssssssssssss+:`
//                 `:osssssssssssssssssoooooossssssssssssssssso:`
//               -+ssssssssssss+/:-.`          `.-:/+ssssssssssss+-
//             -ossssssssss+:.                        .:+sssssssssso:
//           -osssssssss/-                                ./ssssssssso-
//         `+sssssssso:`                                     -ossssssss+`
//        -sssssssso-                                          -ossssssss-
//       :ssssssss:                                              :ssssssss/
//      /ssssssso`                       `````                    `+sssssss/
//     /sssssss+                   `-/ossssssssssssooooooo+`        /sssssss/
//    -sssssss+                  :ossssssssssssssssssssssss`         /sssssss:
//   `ossssss+                 -ossssssssssssssssssssssssss`          +sssssss`
//   /sssssss.                :ssssssso-`    `/ssssss+`````           `sssssss/
//  `ossssss/                -sssssss:         :sssssso`               :sssssss`
//  -sssssss.               `ossssss/           /ssssss+               `sssssss-
//  :sssssso                .sssssss`           `sssssss.               ossssss/
//  /sssssso                :sssssss             ossssss:               +ssssss+
//  /sssssso                -sssssss`            ossssss:               +ssssss+
//  :sssssso`               .sssssss-           `sssssss.               ossssss/
//  .sssssss.                +sssssss.          +sssssso               .sssssss-
//  `ossssss/                .ssssssss:`      -osssssss.               /sssssss`
//   :sssssss.                .osssssssso+++osssssssso.               `sssssss/
//   `osssssso`                `/sssssssssssssssssso:                 +sssssso`
//    -sssssss+                  `-/ossssssssssso/.                  /sssssss-
//     :sssssss+`                     `.-----.`                    `+sssssss/
//      :ssssssso.                                                .osssssss/
//       :ssssssss/`                                             :ssssssss:
//        .ossssssso:                                          -ossssssss-
//         `/sssssssso:`                                    `:ossssssss+`
//           .osssssssss+-`                              `-+ssssssssso-
//             -+ssssssssss+:.`                      `.:+sssssssssso-
//               .+sssssssssssso+:-.``        ``.-:+ossssssssssss+.
//                 `-+ssssssssssssssssssssssssssssssssssssssss+:`
//                    `-/osssssssssssssssssssssssssssssssso/-`
//                        `-:+osssssssssssssssssssssso+/-`
//                             `..:://+++oo+++//::-.`

// Half sprite
// Coded by GGN
//
// Inspiration by:
// - Leonard
// - Excellence in Art
// - dml
// - SPKR
// - Lord knows who/what else

// Started 19 Octomber 2018 20:30

#define PRINT_CODE
#define CYCLES_REPORT
//#define PRINT_DEBUG

#include <assert.h>

#if defined(PRINT_CODE) || defined (PRINT_DEBUG) || defined(WIN32) || defined(WIN64) || defined(__APPLE__) || defined(__linux__) || defined(__CYGWIN__) || defined(__MINGW32__)
#include <stdio.h>
#endif

#define BUFFER_SIZE 32000

// Print debug info to the console if requested
#ifdef PRINT_DEBUG
#define dprintf(...) printf(__VA_ARGS__)
#else
#define dprintf(...)
#endif

// Emit actual code to the console if requested
#ifdef PRINT_CODE
FILE *code_file;
#define cprintf(...) fprintf(code_file,__VA_ARGS__)
#else
#define cprintf(...)
#endif

// Print to both console and code 
#define bprintf(...) dprintf(__VA_ARGS__); cprintf(__VA_ARGS__);

// Set up a namespace for our typedefs (dml wisdom)

namespace brown
{
    typedef int S32;
    typedef short S16;
    typedef char S8;
    typedef unsigned int U32;
    typedef unsigned short U16;
    typedef unsigned char U8;
}

using namespace brown;

// Yes, let's define max because it's a complex math function that can't live inside standard libraries O_o
#define max(a,b) ( a > b ? a : b )

// Instructions opcodes
#define MOVEM_L_TO_REG      (0x4cd8 << 16)
#define MOVEM_L_FROM_REG    (0x48c0 << 16)
#define MOVEM_W_TO_REG      (0x4c98 << 16)
#define MOVEM_W_FROM_REG    (0x4880 << 16)
#define MOVEI_L             0x203c
#define MOVEI_W             0x303c
#define MOVEP_L             ((0x1c9 << 16) | 1)
#define ANDI_L              0x280
#define ORI_L               0x80
#define ANDI_W              0x240
#define ORI_W               0x40
#define MOVED_W             0x3000
#define ORD_W               0x8140
#define ANDD_W              0xc140
#define ADDQ_A1             0x5049
#define LEA_D_A1            0x43e9
#define SWAP                0x4840
#define LEA_PC_A0           0x41fa
#define RTS                 0x4e73

// Effective address encodings

#define EA_A1_POST      0x19
#define EA_D_A1         0x29
#define EA_A1           0x11
#define EA_MOVE_D_A1    (0x29<<6)
#define EA_MOVE_A1_POST (0x19<<6)
#define EA_MOVE_A1      (0x11<<6)

// Misc defines

#define EMIT_SWAP       0x10000

// TODO: some solution to determine endianness here
#define LITTLE_ENDIAN
#ifdef LITTLE_ENDIAN
#define WRITE_LONG(x) ((((x) & 0x000000FF) << 24) | (((x) & 0x0000FF00) << 8) | (((x) & 0x00FF0000) >> 8) | (((x) & 0xFF000000) >> 24))
#define WRITE_WORD(x) ((((x) & 0x00FF) << 8) | (((x) & 0xFF00) >> 8))
#else
#define WRITE_LONG(x) x
#define WRITE_WORD(x) x
#endif

typedef enum _ACTIONS : U16
{
    // Actions, i.e. what instructions should we emit per case.
    // Initially every individual .w write in screen is marked,
    // but gradually these can get grouped into .l and beyond.
    A_DONE          =0x01,
    A_MOVE          =0x02,
    A_OR            =0x04,
    A_AND_OR        =0x08,
    A_AND           =0x10,
    A_AND_OR_LONG   =0x11,
    A_OR_LONG       =0x12,
    A_MOVEM_LONG    =0x14,
    A_MOVEM         =0x18,
} ACTIONS;

typedef struct _MARK
{
    // The struct we use for the initial scan, this will mark down the
    // maximum words that need to be changed in order to mask and draw the sprite
    ACTIONS action;
    U32 offset;
    U16 value_or;
    U16 value_and;
} MARK;

typedef struct _FREQ
{
    // Just a table that will be used to sort immediate values by frequency
    U16 value;
    U16 count;
} FREQ;

// TODO not very satisfied with this - we're taking data from one structure (MARK) and move it to another. Perhaps this struct could be merged with MARK? (Of course then it will slow down the qsort as it'll need to shift more data around)
typedef struct _POTENTIAL_CODE
{
    // A second struct that marks down the potential instructions to be generated.
    // The instructions might change along the way so we'll try to keep it flexible
    U32 base_instruction;  // The instruction to be emitted
    U16 ea;                // And its effective address encoding
    U32 screen_offset;
    U32 value;
    U16 bytes_affected;    // How many bytes the instruction will affect (used for (a1)+ optimisations near the end)
} POTENTIAL_CODE;

// TODO: Hardcoded tables everywhere! That's not so great!

U8 buf_origsprite[BUFFER_SIZE];    // This is where we load our original sprite data
U8 buf_mask[BUFFER_SIZE / 4];      // Buffer used for creating the mask
U8 buf_shift[BUFFER_SIZE];         // Buffer used to create the shifted sprite (this is the buffer which we'll do most of the work)
MARK mark_buf[32000];              // Buffer that flags the actions to be taken in order to draw the sprite
FREQ freqtab[8000];                // Frequency table for the values we're going to process
POTENTIAL_CODE potential[16384];   // What we'll most probably output, bar a few optimisations
U16 output_buf[65536];             // Where the output code will be stored
U16 sprite_data[16384];            // The sprite data we're going to read when we want to draw the frames. Temporary buffer as this data will be stored immediately after the generated code
U16 *write_code = output_buf;

// Our own ghetto memcpy (remember, we don't want to use any library stuff due to this potentially running on a real ST etc)
// size must be multiple of 4. No checks done against this. Bite me.

static inline void copy(U32 *src, U32 *dst, S32 size)
{
    S32 count;
    for (count = (size >> 2) - 1; count >= 0; count--)
    {
        *dst++ = *src++;
    }
}

static inline void clear(U16 *data, S32 size)
{
    size = size / 2;
    do
    {
        *data++ = 0;
    } while (--size >= 0);
}

// Count bits, snippet from https://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
#ifdef PRINT_CODE
static inline U32 count_bits(U32 v)
{
    U32 c;
    v = v - ((v >> 1) & 0x55555555);                    // reuse input as temporary
    v = (v & 0x33333333) + ((v >> 2) & 0x33333333);     // temp
    c = (((v + (v >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24; // count
    return c;
}
#endif

// Code for qsort obtained from https://code.google.com/p/propgcc/source/browse/lib/stdlib/qsort.c

/*
 * from the libnix library for the Amiga, written by
 * Matthias Fleischer and Gunther Nikl and placed in the public
 * domain
 */

/* This qsort function does a little trick:
 * To reduce stackspace it iterates the larger interval instead of doing
 * the recursion on both intervals. 
 * So stackspace is limited to 32*stack_for_1_iteration = 
 * 32*4*(4 arguments+1 returnaddress+11 stored registers) = 2048 Bytes,
 * which is small enough for everybodys use.
 * (And this is the worst case if you own 4GB and sort an array of chars.)
 * Sparing the function calling overhead does improve performance, too.
 */

// Tinkered by GGN to sort FREQ struct members by .count

void qsort(FREQ *base, S16 nmemb)
{
    S16 a, b, c;
    while (nmemb > 1)
    {
        a = 0;
        b = nmemb - 1;
        c = (a + b) / 2; /* Middle element */
        for (;;)
        {
            while ((base[c].count - base[a].count) > 0)
                a++; /* Look for one >= middle */
            while ((base[c].count - base[b].count) < 0)
                b--; /* Look for one <= middle */
            if (a >= b)
                break; /* We found no pair */
            FREQ tmp = base[a]; /* swap them */
            base[a] = base[b];
            base[b] = tmp;
            if (c == a) /* Keep track of middle element */
                c = b;
            else if (c == b)
                c = a;
            a++; /* These two are already sorted */
            b--;
        } /* a points to first element of right intervall now (b to last of left) */
        b++;
        if (b < nmemb - b) /* do recursion on smaller intervall and iteration on larger one */
        {
            qsort(base, b);
            base = &base[b];
            nmemb = nmemb - b;
        }
        else
        {
            qsort(&base[b], nmemb - b);
            nmemb = b;
        }
    }
}

void halve_it()
{
    // Flags that affect how the routine will behave.
    // Setting these up can affect the compiled routine as a lot of code
    // can be excluded.
    // If you set the 2 below values to -1 then extra code for determining the sprite dimensions
    // will be added, plus extra CPU overhead. Just sayin'.
    // (also, this will have an impact on every place in the code
    // where sprite dimensions can be hardcoded by the compiler)
    short sprite_width = 32;            // Sprite width in pixels. -1=auto detect
    short sprite_height = 31;           // Sprite height in pixels. -1=auto detect
    short screen_width = 320;           // Actual screen width in pixels
    short screen_plane_words = screen_width / 16;
    // General constants
    short screen_height = 200;          // Actual screen height in pixels
    short screen_planes = 4;            // How many planes our screen is
    short sprite_planes = 4;            // How many planes our sprite is
    // How many planes we need to mask in order for the sprite to be drawn properly.
    // This is different from sprite_planes because for example we might have a 1bpp sprite
    // that has to be written into a 4bpp backdrop
    short num_mask_planes = 4;
    int use_masking = 1;                // Do we actually want to mask the sprites or is it going to be a special case? (for example, 1bpp sprites)
    int generate_mask = 1;              // Should we auto generate mask or will user supply his/her own?
    int outline_mask = 0;               // Should we outline mask?
    int horizontal_clip = 1;            // Should we output code that enables horizontal clip for x<0 and x>screen_width-sprite_width? (this can lead to HUGE amounts of outputted code depending on sprite width!)
    int generate_background_save_restore_code = 0;
    short i, j, k;
    short loop_count;

    // Bring a copy of the sprite to our screen buffer
    copy((U32 *)buf_origsprite, (U32 *)buf_shift, BUFFER_SIZE);

    // Detect sprite dimensions if requested
    // (quantised to 16 pixels in width)
    if (sprite_width == -1 || sprite_height == -1)
    {
        short x, y;
        U16 *scan_screen = (U16 *)buf_origsprite;
        loop_count = screen_height - 1;
        y = 0;
        do
        {
            S32 found_pixel = 0;
            // TODO: this loop can be faster if we shrink the starting offset.
            //       For example, if we found a pixel at x=100 then we could start the
            //       scan from the corresponding word for subsequent scanlines
            for (x = 0; x < screen_width; x = x + 16)
            {
                U16 temp_word = 0;
                for (i = 0; i < screen_planes; i++)
                {
                    temp_word = temp_word | *scan_screen;
                    scan_screen++;
                }
                if (temp_word)
                {
                    // If we found something then there is one pixel set
                    // in the current 16 pixel block
                    sprite_width = max(x, sprite_width);
                    found_pixel = 1;
                }
                scan_screen = scan_screen + (screen_planes - sprite_planes);
            }
            if (found_pixel)
            {
                // Update screen height
                sprite_height = max(sprite_height, y);
            }
            y++;
        } while (--loop_count != -1);
        // Increment width and height since our scanning is 0 based
        sprite_height++;
        sprite_width = sprite_width + 16;
    }
    // Add one horizontal word to width to allow for shifting
    sprite_width = sprite_width + 16;
    S16 sprite_words = sprite_width / 16;       // How many words is our sprite horizontally including the extra word for shifts (bitplane independant)

    S16 num_actions;
    S16 shift;
    POTENTIAL_CODE *out_potential;

    // Make space for a double pointer table at the beginning as an index to all routines
    // We'll need 16 .l pointers for the preshifts + sprite_width*2 when clipping horizontally
    // (we'll shift the sprite sprite_width times left and as many to the right, for clipping)

    U32 *write_pointers = (U32 *)write_code;
    S16 shift_count = 15;
    if (horizontal_clip)
    {
        write_code = write_code + (16 + (sprite_width - 16 - 1) * 2) * 2;
        cprintf("sprite_routines:\n");
        for (i = 0; i < 16; i++)
        {
            cprintf("dc.l sprite_code_shift_%i\n", i);
        }
        for (i = 0; i < (sprite_width - 16) - 1; i++)
        {
            cprintf("dc.l sprite_code_clip_left_%i\n", i);
        }
        for (i = 0; i < (sprite_width - 16) - 1; i++)
        {
            cprintf("dc.l sprite_code_clip_right_%i\n", i);
        }
        shift_count = 16 + ((sprite_width - 16) - 1) * 2;
    }
    else
    {
        // Just 16 preshifts then. 
        write_code = write_code + 16 * 2;
        cprintf("dc.l ");
        for (i = 0; i < 15; i++)
        {
            cprintf("sprite_code_shift_%i,", i);
        }
        cprintf("sprite_code_shift_15\n");
    }

#ifdef CYCLES_REPORT
        U32 cycles_unoptimised;
        U32 cycles_saved_from_movem_l;
        U32 cycles_saved_from_movem_w;
        U32 cycles_saved_from_move_l;
        U32 cycles_saved_from_movep;
        U32 cycles_saved_from_and_l_or_l;
        U32 cycles_saved_from_registers;
        U32 cycles_saved_from_post_increment;
#endif
    
    // Main - repeats 16 times for 16 preshifts (plus 2*(sprite_width-1) if we're clipping)
    for (shift = 0; shift < shift_count; shift++)
    {
        short x, y;
        U16 *buf = (U16 *)buf_shift;
        U16 *mask = (U16 *)buf_mask;
#ifdef CYCLES_REPORT
        // Reset cycle counters
        cycles_unoptimised = 0;
        cycles_saved_from_movem_l = 0;
        cycles_saved_from_movem_w = 0;
        cycles_saved_from_move_l = 0;
        cycles_saved_from_movep = 0;
        cycles_saved_from_and_l_or_l = 0;
        cycles_saved_from_registers = 0;
        cycles_saved_from_post_increment = 0;
#endif

        if (shift == 0 || (shift_count > 16 && (shift == 16 || shift == 16 + (sprite_width - 16) - 1)))
        {
            // Prepare mask at the first iteration
            // Also when generate clipping code at the respective starts, once for shifting left and once for shifting right
            if (use_masking)
            {
                if (generate_mask)
                {
                    U16 *src = (U16 *)buf_shift;
                    U16 *mask = (U16 *)buf_mask;
                    U16 x, y;
                    for (y = 0; y < sprite_height; y++)
                    {
                        for (x = 0; x < sprite_width; x = x + 16)
                        {
                            U16 gather_planes = 0;
                            for (j = 0; j < num_mask_planes; j++)
                            {
                                // Only gather as many planes as our sprite actually is
                                gather_planes = gather_planes | *src;
                                src++;
                            }
                            *mask++ = ~gather_planes;
                            src += screen_planes - num_mask_planes;
                        }
                        mask = mask + ((screen_width - sprite_width) / 16);
                        src = src + (screen_planes*(screen_width - sprite_width) / 16);
                    }
                    if (outline_mask)
                    {
                        // Add outlining of the mask
                        mask = (U16 *)buf_mask;
                        U16 *mask2 = mask + (screen_width / 16);
                        // OR the mask upwards
                        for (i = 0; i < screen_width*(screen_height - 1) / (8 / sprite_planes) / 2; i++)
                        {
                            *mask |= *mask2;
                            mask++;
                            mask2++;
                        }
                        // OR the mask downwards
                        mask = (U16 *)buf_mask + (screen_width*(screen_height) / 16);
                        mask2 = mask - (screen_width / 16);
                        for (i = 0; i < screen_width*(screen_height - 1) / 16; i++)
                        {
                            mask--;
                            mask2--;
                            *mask |= *mask2;
                        }
                        // OR the mask left and right
                        mask = (U16 *)buf_mask;
                        *mask = (*mask << 1) | (mask[1] >> 15);
                        mask++;
                        for (i = 1; i < screen_width*(screen_height - 1) / 16 - 1; i++)
                        {
                            U16 tmp_mask = *mask;
                            *mask = tmp_mask | (tmp_mask << 1) | (tmp_mask >> 1) | (mask[-1] << 15) | (mask[1] >> 15);
                            mask++;
                        }
                        *mask = *mask | (mask[-1] << 15);

                        // TODO UDLR/UDLR+diagonals also?

                    }
                }
                else
                {
                    // TODO: copy the mask from the user supplied one
                }
            }
        }

        U32 off = 0;
        num_actions = 0;
        MARK *cur_mark = mark_buf;
        mask = (U16 *)buf_mask;
        U16 val_and = *mask;
        U16 val_or;        
        out_potential = potential;                      // Reset potential moves pointer
        *write_pointers++ = WRITE_LONG((U8 *)write_code - (U8 *)output_buf);    // Mark down the entry point for this routine
        U16 *write_sprite_data = sprite_data;
        MARK *marks_end;                                // End of marks buffer
        U16 mark_to_search_for;
        S16 num_freqs;
        POTENTIAL_CODE *cacheable_code;
        U16 *cached_values;
        U16 *write_arranged_values;
        POTENTIAL_CODE *potential_end;
        U16 swaptable[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };                 // SWAP state of registers
        U32 prev_offset;
        U16 distance_between_actions;
        short consecutive_instructions;
        U16 *lea_patch_address;
        U16 lea_offset;
        U16 *read_sprite_data;
        U16 *first_lea_offset;

        // Step 1: determine what actions we need to perform
        //         in order to draw the sprite on screen
        U16 plane_counter = 0;
        for (y = 0; y < sprite_height; y++)
        {
            for (x = 0; x < (sprite_width / 16)*screen_planes; x = x + 1)
            {

                // If we don't have to mask any bit, then skip all bitplanes
                // TODO: this is lazy and wastes cycles as this piece of code and skipword will execute as many times as there are planes
                if (val_and == 0xffff)
                {
                    goto skipword;
                }

                val_or = *buf;

                if (val_or == 0)
                {
                    // Okay, we don't have to OR anything, but perhaps we need to apply the mask
                    if (generate_mask)
                    {
                        if (val_and && plane_counter < num_mask_planes)
                        {
                            // Even if we don't have to OR something, we still have to AND the mask
                            cur_mark->action = A_AND;
                            cur_mark->offset = off;
                            cur_mark->value_or = 0;
                            cur_mark->value_and = val_and;
                            cur_mark++;
                            num_actions++;
#ifdef CYCLES_REPORT
                            cycles_unoptimised = cycles_unoptimised + 20;   // and.w #xxx,d(An)
#endif
                            goto skipword;
                        }
                        else if (val_and==0 && plane_counter<num_mask_planes)
                        {
                            // OR value and AND value is 0, so we just need to MOVE a zero in this address
                            cur_mark->action = A_MOVE;
                            cur_mark->offset = off;
                            cur_mark->value_or = 0;
                            cur_mark->value_and = 0;
                            cur_mark++;
                            num_actions++;
#ifdef CYCLES_REPORT
                            cycles_unoptimised = cycles_unoptimised + 16;   // move.w #xxx,d(An)
#endif
                            goto skipword;
                        }
                    }
                    else if (val_and)
                    {
                        // Special case where we just need to mask something
                        // because the user supplied their own mask
                        // TODO this could happen if we outlined the mask I guess?
                        cur_mark->action = A_AND;
                        cur_mark->offset = off;
                        cur_mark->value_or = 0;
                        cur_mark->value_and = val_and;
                        cur_mark++;
                        num_actions++;
#ifdef CYCLES_REPORT
                        cycles_unoptimised = cycles_unoptimised + 20;   // and.w #xxx,d(An)
#endif
                        goto skipword;
#ifdef CYCLES_REPORT
                        cycles_unoptimised = cycles_unoptimised + 20;   // and.w #xxx,d(An)
#endif
                    }
                    // Nothing to draw here, skip word
                    goto skipword;
                }

                // We have something to do, let's determine what
                cur_mark->offset = off;
                cur_mark->value_or = val_or;

                if (use_masking)
                {
                    if (val_and == 0)
                    {
                        // All pixels in this word have to be punctured.
                        // (i.e. andi.w #0,(a1) / or.w #xxxx,(a1)
                        // A "move" instruction will suffice 
                        cur_mark->action = A_MOVE;
                        cur_mark->value_and = 0;
#ifdef CYCLES_REPORT
                        cycles_unoptimised = cycles_unoptimised + 16;   // move.w #xxx,d(An)
#endif
                    }
                    else if ((val_or | val_and) == 0xffff)
                    {
                        // Special case where mask and sprite data fill all the bits in a word.
                        // We only need to OR in this case. Kudos to Leonard of Oxygene for the idea.
                        cur_mark->action = A_OR;
                        cur_mark->value_and = 0;
#ifdef CYCLES_REPORT
                        cycles_unoptimised = cycles_unoptimised + 20;   // ori.w #xxx,d(An)
#endif
                    }
                    else
                    {
                        // Generic case - we need to AND the mask and OR the sprite value
                        cur_mark->action = A_AND_OR;
                        cur_mark->value_and = val_and;
#ifdef CYCLES_REPORT
                        cycles_unoptimised = cycles_unoptimised + 40;   // andi.w #xxx,d(An) / ori.w #yyy,d(An)
#endif
                    }
                }
                else
                {
                    // We won't mask anything, so just OR things
                    cur_mark->action = A_OR;
                    cur_mark->value_and = 0;
#ifdef CYCLES_REPORT
                    cycles_unoptimised = cycles_unoptimised + 20;   // or.w #xxx,d(An)
#endif
                }
                num_actions++;
                cur_mark++;

            skipword:
                off = off + 2;
                buf++;
                plane_counter++;
                if (plane_counter == screen_planes)
                {
                    mask++;
                    val_and = *mask;
                    plane_counter = 0;
                }
            }
            off = off + ((screen_width - sprite_width) / 8)*screen_planes;
            mask = mask + ((screen_width - sprite_width) / 16);
            buf = buf + (screen_planes*(screen_width - sprite_width) / 16);
            val_and = *mask;
            plane_counter = 0;

        }

        if (num_actions == 0)
        {
            // Nothing to do for this frame, so skip it
            *write_code++ = WRITE_WORD(RTS);
            if (shift < 16)
            {
                cprintf("sprite_code_shift_%i:\n", shift);
            }
            else if (shift < sprite_width - 1)
            {
                cprintf("sprite_code_clip_left_%i:\n", shift - 16);
            }
            else
            {
                cprintf("sprite_code_clip_right_%i:\n", shift - (sprite_width - 1));
            }
            cprintf("rts\n");
            goto skip_frame;
        }

        // Step 2: Try to optimise the actions recorded in step 1

        // Check if we have 12 or more consecutive word moves.
        // We can then use movem.l
        // We assume that a0 will contain source data, a1 will contain
        // the destination buffer and a7 the stack. So this gives us
        // 13 .l registers i.e. 26 .w moves.
        // move.w #x,d(An) = 16 (we need 2 of those to move a .l)
        // move.l #x,d(An) = 24
        // movem.l (An)+,<regs> = 12+8*num_regs
        // movem.l <regs>,d(An) = 12+8*num_regs
        // So a momem.l pair takes 24+16*num_regs
        // num_regs     1   2   3   4   5   6   7  8  9  10 11
        // move.l #    24  48  72  96 120 144 168
        // 2xmove.w #  32  64  96 128 160 192 224
        // 2xmovem.l   40  56  72  88 104 120 136
        // TODO: maybe we could keep a number A_MOVE generated in step 1 and skip this whole chunk if it's 0?
        
        dprintf("\nmovem.l optimisations\n---------------------\n");
        marks_end = cur_mark;           // Save end of marks buffer
        cur_mark = mark_buf;
        MARK *temp_mark;
        U16 temp_off_first;
        U16 temp_off_second;
        for (i = num_actions - 1; i >= 0; i--)
        {
            if (cur_mark->action == A_MOVE)
            {
                S32 num_moves = 1;
                temp_mark = cur_mark + 1;
                temp_off_first = cur_mark->offset;
                while (temp_mark->action == A_MOVE && temp_mark != marks_end && num_moves < 13 * 2)
                {
                    temp_off_second = temp_mark->offset;
                    if (temp_off_second - temp_off_first != 2)
                    {
                        // We do have moves, but they're not consecutive!
                        break;
                    }
                    temp_off_first = temp_off_second;
                    num_moves++;
                    temp_mark++;
                }
                if (num_moves >= 6)
                {
                    // 6 move.w = 3 move.l, and from 3 move.l onwards it's more optimal to go 2x movem.l
                    if (num_moves > 28)
                    {
                        // Woah, easy there tiger! We only have 14 available registers!
                        num_moves = 28;
                    }
                    num_moves = num_moves & -2;                         // Even out the number as we can't move half a longword!

#ifdef CYCLES_REPORT
                    cycles_saved_from_movem_l = cycles_saved_from_movem_l + 8 * (num_moves / 2 - 3); // movem.l (a0)+,regs / movem.l regs,d(A1)
#endif

                    out_potential[1].screen_offset = cur_mark->offset;  // Save screen offset for the second movem
                    if (num_moves <= 8 * 2)
                    {
                        dprintf("movem.l (a0)+,d0-d%i\nmovem.l d0-d%i,$%04x(a1)\ndc.w ", num_moves/2 - 1, num_moves/2 - 1, cur_mark->offset);
                    }
                    else
                    {
                        dprintf("movem.l (a0)+,d0-d7/a2-a%i\nmovem.l d0-d7/a2-a%i,$%04x(a1)\ndc.w ", num_moves/2 - 1 - 8 + 2, num_moves/2 - 1 - 8 + 2, cur_mark->offset);
                    }
                    for (j = num_moves - 1; j >= 0; j--)
                    {
                        *write_sprite_data++ = cur_mark->value_or;
                        dprintf("$%08x, \n", cur_mark->value_or);
                        cur_mark->action = A_DONE;
                        cur_mark++;
                    }
                    dprintf("\n");
                    cur_mark--;         // Rewind pointer back once because we will skip a mark since we increment the pointer below (right?)
                    i = i - (num_moves-1);  // Decrease amount of iterations too
                    // We need to emit 2 pairs of movem.ls: one to read the data from RAM and one to write it back to screen buffer.
                    // Assuming that registers d0-d7 and a2-a6 are free to clobber
                    num_moves = num_moves >> 1;
                    out_potential->base_instruction = MOVEM_L_TO_REG; // movem.l (a0)+,register_list
                    U16 register_mask;
                    if (num_moves <= 8)
                    {
                        // Just data registers
                        register_mask = (1 << num_moves) - 1;
                    }
                    else
                    {
                        // Data and address registers (address registers start at a2)
                        register_mask = 0xff | (((1 << (num_moves - 8 + 2)) - 1) << 10);
                    }
                    out_potential->base_instruction |= register_mask;
                    out_potential->screen_offset = 0;       // No screen offset, this is a source read
                    out_potential->bytes_affected = 0;      // Source read so we're not doing any destination change
                    out_potential++;
                    out_potential->base_instruction = MOVEM_L_FROM_REG;     // movem.l register_list,ea
                    out_potential->base_instruction |= register_mask;
                    out_potential->ea = EA_D_A1;               // let's assume it's d(a1) (could be potentially optimised to (a1)+)
                    out_potential->bytes_affected = 0;          // There's no such thing as movem.l reglist,(An)+, so skip this too
                    out_potential++;

                }
                else
                {
                    // Skip all the scanned marks since they were eliminated
                    cur_mark = temp_mark - 1;
                }
            }
            cur_mark++;
        }

        // Check if we have consecutive moves that did not fall into the movem.l
        // case and see if we can do something using movem.w
        // movem.w (An)+,<regs> takes 12+4*num_regs cycles
        // movem.w <regs>,d(An) takes 12+4*num_regs cycles
        // move.l Dn,d(Am) takes 16 cycles
        // move.w Dn,d(Am) takes 12 cycles
        // We also need to load the values into data registers
        // so in the movem case there is a second movem needed just to load the registers from RAM.
        // For move.w we can skip loading from RAM and do move.w #x,d(An) which is also 16 cycles.
        // Same for move.l - a move.l #x,d(An) takes 24 cycles.
        // So the actual cost for the above 3 cases is:
        // move.l - 24 cycles
        // move.w - 16 cycles
        // movem.w - 24+8*num_regs
        // num_regs  1  2  3  4  5  6  7  8  9  10 11
        // move.l   24 48 72 96
        // move.w   16 32 48 64
        // movem    32 40 48 56
        // It seems like for anything above 3 registers is good for movem.w
        
        dprintf("\nmovem.w optimisations\n---------------------\n");
        cur_mark = mark_buf;
        for (i = num_actions - 1; i >= 0; i--)
        {
            if (cur_mark->action == A_MOVE)
            {
                S32 num_moves = 1;
                temp_mark = cur_mark + 1;
                temp_off_first = cur_mark->offset;
                while (temp_mark->action == A_MOVE && temp_mark != marks_end && num_moves < 13)
                {
                    temp_off_second = temp_mark->offset;
                    if (temp_off_second - temp_off_first != 2)
                    {
                        // We do have moves, but they're not consecutive!
                        break;
                    }
                    temp_off_first = temp_off_second;
                    num_moves++;
                    temp_mark++;
                }
                if (num_moves >= 3)
                {
#ifdef CYCLES_REPORT
                    cycles_saved_from_movem_w = cycles_saved_from_movem_w + 8 * (num_moves - 3); // movem.l (a0)+,regs / movem.l regs,d(A1)
#endif
                    out_potential[1].screen_offset = cur_mark->offset;  // Save screen offset for the second movem
                    dprintf("movem.w (a0)+,d0-d%i\nmovem.w d0-d%i,$%04x(a1)\ndc.w ", num_moves - 1, num_moves - 1, cur_mark->offset);
                    for (j = num_moves - 1; j >= 0; j--)
                    {
                        dprintf("$%04x,", cur_mark->value_or);
                        *write_sprite_data++ = cur_mark->value_or;
                        cur_mark->action = A_DONE;
                        cur_mark++;
                    }
                    dprintf("\n");
                    cur_mark--; // Rewind pointer back once because we will skip a mark since we increment the pointer below (right?)
                    i = i - (num_moves - 1);  // Decrease amount of iterations too
                    // We need to emit 2 pairs of movem.ws: one to read the data from RAM and one to write it back to screen buffer.
                    // Assuming that registers d0-d7 and a2-a6 are free to clobber
                    out_potential->base_instruction = MOVEM_W_TO_REG; // movem.w (a0)+,register_list
                    U16 register_mask;
                    if (num_moves <= 8)
                    {
                        // Just data registers
                        register_mask = (1 << num_moves) - 1;
                    }
                    else
                    {
                        // Data and address registers
                        register_mask = 0xff | (((1 << (num_moves - 8)) - 1) << 10);
                    }
                    out_potential->base_instruction |= register_mask;
                    out_potential->screen_offset = 0;   // No screen offset, this is a source read
                    out_potential->bytes_affected = 0;      // Source read so we're not doing any destination change
                    out_potential++;
                    out_potential->base_instruction = MOVEM_W_FROM_REG;     // movem.w register_list,ea
                    out_potential->base_instruction |= register_mask;
                    out_potential->ea = EA_D_A1;           // let's assume it's d(a1) (could be potentially optimised to (a1)+)
                    out_potential->bytes_affected = num_moves * 2;          // There's no such thing as movem.w reglist,(An)+, so skip this too
                    out_potential++;

                }
            }
            cur_mark++;
        }

        // Check if any consecutive moves survived the above two passes that could be combined in move.ls
        // 2x move.w #x,d(An) = 2x16 cycles.
        //    move.l #x,d(An) = 24 cycles.

        dprintf("\nmove.l optimisations\n--------------------\n");
        cur_mark = mark_buf;
        for (i = num_actions - 1 - 1; i >= 0; i--)
        {
            if (cur_mark->action == A_MOVE)
            {
                if (cur_mark[1].action == A_MOVE && cur_mark[1].offset-cur_mark->offset==2)
                {
                    U32 longval;
                    longval = (cur_mark->value_or << 16) | cur_mark[1].value_or;
                    dprintf("move.w #$%04x,$%04x(a1) - move.w #$%04x,$%04x(a1) -> move.l #$%08x,$%04x(a1)\n", cur_mark->value_or, cur_mark->offset, cur_mark[1].value_or, cur_mark[1].offset, longval, cur_mark->offset);
                    // TODO: Is there a way we can include a longword into the frequency table without massive amounts of pain?
                    //       For example, the two words have to be consecutive into the table and on even boundary so they can be loaded
                    //       in a single register. But, what happens if only one of the two words makes it into top 16 values?
                    // TODO: We could use a2-a6 for this purpose and have a separate sorting phase for longwords
                    cur_mark->action = A_DONE;

                    out_potential->base_instruction = MOVEI_L;  // move.l #xxx,
                    out_potential->ea = EA_MOVE_D_A1;                  // d(a1)
                    out_potential->screen_offset = cur_mark->offset;
                    out_potential->value = longval;
                    out_potential->bytes_affected = 4;          // One longword please
                    out_potential++;

                    cur_mark++;
                    cur_mark->action = A_DONE;

                    i = i - 1;

#ifdef CYCLES_REPORT
                    cycles_saved_from_move_l = cycles_saved_from_move_l + 8; // move.l #xxx,d(An)
#endif
                }
            }
            cur_mark++;
        }

        // (Leonard's movep trick):
        // andi.l #$ff00ff00, (a0); 7
        // or  .l #$00xx00yy, (a0)+; 7
        // andi.l #$ff00ff00, (a0); 7
        // or  .l #$00zz00ww, (a0)+; 7 (total 28)
        // 
        // by:
        // 
        // move.l #$xxyyzzww, dn; 3
        // movep.l dn, 1(a0); 6
        // addq.w #8, a0; 2 (total 11)
        
        // andi/ori.l #xx,d(An) = 32 cycles

        // move.l #xx,d0    = 12 cycles
        // movep.l d0,d(An) = 24 cycles


        dprintf("\nmovep.l optimisations\n---------------------\n");
        cur_mark = mark_buf;
        loop_count = num_actions - 1 - 3;
        {
            if (cur_mark->action == A_AND_OR)
            {
                if (cur_mark[1].action == A_AND_OR &&
                    cur_mark[2].action == A_AND_OR &&
                    cur_mark[3].action == A_AND_OR)
                {
                    if ((cur_mark->value_and == 0xff00) &&
                        (cur_mark[1].value_and == 0xff00) &&
                        (cur_mark[2].value_and == 0xff00) &&
                        (cur_mark[3].value_and == 0xff00) &&
                        ((cur_mark->value_or & 0xff00) == 0) &&
                        ((cur_mark[1].value_or & 0xff00) == 0) &&
                        ((cur_mark[2].value_or & 0xff00) == 0) &&
                        ((cur_mark[3].value_or & 0xff00) == 0))
                    {
#ifdef CYCLES_REPORT
                        cycles_saved_from_movep = cycles_saved_from_movep + 4 * 32 - 12 - 24;
#endif
                        U32 longval;
                        longval = ((cur_mark->value_or << 16) & 0xff000000) | ((cur_mark[1].value_or << 16) & 0x00ff0000) | ((cur_mark[2].value_or >> 8) & 0x0000ff00) | (cur_mark[3].value_or & 0x000000ff);
                        longval = (cur_mark->value_or << 16) | cur_mark[1].value_or;
                        dprintf(";movep case - crack out the champage, woohoo!\nmove.l #%i,d0 - movep.l d0,1(a0)\n", longval);
                        cur_mark->action = A_DONE;

                        out_potential->base_instruction = MOVEP_L; // movep.l d0,1(a1)
                        out_potential->screen_offset = cur_mark->offset;
                        out_potential->value = longval;
                        out_potential->bytes_affected = 0xffff;          // movep is a special case, needs a lea beforehand
                        out_potential++;

                        cur_mark++;
                        cur_mark->action = A_DONE;
                        cur_mark++;
                        cur_mark->action = A_DONE;
                        cur_mark++;
                        cur_mark->action = A_DONE;

                        i = i - 3;
                    }
                }
            }
            cur_mark++;
        } while (--loop_count != -1);

        // Check if we have 2 consecutive AND/OR pairs
        // We can combine those into and.l/or.l pair
        // andi/ori.w #xx,d(an) = 20 cycles
        // andi/ori.l #xx,d(An) = 32 cycles

        dprintf("\nandi.l/ori.l pair optimisations\n-------------------------------\n");
        mark_to_search_for = A_AND_OR;
        if (!use_masking)
        {
            // We were asked not to gerenarte mask code so we're searching for 2 consecutive ORs
            mark_to_search_for = A_OR;
        }
        cur_mark = mark_buf;
        for (i = num_actions - 1 - 1; i >= 0; i--)
        {
            if (cur_mark->action & mark_to_search_for)
            {
                if (cur_mark[1].action == mark_to_search_for && cur_mark[1].offset-cur_mark->offset==2)
                {
                    cur_mark->action = A_DONE;

                    if (use_masking)
                    {
                        // We have to output an and.l
                        out_potential->base_instruction = ANDI_L;   // andi.l #xxx,
                        out_potential->ea = EA_D_A1;                // d(a1)
                        out_potential->screen_offset = cur_mark->offset;
                        out_potential->value = (cur_mark->value_and << 16) | (cur_mark[1].value_and & 0xffff);
                        out_potential->bytes_affected = 4;          // One longword s'il vous plait
                        dprintf("andi.w #$%04x,$%04x(a1) - andi.w #$%04x,$%04x(a1) -> andi.l #$%08x,$%04x(a1)\n", cur_mark->value_and, cur_mark->offset, cur_mark[1].value_and, cur_mark[1].offset, out_potential->value, cur_mark->offset);
                        out_potential++;
                    }

                    out_potential->base_instruction = ORI_L;        // ori.l #xxx,
                    out_potential->ea = EA_D_A1;                    // d(a1)
                    out_potential->screen_offset = cur_mark->offset;
                    out_potential->value = (cur_mark->value_or << 16) | (cur_mark[1].value_or & 0xffff);
                    out_potential->bytes_affected = 4;              // One longword s'il vous plait
                    dprintf("ori.w  #$%04x,$%04x(a1) - ori.w  #$%04x,$%04x(a1) -> ori.l  #$%08x,$%04x(a1)\n", cur_mark->value_or, cur_mark->offset, cur_mark[1].value_or, cur_mark[1].offset, out_potential->value, cur_mark->offset);
                    out_potential++;

                    cur_mark++;
                    cur_mark->action = A_DONE;

                    i = i - 1;

#ifdef CYCLES_REPORT
                    cycles_saved_from_and_l_or_l = cycles_saved_from_and_l_or_l + 2*(2 * 20 - 32); // 2xandi.w + 2xori.w -> andi.l + ori.l
#endif
                }
            }
            cur_mark++;
        }

        // Any stray and.w or or.w pairs get processed here
        dprintf("\nandi.l/ori.l optimisations\n--------------------------\n");
        cur_mark = mark_buf;
        for (i = num_actions - 1 - 1; i >= 0; i--)
        {
            if (use_masking)
            {
                if (cur_mark->action & A_AND)
                {
                    if (cur_mark[1].action & A_AND && cur_mark[1].offset - cur_mark->offset == 2)
                    {
                        out_potential->base_instruction = ANDI_L;   // andi.l #xxx,
                        out_potential->ea = EA_D_A1;                // d(a1)
                        out_potential->screen_offset = cur_mark->offset;
                        out_potential->value = (cur_mark->value_and << 16) | (cur_mark[1].value_and & 0xffff);
                        out_potential->bytes_affected = 4;          // One longword s'il vous plait
                        cur_mark->action = A_DONE;
                        dprintf("andi.w #$%04x,$%04x(a1) - andi.w #$%04x,$%04x(a1) -> andi.l #$%08x,$%04x(a1)\n", cur_mark->value_and, cur_mark->offset, cur_mark[1].value_and, cur_mark[1].offset, out_potential->value, cur_mark->offset);
                        out_potential++;
                        cur_mark++;
                        cur_mark->action = A_DONE;
                        i = i - 1;
#ifdef CYCLES_REPORT
                        cycles_saved_from_and_l_or_l = cycles_saved_from_and_l_or_l + 2 * (2 * 20 - 32); // 2xandi.w -> andi.l
#endif
                    }
                }
            }
            if (cur_mark->action & A_OR)
            {
                if (cur_mark[1].action & A_OR && cur_mark[1].offset - cur_mark->offset == 2)
                {
                    out_potential->base_instruction = ORI_L;        // ori.l #xxx,
                    out_potential->ea = EA_D_A1;                    // d(a1)
                    out_potential->screen_offset = cur_mark->offset;
                    out_potential->value = (cur_mark->value_or << 16) | (cur_mark[1].value_or & 0xffff);
                    out_potential->bytes_affected = 4;              // One longword s'il vous plait
                    cur_mark->action = A_DONE;
                    dprintf("ori.w  #$%04x,$%04x(a1) - ori.w  #$%04x,$%04x(a1) -> ori.l  #$%08x,$%04x(a1)\n", cur_mark->value_or, cur_mark->offset, cur_mark[1].value_or, cur_mark[1].offset, out_potential->value, cur_mark->offset);
                    out_potential++;
                    cur_mark++;
                    cur_mark->action = A_DONE;
                    i = i - 1;
#ifdef CYCLES_REPORT
                    cycles_saved_from_and_l_or_l = cycles_saved_from_and_l_or_l + 2 * (2 * 20 - 32); // 2xori.w -> ori.l
#endif
                }
            }
            cur_mark++;
        }

        // Any actions left from here on are candidates for register caching. Let's mark this.
        cacheable_code = out_potential;


        // Build a "most frequent values" table so we can load things into data registers when possible
        // We could be lazy and just use a huge table which would lead to tons of wasted RAM but let's not do that
        // (I figure this implementation is going to be a tad slow though!)
        cur_mark = mark_buf;
        num_freqs = 0;
        FREQ *freqptr;
        for (i = num_actions - 1; i >= 0; i--)
        {
            // Only proceed if we haven't processed the current action yet
            if (cur_mark->action != A_DONE)
            {
                U16 value;
                S16 num_iters = 0;      // In genreal we only need to iterate the loop below once
                switch (cur_mark->action)
                {
                case A_OR:
                    value = cur_mark->value_or;
                    break;
                case A_AND:
                    value = cur_mark->value_and;
                    break;
                case A_MOVE:
                    value = cur_mark->value_or;
                    break;
                case A_AND_OR:
                    num_iters = 1;      // Run loop twice, once for OR value and once for AND
                    value = cur_mark->value_and;
                    break;
                default:
                    // We shouldn't get here
                    value = -1;
                    dprintf("Creating frequency table: type %i wat", cur_mark->action);
                    break;
                }
                for (j = num_iters; j >= 0; j--)
                {
                    freqptr = freqtab;
                    for (k = num_freqs; k > 0; k--)
                    {
                        // Search through the values found so far and increase count of value if found
                        if (freqptr->value == value)
                        {
                            freqptr->count++;
                            goto value_found;
                        }
                        freqptr++;
                    }
                    // Not found above, so create a new entry
                    num_freqs++;
                    freqptr->value = value;
                    freqptr->count = 1;

                value_found:
                    // If we run a second time, do the OR value
                    value = cur_mark->value_or;
                }
            }
            cur_mark++;
        }

        // Now, sort the list by frequency
        
        qsort(freqtab, num_freqs);

        // Write the top 16 values to the sprite data buffer
        // There's a small chance that there are less than 16 values
        // TODO: this has to be an even number for the movem!
        // TODO: if the sum of frequent values is not worth the movem.l, don't output anything

        freqptr = &freqtab[num_freqs];
        if (num_freqs > 15)
        {
            num_freqs = 15;
        }
        cached_values = write_sprite_data;
        dprintf("\nMost frequent values\n--------------------\n");
        // Keep highest frequency values in low words of registers,
        // since it will probably result in fewer SWAP emissions
        write_arranged_values = write_sprite_data + 1;
        for (i = num_freqs; i >= 0; i--)
        {
            freqptr--;
            *write_arranged_values = freqptr->value;
            write_arranged_values = write_arranged_values + 2;
            if (i == 8)
            {
                write_arranged_values = write_sprite_data;
            }
        }
        write_sprite_data = write_sprite_data + 16;
        for (i = 0; i <= num_freqs; i++)
        {
            if (i < 8)
            {
                dprintf("$%04x <-- d%i hi word\n", cached_values[i * 2], i);
            }
            else
            {
                dprintf("$%04x <-- d%i lo word\n", cached_values[(i - 8) * 2 + 1], i & 7);
            }
        }

        // Anything that remains from the above passes we can safely assume that falls under the and.w/or.w/move.w #xxx,d(An) case
        // TODO: Hmmm, since a2-a6 aren't going to be used by and/or then we could use them for move.w
        dprintf("\nStray and.w/or.w/move.w\n-----------------------\n");
        cur_mark = mark_buf;
        loop_count = num_actions - 1;
        do
        {
            U16 cur_action = cur_mark->action;
            if (cur_action & (A_AND_OR | A_AND | A_OR))
            {
                if (use_masking && cur_action & (A_AND | A_AND_OR))
                {
                    // We have to output an and.w
                    out_potential->base_instruction = ANDI_W;       // andi.w #xxx,
                    out_potential->ea = EA_D_A1;                    // d(a1)
                    out_potential->screen_offset = cur_mark->offset;
                    out_potential->value = cur_mark->value_and;
                    out_potential->bytes_affected = 2;              // Two bytes
                    out_potential++;
                    dprintf("andi.w #$%04x,$%04x(a1)\n", cur_mark->value_and, cur_mark->offset);
                }

                if (cur_action & (A_AND_OR | A_OR))
                {
                    out_potential->base_instruction = ORI_W;            // ori.w #xxx,
                    out_potential->ea = EA_D_A1;                        // d(a1)
                    out_potential->screen_offset = cur_mark->offset;
                    out_potential->value = cur_mark->value_or;
                    out_potential->bytes_affected = 2;                  // Two bytes
                    out_potential++;
                    dprintf("ori.w  #$%04x,$%04x(a1)\n", cur_mark->value_or, cur_mark->offset);
                }
                cur_mark->action = A_DONE;
            }
            if (cur_action == A_MOVE)
            {
                dprintf("move.w #$%04x,$%04x(a1)\n", cur_mark->value_or, cur_mark->offset);
                // Let's not mark the actions as done yet, the values could be used in the frequency table
                cur_mark->action = A_DONE;
                out_potential->base_instruction = MOVEI_W;  // move.w #xxx,
                out_potential->ea = EA_MOVE_D_A1;                  // d(a1)
                out_potential->screen_offset = cur_mark->offset;
                out_potential->value = cur_mark->value_or;
                out_potential->bytes_affected = 2;          // Two bytes
                out_potential++;
            }
            cur_mark++;
        } while (--loop_count != -1);



        // It's almost time to output the shifted frame's code and data.
        // Take into account the frequncy table above and use data registers if we can.
        // Remember: - Only registers d0 to d7 can be used for and/or.w Dx,d(a1) or and/or.w Dx,(a1)+
        //           - Therefore we can load up to 16 .w values.
        //           - Not all of them can be available at the same time, we need to SWAP
        //
        // TODO: of course we could postpone this step and move this code outside this loop. This way we can use 
        //       a common pool of data for all frames. Probably makes the output code more complex. Hilarity might ensue.
        // TODO: could some of the lower frequency values be generated from the registers using simple instructions like
        //       NOT/NEG etc?
        // TODO: we can definitely the gains from this step nullified or even negated if we emit too many SWAPs!
        //       One solution would be to only allow one SWAP per register and then refuse to any transformations for that register
        //       Another could be to just sort all the transformed operations first by register usage, then we could be able to minimise the amount of SWAP emissions
        //       HOWEVER this opens up a larger can of worms: First of all we should sort the ANDs and ORs independantly (as we really don't want to OR before AND for example!)
        //       If we sort though we'd have to emit code in non sequential offsets. This can have an impact in the optimisation step below where we take advantage of post incrementing of the address registers
        //       This is a bit of a headache!
        //       Potentially we could solve this by iterating twice through the data: first without sorting for minimising SWAP emission and then with sorting, and then pick the path that leads to most saves
        //       Do we really want to fall inside that rabbit hole though?
        // andi/ori.w #xx,d(An) = 20 cycles
        // move.w #xxx,d(An)    = 16 cycles
        // and/or.w Dn,d(Am)    = 16 cycles
        // move.w Dn,d(Am)      = 12 cycles

        dprintf("\nData register optimisations\n---------------------------\n");
        potential_end = out_potential;
        for (out_potential = cacheable_code; out_potential < potential_end; out_potential++)
        {
            U16 cur_value = out_potential->value;
            for (i = 0; i < num_freqs; i++)
            {
                if (cur_value == cached_values[i])
                {
#if 1
                    if ((i & 1) == 1)
                    {
                        // Low word required. Does the data register have that?
                        if (swaptable[i >> 1] == 0)
                        {
                            // Yup, no worries
                        }
                        else
                        {
                            // Well we already SWAPped once, game over
                            goto do_not_optimise_register;
                        }
                    }
                    else
                    {
                        // High word required. Do we have that?
                        if (swaptable[i >> 1] == 1)
                        {
                            // Yup, no problems
                        }
                        else
                        {
                            dprintf("swap d%i\n", i >> 1);
                            swaptable[i >> 1] = 1;
                            out_potential->value |= EMIT_SWAP;
#ifdef CYCLES_REPORT
                            cycles_saved_from_registers = cycles_saved_from_registers - 4;  // Negative gain!
#endif
                        }
                    }
#else
                    if ((i & 1) == 0)
                    {
                        // We need the high word of the register
                        if (swaptable[i >> 1])
                        {
                            // The register is already swapped, do nothing
                        }
                        else
                        {
                            // We need to emit SWAP, so place a mark at the upper 16 bits (which are unused)
                            dprintf("swap d%i\n", i >> 1);
                            out_potential->value |= EMIT_SWAP;
                            swaptable[i >> 1] = 1;
#ifdef CYCLES_REPORT
                            cycles_saved_from_registers = cycles_saved_from_registers - 4;  // Negative gain!
#endif
                        }

                    }
                    else
                    {
                        // We need the low word of the register
                        if (swaptable[i >> 1])
                        {
                            // We need to emit SWAP, so place a mark at the upper 16 bits (which are unused)
                            dprintf("swap d%i\n", i >> 1);
                            out_potential->value |= EMIT_SWAP;
                            swaptable[i >> 1] = 0;
#ifdef CYCLES_REPORT
                            cycles_saved_from_registers = cycles_saved_from_registers - 4;  // Negative gain!
#endif
                        }
                        else
                        {
                            // The register is already swapped, do nothing
                        }
                    }
#endif
                    switch (out_potential->base_instruction)
                    {
                    case MOVEI_W:
                        out_potential->base_instruction = MOVED_W;          // Modify instruction
                        out_potential->base_instruction |= i >> 1;          // Register field (D0-D7)
                        dprintf("move.w #$%04x,$%04x(a1) -> move.w d%i,$%04x(a1)\n",out_potential->value,out_potential->screen_offset,i>>1,out_potential->screen_offset);
#ifdef CYCLES_REPORT
                        cycles_saved_from_registers = cycles_saved_from_registers + 4;
#endif
                        break;
                    case ANDI_W:
                        out_potential->base_instruction = ANDD_W;           // Modify instruction
                        out_potential->base_instruction |= (i >> 1) << 9;   // Register field (D0-D7)
                        dprintf("andi.w #$%04x,$%04x(a1) -> and.w d%i,$%04x(a1)\n", out_potential->value, out_potential->screen_offset, i >> 1, out_potential->screen_offset);
#ifdef CYCLES_REPORT
                        cycles_saved_from_registers = cycles_saved_from_registers + 4;
#endif
                        break;
                    case ORI_W:
                        out_potential->base_instruction = ORD_W;            // Modify instruction
                        out_potential->base_instruction |= (i >> 1) << 9;   // Register field (D0-D7)
                        dprintf("ori.w  #$%04x,$%04x(a1) -> or.w  d%i,$%04x(a1)\n", out_potential->value, out_potential->screen_offset, i >> 1, out_potential->screen_offset);
#ifdef CYCLES_REPORT
                        cycles_saved_from_registers = cycles_saved_from_registers + 4;
#endif
                        break;
                    }
                do_not_optimise_register:
                    // This compiles to nothing. It's just there to stop the compiler whining about our goto above
                    cur_value;
                }
            }
        }

        // Check for consecutive move/and/ors and change the generated code to
        // something like <instruction> <data>,(a1)+ or (a1) instead of <instruction> <data>,xx(a1)
        // andi.w #xxx,d(An) = 16 cycles
        // andi.w #xxx,(An)+ = 12 cycles
        // and.w Dn,d(Am)    = 16 cycles
        // and.w Dn,(Am)     = 12 cycles
        // and.w Dn,(Am)+    = 12 cycles
        // move.w Dn,(Am)+   = 8 cycles
        // move.w Dn,d(Am)   = 12 cycles
        // move.w #xxx,(An)+ = 12 cycles
        // move.w #xxx,d(An) = 16 cycles
        // addq.w/l #x,a1    = 8 cycles
        // lea d(a1),a1      = 8 cycles
        // adda.w/l #x,a1    = 12/16 cycles
        // NOTE: OR cycles are identical to AND

        // If we use a data register then we either do andi.w Dn,d(An) or and.w Dn,(an)+
        // For the andi case it's pretty straightforward, number of cycles=16*operations.
        // For the other case we have to consider that we have to do an initial lea d(a1),a1 in order to
        // bring a1 to the proper address, then start and.w Dn,(a1)+. So the number of cycles here are
        // 8+12*operations.
        //       1  2  3  4
        // andi 16 32 48 64
        // and  20 32 44 56
        // Which means we start breaking even after above 3 consecutive (a1)+.
        // Hey, it's the movem.l case once again :)

        dprintf("\nPost increment optimisations\n----------------------------\n");
        prev_offset = potential->screen_offset;
        distance_between_actions = potential->bytes_affected;
        consecutive_instructions = 0;
        
        // NOTE: we're going to scan one entry past the end of the table in order to catch that
        //       corner case where the last instruction is optimisable
        for (out_potential = potential + 1; out_potential < (potential_end + 1); out_potential++)
        {
            if (out_potential->bytes_affected != 0 || out_potential == potential_end)
            {
                // If the distance is 0 then there might be something like an and/or pair.
                // So that also counts as a consecutive instructions
                if ((out_potential->screen_offset - prev_offset == distance_between_actions ||
                    out_potential->screen_offset - prev_offset == 0) &&
                    out_potential != potential_end /* guard against final instruction */ )
                {
                    // We found one consecutive instruction after the other
                    consecutive_instructions = consecutive_instructions + 1;
                }
                else
                {
                    // No more consecutive instructions, so let's see how many fish we caught
                    if (consecutive_instructions >= 3)
                    {
                        // We have a good post-increment case so let's patch some instructions up
                        POTENTIAL_CODE *patch_instructions = out_potential - consecutive_instructions - 1;
                        patch_instructions->bytes_affected = 0xffff;        // Signal lea emission at the start of the block
#ifdef CYCLES_REPORT
                        cycles_saved_from_post_increment = cycles_saved_from_post_increment - 8;    // Initial lea cost, will be compensated from the loop below
#endif 
                        loop_count = consecutive_instructions - 1;
                        do
                        {
                            switch (patch_instructions->ea)
                            {
                            case EA_D_A1:
                                if (patch_instructions->screen_offset - patch_instructions[1].screen_offset == 0)
                                {
                                    patch_instructions->ea = EA_A1;
                                    dprintf("and/or.w <ea>,$%04x(a1) -> (a1)\n", patch_instructions->screen_offset);
#ifdef CYCLES_REPORT
                                    cycles_saved_from_post_increment = cycles_saved_from_post_increment + 4;
#endif 

                                }
                                else
                                {
                                    patch_instructions->ea = EA_A1_POST;
                                    dprintf("and/or.w <ea>,$%04x(a1) -> (a1)+\n", patch_instructions->screen_offset);
#ifdef CYCLES_REPORT
                                    cycles_saved_from_post_increment = cycles_saved_from_post_increment + 4;
#endif 
                                }
                                break;
                            case EA_MOVE_D_A1:
                                if (patch_instructions->screen_offset - patch_instructions[1].screen_offset == 0)
                                {
                                    patch_instructions->ea = EA_MOVE_A1;
                                    dprintf("move.w <ea>,$%04x(a1) -> (a1)\n", patch_instructions->screen_offset);
#ifdef CYCLES_REPORT
                                    cycles_saved_from_post_increment = cycles_saved_from_post_increment + 4;
#endif 
                                }
                                else
                                {
                                    patch_instructions->ea = EA_MOVE_A1_POST;
                                    dprintf("move.w <ea>,$%04x(a1) -> (a1)+\n", patch_instructions->screen_offset);
#ifdef CYCLES_REPORT
                                    cycles_saved_from_post_increment = cycles_saved_from_post_increment + 4;
#endif 
                                }
                                break;
                            }
                            patch_instructions++;
                        } while (--loop_count != -1);
                        // Final instruction will be a (a1), i.e. no post increment
                        switch (patch_instructions->ea)
                        {
                        case EA_D_A1:
                            patch_instructions->ea = EA_A1;
                            dprintf("and/or.w <ea>,$%04x(a1) -> (a1)\n", patch_instructions->screen_offset);
#ifdef CYCLES_REPORT
                            cycles_saved_from_post_increment = cycles_saved_from_post_increment + 4;
#endif 
                            break;
                        case EA_MOVE_D_A1:
                            patch_instructions->ea = EA_MOVE_A1;
                            dprintf("move.w <ea>,$%04x(a1) -> (a1)\n", patch_instructions->screen_offset);
#ifdef CYCLES_REPORT
                            cycles_saved_from_post_increment = cycles_saved_from_post_increment + 4;
#endif 
                            break;
                        }
                        patch_instructions->bytes_affected = 0xfffe;    // Signal updating of lea counter
                    }

                    // Reset our counter and move on
                    consecutive_instructions = 0;
                }
            }
            prev_offset = out_potential->screen_offset;
            distance_between_actions = out_potential->bytes_affected;
        }

        // Finally, emit the damn code!
        
        //U16 screen_offset = 0;
        lea_patch_address = 0;
        lea_offset = 0;
        dprintf("\n\nFinal code:\n-----------\n");
        cprintf(";----------------------------------------------------\n");
        cprintf("; Frame %i\n", shift);
#ifdef CYCLES_REPORT
        bprintf("; Stats:\n");
        bprintf("; Non-optimal number of cycles: %i\n", cycles_unoptimised);
        bprintf("; Savings from movem.l        : %i\n", cycles_saved_from_movem_l);
        bprintf("; Savings from movem.w        : %i\n", cycles_saved_from_movem_w);
        bprintf("; Savings from move.l         : %i\n", cycles_saved_from_move_l);
        bprintf("; Savings from movep          : %i\n", cycles_saved_from_movep);
        bprintf("; Savings from and.l/or.l     : %i\n", cycles_saved_from_and_l_or_l);
        bprintf("; Savings from registers      : %i\n", cycles_saved_from_registers);
        bprintf("; Savings from post increments: %i\n", cycles_saved_from_post_increment);
        bprintf("; Total savings               : %i\n", cycles_saved_from_movem_l + cycles_saved_from_movem_w + cycles_saved_from_move_l + cycles_saved_from_movep + cycles_saved_from_and_l_or_l + cycles_saved_from_registers + cycles_saved_from_post_increment);
        bprintf("; Final number of cycles      : %i\n", cycles_unoptimised - (cycles_saved_from_movem_l + cycles_saved_from_movem_w + cycles_saved_from_move_l + cycles_saved_from_movep + cycles_saved_from_and_l_or_l + cycles_saved_from_registers + cycles_saved_from_post_increment));
#endif
        if (shift < 16)
        {
            cprintf("sprite_code_shift_%i:\n", shift);
        }
        else if (shift < sprite_width-1)
        {
            cprintf("sprite_code_clip_left_%i:\n", shift - 16);
        }
        else
        {
            cprintf("sprite_code_clip_right_%i:\n", shift - (sprite_width - 1));
        }

        // We need to load a0 with the sprite data
        // Safe to assume that there will be data, hopefully!
        // Also, assuming that we won't dump more than 32k of code so the offset can be pc relative!
        // (geez, lots of assumptions)
        cprintf("lea sprite_data_shift_%i(pc),a0\n", shift);
        *write_code++ = WRITE_WORD(LEA_PC_A0);
        first_lea_offset = write_code;
        write_code++;

        for (out_potential = potential; out_potential < potential_end; out_potential++)
        {
            // Emit the movem to load data registers when we reach the point where we need it
            if (out_potential == cacheable_code)
            {
                *write_code++ = WRITE_WORD(MOVEM_L_TO_REG >> 16); // movem.l (a0)+,register_list
                *write_code++ = WRITE_WORD((1 << (num_freqs >> 1)) - 1);
                cprintf("movem.l (a0)+,d0-d%i\n", num_freqs >> 1);
            }

            // Emit a SWAP if required
            if (((out_potential->base_instruction & 0xffc0) == MOVED_W || (out_potential->base_instruction & 0xf1ff) == ORD_W || (out_potential->base_instruction & 0xf1ff) == ANDD_W) && out_potential->value & EMIT_SWAP)
            {
                if (out_potential->base_instruction == MOVED_W)
                {
                    // Get register to swap from move.w Dx,<ea>
                    *write_code++ = WRITE_WORD(SWAP | (out_potential->base_instruction & 7));
                    cprintf("swap D%i\n", out_potential->base_instruction & 7);
                }
                else
                {
                    // Get register to swap from and.w/or.w Dx,<ea>
                    *write_code++ = WRITE_WORD(SWAP | ((out_potential->base_instruction >> 9) & 7));
                    cprintf("swap D%i\n", ((out_potential->base_instruction >> 9) & 7));
                }
            }

            // Emit extra lea if needed - last instruction in a (a1)+/(a1) block should be (a1)
            if (out_potential->bytes_affected == 0xffff)
            {
                // We are in the first instruction of a (a1)+/(a1) instruction block, so we need to output
                // a lea d(a1),a1 instruction. We calculate d relative to previous value of a1, which at
                // the start of the code is at the top left corner of the sprite.
                *write_code++ = WRITE_WORD(LEA_D_A1);
                //lea_patch_address = write_code;
                // Punch a hole in the code to allow for the lea offset but don't fill it yet
                *write_code++ = WRITE_WORD(out_potential->screen_offset - lea_offset);
                lea_offset = out_potential->screen_offset - lea_offset;
                cprintf("lea %i(a1),a1\n", lea_offset);
            }

            // Write opcode
            // TODO: any better way to do this?
            if ((out_potential->base_instruction & 0xf0000000) == 0x40000000)
            {
                // movem - write 2 words
                *write_code++ = WRITE_WORD(out_potential->base_instruction >> 16);
                *write_code++ = WRITE_WORD((out_potential->base_instruction | out_potential->ea) & 0xffff);
            }
            else
            {
                *write_code++ = WRITE_WORD(out_potential->base_instruction | out_potential->ea);
            }
#ifdef PRINT_CODE
            switch (out_potential->base_instruction)
            {
            //case MOVEM_L_FROM_REG:
            //    fprintf(code_file, "movem.l ");
            //    break;
            //case MOVEM_W_FROM_REG:
            //    fprintf(code_file, "movem.w ");
            //    break;
            case MOVEI_W:
                fprintf(code_file, "move.w ");
                break;
            case ANDI_W:
                fprintf(code_file, "andi.w ");
                break;
            case ORI_W:
                fprintf(code_file, "ori.w ");
                break;
            case MOVEI_L:
                fprintf(code_file, "move.l ");
                break;
            case ANDI_L:
                fprintf(code_file, "andi.l ");
                break;
            case ORI_L:
                fprintf(code_file, "ori.l ");
                break;
            default:
                if ((out_potential->base_instruction & 0xffc0) == MOVED_W)
                {
                    fprintf(code_file, "move.w d%i,", out_potential->base_instruction & 7);
                }
                else if ((out_potential->base_instruction & 0xf1ff) == ANDD_W)
                {
                    fprintf(code_file, "and.w d%i,", (out_potential->base_instruction >> 9) & 7);
                }
                else if ((out_potential->base_instruction & 0xf1ff) == ORD_W)
                {
                    fprintf(code_file, "or.w d%i,", (out_potential->base_instruction >> 9) & 7);
                }
                else if ((out_potential->base_instruction & 0xffff0000) == MOVEM_L_TO_REG)
                {
                    U32 c = count_bits(out_potential->base_instruction & 0xffff);
                    if (c <= 8)
                    {
                        fprintf(code_file, "movem.l (a0)+,d0-d%i\n", c - 1);
                    }
                    else if (c == 9)
                    {
                        fprintf(code_file, "movem.l (a0)+,d0-d7/a2\n");
                    }
                    else
                    {
                        fprintf(code_file, "movem.l (a0)+,d0-d7/a2-a%i\n", c - 8 - 1);
                    }
                }
                else if ((out_potential->base_instruction & 0xffff0000) == MOVEM_L_FROM_REG)
                {
                    U32 c = count_bits(out_potential->base_instruction & 0xffff);
                    if (c <= 8)
                    {
                        fprintf(code_file, "movem.l d0-d7/a2-a%i,", c - 1);
                    }
                    else if (c == 9)
                    {
                        fprintf(code_file, "movem.l d0-d7/a2,");
                    }
                    else
                    {
                        fprintf(code_file, "movem.l d0-d7/a2-a%i,", c - 8 - 1);
                    }
                }
                else if ((out_potential->base_instruction & 0xffff0000) == MOVEM_W_TO_REG)
                {
                    U32 c = count_bits(out_potential->base_instruction & 0xffff);
                    fprintf(code_file, "movem.w (a0)+,d0-d%i\n", c - 1);
                }
                else if ((out_potential->base_instruction & 0xffff0000) == MOVEM_W_FROM_REG)
                {
                    U32 c = count_bits(out_potential->base_instruction & 0xffff);
                    fprintf(code_file, "movem.w d0-d%i,", c - 1);
                }
                else
                {
                    fprintf(code_file, "wat? $%04x", out_potential->base_instruction);
                }
                break;
            }
#endif

            // Write immediate values if needed
            switch (out_potential->base_instruction)
            {
            case MOVEI_W:
            case ANDI_W:
            case ORI_W:
                // Output .w immediate value
                *write_code++ = WRITE_WORD((U16)out_potential->value);
                cprintf("#$%04x,", out_potential->value);
                break;

            case MOVEI_L:
            case ANDI_L:
            case ORI_L:
                // Output .l immediate value
                *write_code++ = WRITE_WORD((U16)(out_potential->value >> 16));
                *write_code++ = WRITE_WORD((U16)(out_potential->value));
                cprintf("#$%04x,", out_potential->value);
                break;

            default:
                break;
            }

            switch (out_potential->ea)
            {
            case EA_D_A1:
            case EA_MOVE_D_A1:
                *write_code++ = WRITE_WORD((U16)out_potential->screen_offset - lea_offset);
                cprintf("$%04x(a1)\n", out_potential->screen_offset - lea_offset);
                break;

            case EA_A1_POST:
            case EA_MOVE_A1_POST:
                cprintf("(a1)+\n");
                break;

            default:
                break;
            }

            // Update lea counter at the end of the (a1)+/(a1) block
            if (out_potential->bytes_affected == 0xfffe)
            {
                lea_offset = out_potential->screen_offset;
            }


            if (out_potential->ea == EA_A1 || out_potential->ea == EA_MOVE_A1)
            {
                cprintf("(a1)\n");
            }

            // Patch lea offset if this is the first instruction in a (a1)+/(a1) block
            if ((out_potential->ea == EA_MOVE_A1_POST || out_potential->ea == EA_A1_POST) 
                && out_potential->screen_offset != 0 /* Guard against first instruction */
                )
            {
                if (lea_patch_address != 0)
                {
                    // Write previous lea offset if there is one
                    *lea_patch_address = out_potential->screen_offset - lea_offset;
                }
            }

        }

        // Final touch - RTS
        *write_code++ = WRITE_WORD(RTS);
        cprintf("rts\n");

        // Now that we know the offset which the sprite data will be written,
        // let's fill in that lea gap we left at the beginning.
        *first_lea_offset = WRITE_WORD((U8 *)write_code - (U8 *)first_lea_offset);

        // Glue the sprite_data buffer immediately after the code
        
        read_sprite_data = sprite_data;
#ifdef PRINT_CODE
        fprintf(code_file, "sprite_data_shift_%i:\ndc.w ", shift);
        j = 0;
        for (i = write_sprite_data - sprite_data - 1; i >= 0; i--)
        {
            if (j != 15)
            {
                if (i != 0)
                {
                    fprintf(code_file, "$%04x,", *read_sprite_data++);
                }
                else
                {
                    fprintf(code_file, "$%04x\n", *read_sprite_data++);
                }
                j++;
            }
            else
            {
                j = 0;
                if (i != 0)
                {
                    fprintf(code_file, "$%04x\ndc.w ", *read_sprite_data++);
                }
                else
                {
                    // Corner case, if last word to be output is at the end of a printed line, don't print spurious "dc.w"
                    fprintf(code_file, "$%04x\n", *read_sprite_data++);
                }
            }

        }
#endif
        loop_count = write_sprite_data - sprite_data - 1;
        do
        {
            *write_code++ = *read_sprite_data++;
        } while (--loop_count != -1);

        //
        // Prepare for next frame
        //

        clear((U16 *)potential, sizeof(potential));
    skip_frame:

        // Shift sprite by one place right
        // Where's 68000's roxl when you need it, right?
        if (shift < 15)
        {
            buf = (U16 *)buf_shift;
            for (y = 0; y < sprite_height; y++)
            {
                for (plane_counter = 0; plane_counter < screen_planes; plane_counter++)
                {
                    // Point to the end of the scanline as we'll shift from right to left
                    U16 *line_buf = buf + (sprite_words - 1)*screen_planes + plane_counter;
                    for (x = 0; x < sprite_words - 1; x++)
                    {
                        *line_buf = ((line_buf[-screen_planes] & 1) << 15) | (*line_buf >> 1);
                        line_buf = line_buf - screen_planes;
                    }
                    // First word of the scanline - just shift the damn thing and leave a 0 at the leftmost screen bit
                    *line_buf = *line_buf >> 1;
                }
                // Advance to next scanline
                buf = buf + (screen_width / (16 / screen_planes));
            }

            // Shift mask by one place right
            mask = (U16 *)buf_mask;
            for (y = 0; y < sprite_height; y++)
            {
                // Point to the end of the scanline as we'll shift from right to left
                // Our counters and offsets here are (sprite_words+1) due to the fact that sprite_words counts sprite words without the extra word for shifting
                U16 *line_mask = mask + (sprite_words - 1);
                for (x = 0; x < sprite_words - 1; x++)
                {
                    *line_mask = ((line_mask[-1] & 1) << 15) | (*line_mask >> 1);
                    line_mask--;
                }
                // First word of the scanline - we also have to generate a "1" at the leftmost pixel since this is a mask and we'll be shifting it one place to the right
                *line_mask = (1 << 15) | (*line_mask >> 1);
                // Advance to next scanline
                mask = mask + screen_plane_words;
            }
        }
        else if (horizontal_clip)
        {
            buf = (U16 *)buf_shift;
            // Only execute the following if we have any clipping to do
            // (so if horizontal_clip is a static value at compile time this will not generate any code at all)
            if (shift < 16 + (sprite_width - 16) - 1 - 1)
            {
                if (shift == 16 - 1)
                {
                    // Let's bring back a fresh copy of the sprite
                    // Mask will be generated and shifted at the start of next iteration
                    copy((U32 *)buf_origsprite, (U32 *)buf_shift, BUFFER_SIZE);
                }
                // Clipping to the left (for example sprite's top left x<0)
                for (y = 0; y < sprite_height; y++)
                {
                    for (plane_counter = 0; plane_counter < screen_planes; plane_counter++)
                    {
                        // Point to the start of the scanline as we'll shift from left to right
                        U16 *line_buf = buf + plane_counter;
                        for (x = 0; x < sprite_words; x++)
                        {
                            *line_buf = (*line_buf << 1) | ((line_buf[screen_planes] >> 15) & 1);
                            line_buf = line_buf + screen_planes;
                        }
                    }
                    // Advance to next scanline
                    buf = buf + (screen_width / (16 / screen_planes));
                }
                if (shift != 16 - 1)
                {
                    // Shift the mask too if it's not the first clipping iteration
                    // (first clipping iteration is handled at the start of the loop, because we need to regenerate the mask there)
                    mask = (U16 *)buf_mask;
                    for (y = 0; y < sprite_height; y++)
                    {
                        // Point to the start of the scanline as we'll shift from left to right
                        U16 *line_mask = mask;
                        for (x = 0; x < sprite_words - 1; x++)
                        {
                            *line_mask = (*line_mask << 1) | ((line_mask[1] >> 15) & 1);
                            line_mask++;
                        }
                        // Last word of the scanline - we also have to generate a "1" at the rightmost pixel since this is a mask and we'll be shifting it one place to the right
                        *line_mask = (*line_mask << 1) | 1;
                        // Advance to next scanline
                        mask = mask + screen_plane_words;
                    }
                }
            }
            else
            {
                if (shift == 16 + (sprite_width - 16) - 1 - 1)
                {
                    // Let's bring back a fresh copy of the sprite, but shifted 16 pixels right
                    // Mask will be generated and shifted at the start of next iteration
                    copy((U32 *)buf_origsprite, (U32 *)buf_shift, BUFFER_SIZE);
                }
                // Clipping to the right (for example sprite's top left x > screen_width minus sprite_width)
                for (y = 0; y < sprite_height; y++)
                {
                    for (plane_counter = 0; plane_counter < screen_planes; plane_counter++)
                    {
                        // Point to the end of the scanline as we'll shift from right to left
                        U16 *line_buf = buf + (sprite_words - 1 - 1)*screen_planes + plane_counter;
                        for (x = 0; x < sprite_words - 1 - 1; x++)
                        {
                            *line_buf = ((line_buf[-screen_planes] & 1) << 15) | (*line_buf >> 1);
                            line_buf = line_buf - screen_planes;
                        }
                        // First word of the scanline - just shift the damn thing and leave a 0 at the leftmost screen bit
                        *line_buf = *line_buf >> 1;
                    }
                    // Advance to next scanline
                    buf = buf + (screen_width / (16 / screen_planes));
                }
                if (shift != 16 + (sprite_width - 16) - 1 - 1)
                {
                    // Shift the mask too if it's not the first clipping iteration
                    // (first clipping iteration is handled at the start of the loop, because we need to regenerate the mask there)
                    mask = (U16 *)buf_mask;
                    for (y = 0; y < sprite_height; y++)
                    {
                        // Point to the end of the scanline as we'll shift from right to left
                        // Our counters and offsets here are (sprite_words+1) due to the fact that sprite_words counts sprite words without the extra word for shifting
                        U16 *line_mask = mask + (sprite_words - 1);
                        *line_mask-- = 0xffff;
                        for (x = 0; x < sprite_words - 1 - 1; x++)
                        {
                            *line_mask = ((line_mask[-1] & 1) << 15) | (*line_mask >> 1);
                            line_mask--;
                        }
                        // First word of the scanline - we also have to generate a "1" at the leftmost pixel since this is a mask and we'll be shifting it one place to the right
                        *line_mask = (1 << 15) | (*line_mask >> 1);
                        // Advance to next scanline
                        mask = mask + screen_plane_words;
                    }
                }

            }
        }
    }
}

typedef struct _endianess
{
    union
    {
        U8 little;
        U16 big;
    };
} endianness;

int main(int argc, char ** argv)
{
    endianness endian;
    endian.big = 1;

    // Let's say we load a .pi1 file
#if defined(WIN32) || defined(WIN64) || defined(__APPLE__) || defined(__linux__) || defined(__CYGWIN__) || defined(__MINGW32__)
    FILE *sprite = fopen("sprite.pi1","r");
    assert(sprite);
    fseek(sprite, 34, 0);
    fread(buf_origsprite, BUFFER_SIZE, 1, sprite);
    fclose(sprite);
#endif

#ifdef PRINT_CODE
    code_file = fopen("code.s", "w");
#endif

    // Byte swap the array if little endian
    if (endian.little)
    {
        int i;
        U16 *buf = (U16 *)buf_origsprite;
        for (i = 0;i < BUFFER_SIZE / 2;i++)
        {
            *buf = (*buf >> 8) | (*buf << 8);
            buf++;
        }
    }

    halve_it();

#ifdef PRINT_CODE
    fclose(code_file);
#endif

#if defined(WIN32) || defined(WIN64) || defined(__APPLE__) || defined(__linux__) || defined(__CYGWIN__) || defined(__MINGW32__)
    // Write the actual binary to a file
    // This should be incbinable into projects
    FILE *binary_output = fopen("code.dat", "wb");
    fwrite(output_buf, write_code - output_buf, 1, binary_output);
    fclose(binary_output);
#endif

    return 0;
}
