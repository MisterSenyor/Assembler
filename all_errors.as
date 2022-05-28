; missing space after label
test:mov r5, r4
; illegal comma
mov,r2,r3
; illegal comma after last operand
mov r2,r4,
; missing comma
cmp #10 #40
; illegal character after string
.string "hellotheregeneralkenobi",
; warning - label in extern line
test: .extern LABEL
; warning - label in entry line
test: .entry WHATTTTT
; illegal character after extern label
.extern GROGU,
; unknown command
grogu r4, r5
; missing comma
mov r1 r4
;illegal character after last arg
mov test, r4,
; not enough args
mov #45
; label has illegal character
te_st: mov r4, r5
; label declared twice
grogu: mov r1, r4
grogu: mov r3, r4
; label is saved sequence
mov: mov r3, r4
; non-matching addressing mode
mov #43, #54
; illegal character in immediate
mov #4_5, grogu
; illegal character in label
gro_gu: mov r4, r5
; index addressing method does not contain reg
mov grogu[], r12
; extraneous text after reg
mov kenobi[r12_], r1
; ... second time
mov r12--, r1
; illegal reg number
mov anakin[r23], r1
; ... second time
mov chewy, r43
; missing " around string
.string hello
; nondigit character in data
TEST: .data 56, 5_4, 23
;repeating commas
mov r4, , r5
;long label
abvdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz: stop
