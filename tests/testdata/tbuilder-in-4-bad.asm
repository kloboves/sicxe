test     START     0
         LDA       #5
foo      ADDR      A, X
         LDA       =foo     . error: relative expression
a        RESW      5
         END       test
