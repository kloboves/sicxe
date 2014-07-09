test     START    1000

x        EQU      10
         EXTREF   y
         EXTDEF   c

a        FLOAT
b        SHIFTL   A, 5
c        ADD      =10
d        EQU      *

         ORG      3 * 500
e        LDCH     =x'0a'
         LDA      =x'123456'
         LDF      =f'0.0'
n        EQU      *

         USE      foo
f        LDA      =X'00000a'
         LDCH     =[2 * 5]
         LDF      =x'00'
         LDF      =f'1234.0'
         LTORG
g        EQU      *

         USE
h        WORD     d - a
i        BYTE     x + 2
j        EQU      *

k        EQU      a + 5
l        EQU      j - c

         USE      bar
m        LDA      =l

         USE
         LDA      =x
         LDCH     =x
         LDF      =f'0.0'

         EXTDEF   f

         END      a
