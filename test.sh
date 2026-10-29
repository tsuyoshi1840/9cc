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
echo OK
