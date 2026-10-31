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

assert 0 "int main() { return 0; }"
assert 42 "int main() { return 42; }"
assert 21 "int main() { return 5+20-4; }"
assert 41 "int main() { return  12 + 34 - 5 ; }"
assert 47 "int main() { return 5+6*7; }"
assert 15 "int main() { return 5*(9-6); }"
assert 4 "int main() { return (3+5)/2; }"
assert 10 "int main() { return -10+20; }"
assert 10 "int main() { return - -10; }"

assert 3 "int main() { int a; a=3; return a; }"
assert 8 "int main() { int a=3; int b=5; return a+b; }"
assert 14 "int main() { int a=3; int b=5*6-8; return a+b/2; }"
assert 6 "int main() { int a; int b; a=b=3; return a+b; }"
assert 6 "int main() { int foo=1; int bar=2+3; return foo+bar; }"
assert 30 "int main() { int count=10; count=count*3; return count; }"
assert 7 "int main() { int x1=3; int _y=4; return x1+_y; }"

assert 1 "int main() { return 1==1; }"
assert 0 "int main() { return 1==2; }"
assert 1 "int main() { return 1!=2; }"
assert 1 "int main() { return 1<2; }"
assert 0 "int main() { return 2<1; }"
assert 1 "int main() { return 2<=2; }"
assert 1 "int main() { return 3>2; }"
assert 0 "int main() { return 2>=3; }"

assert 5 "int main() { return 5; return 8; }"
assert 3 "int main() { int returnx=3; return returnx; }"

assert 3 "int main() { if (1) return 3; return 5; }"
assert 5 "int main() { if (0) return 3; return 5; }"
assert 2 "int main() { int a=1; if (a==2) return 1; else return 2; }"
assert 10 "int main() { int i=0; while (i<10) i=i+1; return i; }"
assert 55 "int main() { int i; int s=0; for (i=1; i<=10; i=i+1) s=s+i; return s; }"
assert 3 "int main() { for (;;) return 3; return 5; }"

assert 3 "int main() { {1; {2;} return 3;} }"
assert 55 "int main() { int i=0; int j=0; while (i<=10) {j=j+i; i=i+1;} return j; }"
assert 5 "int main() { {} return 5; }"

assert 3 "int main() { return ret3(); }"
assert 8 "int main() { return add(3, 5); }"
assert 21 "int main() { return add6(1,2,3,4,5,6); }"
assert 7 "int main() { return add2(3, 4); } int add2(int x, int y) { return x+y; }"
assert 55 "int main() { return fib(10); } int fib(int x) { if (x<=1) return x; return fib(x-1)+fib(x-2); }"

assert 3 "int main() { int x=3; int *y=&x; return *y; }"
assert 5 "int main() { int x=3; int *y=&x; *y=5; return x; }"

assert 3 "int main() { int x=3; int *y=&x; int **z=&y; return **z; }"
assert 3 "int main() { int x=3; int y=5; int *p=&y; return *(p+1); }"
assert 5 "int main() { int x=3; int y=5; int *p=&x; return *(p-1); }"
assert 7 "int main() { int x=3; int y=5; int *p=&y; *(p+1)=7; return x; }"
assert 4 "int main() { int x; return sizeof(x); }"
assert 8 "int main() { int *p; return sizeof(p); }"
assert 4 "int main() { int *p; return sizeof(*p); }"
assert 8 "int main() { int x; return sizeof(&x); }"
assert 4 "int main() { int x; return sizeof(x+3); }"
assert 8 "int main() { int *p; return sizeof(p+1); }"
assert 4 "int main() { return sizeof(1); }"
assert 4 "int main() { return sizeof 1; }"
assert 4 "int main() { int x; return sizeof(x); }"
assert 8 "int main() { int *p; return sizeof(p); }"
assert 4 "int main() { int *p; return sizeof(*p); }"
assert 8 "int main() { int x; return sizeof(&x); }"
assert 4 "int main() { int x; return sizeof(x+3); }"
assert 8 "int main() { int *p; return sizeof(p+1); }"
assert 4 "int main() { return sizeof(1); }"
assert 4 "int main() { return sizeof 1; }"
assert 4 "int main() { int x; return sizeof(x); }"
assert 8 "int main() { int *p; return sizeof(p); }"
assert 4 "int main() { int *p; return sizeof(*p); }"
assert 8 "int main() { int x; return sizeof(&x); }"
assert 4 "int main() { int x; return sizeof(x+3); }"
assert 8 "int main() { int *p; return sizeof(p+1); }"
assert 4 "int main() { return sizeof(1); }"
assert 4 "int main() { return sizeof 1; }"
assert 4 "int main() { int x; return sizeof(x); }"
assert 8 "int main() { int *p; return sizeof(p); }"
assert 4 "int main() { int *p; return sizeof(*p); }"
assert 8 "int main() { int x; return sizeof(&x); }"
assert 4 "int main() { int x; return sizeof(x+3); }"
assert 8 "int main() { int *p; return sizeof(p+1); }"
assert 4 "int main() { return sizeof(1); }"
assert 4 "int main() { return sizeof 1; }"
echo OK

