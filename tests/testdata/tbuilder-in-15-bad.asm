test    START    10 * 10
        SUBR     X, L
foo     ADDR     A, X
        ADD      #20
foo     EQU      test + 20    . error: duplicate symbol
        END      test
