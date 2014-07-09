test     START     5000
         WORD      f'123.123'
a        ADD       @test
b        EQU       *

         ORG       5022
         LDA       #15
k        SUBR      X, L

         ORG       0xffff0
c        RESB      16
d        EQU       *

         USE
         LDCH      =c'a'
         USE       foo
         USE       bar
         USE

         EXTREF    e, f, g
         LDA       e - f
i        WORD      e - f + g
         LTORG
         LDA       #10
         END       a
