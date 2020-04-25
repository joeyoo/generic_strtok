PROFILE = 0

CC = gcc
CFLAGS = -Wall -g -O2 -std=gnu11

ifeq ($(PROFILE), 1)
	STRTOK_FLAGS = -g -fprofile-arcs -ftest-coverage -DPROF -o prof
else
	STRTOK_FLAGS = $(CFLAGS) -c -Wpedantic
	TEST_FLAGS = $(CFLAGS) -mrdrnd -Wall -Wno-unused-function
endif

default: test strok_a strok_b strok_naive

test: strok_a strok_b strok_naive
	$(CC) $(TEST_FLAGS) -o test src/test.c build/strok_a.o build/strok_b.o build/strok_naive.o

strok_a:
	$(CC) $(STRTOK_FLAGS) src/strok_a.c -o build/strok_a.o

strok_b:
	$(CC) $(STRTOK_FLAGS) src/strok_b.c -o build/strok_b.o

strok_naive:
	$(CC) $(STRTOK_FLAGS) src/strok_naive.c -o build/strok_naive.o

clean:
	$(RM) test build/*.o
