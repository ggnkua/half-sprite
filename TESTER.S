;                OPT X+

                move.w  #2,-(SP)
                trap    #14
                addq.l  #6,SP
                move.l  D0,screen

                clr.l   -(SP)
                move.w  #$20,-(SP)
                trap    #1
                addq.l  #6,SP

start:
;                movea.l screen,A0
;                move.l  #32000/4-1,D7
;;                moveq   #-1,D0
;                moveq   #0,D0
;clrscr:         move.l  D0,(A0)+
;                dbra    D7,clrscr


                movem.l refpic+2,D0-D7
                movem.l D0-D7,$FFFF8240.w
                clr.b   $FFFF8260.w

                lea     refpic+34,A0
                movea.l screen,A1
                lea     16000(A1),A1
                moveq   #40,D7
copyline:       movem.l (A0),D0-D6
                movem.l D0-D6,(A1)
                lea     160(A0),A0
                lea     160(A1),A1
                dbra    D7,copyline


;                bsr     sprite_code_shift_0

                lea a,a6
                move.w #16+31+31-1,loop_count

draw_loop:      movea.l screen,A1

                move.l a1,a0
                moveq #0,d0
                move.w #32000/4-1,d7
clr_scr:        move.l d0,(a0)+
                dbra d7,clr_scr

                move.l (a6),a0
                jsr (a0)
                addq.w #4,a6

                move.w #1,-(sp)
                trap #1
                addq.w #2,sp

                subq.w #1,loop_count
                bge.s draw_loop


                if 0
                movea.l screen,A0
                lea     16000(A0),A1
                move.w  #80*35,D7
cmploop:        move.w  (A0)+,D0
                cmp.w   (A1),D0
                bne.s   noclr
                clr.w   (A1)
noclr:          addq.w  #2,A1
                dbra    D7,cmploop
                endif

;                jsr     clr_0


                illegal

loop_count: ds.w 1

screen:         DS.L 1

refpic:         incbin "SPRITE.PI1"


a:
                include "code.s"
