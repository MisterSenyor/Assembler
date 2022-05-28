
;assembler test
.entry LIST
.extern W
MAIN: add r3, LIST
LOOP: prn #48
lea STR, r8
    inc    r6
           mov r3, r10
            sub r1, r4
            bne END
            cmp val, #-6
            bne END[r15]
            dec K9
            .entry MAIN
            sub LOOP[r10] ,r14
            END: stop
            STR: .string "abcd"
            LIST: .data 6, -9
            .data -100
            .entry K9
            K9: .data 31
            .extern val
