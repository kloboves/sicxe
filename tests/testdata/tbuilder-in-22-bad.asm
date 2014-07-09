test    START     0
foo     SUB       #20
        EXTDEF    foo
        EXTREF    bar, bar, foo    . warning: duplicate import, error: import exported
        END       test
