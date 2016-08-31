CFLAGS = -O2 -Wall -Wextra 

all:
	$(CC) $(CFLAGS) src/bitspec.c src/inih/ini.c -o bitspec

leak-check:
	valgrind -v --leak-check=full --show-leak-kinds=all \
	./bitspec example/test.spec 0xFF

.PHONY: clean

clean:
	-rm -f bitspec gmon.out

