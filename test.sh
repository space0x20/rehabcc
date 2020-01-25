#!/bin/bash
try() {
    expected="$1"
    input="$2"

    ./rehabcc "$input" > tmp.s
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

try 0 '0;'
try 42 '42;'
try 21 '5+20-4;'
try 21 '5 + 20 - 4;'

# ステップ5
try 47 '5 + 6 * 7;'
try 15 '5 * (9 - 6);'
try 4  '(3 + 5) / 2;'

# ステップ6
try 10 '-10 + 20;'

# ステップ7
try 1 '42 == 42;'
try 0 '42 == 24;'
try 0 '42 != 42;'
try 1 '42 != 24;'
try 0 '42 < 41;'
try 0 '42 < 42;'
try 1 '42 < 43;'
try 0 '42 <= 41;'
try 1 '42 <= 42;'
try 1 '42 <= 43;'
try 1 '42 > 41;'
try 0 '42 > 42;'
try 0 '42 > 43;'
try 1 '42 >= 41;'
try 1 '42 >= 42;'
try 0 '42 >= 43;'

# ステップ8
try 3 'a = 1; b = 2; a + b;'


echo OK
rm -f tmp tmp.s
