#!/bin/bash
assert() {
  expected="$1"
  input="$2"

  ./9cc "$input" > tmp.s
  gcc -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 0 "0;"
assert 42 "42;"
assert 21 "5+20-4;"
assert 41 " 12 + 34 - 5 ;"
assert 47 "5+6*7;"
assert 15 "5*(9-6);"
assert 4 "(3+5)/2;"
assert 10 "-10+20;"
assert 10 "- -10;"
assert 3 "a=3;"
assert 8 "a=3; b=5; a+b;"
assert 14 "a=3; b=5*6-8; a+b/2;"
assert 6 "a=b=3; a+b;"

assert 6 "foo=1; bar=2+3; foo+bar;"
assert 30 "count=10; count=count*3; count;"
assert 7 "x1=3; _y=4; x1+_y;"
assert 14 "a=3; b=5*6-8; return a+b/2;"
assert 5 "return 5; return 8;"
assert 3 "returnx=3; returnx;"
assert 3 "if (1) return 3; return 5;"
assert 5 "if (0) return 3; return 5;"
assert 2 "a=1; if (a==2) return 1; else return 2;"
assert 10 "i=0; while (i<10) i=i+1; return i;"
assert 55 "s=0; for (i=1; i<=10; i=i+1) s=s+i; return s;"
assert 3 "for (;;) return 3; return 5;"
assert 3 "{1; {2;} return 3;}"
assert 55 "i=0; j=0; while (i<=10) {j=j+i; i=i+1;} return j;"
assert 10 "a=0; if (1) {a=5; a=a*2;} return a;"
assert 5 "{} return 5;"
echo OK
