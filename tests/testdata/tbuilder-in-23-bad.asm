test    START    0
        EXTDEF   foo
foo     +ADD      #10
        EXTREF   baz
        EXTDEF   foo, bar, bar, baz   . warning: duplicate export, error: export imported
        END      test
