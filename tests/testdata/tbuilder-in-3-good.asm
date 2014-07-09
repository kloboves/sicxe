test     START     0xffff0
         USE       foo
         USE
         RESB      12
a        +ADD      #1000
b        EQU       *
         END       a
