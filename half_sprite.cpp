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

#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#define BUFFER_SIZE 32000
#ifdef PRINT_DEBUG
#define dprintf(...) printf(__VA_ARGS__)
#else
#define dprintf(...)
#endif
#ifdef PRINT_CODE
#define cprintf(...) printf(__VA_ARGS__)
#else
#define cprintf(...)
#endif

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

// Yes, let's define max because it's a complex internal function O_o
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
#define MOVED_W             0x2000
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
MARK mark_buf[32000];                   // Buffer that flags the actions to be taken in order to draw the sprite
FREQ freqtab[8000];                     // Frequency table for the values we're going to process
POTENTIAL_CODE potential[16384];        // What we'll most probably output, bar a few optimisations
U16 output_buf[65536];             // Where the output code will be stored
U16 sprite_data[16384];            // The sprite data we're going to read when we want to draw the frames. Temporary buffer as this data will be stored immediately after the generated code

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

void qsort(FREQ *base, size_t nmemb)
{
    size_t a, b, c;
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
            //for (i = 0; i < size; i++) /* swap them */
            //{
            //    FREQ tmp = base[a + i];
            //    base[a + i] = base[b + i];
            //    base[b + i] = tmp;
            //}
            FREQ tmp = base[a];
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
    short sprite_width = -1;              // Sprite width in pixels. -1=auto detect
    short sprite_height = -1;             // Sprite height in pixels. -1=auto detect
    short screen_width = 320;             // Actual screen width in pixels
    // General constants
    short screen_height = 200;            // Actual screen height in pixels
    short screen_planes = 4;              // How many planes our screen is
    short sprite_planes = 4;              // How many planes our sprite is
    // How many planes we need to mask in order for the sprite to be drawn properly.
    // This is different from sprite_planes because for example we might have a 1bpp sprite
    // that has to be written into a 4bpp backdrop
    short num_mask_planes = 4;
    int outline_mask = 0;               // Should we outline mask?
    int generate_mask = 1;              // Should we auto generate mask or will user supply his/her own?
    int use_masking = 1;                // Do we actually want to mask the sprites or is it going to be a special case? (for example, 1bpp sprites)
    int horizontal_clip = 1;            // Should we output code that enables horizontal clip for x<0 and x>screen_width-sprite_width? (this can lead to HUGE amounts of outputted code depending on sprite width!)
    short i, j, k;
    short loop_count;
    U16 *write_code = output_buf;

    memcpy(buf_shift, buf_origsprite, BUFFER_SIZE);

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
            int found_pixel = 0;
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
    }
    // Increment width and height since our scanning is 0 based
    sprite_height++;
    sprite_width = sprite_width + 16;
    // Add one horizontal word to width to allow for shifting
    sprite_width = sprite_width + 16;

    // Prepare mask
    if (use_masking)
    {
        if (generate_mask)
        {
            U16 *src = (U16 *)buf_origsprite;
            U16 *mask = (U16 *)buf_mask;
            U16 x, y;
            for (y = 0; y < sprite_height; y++)
            {
                for (x = 0; x < sprite_width; x=x+16)
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
                for (i = 0; i < screen_width*(screen_height - 1) / (8/sprite_planes) / 2; i++)
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
                *mask = (*mask<<1) | (mask[1] >> 15);
                mask++;
                for (i = 1;i < screen_width*(screen_height - 1) / 16 - 1;i++)
                {
                    U16 tmp_mask = *mask;
                    *mask = tmp_mask | (tmp_mask << 1) | (tmp_mask >> 1) | (mask[-1] << 15) | (mask[1] >> 15);
                    mask++;
                }
                *mask = *mask | (mask[-1] << 15);

                // TODO UDLR/UDLR+diagonals also?

            }
        }
    }

    int num_actions;
    int shift;
    POTENTIAL_CODE *out_potential;

    // Make space for a double pointer table at the beginning as an index to all routines
    // We'll need 16 .l pointers for the preshifts + sprite_width*2 when clipping horizontally
    // (we'll shift the sprite sprite_width times left and as many to the right, for clipping)

    U32 *write_pointers = (U32 *)write_code;
    if (horizontal_clip)
    {
        write_code = write_code + (16 + sprite_width * 2) * 2;
        cprintf("sprite_routines:\n");
        cprintf("dc.l ");
        for (i = 0;i < 15;i++)
        {
            cprintf("sprite_code_shift_%i,", i);
        }
        cprintf("sprite_code_shift_15\n");
        cprintf("dc.l ");
        for (i = 0;i < sprite_width;i++)
        {
            cprintf("sprite_code_clip_right_%i,", i);
        }
        cprintf("sprite_code_clip_right_%i\n", sprite_width);
        cprintf("dc.l ");
        for (i = 0;i < sprite_width;i++)
        {
            cprintf("sprite_code_clip_left_%i,", i);
        }
        cprintf("sprite_code_clip_left_%i\n", sprite_width);
    }
    else
    {
        // Just 16 preshifts then. 
        write_code = write_code + 16 * 2;
        cprintf("dc.l ");
        for (i = 0;i < 15;i++)
        {
            cprintf("sprite_code_shift_%i,", i);
        }
        cprintf("sprite_code_shift_15\n");
    }

    // Main - repeats 16 times for 16 preshifts
    // TODO: If we output clipped sprites then we need to shift left and right as many steps as the sprite's width
    for (shift = 0; shift < 16; shift++)
    {
        int off = 0;
        num_actions = 0;
        MARK *cur_mark = mark_buf;
        U16 *buf = (U16 *)buf_shift;
        U16 *mask = (U16 *)buf_mask;
        U16 val_and = *mask;
        U16 val_or;
        
        out_potential = potential;                      // Reset potential moves pointer
        *write_pointers++ = write_code - output_buf;    // Mark down the entry point for this routine
        U16 *write_sprite_data = sprite_data;

        // Step 1: determine what actions we need to perform
        //         in order to draw the sprite on screen
        U16 plane_counter = 0;
        short x, y;
        for (y = 0; y < sprite_height; y++)
        {
            for (x = 0; x < (sprite_width / 16)*screen_planes; x = x + 1)
            {

                // If we don't have to mask any bit, then skip all bitplanes
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
                        }
                    }
                    if (val_and == 0)
                    {
                        // Special case where we just need to mask something
                        // because the user supplied their own mask
                        // TODO this could happen if we outlined the mask I guess?
                        cur_mark->action = A_MOVE;
                        cur_mark->offset = off;
                        cur_mark->value_or = 0;
                        cur_mark->value_and = val_and;
                        cur_mark++;
                        num_actions++;
                        goto skipword;
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
                    }
                    else if ((val_or | val_and) == 0xffff)
                    {
                        // Special case where mask and sprite data fill all the bits in a word.
                        // We only need to OR in this case. Kudos to Leonard of Oxygene for the idea.
                        cur_mark->action = A_OR;
                        cur_mark->value_and = 0;
                    }
                    else
                    {
                        // Generic case - we need to AND the mask and OR the sprite value
                        cur_mark->action = A_AND_OR;
                        cur_mark->value_and = val_and;
                    }
                }
                else
                {
                    // We won't mask anything, so just OR things
                    cur_mark->action = A_OR;
                    cur_mark->value_and = 0;
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

        // Step 2: Try to optimise the actions recorded in step 1

        // Check if we have 6 or more consecutive word moves.
        // We can then use movem.l
        // We assume that a0 will contain source data, a1 will contain
        // the destination buffer and a7 the stack. So this gives us
        // 13 .l registers i.e. 26 .w moves.
        // TODO: maybe we could keep a number A_MOVE generated in step 1 and skip this whole chunk if it's 0?
        
        dprintf("\nmovem.l optimisations\n---------------------\n");
        MARK *marks_end = cur_mark;           // Save end of marks buffer
        cur_mark = mark_buf;
        MARK *temp_mark;
        U16 temp_off_first;
        U16 temp_off_second;
        for (i = num_actions - 1; i >= 0; i--)
        {
            if (cur_mark->action == A_MOVE)
            {
                int num_moves = 1;
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
                    // 6 move.w = 3 move.l, and from 3 move.l onwards it's more optimal to go movem.l
                    num_moves = num_moves & -2;                         // Even out the number as we can't move half a longword!
                    out_potential[1].screen_offset = cur_mark->offset;  // Save screen offset for the second movem
                    dprintf("movem.l (a0)+,d0-d%i\nmovem.l d0-d%i,$%04x(a1)\ndc.w ", num_moves - 1, num_moves - 1, cur_mark->offset);
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
                        // Data and address registers
                        register_mask = 0xff | (((1 << (num_moves - 8)) - 1) << 10);
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
        // movem.w <regs>,d(An) takes 16+4*num_regs cycles
        // move.l Dn,d(Am) takes 16 cycles
        // move.w Dn,d(Am) takes 12 cycles
        // We also need to load the values into data registers
        // so in the movem case there is a second movem needed just to load the registers from RAM.
        // For move.w we can skip loading from RAM and do move.w #x,d(An) which is also 16 cycles.
        // Same for move.l - a move.l #x,d(An) takes 24 cycles.
        // So the actual cost for the above 3 cases is:
        // move.l - 24 cycles
        // move.w - 16 cycles
        // movem.w - 32+8*num_regs
        // num_regs  1  2  3  4  5  6  7  8  9  10 11
        // move.l   24 48 72 96
        // move.w   16 32 48 64
        // movem    40 48 56 56
        // It seems like for anything above 3 registers is good for movem.w (so, a bit like movem.l then)
        // TODO: All said, this check might not hit very often - an even number of A_MOVE that make sense will
        // be handled by movem.l above. Odd numbers will also be handled by movem.l and the residual
        // A_MOVE will be just a single move.w. So realistically this will only get used for
        // exactly 3-5 A_MOVEs. That is, unless I'm missing something.
        // So this block of code should be simplified to just chekcing for 3-5 A_MOVEs
        
        dprintf("\nmovem.w optimisations\n---------------------\n");
        cur_mark = mark_buf;
        for (i = num_actions - 1; i >= 0; i--)
        {
            if (cur_mark->action == A_MOVE)
            {
                int num_moves = 1;
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
                    // TODO: this is probably a redundant check if only 3 to 5 movem.ws are ever used
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
        
        dprintf("\nmove.l optimisations\n--------------------\n");
        cur_mark = mark_buf;
        int num_movel = 0;
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

                    num_movel = num_movel + 1;

                    i = i - 1;
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
        
        dprintf("\nandi.l/ori.l pair optimisations\n-------------------------------\n");
        U16 mark_to_search_for = A_AND_OR;
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
                        cur_mark->action = A_DONE;

                        out_potential->base_instruction = ANDI_L;   // andi.l #xxx,
                        out_potential->ea = EA_D_A1;                // d(a1)
                        out_potential->screen_offset = cur_mark->offset;
                        out_potential->value = (cur_mark->value_and << 16) | (cur_mark[1].value_and & 0xffff);
                        out_potential->bytes_affected = 4;          // One longword s'il vous plait
                        dprintf("andi.w #$%04x,$%04x(a1) - andi.w #$%04x,$%04x(a1) -> andi.l #$%08x,$%04x(a1)\n", cur_mark->value_and, cur_mark->offset, cur_mark[1].value_and, cur_mark[1].offset, out_potential->value, cur_mark->offset);
                        out_potential++;
                        cur_mark++;
                        cur_mark->action = A_DONE;
                        i = i - 1;
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
                    dprintf("ori.w  #$%04x,$%04x(a1) - ori.w  #$%04x,$%04x(a1) -> ori.l  #$%08x,$%04x(a1)\n", cur_mark->value_or, cur_mark->offset, cur_mark[1].value_or, cur_mark[1].offset, out_potential->value, cur_mark->offset);
                    out_potential++;
                    cur_mark++;
                    cur_mark->action = A_DONE;
                    i = i - 1;
                }
            }
            cur_mark++;
        }

        // Any actions left from here on are candidates for register caching. Let's mark this.
        POTENTIAL_CODE *cacheable_code = out_potential;

        // Any leftover moves are just going to be move.ws
        // TODO: Hmmm, since a2-a6 aren't going to be used by and/or then we could use them for move.w
        
        dprintf("\nmove.w\n------\n");
        cur_mark = mark_buf;
        for (i = num_actions - 1; i >= 0; i--)
        {
            if (cur_mark->action == A_MOVE)
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
        }

        // Build a "most frequent values" table so we can load things into data registers when possible
        // We could be lazy and just use a huge table which would lead to tons of wasted RAM but let's not do that
        // (I figure this implementation is going to be a tad slow though!)
        cur_mark = mark_buf;
        int num_freqs = 0;
        FREQ *freqptr;
        for (i = num_actions - 1; i >= 0; i--)
        {
            // Only proceed if we haven't processed the current action yet
            if (cur_mark->action != A_DONE)
            {
                U16 value = cur_mark->value_or;
                int num_iters = 1;      // Run loop twice, one for OR value and one for AND
                if (!generate_mask)
                {
                    // ...except if we don't generate mask code, in which case run loop only once
                    // (all this stuff should compile to static code so it's not as bloated as you might think with all these ifs and variable for boundaries :))
                    num_iters = 0;
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
                    value = cur_mark->value_and;
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
        U16 *cached_values = write_sprite_data;
        dprintf("\nMost frequent values\n--------------------\n");
        // Keep highest frequency values in low words of registers,
        // since it will probably result in fewer SWAP emissions
        U16 *write_arranged_values = write_sprite_data + 1;
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

        // Anything that remains from the above passes we can safely assume that falls under the and.w/or.w case
        
        dprintf("\nStray and.w/or.w\n----------------\n");
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

        dprintf("\nData register optimisations\n---------------------------\n");
        POTENTIAL_CODE *potential_end = out_potential;
        U16 swaptable[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };                 // SWAP state of registers
        for (out_potential = cacheable_code; out_potential < potential_end; out_potential++)
        {
            U16 cur_value = out_potential->value;
            for (i = 0; i < num_freqs; i++)
            {
                if (cur_value == cached_values[i])
                {
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
                        }
                        else
                        {
                            // The register is already swapped, do nothing
                        }
                    }
                    switch (out_potential->base_instruction)
                    {
                    case MOVEI_W:
                        out_potential->base_instruction = MOVED_W;          // Modify instruction
                        out_potential->base_instruction |= i >> 1;          // Register field (D0-D7)
                        dprintf("move.w #$%04x,$%04x(a1) -> move.w d%i,$%04x(a1)\n",out_potential->value,out_potential->screen_offset,i>>1,out_potential->screen_offset);
                        break;
                    case ANDI_W:
                        out_potential->base_instruction = ANDD_W;           // Modify instruction
                        out_potential->base_instruction |= (i >> 1) << 9;   // Register field (D0-D7)
                        dprintf("andi.w #$%04x,$%04x(a1) -> and.w d%i,$%04x(a1)\n", out_potential->value, out_potential->screen_offset, i >> 1, out_potential->screen_offset);
                        break;
                    case ORI_W:
                        out_potential->base_instruction = ORD_W;            // Modify instruction
                        out_potential->base_instruction |= (i >> 1) << 9;   // Register field (D0-D7)
                        dprintf("ori.w  #$%04x,$%04x(a1) -> or.w  d%i,$%04x(a1)\n", out_potential->value, out_potential->screen_offset, i >> 1, out_potential->screen_offset);
                        break;

                    }
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

        // We have a few strategies to consider here:
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
        U32 prev_offset = potential->screen_offset;
        U16 distance_between_actions = potential->bytes_affected;
        short consecutive_instructions = 0;
        
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
                                }
                                else
                                {
                                    patch_instructions->ea = EA_A1_POST;
                                    dprintf("and/or.w <ea>,$%04x(a1) -> (a1)+\n", patch_instructions->screen_offset);
                                }
                                break;
                            case EA_MOVE_D_A1:
                                if (patch_instructions->screen_offset - patch_instructions[1].screen_offset == 0)
                                {
                                    patch_instructions->ea = EA_MOVE_A1;
                                    dprintf("move.w <ea>,$%04x(a1) -> (a1)\n", patch_instructions->screen_offset);
                                }
                                else
                                {
                                    patch_instructions->ea = EA_MOVE_A1_POST;
                                    dprintf("move.w <ea>,$%04x(a1) -> (a1)+\n", patch_instructions->screen_offset);
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
                            break;
                        case EA_MOVE_D_A1:
                            patch_instructions->ea = EA_MOVE_A1;
                            dprintf("move.w <ea>,$%04x(a1) -> (a1)\n", patch_instructions->screen_offset);
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
        U16 *lea_patch_address = 0;
        U16 lea_offset = 0;
        dprintf("\n\nFinal code:\n-----------\n");
        cprintf("sprite_code_shift_%i:\n", shift);

        // We need to load a0 with the sprite data
        // Safe to assume that there will be data, hopefully!
        // Also, assuming that we won't dump more than 32k of code so the offset can be pc relative!
        // (geez, lots of assumptions)
        cprintf("lea sprite_data_shift_%i(pc),a0\n", shift);
        *write_code++ = LEA_PC_A0;
        U16 *first_lea_offset = write_code;
        write_code++;

        for (out_potential = potential; out_potential < potential_end; out_potential++)
        {
            // Emit the movem to load data registers when we reach the point where we need it
            if (out_potential == cacheable_code)
            {
                *write_code++ = MOVEM_L_TO_REG >> 16; // movem.l (a0)+,register_list
                *write_code++ = (1 << (num_freqs >> 1)) - 1;
                cprintf("movem.l (a0)+,d0-d%i\n", num_freqs >> 1);
            }

            // Emit a SWAP if required
            if (((out_potential->base_instruction & 0xffc0) == MOVED_W || (out_potential->base_instruction & 0xf1ff) == ORD_W || (out_potential->base_instruction & 0xf1ff) == ANDD_W) && out_potential->value & EMIT_SWAP)
            {
                if (out_potential->base_instruction == MOVED_W)
                {
                    *write_code++ = SWAP | (out_potential->base_instruction & 7);
                    cprintf("swap D%i\n", out_potential->base_instruction & 7);
                }
                else
                {
                    *write_code++ = SWAP | ((out_potential->base_instruction >> 9) & 7);
                    cprintf("swap D%i\n", ((out_potential->base_instruction >> 9) & 7));
                }
            }

            // Emit extra lea if needed - last instruction in a (a1)+/(a1) block should be (a1)
            if (out_potential->bytes_affected == 0xffff)
            {
                // We are in the first instruction of a (a1)+/(a1) instruction block, so we need to output
                // a lea d(a1),a1 instruction. We calculate d relative to previous value of a1, which at
                // the start of the code is at the top left corner of the sprite.
                *write_code++ = LEA_D_A1;
                //lea_patch_address = write_code;
                // Punch a hole in the code to allow for the lea offset but don't fill it yet
                *write_code++ = out_potential->screen_offset - lea_offset;
                lea_offset = out_potential->screen_offset - lea_offset;
                cprintf("lea %i(a1),a1\n", lea_offset);
            }

            // Write opcode
            *write_code++ = out_potential->base_instruction | out_potential->ea;
#ifdef PRINT_CODE
            switch (out_potential->base_instruction)
            {
            case MOVEM_L_FROM_REG:
                printf("movem.l ");
                break;
            case MOVEM_W_FROM_REG:
                printf("movem.w ");
                break;
            case MOVEI_W:
                printf("move.w ");
                break;
            case ANDI_W:
                printf("andi.w ");
                break;
            case ORI_W:
                printf("ori.w ");
                break;
            case MOVEI_L:
                printf("move.l ");
                break;
            case ANDI_L:
                printf("andi.l ");
                break;
            case ORI_L:
                printf("ori.l ");
                break;
            default:
                if ((out_potential->base_instruction & 0xffc0) == MOVED_W)
                {
                    printf("move.w d%i,", out_potential->base_instruction & 7);
                }
                else if ((out_potential->base_instruction & 0xf1ff) == ANDD_W)
                {
                    printf("and.w d%i,", (out_potential->base_instruction >> 9) & 7);
                }
                else if ((out_potential->base_instruction & 0xf1ff) == ORD_W)
                {
                    printf("or.w d%i,", (out_potential->base_instruction >> 9) & 7);
                }
                else if ((out_potential->base_instruction & 0xffff0000) == MOVEM_L_TO_REG)
                {
                    U32 c;
                    U32 v = out_potential->base_instruction & 0xffff;
                    v = v - ((v >> 1) & 0x55555555);                    // reuse input as temporary
                    v = (v & 0x33333333) + ((v >> 2) & 0x33333333);     // temp
                    c = ((v + (v >> 4) & 0xF0F0F0F) * 0x1010101) >> 24; // count
                    printf("movem.l (a0)+,d0-d%i\n", c);
                }
                else if ((out_potential->base_instruction & 0xffff0000) == MOVEM_L_FROM_REG)
                {
                    U32 c;
                    U32 v = out_potential->base_instruction & 0xffff;
                    v = v - ((v >> 1) & 0x55555555);                    // reuse input as temporary
                    v = (v & 0x33333333) + ((v >> 2) & 0x33333333);     // temp
                    c = ((v + (v >> 4) & 0xF0F0F0F) * 0x1010101) >> 24; // count
                    printf("movem.w d0-d%i,", c);
                }
                else if ((out_potential->base_instruction & 0xffff0000) == MOVEM_W_TO_REG)
                {
                    U32 c;
                    U32 v = out_potential->base_instruction & 0xffff;
                    v = v - ((v >> 1) & 0x55555555);                    // reuse input as temporary
                    v = (v & 0x33333333) + ((v >> 2) & 0x33333333);     // temp
                    c = ((v + (v >> 4) & 0xF0F0F0F) * 0x1010101) >> 24; // count
                    printf("movem.w (a0)+,d0-d%i\n",c-1);
                }
                else if ((out_potential->base_instruction & 0xffff0000) == MOVEM_W_FROM_REG)
                {
                    U32 c;
                    U32 v = out_potential->base_instruction & 0xffff;
                    v = v - ((v >> 1) & 0x55555555);                    // reuse input as temporary
                    v = (v & 0x33333333) + ((v >> 2) & 0x33333333);     // temp
                    c = ((v + (v >> 4) & 0xF0F0F0F) * 0x1010101) >> 24; // count
                    printf("movem.w d0-d%i,", c-1);
                }
                else
                {
                    printf("wat? $%04x", out_potential->base_instruction);
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
                *write_code++ = (U16)out_potential->value;
                cprintf("#$%04x,", out_potential->value);
                break;

            case MOVEI_L:
            case ANDI_L:
            case ORI_L:
                // Output .l immediate value
                *write_code++ = (U16)(out_potential->value >> 16);
                *write_code++ = (U16)(out_potential->value);
                cprintf("#$%04x,", out_potential->value);
                break;

            default:
                break;
            }

            // Write screen offsets if needed
            //if (((out_potential->base_instruction) & 0xffff0000) == MOVEM_L_FROM_REG || (out_potential->base_instruction & 0xffff0000) == MOVEM_W_FROM_REG)
            //{
            //    *write_code++ = (U16)out_potential->screen_offset - lea_offset;
            //    cprintf("$%04x(a1)\n", out_potential->screen_offset - lea_offset);
            //}

            switch (out_potential->ea)
            {
            case EA_D_A1:
            case EA_MOVE_D_A1:
                *write_code++ = (U16)out_potential->screen_offset - lea_offset;
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
        *write_code++ = RTS;
        cprintf("rts\n");

        // Now that we know the offset which the sprite data will be written,
        // let's fill in that lea gap we left at the beginning.
        *first_lea_offset = write_code - first_lea_offset;

        // Glue the sprite_data buffer immediately after the code
        
        U16 *read_sprite_data=sprite_data;
#ifdef PRINT_CODE
        printf("sprite_data_shift_%i:\ndc.w ", shift);
        j = 0;
        for (i = write_sprite_data - sprite_data - 1; i >= 0; i--)
        {
            if (j != 15)
            {
                if (i != 0)
                {
                    printf("$%04x,", *read_sprite_data++);
                }
                else
                {
                    printf("$%04x\n", *read_sprite_data++);
                }
                j++;
            }
            else
            {
                j = 0;
                if (i != 0)
                {
                    printf("$%04x\ndc.w ", *read_sprite_data++);
                }
                else
                {
                    // Corner case, if last word to be output is at the end of a printed line, don't print spurious "dc.w"
                    printf("$%04x\n", *read_sprite_data++);
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
        // TODO: If we want to clip sprites then a lot more shifting needs to happen
        //

        // Shift screen by one place right
        // Where's 68000's roxl when you need it, right?
        buf = (U16 *)buf_shift;
        short screen_plane_words = screen_width / 16;
        for (y = 0;y < screen_height;y++)
        {
            for (plane_counter = 0; plane_counter < screen_planes; plane_counter++)
            {
                U16 *line_buf = buf + (screen_plane_words - 1)*screen_planes + plane_counter;
                for (x = 0; x < screen_width / 16 - 1; x++)
                {
                    *line_buf = ((line_buf[-screen_plane_words] & 1) << 15) | (*line_buf >> 1);
                    line_buf = line_buf - screen_planes;
                }
                *line_buf = *line_buf >> 1;
            }
            buf = buf + (screen_width / (16 / screen_planes));
        }

        // Shift mask by one place right
        mask = (U16 *)buf_mask;
        for (y = 0;y < screen_height;y++)
        {
            U16 *line_mask = mask + (screen_plane_words - 1);
            for (x = 0; x < screen_width / 16 - 1; x++)
            {
                *line_mask = ((line_mask[-1] & 1) << 15) | (*line_mask >> 1);
                line_mask--;
            }
            *line_mask = *line_mask >> 1;
            mask = mask + screen_plane_words;
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
    FILE *sprite = fopen("sprite.pi1","r");
    assert(sprite);
    fseek(sprite, 34, 0);
    fread(buf_origsprite, BUFFER_SIZE, 1, sprite);
    fclose(sprite);

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
    return 0;
}