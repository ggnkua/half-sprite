del code_assembled.dat
del code.o
rmac -l*code.lst -fb code.s
rln -z -n -rw -a 0 x x code.o -o code_assembled.dat
