CFLAGS=-std=c11 -g -static

9cc: 9cc.c

# 9cc で test/test.c をコンパイルして、gcc で作った道具箱（common.c）とつないで実行する
test: 9cc test/test.c test/common.c
	./9cc test/test.c > tmp.s
	cc -o tmp tmp.s test/common.c
	./tmp

clean:
	rm -f 9cc *.o *~ tmp*

.PHONY: test clean
