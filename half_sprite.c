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

#define BUFFER_SIZE 32000

uint8_t buf_origsprite[BUFFER_SIZE];    // This is where we load our original sprite data
uint8_t buf_mask[BUFFER_SIZE/4];        // Buffer used for creating the mask
uint8_t buf_shift[BUFFER_SIZE];         // Buffer used to create the shifted sprite (this is the buffer which we'll do most of the work)

enum ACTIONS
{
    A_NONE,
    A_MOVE,
    A_OR,
    A_AND_OR,
    A_AND,
    A_AND_OR_LONG,
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

int main(int argc, char ** argv)
{
    int screen_width = 320;
    int screen_height = 200;
    int num_planes = 4;                 // How many planes our sprite is
    // How many planes we need to mask in order for the sprite to be drawn properly.
    // This is different from num_planes because for example we might have a 1bpp sprite
    // that has to be written into a 4bpp backdrop
    int num_mask_planes = 4;
    int generate_mask = 1;              // Should we auto generate mask or will user supply his/her own?
    int outline_mask = 0;               // Should we outline mask?
    int use_masking = 1;                // Do we actually want to mask the sprites or is it going to be a special case? (for example, 1bpp sprites)
    int horizontal_clip = 1;            // Should we output code that enables horizontal clip for x<0 and x>screen_width-sprite_width? (this can lead to HUGE amounts of outputted code depending on sprite width!)
    int i, j;

    memcpy(buf_shift, buf_origsprite, BUFFER_SIZE);

    // Prepare mask
    if (use_masking)
    {
        if (generate_mask)
        {
            uint16_t *src = (uint16_t *)buf_origsprite;
            uint16_t *mask = (uint16_t *)buf_mask;
            for (i = 0; i < BUFFER_SIZE / num_planes; i++)
            {
                *mask++ = *src++ | *src++ | *src++ | *src++;
            }
            if (outline_mask)
            {
                // Add outlining of the mask
                // TODO
                // (UDLR/UDLR+diagonals also?)
            }
        }
    }

    int num_actions;
    int shift;

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
                // TODO: bounds check
                while (temp_mark->action == A_MOVE)
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
                }
                uint32_t longval;
                temp_mark = marks;
                for (j = num_moves/2 - 1; j >= 0; j++)
                {
                    longval = (marks->value_or << 16) | marks[1].value_or;
                    printf("dc.l %i\n", longval);
                    marks->action = A_NONE;
                    marks++;
                    marks->action = A_NONE;
                    marks++;
                }
            }
        }

        // Check if we have consecutive moves that did not fall into the movem.l
        // case and see if we can do something using movem.w
        // TODO: does this even make sense?
        // TODO: check clock cycles to determine from which threshold this starts to make sense
        {

        }

        // Check if any consecutive moves survived the above two passes that could be combined in move.ls
        {

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
            if (marks->action != A_NONE)
            {
                uint16_t value = marks->value_or;
                int num_iters = 1;      // Run loop twice, one for OR value and one for AND
                if (!generate_mask)
                    // ...except if we don't generate mask code, in which case run loop only once
                    // (all this stuff should compile to static code so it's not as bloated as you might think with all these ifs and variable for boundaries :))
                    num_iters = 0;
                for (j = num_iters; j >= 0; j--)
                {
                    freqptr = freqtab;
                    for (i = num_freqs - 1; i >= 0; i--)
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
                    if (i == 0)
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

        /* TODO: (Leonard's movep trick):
        andi.l #$ff00ff00, (a0); 7
            or .l #$00xx00yy, (a0)+; 7
            andi.l #$ff00ff00, (a0); 7
            or .l #$00zz00ww, (a0)+; 7 (total 28)

            by:

        move.l #$xxyyzzww, dn; 3
            movep.l dn, 1(a0); 6
            addq.w #8, a0; 2 (total 11)*/

        // Check if we have 2 consecutive AND/OR pairs
        // We can combine those into and.l/or.l pair
        if (use_masking)
        {

        }
        else
        {
            // We were asked not to gerenarte mask code so we're searching for 2 consecutive ORs
        }
        

        // Anything that remained from the above passes we can safely assume that falls under the and.w/or.w case
        if (use_masking)
        {

        }
        else
        {
            // We were asked not to gerenarte mask code so we're searching for loose ORs
        }

        // Finally, it's time to output the shifted frame's code and data
        // TODO: of course we could postpone this step and move this code outside this loop. This way we can use 
        //       a common pool of data for all frames. Probably makes the output code more complex. Hilarity will ensue.
        {

        }

        // Shift sprite by one place right
        // Where's 68000's roxr when you need it, right?
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