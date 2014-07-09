. address overflow is detected when blocks are ordered
start    START     0xffff0
         RESB      3
         USE       foo
         RESB      10
         USE
         RESB      4
         END       start
