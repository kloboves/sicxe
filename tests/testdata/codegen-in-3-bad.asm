test     START     0

         ORG       1000
a        EQU       *
         BASE      a

         ORG       5100
b        WORD      0

         ORG       7150
         LDA       b

         END       test
