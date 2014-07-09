test     START     0

         EXTDEF    foo

         FLOAT

         CLEAR     A
         CLEAR     S
         ADDR      B, L
foo      SUBR      S, T
         SHIFTL    X, 10
         SHIFTR    S, 15

         EXTREF    bar, baz

         LDA       foo
         LDT       @foo
         LDS       #[foo - 2]
         LDS       #0x20

         +LDT      #[bar - foo + 1000]
         LDA       #[foo - bar]
         LDS       #bar
         LDL       #[foo + bar - foo - bar + 15 + baz]

         WORD      foo
         WORD      bar - foo
         WORD      x'123456'
         BYTE      bar - bar + foo - foo + baz

         BASE      10000
         LDA       #11000
         NOBASE

         ORG       5000
a        EQU       *
         ORG       6000
         BASE      a
         LDA       @b      . should use base addressing
         LDB       @c      . should use PC-relative addressing
         LDF       =f'1234.5'
         NOBASE
         ORG       8000
c        WORD      2000
         ORG       9000
b        WORD      1000
         LTORG

         USE       example
         LDX       20000      . should use SIC addressing
         LDL       20000, X   . should use SIC addressing

         USE
         LDCH      =15
         LDA       =100
         RESB      100
         RESW      100
         BYTE      c'Hello, world!'

         END       test
