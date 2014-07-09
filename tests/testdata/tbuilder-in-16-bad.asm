test    START     0
        EXTDEF    foo
foo     EQU       1 * 2 * 3 * 4 + 500     . error: exported symbol must be relative
        END       test
