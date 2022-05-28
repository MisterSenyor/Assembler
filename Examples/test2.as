TEST1: mov r4, r5

; test2

macro M1
    ;this macro contains lines of code
    ; and comments
    sub TEST1, r3
endm
.string "hellotheregeneralkenobi"
mov r2,       r5
.entry TEST1

.extern TEST2

;comment
M1

stop   
