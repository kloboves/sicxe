test    START    100
        LDA      #100
foo     STCH     100
        RESB     foo + 200    . error: relative expression
        END      test
