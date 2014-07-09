test    START    0
        CLEAR    A
        EXTREF   foo
foo     EQU      test + 10   . error: defining external imported symbol
        END      start
