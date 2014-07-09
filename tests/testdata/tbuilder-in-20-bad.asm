test    START    0
        ADDR     A, X
        SHIFTL   A, 14
        EXTREF   foo
        ADDR     L, X
foo     ADD      #10    . error: defining external imported symbol
        MULR     X, B
        END      test
