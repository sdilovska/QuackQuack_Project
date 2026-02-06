# Sum 1..10 into r0 (result should be 55)
    irmovb $0, r0
    irmovb $1, r1
    irmovb $11, r2
    irmovb $1, r3
loop:
    addw r1, r0
    addw r3, r1
    cmpw r1, r2
    je done
    jmp loop
done:
    halt
