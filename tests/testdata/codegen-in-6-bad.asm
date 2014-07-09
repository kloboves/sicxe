test    START    0
        ORG      0xfffff
a       BYTE     10

        USE
        BASE     a
        LDCH     a
        NOBASE

        BASE     a + 1
        END      test
