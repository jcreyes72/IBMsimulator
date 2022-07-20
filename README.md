The Project Instructions file contains a thorough description of the functionalities of the program. Compile instructions are located at the bottom of this file

Instructions Implemented with Opcodes:

RR opcodes

    18 LR  - Load Register - load the contents of R2 into R1
    19 CR  - Compare Register – compare the contents of R1 with the
           contents of R2 and set the condition code
    1A AR  - Add Register - add the contents of R2 to the contents of R1,
           place the result into R1, and set the condition code
           (i.e., R1 = R1 + R2)
    1B SR  - Subtract Register - subtract the contents of R2 from the
           contents of R1, place the result into R1, and set the
           condition code (i.e., R1 = R1 – R2)
           
RX opcodes 

    41 LA  - Load Address - load the 24-bit effective address into R1 and
           set the high 8 bits to zeros (there is no alignment check
           nor range check on the address; it is treated as an
           immediate value that will be loaded into the register)
    46 BCT - Branch on Count - decrement the contents of R1 and branch to
           the effective address if the result is not equal to zero
           (the effective address is calculated before the decrement)
    47 BC  - Branch on Condition - treat the R1 field as a branch mask
           and branch to the effective address if the current
           condition code value is one of the values selected by
           the mask bit(s)
    50 ST  - Store - store the contents of R1 into the memory word at the
           effective address (align check and range check the address)
    58 L   - Load - load the contents of the memory word at the effective
           address into R1 (align check and range check the address)
    59 C   - Compare – compare the contents of R1 with the contents of
           the memory word at the effective address and set the
           condition code (align check and range check the address)
           
TO COMPILE:
    The "input data files" folder contains input that can be used to run the simulator. 
        
        1) Compile the sim.c file with gcc
        2) Run a.out alongside a specific input file (i.e "./a.out < simple_loop.in")
        3) There is also a verbose method that displays more information. It can be implemented by adding "-v" to the a.out command shown above (i.e ./a.out -v < simple_loop.in)
    
