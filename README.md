# Tumasulo-Simulation-Cpp

An architectural simulator capable of assessing the performance of a simplified out-of-order 16-bit RISC processor that uses Tomasulo’s algorithm with speculation.

The project supports RISCV instructions in a file (instructions.txt).
Instructions written in the txt file must be from of the following:

### BEQ

### JAL

### ADD

### ADDI

### MUL

### NAND

### LW

### SW

(Should be all capital letters)

The project also contains testcases to use.

Data in the data memory must be inserted manually by the user before running.

The project still has the flaw of not being able to stop when all instructions are issued and committed.

Labels are supported but they should be put on a seperate line alone with no column (:)
