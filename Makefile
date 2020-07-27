BATCHLIB_DIR=${HOME}/FIRESTARTER/batchlib

main: batch
	gcc -Wall -std=c99 -L. -Wl,-rpath=.  main.c -lbatch
	#gcc -Wall -std=c99 -L${BATCHLIB_DIR} -Wl,-rpath=${BATCHLIB_DIR}  main.c -lbatch

batch:
	gcc -Wall -fPIC -std=c99 -g -c batch.c
	ld -shared batch.o -o libbatch.so
	ar rcs libbatch.a batch.o

main_debug: batch_debug
	gcc -DDEBUG -Wall -std=c99 -L. -Wl,-rpath=.  main.c -lbatch

batch_debug:
	gcc -DDEBUG -Wall -fPIC -std=c99 -g -c batch.c
	ld -shared batch.o -o libbatch.so
	ar rcs libbatch.a batch.o

clean:
	rm -rf a.out batch.o batch.so
