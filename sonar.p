.origin 0
.entrypoint START

#include "sonar.hp"
#include "sonar_common.hp"

START:
    // Configure the block index register for PRU0 by setting c24_blk_index[7:0] and
    // c25_blk_index[7:0] field to 0x00 and 0x00, respectively.  This will make C24 point
    // to 0x00000000 (PRU0 DRAM) and C25 point to 0x00002000 (PRU1 DRAM).
    MOV       r0, 0x00000000
    MOV       r1, PRU0_CTRL + CTBIR_0
    ST32      r0, r1

    LBCO    r1, C24, 0, 4

    // initialize r2 with r1
    MOV     r2, r1

    // initialize output pin to zero
    CLR r30.t0

POLL:
    LBCO    r1, C24, 0, 4

    // is r1 zero? Then we exit...
    MOV     r4, REQUEST_EXIT
    QBEQ    END, r1, r4

    // compare with r2? Was there a change?
    QBNE    HANDLE_EVENT, r1, r2
    JMP     POLL

HANDLE_EVENT:

    // reset status
    MOV     r7, RESULT_NONE

    // save r1 for next poll
    MOV     r2, r1

    // ------------------------------
    // Parallax PING)) Protocol Below
    // ------------------------------
    // This style processing means that ARM will have to wait up to 21msecs+ to shut us down.
    // We could check for additional input (exit code) while we loop, but this is really 
    // not a big deal for now.
    //

    //
    // set pin to one
    //
    SET r30.t0

    //
    // loop 5usec (500 x 5nsec x 2)
    //
    MOV r5, 0x000001f4
DELAY1:
    SUB r5, r5, 1
    QBNE DELAY1, r5, 0

    //
    // set pin to zero
    //
    CLR r30.t0

    //
    // wait 5usec (500 x 5nsec x 2)
    //
    MOV r5, 0x000001f4
DELAY2:
    SUB r5, r5, 1
    QBNE DELAY2, r5, 0

    //
    // loop until pin == 1 or 1msec expires (66666 x 5 nsec x 3)
    //
    MOV r5, 0x0001046a
WAIT_SET:
    SUB r5, r5, 1
    QBEQ TIMEOUT, r5, 0
    QBBC WAIT_SET,  r31.t2

    //
    // loop/poll for pin == 1 or 20msec expires (1000000 x 5nsec x 4)
    //
    MOV r6, 0x00000000
    MOV r5, 0x000f4240
WAIT_UNSET:
    SUB r5, r5, 1
    ADD r6, r6, 1
    QBEQ TIMEOUT, r5, 0
    QBBS WAIT_UNSET,  r31.t2
    MOV r7, RESULT_OK
    JMP STORE_RES
TIMEOUT:
    // timeout, set zero
    MOV r6, 0x00000000
    MOV r7, RESULT_TIMEOUT

STORE_RES:
    // store result to +4 and +8
    SBCO    r6, C24, 4, 4
    SBCO    r7, C24, 8, 4

    // Send notification to Host for new data
    MOV     R31.b0, PRU0_ARM_INTERRUPT+16

    // next processing
    JMP     POLL

END:
    // store zero to +4, +8
    MOV r6, 0x00000000
    MOV r7, RESULT_SHUTDOWN
    SBCO    r6, C24, 4, 4
    SBCO    r7, C24, 8, 4

    // Send notification to Host for program completion
    MOV     R31.b0, PRU0_ARM_INTERRUPT+16
    HALT
