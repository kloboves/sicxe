test    START     0
a       EQU       10 * 100 + 1234
        EXTDEF    a            . error: export absolute symbol
        END       test
