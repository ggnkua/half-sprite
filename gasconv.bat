sed -i -e "s/%%a0/a0/gI" ^
-e "s/%%a0/a0/gI" ^
-e "s/%%a1/a1/gI" ^
-e "s/%%a2/a2/gI" ^
-e "s/%%a3/a3/gI" ^
-e "s/%%a4/a4/gI" ^
-e "s/%%a5/a5/gI" ^
-e "s/%%a6/a6/gI" ^
-e "s/%%fp/a6/gI" ^
-e "s/%%sp/sp/gI" ^
-e "s/%%d0/d0/gI" ^
-e "s/%%d1/d1/gI" ^
-e "s/%%d2/d2/gI" ^
-e "s/%%d3/d3/gI" ^
-e "s/%%d4/d4/gI" ^
-e "s/%%d5/d5/gI" ^
-e "s/%%d6/d6/gI" ^
-e "s/%%d7/d7/gI" ^
-e "s/jle/ble/gI" ^
-e "s/jlt/blt/gI" ^
-e "s/jge/bge/gI" ^
-e "s/jgt/bgt/gI" ^
-e "s/jmi/bmi/gI" ^
-e "s/jpl/bpl/gI" ^
-e "s/jhi/bhi/gI" ^
-e "s/jlo/blo/gI" ^
-e "s/jeq/beq/gI" ^
-e "s/jne/bne/gI" ^
-e "s/jcc/bcc/gI" ^
-e "s/jra/bra/gI" ^
-e "s/jls/bls/gI" ^
-e "s/#NO_APP//gI" ^
-e "s/\.long/dc.l/gI" ^
-e "s/\.zero/ds.b/gI" ^
%1
rem this works with cygwin's awk, but not with cmder's awk. In any case it needs awk>4 for inplace to work sigh, linux tools...
rem awk -i inplace -f mot2devpac.awk %1
awk -f mot2devpac.awk %1 >%1.lolol
del %1
move %1.lolol %1

