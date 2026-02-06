# r0=5, r1=7, call add, result in r0
    irmovb $5, r0
    irmovb $7, r1
    call add
    halt
add:
    pushw r2
    rrmovw r0, r2
    addw r1, r2
    rrmovw r2, r0
    popw r2
    ret
