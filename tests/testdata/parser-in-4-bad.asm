program4  START       0
          START       0
. test
test   . test
          1234
test      +                 . test
test      +  ADD      a
bar       +LTORG
test      +ADDR       A, X
test      FOO         X, B
          FLOAT
          CLEAR
          ADDR
          CLEAR       123
          CLEAR       FOO
          CLEAR       A, FOO
test      ADDR        A, X, Y, Z
          ADDR        123, 123
          ADDR        FOO, BAR
test      SHIFTL      FOO, 10
          SHIFTL      X, 10
          SHIFTR      X, A
          SHIFTL      X, 100
          SHIFTL      X 100
          SHIFTL      A FOO

          LDA         #-
          LDA         #[
          LDA         #[-
          LDA         #[-*
          LDA         #[-c'a'
          LDA         #[-a      .test
          LDA         #[-a a
          LDA         #[-a c'a'
          LDA         #[-a*
          LDA         #[-a*b
          LDA         #[-a*b,
          LDA         #[-a*b+c-d/a[

          RSUB
          RSUB        test
          RSUB        test, X
          RSUB        @test, X
          ADD
          +ADD        ,
          +ADD        +
          +ADD        *
          +ADD        =
          +ADD        #
          +ADD        @
          STF         =F'0.0'
          STA         =3
          STCH        =3
          LDF         =[-3 + 10]
          LDF         =123
          LDF         =F'0.0'
          LDF         =x'123456123456'
          LDF         =x'1234561234567'
          LDF         =c'123456'
          LDF         =c'1234567'
          LDCH        =f'0.0'
          LDB         =f'0.0'
          LDCH        =c'12'
          LDL         =c'1234'
          LDA         =[1 + 2 + 3 + 4 * 5 + test]
          LDA         #[3 + test + 10 * 2]
          LDA         #3 + 10 * 2
          LDB         @[1 * 2 * 3]
          LDB         @1 + 3 + test
          LDL         [1 + 2 + 3]
          LDL         1 + 2 + 3
          LDA         1 + 2 [
          LDA         1 + 2 @
          LDA         1 + 2 ,
          LDA         1 + 2 , 123
          LDA         1 + 2 , c'x'
          LDA         1 + 2 , FOO
          LDA         1 + 2 , A
          LDA         1 + 2 , X
          LDL         #[1 + 3 + 4], X
          LDL         @test, X
          LDL         =test, X
          LDL         test, X

test      START
test      END         test
          END
test      ORG         0
          ORG
          EQU         0
test      EQU         +
test      EQU         *
test      EQU         [1 + 2 + 3 + 4]
test      EQU         1 + 2 + 3 + 4
test      EQU         c'123'
test      USE
          USE
          USE         123
          USE         test
          USE         test + test
test      LTORG
test      BASE
          BASE
          BASE        test
test      NOBASE
          NOBASE
test      EXTDEF      test
test      EXTREF      test
          EXTDEF      abcdefgh, a123456
          EXTREF
          EXTDEF
          EXTREF      ABCDEFGH
          EXTREF      a a a
          EXTREF      a + a + a
a         WORD
a         BYTE
a         BYTE        c'1234'  bla
a         WORD        f'0.0'
a         WORD        a + b + 3
c         BYTE        a + b + 3
d         WORD        [a + b + c]
a         RESB
a         RESW
a         RESW        10
a         RESB        10


          END         program4
