. code intersections are:
. -> 2020 - 2021
. -> 2040 - 2041
. -> 2060 - 2061
. -> 2070 - 2090
test     START     2000
         RESB      10
         ORG       2020
         RESB      21
         USE
         BYTE      0
         USE       test
         RESB      10
         ORG       2040
         RESB      21
         ORG       2060
         RESB      20
         ORG       2080
         RESB      20
         ORG       2070
         RESB      10
         ORG       2080
         RESB      10
         END       test
