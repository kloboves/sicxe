test      START     0

          LDA       a

. make sure PC-relative is not selected with extended
          +LDA      a
          +LDB      b

          BASE      c
          LDA       b

. make sure base is not selected with extended
          +LDA      b
          BASE      a
          +LDA      b

          ORG       0x300
a         WORD      1

          ORG       0x40000
c         EQU       *
          RESB      1000
b         WORD      2

          END       test
