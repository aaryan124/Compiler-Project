# =====================================
# BASIC VARIABLES
# =====================================

x = 10
y = 20.1234
z = "Hello, World!"
msg = 'Single quoted string'
escaped = "He said \"hello\""

_value = 42
counter123 = 999

# =====================================
# ARITHMETIC OPERATORS
# =====================================

a = 5 + 3
b = 10 - 4
c = 6 * 7
d = 20 / 5
e = 17 % 3

result = a + b * c - d / e

# =====================================
# BITWISE OPERATORS
# =====================================

bit_and = 5 & 3
bit_or = 5 | 3
bit_xor = 5 ^ 3
bit_not = ~5

shift_left = 5 << 1
shift_right = 10 >> 2

combined_bits = (a & b) | (c ^ d)

# =====================================
# COMPARISON OPERATORS
# =====================================

cond1 = x == y
cond2 = x != y
cond3 = x < y
cond4 = x > y
cond5 = x <= y
cond6 = x >= y

# =====================================
# LOGICAL OPERATORS
# =====================================

logic1 = True and False
logic2 = True or False
logic3 = not False

# =====================================
# IMPORT STATEMENTS
# =====================================

import math
import sklearn as sk

from math import sqrt
from os import path as ospath

# =====================================
# LISTS AND DICTIONARIES
# =====================================

numbers = [1, 2, 3, 4, 5]
matrix = [[1,2], [3,4]]

person = {
    "name": "Alice",
    "age": 25
}

# =====================================
# FUNCTION DEFINITIONS
# =====================================

def add(a, b):
    return a + b

def factorial(n):
    result = 1
    i = 1
    while i <= n:
        result = result * i
        i = i + 1
    return result

# =====================================
# CONDITIONAL STATEMENTS
# =====================================

if x > 10:
    print("x is big")
elif x == 10:
    print("x is exactly ten")
else:
    print("x is small")

# =====================================
# FOR LOOP
# =====================================

for i in numbers:
    print(i)

# =====================================
# WHILE LOOP
# =====================================

i = 0
while i < 5:
    print(i)
    i = i + 1

# =====================================
# CLASS DEFINITION
# =====================================

class Calculator:

    def add(self, a, b):
        return a + b

    def multiply(self, a, b):
        return a * b

# =====================================
# OBJECT USAGE
# =====================================

calc = Calculator()
value = calc.add(3, 4)
print(value)

# =====================================
# DOT OPERATOR
# =====================================

value = math.sqrt(16)
name_length = person["name"]

# =====================================
# BOOLEAN AND NONE
# =====================================

flag = True
nothing = None
enabled = False

# =====================================
# COMPLEX EXPRESSIONS
# =====================================

complex_result = (a + b) * (c - d) / e

bit_complex = ((a << 2) & (b >> 1)) | (~c)

# =====================================
# COMMENTS TEST
# =====================================

# This is a comment
# Another comment
x = 5  # inline comment

# =====================================
# NESTED BLOCKS (INDENT / DEDENT TEST)
# =====================================

if x > 0:
    if x > 5:
        print("greater than five")
    else:
        print("between 1 and 5")
else:
    print("zero or negative")