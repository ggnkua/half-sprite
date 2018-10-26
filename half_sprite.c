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
// - Lord knows what else

// Started 19 Octomber 2018 20:30

#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

// Yes, let's define max because it's a complex internal function O_o
#define max(a,b) ( a > b ? a : b )

// TODO: For now we're assuming ST Low (320x200x4bpp). We should account for different screen depths and resolutions
// TODO: Hardcoded tables everywhere! That's not so great!

#define BUFFER_SIZE 32000
#define PRINT_DEBUG

uint8_t buf_origsprite[BUFFER_SIZE];    // This is where we load our original sprite data
uint8_t buf_mask[BUFFER_SIZE/4];        // Buffer used for creating the mask
uint8_t buf_shift[BUFFER_SIZE];         // Buffer used to create the shifted sprite (this is the buffer which we'll do most of the work)
uint16_t output_buf[65536];             // Where the output code will be stored

typedef enum _ACTIONS
{
    // Actions, i.e. what instructions should we emit per case.
    // Initially every individual .w write in screen is marked,
    // but gradually these can get grouped into .l and beyond.
    A_DONE,
    A_MOVE,
    A_OR,
    A_AND_OR,
    A_AND,
    A_AND_OR_LONG,
    A_OR_LONG,
    A_MOVEM_LONG,
    A_MOVEM,
} ACTIONS;

typedef struct _MARK
{
    ACTIONS action;
    uint32_t offset;
    uint16_t value_or;
    uint16_t value_and;
} MARK;

typedef struct _FREQ
{
    uint16_t value;
    uint16_t count;
} FREQ;

MARK mark_buf[32000];                   // Buffer that flags the actions to be taken in order to draw the sprite
FREQ freqtab[8000];                     // Frequency table for the values we're going to process

// TODO not very satisfied with this - we're taking data from one structure (MARK) and move it to another. Perhaps this struct could be merged with MARK? (Of course then it will slow down the qsort as it'll need to shift more data around)
typedef struct _POTENTIAL_CODE
{
    uint32_t base_instruction;
    uint16_t ea;
    uint32_t screen_offset;
    uint32_t value;
    uint16_t bytes_affected;
} POTENTIAL_CODE;

POTENTIAL_CODE potential[16384];            // What we'll most probably output, bar a few optimisations
uint16_t sprite_data[16384];                // The sprite data we're going to read when we want to draw the frames

// Instructions opcodes
#define MOVEM_L_TO_REG (0x4cd8 << 16)
#define MOVEM_L_FROM_REG (0x48c0 << 16)
#define MOVEM_W_TO_REG (0x4c98 << 16)
#define MOVEM_W_FROM_REG (0x48c0 << 16)
#define MOVEI_L (0x203c)
#define MOVEI_W (0x303c)
#define MOVEP_L ((0x1c9 << 16) | 1)
#define ANDI_L (0x280)
#define ORI_L (0x80)
#define ANDI_W (0x240)
#define ORI_W (0x40)
#define MOVED_W (0x2000)
#define ORD_W (0x8140)
#define ANDD_W (0xc140)
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

