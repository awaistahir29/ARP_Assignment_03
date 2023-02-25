# install <pathname>

unzip include.zip
unzip src.zip 
mkdir bin
mkdir out
mkdir log
gcc src/master/master.c -pthread -lncurses -lbmp -lm -lrt -o bin/master
gcc src/processA/processA.c -pthread -lncurses -lbmp -lm -lrt -o bin/processA
gcc src/processB/processB.c -pthread -lncurses -lbmp -lm -lrt -o bin/processB

mkdir logs

echo install completed
