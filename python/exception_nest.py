#!/usr/bin/env python3
# Generates exceptionally useless code :-P
# also
# the interpreter crashes with
# CPython 3,7, 3.8
# iMac - macOS Catalina 10.15.3
# Raspberry Pi 4
#
# depth:
#     0..10: Fine
#    11..20: Fatal Python error: XXX block stack overflow (Abort)
#    21..99: SyntaxError: too many statically nested blocks
#    >100:   IndentationError: too many levels of indentation
#
# in PyPy 7.3.0 the maximum for a is 2451 and a RecursionError is raised
# Running in ipython doesn't cause the Fatal Error
depth = 15

# Generate code to demonstrate the issue
for i in range(depth):
    print(f"{' '*4*i}try:")  # from {'e' if i else 'Exception()'}
    print(f"{' '*4*(i+1)}raise Exception")
    print(f"{' '*4*i}except Exception as e:")
    if i+1 == depth:
        print(f"{' '*4*(i+1)}pass")
