test    START    0
        ORG      0xffffe
        BYTE     0
        BYTE     1
        BYTE     2        . error: address overflow
        BYTE     3
        END      test
