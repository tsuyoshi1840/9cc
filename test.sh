#!/bin/bash
cat <<EOF | gcc -xc -c -o tmp2.o -
int ret3() { return 3; }
int ret5() { return 5; }
int add(int x, int y) { return x+y; }
int sub(int x, int y) { return x-y; }
int add6(int a, int b, int c, int d, int e, int f) { return a+b+c+d+e+f; }
EOF

assert() {
  expected="$1"
  input="$2"

  ./9cc "$input" > tmp.s
  gcc -o tmp tmp.s tmp2.o
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}


assert 0 "main() { 0; }"
assert 42 "main() { 42; }"
assert 21 "main() { 5+20-4; }"
assert 41 "main() {  12 + 34 - 5 ; }"
assert 47 "main() { 5+6*7; }"
assert 15 "main() { 5*(9-6); }"
assert 4 "main() { (3+5)/2; }"
assert 10 "main() { -10+20; }"
assert 10 "main() { - -10; }"
assert 3 "main() { a=3; }"
assert 8 "main() { a=3; b=5; a+b; }"
assert 14 "main() { a=3; b=5*6-8; a+b/2; }"
assert 6 "main() { a=b=3; a+b; }"

assert 6 "main() { foo=1; bar=2+3; foo+bar; }"
assert 30 "main() { count=10; count=count*3; count; }"
assert 7 "main() { x1=3; _y=4; x1+_y; }"
assert 14 "main() { a=3; b=5*6-8; return a+b/2; }"
assert 5 "main() { return 5; return 8; }"
assert 3 "main() { returnx=3; returnx; }"
assert 3 "main() { if (1) return 3; return 5; }"
assert 5 "main() { if (0) return 3; return 5; }"
assert 2 "main() { a=1; if (a==2) return 1; else return 2; }"
assert 10 "main() { i=0; while (i<10) i=i+1; return i; }"
assert 55 "main() { s=0; for (i=1; i<=10; i=i+1) s=s+i; return s; }"
assert 3 "main() { for (;;) return 3; return 5; }"
assert 3 "main() { {1; {2;} return 3;} }"
assert 55 "main() { i=0; j=0; while (i<=10) {j=j+i; i=i+1;} return j; }"
assert 10 "main() { a=0; if (1) {a=5; a=a*2;} return a; }"
assert 5 "main() { {} return 5; }"
assert 3 "main() { return ret3(); }"
assert 5 "main() { return ret5(); }"
assert 8 "main() { return add(3, 5); }"
assert 2 "main() { return sub(5, 3); }"
assert 21 "main() { return add6(1,2,3,4,5,6); }"
assert 10 "main() { a=add(1,2); b=sub(10,3); return a+b; }"
assert 7 "main() { return add2(3, 4); } add2(x, y) { return x+y; }"
assert 1 "main() { return sub2(4, 3); } sub2(x, y) { return x-y; }"
assert 55 "main() { return fib(10); } fib(x) { if (x<=1) return x; return fib(x-1)+fib(x-2); }"
assert 21 "main() { return sum6(1,2,3,4,5,6); } sum6(a,b,c,d,e,f) { return a+b+c+d+e+f; }"
echo OK
