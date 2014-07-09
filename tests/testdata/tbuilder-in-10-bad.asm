test     START    2000
         ADDR     B, L
         ORG      4000
         LDA      #10
         ORG      0x100 * 0x1000    . error: too large
         ADD      #10
         USE
         ADDR     A, X
         END      test
