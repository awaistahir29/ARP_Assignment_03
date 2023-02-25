gcc src/master.c -pthread -lncurses -lbmp -lm -lrt -o bin/master
gcc src/processA.c -pthread -lncurses -lbmp -lm -lrt -o bin/processA
gcc src/processB.c -pthread -lncurses -lbmp -lm -lrt -o bin/processB
./bin/master
