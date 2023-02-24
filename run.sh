gcc src/master.c -lncurses -lbmp -lm -o bin/master
gcc src/processA.c -lncurses -lbmp -lm -lrt -o bin/processA
gcc src/processB.c -lncurses -lbmp -lm -lrt -o bin/processB
./bin/master
