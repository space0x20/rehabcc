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

try 0 'return 0;'
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

# ステップ10
try 3 'foo = 1; bar = 2; foo + bar;'

# ステップ11
try 3 'foo = 1; bar = 2; return foo + bar;'
try 3 'foo = 1; bar = 2; return foo + bar; return 42;'

# ステップ12
try 1 'if (1) return 1; return 2;'
try 2 'if (0) return 1; return 2;'
try 1 'if (1) return 1; else return 2; return 3;'
try 2 'if (0) return 1; else return 2; return 3;'
try 2 'if (0) return 1; else if (1) return 2; else return 3; return 4;'
try 3 'if (0) return 1; else if (0) return 2; else return 3; return 4;'

try 5 'x = 0; while (x < 5) x = x + 1; return x;'
try 5 'for (x = 0; x < 5; x = x + 1) 0; return x;'
try 5 'x = 0; for (; x < 5; x = x + 1) 0; return x;'
try 5 'for (x = 0; ; x = x + 1) if (x == 5) return x;'
try 5 'for (x = 0; x < 5;) x = x + 1; return x;'

echo OK
rm -f tmp tmp.s
