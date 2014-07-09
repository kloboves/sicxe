test    START     0
        ADD       #[a + b + c]
        SUB       @[a + b + c]
        MUL       a + b + c, X
        BYTE      e + f + g
        WORD      f + g + h
        EXTDEF    h, i, j, k, p
p       ADD       #0
r       SUB       #0
        EXTDEF    r
        ADD       #p
        ADD       #r
        END       test