static inline void qsort(FREQ *base, size_t nmemb)
{
    static short size = 4; //will sort longwords only
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
    int screen_width = 320;
    int screen_height = 200;
    int num_planes = 4;                 // How many planes our sprite is
    // How many planes we need to mask in order for the sprite to be drawn properly.
    // This is different from num_planes because for example we might have a 1bpp sprite
    // that has to be written into a 4bpp backdrop
    int num_mask_planes = 4;            // TODO this does nothing for now!
    int generate_mask = 1;              // Should we auto generate mask or will user supply his/her own?
    int outline_mask = 1;               // Should we outline mask?
    int use_masking = 1;                // Do we actually want to mask the sprites or is it going to be a special case? (for example, 1bpp sprites)
    int horizontal_clip = 1;            // Should we output code that enables horizontal clip for x<0 and x>screen_width-sprite_width? (this can lead to HUGE amounts of outputted code depending on sprite width!)
    int i, j, k;

    memcpy(buf_shift, buf_origsprite, BUFFER_SIZE);
    uint16_t *write_sprite_data = sprite_data;

    // Prepare mask
    if (use_masking)
    {
        if (generate_mask)
        {
            uint16_t *src = (uint16_t *)buf_origsprite;
            uint16_t *mask = (uint16_t *)buf_mask;
            for (i = 0; i < BUFFER_SIZE / num_planes / 2; i++)
            {
                *mask++ = *src | src[1] | src[2] | src[3];
                src += 4;
            }
            if (outline_mask)
            {
                // Add outlining of the mask
                mask = (uint16_t *)buf_mask;
                uint16_t *mask2 = mask + (screen_width / 16);
                // OR the mask upwards
                for (i = 0; i < screen_width*(screen_height - 1) / (8/num_planes) / 2; i++)
                {
                    *mask |= *mask2;
                    mask++;
                    mask2++;
                }
                // OR the mask downwards
                mask = (uint16_t *)buf_mask + (screen_width*(screen_height) / 16);
                mask2 = mask - (screen_width / 16);
                for (i = 0; i < screen_width*(screen_height - 1) / 16; i++)
                {
                    mask--;
                    mask2--;
                    *mask |= *mask2;
                }
                // OR the mask left and right
                // TODO: this code is currently probably wrong
                mask = (uint16_t *)buf_mask;
                *mask = (*mask<<1) | (mask[1] >> 15);
                mask++;
                for (i = 1;i < screen_width*(screen_height - 1) / 16 - 1;i++)
                {
                    *mask = *mask | (*mask << 1) | (*mask >> 1) | (mask[-1] << 15) | (mask[1] >> 15);
                    mask++;
                }
                *mask = *mask | (mask[-1] << 15);

                // TODO UDLR/UDLR+diagonals also?

            }
        }
    }

    int num_actions;
    int shift;
    POTENTIAL_CODE *out_potential = potential;

    // Main - repeats 16 times for 16 preshifts
    for (shift = 0; shift < 16; shift++)
    {
        int off = 0;
        num_actions = 0;
        MARK *cur_mark = mark_buf;
        uint16_t *buf = (uint16_t *)buf_shift;
        uint16_t *mask = (uint16_t *)buf_mask;

        // Step 1: determine what actions we need to perform
        //         in order to draw the sprite on screen

        for (i = BUFFER_SIZE / 2 - 1; i >= 0; i--)
        {
            uint16_t val_or = *buf;
            uint16_t val_and = *mask;   // TODO: can be optimised as one read per 4 src words

            if (val_or == 0)
            {
                if (generate_mask)
                {
                    // Nothing to draw here, skip word
                    goto skipword;
                }
                if (val_and == 0)
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
                    goto skipword;
                }
            }

            // We have something to do, let's determine what
            cur_mark->offset = off;
            cur_mark->value_or = val_or;

            if (use_masking)
            {
                if (val_and == 0xffff)
                {
                    // No sprite holes to be punctured.
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
            if ((i & 3) == 0)
            {
                mask++;
            }
        }

        // Step 2: Try to optimise the actions recorded in step 1

        // Check if we have 6 or more consecutive word moves.
        // We can then use movem.l
        // We assume that a0 will contain source data, a1 will contain
        // the destination buffer and a7 the stack. So this gives us
        // 13 .l registers i.e. 26 .w moves.
        MARK *marks_end = cur_mark;           // Save end of marks buffer
        cur_mark = mark_buf;
        MARK *temp_mark;
        uint16_t temp_off_first;
        uint16_t temp_off_second;
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
                    for (j = num_moves - 1; j >= 0; j--)
                    {
                        *write_sprite_data++ = cur_mark->value_or;
#ifdef PRINT_DEBUG
                        printf("movem.l \ndc.w %i\n", cur_mark->value_or);
#endif
                        cur_mark->action = A_DONE;
                        cur_mark++;
                    }
                    cur_mark--;         // Rewind pointer back once because we will skip a mark since we increment the pointer below (right?)
                    i = i - (num_moves-1);  // Decrease amount of iterations too
                    // We need to emit 2 pairs of movem.ls: one to read the data from RAM and one to write it back to screen buffer.
                    // Assuming that registers d0-d7 and a2-a6 are free to clobber
                    num_moves = num_moves >> 1;
                    out_potential->base_instruction = MOVEM_L_TO_REG; // movem.l (a0)+,register_list
                    uint16_t register_mask;
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
                    //out_potential->ea = 0x18;             // (a0)+
                    out_potential->screen_offset = 0;       // No screen offset, this is a source read
                    out_potential->bytes_affected = 0;      // Source read so we're not doing any destination change
                    out_potential++;
                    out_potential->base_instruction = MOVEM_L_FROM_REG;     // movem.l register_list,ea
                    out_potential->base_instruction |= register_mask;
                    out_potential->ea = 0x29;               // let's assume it's d(a1) (could be potentially optimised to (a1)+)
                    out_potential->bytes_affected = num_moves * 4;          // 4 bytes per register
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
                    for (j = num_moves - 1; j >= 0; j--)
                    {
#ifdef PRINT_DEBUG
                        printf("movem.w \ndc.w %i\n", cur_mark->value_or);
#endif
                        *write_sprite_data++ = cur_mark->value_or;
                        cur_mark->action = A_DONE;
                        cur_mark++;
                    }
                    cur_mark--; // Rewind pointer back once because we will skip a mark since we increment the pointer below (right?)
                    i = i - (num_moves - 1);  // Decrease amount of iterations too
                    // We need to emit 2 pairs of movem.ws: one to read the data from RAM and one to write it back to screen buffer.
                    // Assuming that registers d0-d7 and a2-a6 are free to clobber
                    out_potential->base_instruction = MOVEM_W_TO_REG; // movem.w (a0)+,register_list
                    uint16_t register_mask;
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
                    //out_potential->ea = 0x18;         // (a0)+
                    out_potential->screen_offset = 0;   // No screen offset, this is a source read
                    out_potential->bytes_affected = 0;      // Source read so we're not doing any destination change
                    out_potential++;
                    out_potential->base_instruction = MOVEM_W_FROM_REG;     // movem.l register_list,ea
                    out_potential->base_instruction |= register_mask;
                    out_potential->ea = 0x29;           // let's assume it's d(a1) (could be potentially optimised to (a1)+)
                    out_potential->bytes_affected = num_moves * 2;          // 2 bytes per register
                    out_potential++;

                }
            }
            cur_mark++;
        }

        // Check if any consecutive moves survived the above two passes that could be combined in move.ls
        cur_mark = mark_buf;
        int num_movel = 0;
        for (i = num_actions - 1 - 1; i >= 0; i--)
        {
            if (cur_mark->action == A_MOVE)
            {
                //if (cur_mark+1!=cur_mark_end)
                //{
                if (cur_mark[1].action == A_MOVE && cur_mark[1].offset-cur_mark->offset==2)
                {
                    uint32_t longval;
                    longval = (cur_mark->value_or << 16) | cur_mark[1].value_or;
#ifdef PRINT_DEBUG
                    printf("move.l (a0)+,%i(a1)\ndc.l $%x\n", cur_mark->offset, longval);
#endif
                    // TODO: Is there a way we can include these two words into the frequency table without massive amounts of pain?
                    //       For example, the two words have to be consecutive into the table and on even boundary so they can be loaded
                    //       in a single register. But, what happens if only one of the two words makes it into top 16 values?
                    cur_mark->action = A_DONE;

                    out_potential->base_instruction = MOVEI_L;  // move.l #xxx,
                    out_potential->ea = 0xa40;                  // d(a1)
                    out_potential->screen_offset = cur_mark->offset;
                    out_potential->value = longval;
                    out_potential->bytes_affected = 4;          // One longword please
                    out_potential++;

                    cur_mark++;
                    cur_mark->action = A_DONE;

                    num_movel = num_movel + 1;

                    i = i - 1;
                }
                //}
            }
            cur_mark++;
        }

        /* (Leonard's movep trick):
           andi.l #$ff00ff00, (a0); 7
           or  .l #$00xx00yy, (a0)+; 7
           andi.l #$ff00ff00, (a0); 7
           or  .l #$00zz00ww, (a0)+; 7 (total 28)
           
           by:
           
           move.l #$xxyyzzww, dn; 3
           movep.l dn, 1(a0); 6
           addq.w #8, a0; 2 (total 11)*/

        cur_mark = mark_buf;
        for (i = num_actions - 1 - 3; i >= 0; i--)
        {
            if (cur_mark->action == A_AND_OR)
            {
                //if (cur_mark+1!=cur_mark_end)
                //{
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
                        uint32_t longval;
                        longval = ((cur_mark->value_or << 16) & 0xff000000) | ((cur_mark[1].value_or << 16) & 0x00ff0000) | ((cur_mark[2].value_or >> 8) & 0x0000ff00) | (cur_mark[3].value_or & 0x000000ff);
                        longval = (cur_mark->value_or << 16) | cur_mark[1].value_or;
#ifdef PRINT_DEBUG
                        printf(";movep case - crack out the champage, woohoo!\nmove.l #%i,d0\nmovep.l d0,1(a0)\n", cur_mark->offset);
#endif
                        cur_mark->action = A_DONE;

                        out_potential->base_instruction = MOVEP_L; // movep.l d0,1(a1)
                        // out_potential->ea=0xa40;                // d(a1)
                        out_potential->screen_offset = cur_mark->offset;
                        out_potential->value = longval;
                        out_potential->bytes_affected = 0;          // movep is a special case, disregard
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
                //}
            }
            cur_mark++;
        }

        // Check if we have 2 consecutive AND/OR pairs
        // We can combine those into and.l/or.l pair
        uint16_t mark_to_search_for = A_AND_OR;
        if (!use_masking)
        {
            // We were asked not to gerenarte mask code so we're searching for 2 consecutive ORs
            mark_to_search_for = A_OR;
        }
        cur_mark = mark_buf;
        for (i = num_actions - 1 - 1; i >= 0; i--)
        {
            if (cur_mark->action == mark_to_search_for)
            {
                //if (cur_mark+1!=cur_mark_end)
                //{
                if (cur_mark[1].action == mark_to_search_for && cur_mark[1].offset-cur_mark->offset==2)
                {
                    uint32_t longval;
                    longval = (cur_mark->value_or << 16) | cur_mark[1].value_or;
#ifdef PRINT_DEBUG
                    printf("move.l (a0)+,%i(a1)\ndc.l %i\n", cur_mark->offset, longval);
#endif
                    cur_mark->action = A_DONE;

                    if (use_masking)
                    {
                        // We have to output an and.l
                        out_potential->base_instruction = ANDI_L; // andi.l #xxx,
                        out_potential->ea = 0x29;                // d(a1)
                        out_potential->screen_offset = cur_mark->offset;
                        out_potential->value = cur_mark->value_and;
                        out_potential->bytes_affected = 4;          // One longword s'il vous plait
                        out_potential++;
                    }

                    out_potential->base_instruction = ORI_L;     // ori.l #xxx,
                    out_potential->ea = 0x29;                 // d(a1)
                    out_potential->screen_offset = cur_mark->offset;
                    out_potential->value = cur_mark->value_or;
                    out_potential->bytes_affected = 4;          // One longword s'il vous plait
                    out_potential++;

                    cur_mark++;
                    cur_mark->action = A_DONE;

                    i = i - 1;
                }
                //}
            }
            cur_mark++;
        }

        // Any actions left from here on are candidates for register caching. Let's mark this.
        POTENTIAL_CODE *cacheable_code = out_potential;

        // Any leftover moves are just going to be move.ws
        // TODO: Hmmm, since a2-a6 aren't going to be used by and/or then we could use them for move.w
        for (i = num_actions - 1; i >= 0; i--)
        {
            if (cur_mark->action == A_MOVE)
            {
#ifdef PRINT_DEBUG
                printf("move.w #$%x,%i(a1)\n", cur_mark->value_or, cur_mark->offset);
#endif
                // Let's not mark the actions as done yet, the values could be used in the frequency table
                //cur_mark->action = A_DONE;
                out_potential->base_instruction = MOVEI_W;  // move.w #xxx,
                out_potential->ea = 0xa40;                  // d(a1)
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
                uint16_t value = cur_mark->value_or;
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
                            break;
                        }
                        freqptr++;
                    }
                    // TODO any way to avoid this check?
                    if (k == 0)
                    {
                        // Not found above, so create a new entry
                        num_freqs++;
                        freqptr->value = value;
                        freqptr->count = 1;
                    }
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
        if (num_freqs > 15)
        {
            num_freqs = 15;
        }
        uint16_t *cached_values = write_sprite_data;
        for (i = num_freqs; i >= 0; i--)
        {
            freqptr--;
            *write_sprite_data++ = freqptr->value;
        }

        // Anything that remained from the above passes we can safely assume that falls under the and.w/or.w case
        mark_to_search_for = A_AND_OR;
        if (!use_masking)
        {
            // We were asked not to gerenarte mask code so we're searching for loose ORs
            mark_to_search_for = A_OR;
        }
        cur_mark = mark_buf;
        for (i = num_actions - 1; i >= 0; i--)
        {
            if (cur_mark->action == mark_to_search_for)
            {
                uint32_t longval;
                longval = (cur_mark->value_or << 16) | cur_mark[1].value_or;
#ifdef PRINT_DEBUG
                printf("and/or.w (a0)+,%i(a1)\ndc.l %i\n", cur_mark->offset, longval);
#endif
                cur_mark->action = A_DONE;

                if (use_masking)
                {
                    // We have to output an and.l
                    out_potential->base_instruction = ANDI_W;   // andi.w #xxx,
                    out_potential->ea = 0x29;                   // d(a1)
                    out_potential->screen_offset = cur_mark->offset;
                    out_potential->value = cur_mark->value_and;
                    out_potential->bytes_affected = 2;          // Two bytes
                    out_potential++;
                }

                out_potential->base_instruction = ORI_W;        // ori.w #xxx,
                out_potential->ea = 0x29;                       // d(a1)
                out_potential->screen_offset = cur_mark->offset;
                out_potential->value = cur_mark->value_or;
                out_potential->bytes_affected = 2;          // Two bytes
                out_potential++;

                cur_mark++;
                cur_mark->action = A_DONE;

            }
            cur_mark++;
        }


        // It's almost time to output the shifted frame's code and data.
        // Take into account the frequncy table above and use data registers if we can.
        // Remember: - Only registers d0 to d7 can be used for and/or.w Dx,d(a1) or and/or.w Dx,(a1)+
        //           - Therefore we can load up to 16 .w values.
        //           - Not all of them can be available at the same time, we need to SWAP
        //
        // TODO: of course we could postpone this step and move this code outside this loop. This way we can use 
        //       a common pool of data for all frames. Probably makes the output code more complex. Hilarity might ensue.
        POTENTIAL_CODE *potential_end = out_potential;
        for (out_potential = cacheable_code; out_potential < potential_end; out_potential++)
        {
            uint16_t cur_value = out_potential->value;
            for (i = 0; i < num_freqs; i++)
            {
                if (cur_value == cached_values[i])
                {
                    switch (cur_value)
                    {
                    case MOVEI_W:
                        out_potential->base_instruction = MOVED_W;          // Modify instruction
                        out_potential->base_instruction |= i >> 1;          // Register field (D0-D7)
                        break;
                    case ANDI_W:
                        out_potential->base_instruction = ORD_W;            // Modify instruction
                        out_potential->base_instruction |= (i >> 1) << 9;   // Register field (D0-D7)
                        break;
                    case ORI_W:
                        out_potential->base_instruction = ORD_W;            // Modify instruction
                        out_potential->base_instruction |= (i >> 1) << 9;   // Register field (D0-D7)
                        break;

                    }
                    if ((i & 1) == 0)
                    {
                        // We need to emit SWAP, so place a mark at the upper 16 bits (which are unused)
                        // TODO: This is JUNK! We'll need to emit a swap before and after the move in order
                        //       to bring the register back to its original state. We can do better than this!
                        //       (i.e. keep a SWAP state for all registers)
                        out_potential->value |= 0x10000;
                    }
                }
            }
        }

        // Check for consecutive moves or movems and change the generated code to
        // something like <instruction> <data>,(a1)+ instead of <instruction> <data>,xx(a1)
        // andi.w #xxx,d(An) = 16 cycles
        // andi.w #xxx,(An)+ = 12 cycles
        // and.w Dn,d(Am)    = 16 cycles
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

        uint32_t prev_offset = potential->screen_offset;
        uint16_t distance_between_actions = potential->bytes_affected;
        for (out_potential = potential + 1;out_potential < potential_end;out_potential++)
        {
            int consecutive_instructions = 0;
            if (out_potential->bytes_affected != 0)
            {
                if (out_potential->screen_offset - prev_offset == distance_between_actions)
                {
                    consecutive_instructions = consecutive_instructions + 1;
                }
                else
                {
                    if (consecutive_instructions >= 4)
                    {
                        // We have a good post-increment case so let's patch some instructions up
                        POTENTIAL_CODE *patch_instructions = out_potential - consecutive_instructions + 1;
                        for (i = consecutive_instructions - 1;i >= 0;i--)
                        {
                            //switch (patch_instructions->ea)
                            //{
                            //    case 
                            //}
                        }
                    }
                    else
                    {
                        consecutive_instructions = 0;
                    }
                }
            }
            prev_offset = out_potential->screen_offset;
            distance_between_actions = out_potential->bytes_affected;
        }

        //
        // Prepare for next frame
        //

        // Shift sprite by one place right
        // Where's 68000's roxl when you need it, right?
        for (i = BUFFER_SIZE / 2; i > 1; i--)
        {
            *buf = ((buf[-4] & 1) << 7) | (*buf >> 1);
            buf--;
        }
        buf_shift[0] = buf_shift[0] >> 1;

        // Shift mask by one place right
        for (i = BUFFER_SIZE / num_planes; i > 1; i--)
        {
            buf_mask[i] = ((buf_mask[i - 1] & 1) << 7) | (buf_mask[i] >> 1);
        }
        buf_mask[0] = buf_mask[0] >> 1;

    }
}

typedef struct _endianess
{
    union
    {
        uint8_t little;
        uint16_t big;
    };
} endianness;

int main(int argc, char ** argv)
{
    endianness endian;
    endian.big = 1;

    // Let's say we load a .pi1 file
    FILE *sprite = fopen("sprite.pi1","r");
    assert(sprite);
    fseek(sprite, 32, 0);
    fread(buf_origsprite, BUFFER_SIZE, 1, sprite);
    fclose(sprite);

    // Byte swap the array if little endian
    if (endian.little)
    {
        int i;
        uint16_t *buf = (uint16_t *)buf_origsprite;
        for (i = 0;i < BUFFER_SIZE / 2;i++)
        {
            *buf++ = (*buf >> 8) | (*buf << 8);
        }
    }

    halve_it();
    return 0;
}