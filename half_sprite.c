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

// TODO: For now we're assuming ST Low (320x200x4bpp). We should account for different screen depths and resolutions
// TODO: Hardcoded tables everywhere! That's not so great!

#define BUFFER_SIZE 32000

uint8_t buf_origsprite[BUFFER_SIZE];    // This is where we load our original sprite data
uint8_t buf_mask[BUFFER_SIZE/4];        // Buffer used for creating the mask
uint8_t buf_shift[BUFFER_SIZE];         // Buffer used to create the shifted sprite (this is the buffer which we'll do most of the work)
uint16_t output_buf[65536];             // Where the output code will be stored

enum ACTIONS
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
};

typedef struct _MARK
{
    int action;
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
} POTENTIAL_CODE;

POTENTIAL_CODE potential[16384];           // What we'll most probably output, bar a few optimisations


// Code for qsort obtained from https://code.google.com/p/propgcc/source/browse/lib/stdlib/qsort.c
// Butchered by ggn as usual

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
    static short size = 4; //will sort longwords only
    size_t i, a, b, c;
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
            for (i = 0; i < size; i++) /* swap them */
            {
                FREQ tmp = base[a + i];
                base[a + i] = base[b + i];
                base[b + i] = tmp;
            }
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


int main(int argc, char ** argv)
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
    int outline_mask = 0;               // Should we outline mask?
    int use_masking = 1;                // Do we actually want to mask the sprites or is it going to be a special case? (for example, 1bpp sprites)
    int horizontal_clip = 1;            // Should we output code that enables horizontal clip for x<0 and x>screen_width-sprite_width? (this can lead to HUGE amounts of outputted code depending on sprite width!)
    int i, j, k;
    
    memcpy(buf_shift, buf_origsprite, BUFFER_SIZE);
    
    // Prepare mask
    if (use_masking)
    {
        if (generate_mask)
        {
            uint16_t *src = (uint16_t *)buf_origsprite;
            uint16_t *mask = (uint16_t *)buf_mask;
            for (i = 0; i < BUFFER_SIZE / (8/num_planes)/2; i++)
            {
                *mask++ = *src++ | *src++ | *src++ | *src++;
            }
            if (outline_mask)
            {
                // Add outlining of the mask
                mask = (uint16_t *)buf_mask;
                uint16_t *mask2=mask+(screen_width/num_planes/2);
                // OR the mask upwards
                for (i=0;i<screen_width*(screen_height-1)/num_planes/2;i++)
                {
                    *mask|=*mask2;
                    mask++;
                    mask2++;
                }
                // OR the mask downwards
                mask=(uint16_t *)buf_mask+(screen_width*(screen_height)/num_planes/2);
                mask2=mask-(screen_width/num_planes/2);
                for (i=0;i<screen_width*(screen_height-1)/num_planes/2;i++)
                {
                    *mask|=*mask2;
                    mask--;
                    mask2--;
                }
                // OR the mask left and right
                mask = (uint16_t *)buf_mask+1;
                *mask=*mask|(mask[1]>>15);
                mask++;
                for (i=1;i<screen_width*(screen_height-1)/num_planes/2-1;i++)
                {
                    *mask=*mask|(*mask<<1)|(*mask>>1)|(mask[-1]<<15)|(mask[1]>>15);
                    mask++;
                }
                *mask=*mask|(mask[-1]<<15);
                
                // TODO UDLR/UDLR+diagonals also?
                
            }
        }
    }
    
    int num_actions;
    int shift;
    POTENTIAL_CODE *out_potential=potential;
    
    // Main - repeats 16 times for 16 preshifts
    for (shift = 0; shift < 16; shift++)
    {
        int off = 0;
        num_actions = 0;
        MARK *marks = (MARK *)mark_buf;
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
                if (!generate_mask)
                {
                    // Nothing to draw here, skip word
                    goto skipword;
                }
                if (val_and == 0)
                {
                    // Special case where we just need to mask something
                    // because the user supplied their own mask
                    // TODO this could happen if we outlined the mask I guess?
                    marks->action = A_AND;
                    marks->offset = off;
                    marks->value_or = 0;
                    marks->value_and = val_and;
                    marks++;
                    num_actions++;
                    goto skipword;
                }
            }
            
            // We have something to do, let's determine what
            marks->offset = off;
            marks->value_or = val_or;
            
            if (use_masking)
            {
                if (val_and == 0xffff)
                {
                    // No sprite holes to be punctured.
                    // A "move" instruction will suffice
                    marks->action = A_MOVE;
                    marks->value_and = 0;
                }
                else if ((val_or | val_and) == 0xffff)
                {
                    // Special case where mask and sprite data fill all the bits in a word.
                    // We only need to OR in this case. Kudos to Leonard of Oxygene for the idea.
                    marks->action = A_OR;
                    marks->value_and = 0;
                }
                else
                {
                    // Generic case - we need to AND the mask and OR the sprite value
                    marks->action = A_AND_OR;
                    marks->value_and = val_and;
                }
            }
            else
            {
                // We won't mask anything, so just OR things
                marks->action = A_OR;
                marks->value_and = 0;
            }
            num_actions++;
            marks++;
            
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
        MARK *marks_end=marks;           // Save end of marks buffer
        marks = (MARK *)mark_buf;
        MARK *temp_mark;
        uint16_t temp_off_first;
        uint16_t temp_off_second;
        for (i = num_actions - 1; i >= 0; i--)
        {
            if (marks->action == A_MOVE)
            {
                int num_moves = 1;
                temp_mark = marks+1;
                temp_off_first = marks->offset;
                while (temp_mark->action == A_MOVE && temp_mark!=marks_end && num_moves<13*2)
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
                    num_moves = num_moves & ~2; // Even out the number as we can't move half a longword!
                    uint32_t longval;
                    temp_mark = marks;
                    for (j = num_moves/2 - 1; j >= 0; j++)
                    {
                        longval = (marks->value_or << 16) | marks[1].value_or;
                        printf("movem.l \ndc.l %i\n", longval);
                        marks->action = A_DONE;
                        marks++;
                        marks->action = A_DONE;
                        marks++;        
                    }
                    marks--; // Rewind pointer back once because we will skip a mark since we increment the pointer below (right?)
                    // We need to emit 2 pairs of movem.ls: one to read the data from RAM and one to write it back to screen buffer.
                    // Assuming that registers d0-d7 and a2-a6 are free to clobber
                    num_moves=num_moves>>1;
                    out_potential->base_instruction=0x4cc0<<16; // movem.l ea,register_list
                    uint16_t register_mask;
                    if (num_moves<=8)
                    {
                        // Just data registers
                        register_mask=(1<<num_moves)-1;
                    }
                    {
                        // Data and address registers
                        register_mask=0xff|(((1<<(num_moves-8))-1)<<10);
                    }
                    out_potential->base_instruction|=register_mask;
                    out_potential->ea=0x18;        // (a0)+
                    out_potential->screen_offset=marks[-1].offset;
                    //out_potential->value=
                    out_potential++;
                    out_potential->base_instruction=0x48c0<<16; // movem.l ea,register_list
                    out_potential->base_instruction|=register_mask;
                    out_potential->ea=0x29;        // d(a1)
                    out_potential->screen_offset=out_potential[-1].screen_offset;
                    //out_potential->value=
                    out_potential++;
                }
            }
            marks++;
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
        // exactly 3 A_MOVEs. That is, unless I'm missing something.
        // So this block of code should be simplified to just chekcing for 3 A_MOVEs
        marks = (MARK *)mark_buf;
        for (i = num_actions - 1; i >= 0; i--)
        {
            if (marks->action == A_MOVE)
            {
                int num_moves = 1;
                temp_mark = marks+1;
                temp_off_first = marks->offset;
                while (temp_mark->action == A_MOVE && temp_mark!=marks_end && num_moves<13)
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
                temp_mark = marks;
                for (j = num_moves - 1; j >= 0; j++)
                {
                    printf("movem.w \ndc.w %i\n", marks->value_or);
                    marks->action = A_DONE;
                    marks++;
                }
                    marks--; // Rewind pointer back once because we will skip a mark since we increment the pointer below (right?)
                }
            }
            marks++;
        }
        
        // Check if any consecutive moves survived the above two passes that could be combined in move.ls
        marks = (MARK *)mark_buf;
        for (i = num_actions - 1 - 1; i >= 0; i--)
        {
            if (marks->action == A_MOVE)
            {
                //if (marks+1!=marks_end)
                //{
                    if (marks[1].action==A_MOVE)
                    {
                        uint32_t longval;
                        longval = (marks->value_or << 16) | marks[1].value_or;
                        printf("move.l (a0)+,%i(a1)\ndc.l %i\n", marks->offset, longval);
                        marks->action = A_DONE;
                        
                        out_potential->base_instruction=0x203c; // move.l #xxx,
                        out_potential->ea=0xa40;                // d(a1)
                        out_potential->screen_offset=marks->offset;
                        out_potential->value=longval;
                        out_potential++;
                        
                        marks++;
                        marks->action = A_DONE;
                        
                    }
                //}
            }
            marks++;
        }
        
        // Any leftover moves are just going to be move.ws
        for (i = num_actions - 1; i >= 0; i--)
        {
            if (marks->action == A_MOVE)
            {
                printf("move.w #%x,%i(a1)\n", marks->value_or, marks->offset);
                marks->action = A_DONE;
                out_potential->base_instruction=0x303c; // move.w #xxx,
                out_potential->ea=0xa40;                // d(a1)
                out_potential->screen_offset=marks->offset;
                out_potential->value=marks->value_or;
                out_potential++;
            }
            marks++;
        }
        
        
        // Build a "most frequent values" table so we can load things into data registers when possible
        // We could be lazy and just use a huge table which would lead to tons of wasted RAM but let's not do that
        // (I figure this is going to be a tad slow though!)
        marks = (MARK *)mark_buf;
        int num_freqs = 0;
        FREQ *freqptr;
        for (i = num_actions - 1; i >= 0; i--)
        {
            // Only proceed if we haven't processed the current action yet
            if (marks->action != A_DONE)
            {
                uint16_t value = marks->value_or;
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
                    for (k = num_freqs; k >= 0; k--)
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
                    value = marks->value_and;
                }
            }
            marks++;
        }
        
        // Now, sort the list by frequency
        qsort(freqtab, num_freqs);
        
        /* TODO: (Leonard's movep trick):
            andi.l #$ff00ff00, (a0); 7
            or  .l #$00xx00yy, (a0)+; 7
            andi.l #$ff00ff00, (a0); 7
            or  .l #$00zz00ww, (a0)+; 7 (total 28)
            
            by:
            
        move.l #$xxyyzzww, dn; 3
            movep.l dn, 1(a0); 6
            addq.w #8, a0; 2 (total 11)*/

        marks = (MARK *)mark_buf;
        for (i = num_actions - 1 -3; i >= 0; i--)
        {
            if (marks->action == A_AND_OR)
            {
                //if (marks+1!=marks_end)
                //{
                    if (marks[1].action==A_AND_OR &&
                    marks[2].action==A_AND_OR &&
                    marks[3].action==A_AND_OR)
                    {
                    if (  (marks->value_and==0xff00) &&
                        (marks[1].value_and==0xff00) &&
                        (marks[2].value_and==0xff00) &&
                        (marks[3].value_and==0xff00) &&
                        (  (marks->value_or&0xff00)==0) &&
                        ((marks[1].value_or&0xff00)==0) &&
                        ((marks[2].value_or&0xff00)==0) &&
                        ((marks[3].value_or&0xff00)==0))                        
                        {
                        uint32_t longval;
                        longval=((marks->value_or<<16)&0xff000000)|((marks[1].value_or<<16)&0x00ff0000)|((marks[2].value_or>>8)&0x0000ff00)|(marks[3].value_or&0x000000ff);
                        longval = (marks->value_or << 16) | marks[1].value_or;
                        printf(";movep case - crack out the champage, woohoo!\nmove.l #%i,d0\nmovep.l d0,1(a0)\n", marks->offset, longval);
                        marks->action = A_DONE;
                        
                        out_potential->base_instruction=(0x1c9<<16)|1; // movep.l d0,1(a1)
                        // out_potential->ea=0xa40;                // d(a1)
                        out_potential->screen_offset=marks->offset;
                        out_potential->value=longval;
                        out_potential++;
                            
                        marks++;
                        marks->action = A_DONE;
                        marks++;
                        marks->action = A_DONE;
                        marks++;
                        marks->action = A_DONE;
                    }
                    }
                //}
            }
            marks++;
        }

        
        // Check if we have 2 consecutive AND/OR pairs
        // We can combine those into and.l/or.l pair
        uint16_t mark_to_search_for=A_AND_OR;
        if (!use_masking)
        {
            // We were asked not to gerenarte mask code so we're searching for 2 consecutive ORs
            mark_to_search_for=A_OR;
        }
        marks = (MARK *)mark_buf;
        for (i = num_actions - 1 - 1; i >= 0; i--)
        {
            if (marks->action == mark_to_search_for)
            {
                //if (marks+1!=marks_end)
                //{
                    if (marks[1].action==mark_to_search_for)
                    {
                        uint32_t longval;
                        longval = (marks->value_or << 16) | marks[1].value_or;
                        printf("move.l (a0)+,%i(a1)\ndc.l %i\n", marks->offset, longval);
                        marks->action = A_DONE;

                        if (use_masking)
                        {
                            // We have to output a and.l
                            out_potential->base_instruction=0x280; // andi.l #xxx,
                            out_potential->ea=0x29;                // d(a1)
                            out_potential->screen_offset=marks->offset;
                            out_potential->value=marks->value_and;
                            out_potential++;
                        }
                        
                        out_potential->base_instruction=0x80;     // ori.l #xxx,
                        out_potential->ea=0x29;                 // d(a1)
                        out_potential->screen_offset=marks->offset;
                        out_potential->value=marks->value_or;
                        
                        
                        marks++;
                        marks->action = A_DONE;
                        
                    }
                //}
            }
            marks++;
        }
        
        // Anything that remained from the above passes we can safely assume that falls under the and.w/or.w case
        mark_to_search_for=A_AND_OR;
        if (use_masking)
        {
            // We were asked not to gerenarte mask code so we're searching for loose ORs
            mark_to_search_for=A_OR;
        }
        marks = (MARK *)mark_buf;
        for (i = num_actions - 1; i >= 0; i--)
        {
            if (marks->action == mark_to_search_for)
            {
                uint32_t longval;
                longval = (marks->value_or << 16) | marks[1].value_or;
                printf("move.w (a0)+,%i(a1)\ndc.l %i\n", marks->offset, longval);
                marks->action = A_DONE;

                if (use_masking)
                {
                    // We have to output an and.l
                    out_potential->base_instruction=0x240; // andi.w #xxx,
                    out_potential->ea=0x29;                // d(a1)
                    out_potential->screen_offset=marks->offset;
                    out_potential->value=marks->value_and;
                    out_potential++;
                }
                        
                out_potential->base_instruction=0x40;     // ori.w #xxx,
                out_potential->ea=0x29;                 // d(a1)
                out_potential->screen_offset=marks->offset;
                out_potential->value=marks->value_or;
                        
                marks++;
                marks->action = A_DONE;
                
            }
            marks++;
        }
        

        // Finally, it's time to output the shifted frame's code and data.
        // Take into account the frequncy table above and use data registers if we can.
        //
        // TODO: of course we could postpone this step and move this code outside this loop. This way we can use 
        //       a common pool of data for all frames. Probably makes the output code more complex. Hilarity might ensue.
        POTENTIAL_CODE *potential_end=out_potential;
        out_potential=potential;
        {
            
        }
        
        // Check for consecutive moves or movems and change the generated code to
        // something like <instruction> <data>,(a1)+ instead of <instruction> <data>,xx(a1) 
        {
            
        }
        
        //
        // Prepare for next frame
        //
        
        // Shift sprite by one place right
        // Where's 68000's roxl when you need it, right?
        for (i = BUFFER_SIZE/2; i > 1; i--)
        {
            *buf = ((buf[-4] & 1) << 7) | (*buf >> 1);
            buf--;
        }
        buf_shift[0] = buf_shift[0] >> 1;
        
        // Shift mask by one place right
        for (i = BUFFER_SIZE/num_planes; i > 1; i--)
        {
            buf_mask[i] = ((buf_mask[i - 1] & 1) << 7) | (buf_mask[i] >> 1);
        }
        buf_mask[0] = buf_mask[0] >> 1;
        
    }
    
    return 0;
}