. test input sample

p1    START      0

i1    FLOAT
i2    ADDR       B, L
      SHIFTL     S, 10
i3    SHIFTR     L, 5
i4    TIXR       B
i5    RSUB
i6    ADD        =[1 + 2]
      +ADD       test + 3
      ADD        test + bar + 3 * 5, X
i7    J          @[foo + 10]
      LDF        =F'0.0'
      LDF        =X'1234'
      LDA        =X'123456'
      LDCH       =C'a'


      ORG        1000 + 0x120
test  EQU        *
test  EQU        p1 + a + 5 * 10
      USE
      USE        test          . foo bar baz
      LTORG
      BASE       a + 10
      BASE       100 * 0x4
      NOBASE
      EXTREF     abcdef, test
      EXTDEF     a,b,c,d,e,f
a     BYTE       1 + 2 + 3
a     BYTE       c'hello world'
a     BYTE       F'0.0'
b     WORD       1* 2* 3         . test comment
b     WORD       x'12345'
c     RESB       10
d     RESW       5 * 10

      END        p1

