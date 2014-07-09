test     START    2000
         ADDR     B, L
         ORG      4000
         LDA      #10
         ORG      1000    . error: less than start address
         ADD      #10
         USE
         ADDR     A, X
         END      test
