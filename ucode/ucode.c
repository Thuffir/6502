/***********************************************************************************************************************
 *
 * 6502 Microcode
 *
 * by Thuffir
 *
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org/>
 *
 **********************************************************************************************************************/
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/***********************************************************************************************************************
 * Control logic input and output signals
 **********************************************************************************************************************/
// Source select (4 bits)
#define SRC_POS     0
#define OE_PCL      ( 1 << SRC_POS)
#define OE_PCH      ( 2 << SRC_POS)
#define OE_ADDRL    ( 3 << SRC_POS)
#define OE_ADDRH    ( 4 << SRC_POS)
#define OE_RAM      ( 5 << SRC_POS)
#define OE_ACC      ( 6 << SRC_POS)
#define OE_X        ( 7 << SRC_POS)
#define OE_Y        ( 8 << SRC_POS)
#define OE_SP       ( 9 << SRC_POS)
#define OE_ALU_A    (10 << SRC_POS)
#define OE_ALU_B    (11 << SRC_POS)
#define OE_FLAGS    (12 << SRC_POS)
#define OE_ALU      (13 << SRC_POS)
#define OE_CONST    (14 << SRC_POS)

// Destination select (4 bits)
#define DST_POS     4
#define WR_PCL      ( 1 << DST_POS)
#define WR_PCH      ( 2 << DST_POS)
#define WR_ADDRL    ( 3 << DST_POS)
#define WR_ADDRH    ( 4 << DST_POS)
#define WR_RAM      ( 5 << DST_POS)
#define WR_ACC      ( 6 << DST_POS)
#define WR_X        ( 7 << DST_POS)
#define WR_Y        ( 8 << DST_POS)
#define WR_SP       ( 9 << DST_POS)
#define WR_ALU_A    (10 << DST_POS)
#define WR_ALU_B    (11 << DST_POS)
#define WR_FLAGS    (12 << DST_POS)
#define WR_INST     (13 << DST_POS)

// ALU operations (4 Bits, shared input with constant register input)
#define ALU_POS     8
#define ALU_ADD     ( 0 << ALU_POS)   // Binary add
#define ALU_SUB     ( 1 << ALU_POS)   // Binary subtract
#define ALU_DADD    ( 2 << ALU_POS)   // Binary or BCD add
#define ALU_DSUB    ( 3 << ALU_POS)   // Binary or BCD subtract
#define ALU_AND     ( 4 << ALU_POS)
#define ALU_OR      ( 5 << ALU_POS)
#define ALU_XOR     ( 6 << ALU_POS)
#define ALU_SHR     ( 7 << ALU_POS)
#define ALU_CLRBIT  ( 8 << ALU_POS)   // ALU Clear Bit
#define ALU_SETBIT  ( 9 << ALU_POS)   // ALU Set Bit
#define ALU_TSTBIT  (10 << ALU_POS)   // ALU Test Bit
#define ALU_INVA    (11 << ALU_POS)   // ALU Invert A

// Constant register output (4 Bits, shared input with ALU operation)
#define CONST_POS   8
#define CONST_0     ( 0 << CONST_POS)
#define CONST_1     ( 1 << CONST_POS)
#define CONST_2     ( 2 << CONST_POS)
#define CONST_3     ( 3 << CONST_POS)
#define CONST_4     ( 4 << CONST_POS)
#define CONST_5     ( 5 << CONST_POS)
#define CONST_6     ( 6 << CONST_POS)
#define CONST_7     ( 7 << CONST_POS)
#define CONST_F8    ( 8 << CONST_POS)
#define CONST_F9    ( 9 << CONST_POS)
#define CONST_FA    (10 << CONST_POS)
#define CONST_FB    (11 << CONST_POS)
#define CONST_FC    (12 << CONST_POS)
#define CONST_FD    (13 << CONST_POS)
#define CONST_FE    (14 << CONST_POS)
#define CONST_FF    (15 << CONST_POS)

// Break opcode as constant for interrupt opcode injection
#define CONST_BRK   (CONST_0)

// Flags input to uCode ROM (4 bits)
#define FLAGIN_POS  12
#define FLAGIN_C    ( 0 << FLAGIN_POS)  // Carry flag
#define FLAGIN_Z    ( 1 << FLAGIN_POS)  // Carry flag
#define FLAGIN_V    ( 2 << FLAGIN_POS)  // Overflow flag
#define FLAGIN_N    ( 3 << FLAGIN_POS)  // Negative flag
#define FLAGIN_CALU ( 4 << FLAGIN_POS)  // Carry of the last ALU output
#define FLAGIN_NB   ( 5 << FLAGIN_POS)  // Actual sign bit of the B register
#define FLAGIN_INTR ( 6 << FLAGIN_POS)  // Signal for all interrupts (INT, NMI, RST)
#define FLAGIN_NMI  ( 7 << FLAGIN_POS)  // NMI
#define FLAGIN_RST  ( 8 << FLAGIN_POS)  // Reset
#define FLAGIN_ALOV ( 9 << FLAGIN_POS)  // ADDRL overflow signal
#define FLAGIN_ZALU (10 << FLAGIN_POS)  // Zero of the last ALU output

// Address output select (2 bits)
#define SELADDR_POS 16
#define ADDR_PC     ( 0 << SELADDR_POS) // Address is driven by the PC registers
#define ADDR_FULL   ( 1 << SELADDR_POS) // Address is driven by the ADDR registers
#define ADDR_ZP     ( 2 << SELADDR_POS) // Address is driven by ADDRL in ZERO page
#define ADDR_SP     ( 3 << SELADDR_POS) // Address is driven by the SP register in the stack page

// Other signals
#define CI_ALU      ( 1 << 18)          // ALU Carry Input
#define INC_PC      ( 1 << 19)          // PC register increment enable
#define RST_USTEP   ( 1 << 20)          // End of uSteps, reset uStep counter
#define UPD_C       ( 1 << 21)          // Update the Carry flag in the PS register
#define UPD_Z       ( 1 << 22)          // Update the Zero flag in the PS register
#define UPD_I       ( 1 << 23)          // Update the Interrupt Disable flag in the PS register
#define UPD_D       ( 1 << 24)          // Update the Decimal flag in the PS register
#define UPD_V       ( 1 << 25)          // Update the Overflow flag in the PS register
#define UPD_N       ( 1 << 26)          // Update the Negative flag in the PS register
#define INC_ADDRL   ( 1 << 27)          // ADDRL register increment enable
#define INC_ADDRH   ( 1 << 28)          // ADDRH register increment enable
#define CNT_SP      ( 1 << 29)          // SP register increment or decrement enable
#define DIR_SP      ( 1 << 30)          // SP register increment (0) or decrement (1)
#define INT_HOLD    ( 1 << 31)          // Hold the interrupt signals so they do not change

// Update Negative and Zero flags
#define UPD_NZ      (UPD_N | UPD_Z)

// SP Operations
#define DEC_SP    (CNT_SP | DIR_SP)     // Decrement SP
#define INC_SP    (CNT_SP)              // Increment SP

// uCode ROM input bit positions
#define ROM_USTEP_POS  0                // uStep counter position (4 bits)
#define ROM_INSTR_POS  4                // Opcode position (8 bits)
#define ROM_FLAGS_POS 12                // Multiplexed flags input (1 bit)

// Max numbers
#define INSTR_MAX     ( 1 << (ROM_FLAGS_POS - ROM_INSTR_POS)) // Number of maximum opcodes (256)
#define USTEP_MAX     ( 1 << (ROM_INSTR_POS - ROM_USTEP_POS)) // Number of maximum uSteps (16)
#define UCODE_SIZE    ( 1 << (ROM_FLAGS_POS + 1))             // Size of the uCode ROM (8 kB)

// Define an opcode and begin the first uStep
#define USTEP_DEFINE(inst) { \
  unsigned int _inst = (inst); \
  uStep[_inst] = 1; \
  instr[inst] = #inst

// Set an uStep for one flag input position
#define USTEP_FLAG_STEP(flag, ucode) \
  rom[((flag) << ROM_FLAGS_POS) | (_inst << ROM_INSTR_POS) | (uStep[_inst] << ROM_USTEP_POS)] = (ucode)

// Set different uSteps for both flag input positions
#define USTEP_FLAG(ucodeFlagZero, ucodeFlagOne) \
  USTEP_FLAG_STEP(0, ucodeFlagZero); \
  USTEP_FLAG_STEP(1, ucodeFlagOne); \
  uStep[_inst]++

// Set the same uStep for both flag input positions
#define USTEP_STEP(ucode) USTEP_FLAG(ucode, ucode)
// Last uStep (sets RST_USTEP)
#define USTEP_LAST(ucode) USTEP_STEP((ucode) | RST_USTEP); }
// Last uSteps for different flag input positions (sets RST_USTEP)
#define USTEP_FLST(ucodeFlagZero, ucodeFlagOne) USTEP_FLAG(ucodeFlagZero | RST_USTEP, ucodeFlagOne | RST_USTEP); }

/***********************************************************************************************************************
 * uCode step templates for different addressing modes
 **********************************************************************************************************************/
// Zero-Page Addressing
#define USTEP_ADDR_Z \
  /* Load ZP address to ADDRL */ \
  USTEP_STEP(OE_RAM | INC_PC | WR_ADDRL)

// Indexed Addressing: Zero-Page,X and Zero-Page,Y
#define USTEP_ADDR_Z_IDX(oe) \
  /* Load ZP address to A */ \
  USTEP_STEP(OE_RAM | INC_PC | WR_ALU_A); \
  /* Load X or Y to B */ \
  USTEP_STEP(oe | WR_ALU_B); \
  /* Add the together into ADDRL*/ \
  USTEP_STEP(OE_ALU | ALU_ADD | WR_ADDRL)

// Load indirect address from Zero-Page pointed by ADDRL
#define USTEP_ADDR_INDZ \
  /* Put address low byte into A temporarily */ \
  USTEP_STEP(OE_RAM | ADDR_ZP | WR_ALU_A | INC_ADDRL);\
  /* Load address high byte into ADDRH */ \
  USTEP_STEP(OE_RAM | ADDR_ZP | WR_ADDRH); \
  /* Load saved address low byte into ADDRL */ \
  USTEP_STEP(OE_ALU_A | WR_ADDRL)

// Zero-Page Indirect (65C02)
#define USTEP_ADDR_Z_IND \
  /* Load zero page address into ADDRL */ \
  USTEP_ADDR_Z; \
  /* Put indirect address into ADDRL and ADDRH from zero page */ \
  USTEP_ADDR_INDZ;

// Pre-Indexed Indirect, "(Zero-Page,X)"
#define USTEP_ADDR_ZX_IND \
  /* Load zero page base address + X into ADDRL */ \
  USTEP_ADDR_Z_IDX(OE_X); \
  /* Put indirect address into ADDRL and ADDRH from zero page */ \
  USTEP_ADDR_INDZ;

// Absolute Addressing
#define USTEP_ADDR_ABS \
  /* Load address into ADDR registers */ \
  USTEP_STEP(OE_RAM | INC_PC | WR_ADDRL); \
  USTEP_STEP(OE_RAM | INC_PC | WR_ADDRH)

// Indexed Addressing: Absolute,X and Absolute,Y
#define USTEP_ADDR_ABS_IDX(oe) \
  /* Load base address low byte into A */ \
  USTEP_STEP(OE_RAM | INC_PC | WR_ALU_A); \
  /* Load base address high byte into ADDRH */ \
  USTEP_STEP(OE_RAM | INC_PC | WR_ADDRH); \
  /* Load X or Y into B */ \
  USTEP_STEP(oe | WR_ALU_B); \
  /* ADDRL + X/Y -> ADDRL */ \
  USTEP_STEP(OE_ALU | ALU_ADD | WR_ADDRL); \
  /* Increment ADDRH if page boundary has been crossed */ \
  USTEP_FLAG( \
    FLAGIN_CALU, \
    FLAGIN_CALU | INC_ADDRH \
  )

// Post-Indexed Indirect, "(Zero-Page),Y"
#define USTEP_ADDR_ZY_IND \
  /* Load zero page address into ADDRL */ \
  USTEP_STEP(OE_RAM | INC_PC | WR_ADDRL); \
  /* Load base address low byte from zero page into A */ \
  USTEP_STEP(OE_RAM | ADDR_ZP | WR_ALU_A | INC_ADDRL); \
  /* Load base address high byte from zero page into ADDRH */ \
  USTEP_STEP(OE_RAM | ADDR_ZP | WR_ADDRH); \
  /* Y -> B */ \
  USTEP_STEP(OE_Y | WR_ALU_B); \
  /* Base address low byte + Y -> ADDRL */ \
  USTEP_STEP(OE_ALU | ALU_ADD | WR_ADDRL); \
  /* Increment ADDRH if page boundary has been crossed */ \
  USTEP_FLAG( \
    FLAGIN_CALU, \
    FLAGIN_CALU | INC_ADDRH \
  )

// Load data into a register updating N and Z flags
#define LD_NZ(addr, wr) USTEP_LAST(OE_RAM | addr | UPD_NZ | wr)

// Supported CPU types
enum {
  CPU_6502,
  CPU_65C02
} cpuType = CPU_6502;

// Opcode nmemonics as strings
char *instr[INSTR_MAX];
// Number of uSteps used for a specific opcode
uint8_t uStep[INSTR_MAX];
// uCode rom content
uint32_t rom[UCODE_SIZE];

/***********************************************************************************************************************
 * Generate 6502 microcode
 **********************************************************************************************************************/
static void GenerateUcode6502(void)
{
  // All valid 6502 instruction opcodes (151)
  enum {
    // Legend:
    // *  - add 1 to cycles if page boundary is crossed
    // The number of cycles here are the original 6502 cycles and included only for reference.

    // ADC - Add Memory to Accumulator with Carry (A + M + C -> A, C)
    // N Z C I D V
    // + + + - - +
    // ENUM   = Opcode   | Addressing   | Assembler    | B | Cycles
    ADC_IMM   = 0x69, // | immediate    | ADC #oper    | 2 | 2
    ADC_Z     = 0x65, // | zeropage     | ADC oper     | 2 | 3
    ADC_ZX    = 0x75, // | zeropage,X   | ADC oper,X   | 2 | 4
    ADC_ABS   = 0x6D, // | absolute     | ADC oper     | 3 | 4
    ADC_ABSX  = 0x7D, // | absolute,X   | ADC oper,X   | 3 | 4*
    ADC_ABSY  = 0x79, // | absolute,Y   | ADC oper,Y   | 3 | 4*
    ADC_INDX  = 0x61, // | (indirect,X) | ADC (oper,X) | 2 | 6
    ADC_INDY  = 0x71, // | (indirect),Y | ADC (oper),Y | 2 | 5*

    // AND - AND Memory with Accumulator (A AND M -> A)
    // N Z C I D V
    // + + - - - -
    // ENUM   = Opcode   | Addressing   | Assembler    | B | Cycles
    AND_IMM   = 0x29, // | immediate    | AND #oper    | 2 | 2
    AND_Z     = 0x25, // | zeropage     | AND oper     | 2 | 3
    AND_ZX    = 0x35, // | zeropage,X   | AND oper,X   | 2 | 4
    AND_ABS   = 0x2D, // | absolute     | AND oper     | 3 | 4
    AND_ABSX  = 0x3D, // | absolute,X   | AND oper,X   | 3 | 4*
    AND_ABSY  = 0x39, // | absolute,Y   | AND oper,Y   | 3 | 4*
    AND_INDX  = 0x21, // | (indirect,X) | AND (oper,X) | 2 | 6
    AND_INDY  = 0x31, // | (indirect),Y | AND (oper),Y | 2 | 5*

    // ASL - Shift Left One Bit (Memory or Accumulator) (C <- [76543210] <- 0)
    // N Z C I D V
    // + + + - - -
    // ENUM   = Opcode   | Addressing   | Assembler    | B | Cycles
    ASL_A     = 0x0A, // | accumulator  | ASL A        | 1 | 2
    ASL_Z     = 0x06, // | zeropage     | ASL oper     | 2 | 5
    ASL_ZX    = 0x16, // | zeropage,X   | ASL oper,X   | 2 | 6
    ASL_ABS   = 0x0E, // | absolute     | ASL oper     | 3 | 6
    ASL_ABSX  = 0x1E, // | absolute,X   | ASL oper,X   | 3 | 7

    // Branch on flag clear oder set
    // N Z C I D V
    // - - - - - -
    // ** - add 1 to cycles if branch occurs on same page,  add 2 to cycles if branch occurs to different page
    // ENUM   = Opcode   | Addressing   | Assembler    | B | Cyc | Operation
    BCC       = 0x90, // | relative     | BCC oper     | 2 | 2** | branch on C = 0
    BCS       = 0xB0, // | relative     | BCS oper     | 2 | 2** | branch on C = 1
    BNE       = 0xD0, // | relative     | BNE oper     | 2 | 2** | branch on Z = 0
    BEQ       = 0xF0, // | relative     | BEQ oper     | 2 | 2** | branch on Z = 1
    BPL       = 0x10, // | relative     | BPL oper     | 2 | 2** | branch on N = 0
    BMI       = 0x30, // | relative     | BMI oper     | 2 | 2** | branch on N = 1
    BVC       = 0x50, // | relative     | BVC oper     | 2 | 2** | branch on V = 0
    BVS       = 0x70, // | relative     | BVS oper     | 2 | 2** | branch on V = 1

    // BI T- Test Bits in Memory with Accumulator (A AND M, M7 -> N, M6 -> V)
    //  N Z C I D V
    // M7 + - - - M6
    // ENUM   = Opcode   | Addressing   | Assembler    | B | Cycles
    BIT_Z     = 0x24, // | zeropage     | BIT oper     | 2 | 3
    BIT_ABS   = 0x2C, // | absolute     | BIT oper     | 3 | 4

    // BRK - Force Break (push PC+2, push SR)
    // N Z C I D V
    // - - - + - -
    // ENUM   = Opcode   | Addressing   | Assembler    | B | Cycles
    BRK       = 0x00, // | implied      | BRK          | 1 | 7

    // Flag Instructions
    // N Z C I D V
    // - - + + + +
    // ENUM   = Opcode   | Addressing   | Assembler    | B | C | Operation
    CLC       = 0x18, // | implied      | CLC          | 1 | 2 | 0 -> C
    SEC       = 0x38, // | implied      | SEC          | 1 | 2 | 1 -> C
    CLD       = 0xD8, // | implied      | CLD          | 1 | 2 | 0 -> D
    SED       = 0xF8, // | implied      | SED          | 1 | 2 | 1 -> D
    CLI       = 0x58, // | implied      | CLI          | 1 | 2 | 0 -> I
    SEI       = 0x78, // | implied      | SEI          | 1 | 2 | 1 -> I
    CLV       = 0xB8, // | implied      | CLV          | 1 | 2 | 0 -> V

    // Compare Memory with Accumulator (A - M)
    // N Z C I D V
    // + + + - - -
    // ENUM   = Opcode   | Addressing   | Assembler    | B | Cycles
    CMP_IMM   = 0xC9, // | immediate    | CMP #oper    | 2 | 2
    CMP_Z     = 0xC5, // | zeropage     | CMP oper     | 2 | 3
    CMP_ZX    = 0xD5, // | zeropage,X   | CMP oper,X   | 2 | 4
    CMP_ABS   = 0xCD, // | absolute     | CMP oper     | 3 | 4
    CMP_ABSX  = 0xDD, // | absolute,X   | CMP oper,X   | 3 | 4*
    CMP_ABSY  = 0xD9, // | absolute,Y   | CMP oper,Y   | 3 | 4*
    CMP_INDX  = 0xC1, // | (indirect,X) | CMP (oper,X) | 2 | 6
    CMP_INDY  = 0xD1, // | (indirect),Y | CMP (oper),Y | 2 | 5*

    // CPX - Compare Memory and Index X (X - M)
    // N Z C I D V
    // + + + - - -
    // ENUM   = Opcode   | Addressing   | Assembler    | B | Cycles
    CPX_IMM   = 0xE0, // | immediate    | CPX #oper    | 2 | 2
    CPX_Z     = 0xE4, // | zeropage     | CPX oper     | 2 | 3
    CPX_ABS   = 0xEC, // | absolute     | CPX oper     | 3 | 4

    // CPY -  Compare Memory and Index Y (Y - M)
    // N Z C I D V
    // + + + - - -
    // ENUM   = Opcode   | Addressing   | Assembler    | B | Cycles
    CPY_IMM   = 0xC0, // | immediate    | CPY #oper    | 2 | 2
    CPY_Z     = 0xC4, // | zeropage     | CPY oper     | 2 | 3
    CPY_ABS   = 0xCC, // | absolute     | CPY oper     | 3 | 4

    // DEC - Decrement Memory by One (M - 1 -> M)
    // N Z C I D V
    // + + - - - -
    DEC_Z     = 0xC6, // | zeropage     | DEC oper     | 2 | 5
    DEC_ZX    = 0xD6, // | zeropage,X   | DEC oper,X   | 2 | 6
    DEC_ABS   = 0xCE, // | absolute     | DEC oper     | 3 | 6
    DEC_ABSX  = 0xDE, // | absolute,X   | DEC oper,X   | 3 | 7

    // INC - Increment Memory by One (M + 1 -> M)
    // N Z C I D V
    // + + - - - -
    // ENUM   = Opcode   | Addressing   | Assembler    | B | Cycles
    INC_Z     = 0xE6, // | zeropage     | INC oper     | 2 | 5
    INC_ZX    = 0xF6, // | zeropage,X   | INC oper,X   | 2 | 6
    INC_ABS   = 0xEE, // | absolute     | INC oper     | 3 | 6
    INC_ABSX  = 0xFE, // | absolute,X   | INC oper,X   | 3 | 7

    // Register increment / decrement
    // N Z C I D V
    // + + - - - -
    // ENUM   = Opcode   | Addressing   | Assembler    | B | C | Operation
    INX       = 0xE8, // | implied      | INX          | 1 | 2 | X + 1 -> X
    DEX       = 0xCA, // | implied      | DEX          | 1 | 2 | X - 1 -> X
    INY       = 0xC8, // | implied      | INY          | 1 | 2 | Y + 1 -> Y
    DEY       = 0x88, // | implied      | DEY          | 1 | 2 | Y - 1 -> Y

    // EOR - Exclusive-OR Memory with Accumulator (A EOR M -> A)
    // N Z C I D V
    // + + - - - -
    // ENUM   = Opcode   | Addressing   | Assembler    | B | Cycles
    EOR_IMM   = 0x49, // | immediate    | EOR #oper    | 2 | 2
    EOR_Z     = 0x45, // | zeropage     | EOR oper     | 2 | 3
    EOR_ZX    = 0x55, // | zeropage,X   | EOR oper,X   | 2 | 4
    EOR_ABS   = 0x4D, // | absolute     | EOR oper     | 3 | 4
    EOR_ABSX  = 0x5D, // | absolute,X   | EOR oper,X   | 3 | 4*
    EOR_ABSY  = 0x59, // | absolute,Y   | EOR oper,Y   | 3 | 4*
    EOR_INDX  = 0x41, // | (indirect,X) | EOR (oper,X) | 2 | 6
    EOR_INDY  = 0x51, // | (indirect),Y | EOR (oper),Y | 2 | 5*

    // JMP -  Jump to New Location ((PC+1) -> PCL, (PC+2) -> PCH)
    // N Z C I D V
    // - - - - - -
    // ENUM   = Opcode   | Addressing   | Assembler    | B | Cycles
    JMP_ABS   = 0x4C, // | absolute     | JMP oper     | 3 | 3
    JMP_IND   = 0x6C, // | indirect     | JMP (oper)   | 3 | 5

    // JSR -  Jump to New Location Saving Return Address (push (PC+2), (PC+1) -> PCL, (PC+2) -> PCH)
    // N Z C I D V
    // - - - - - -
    // ENUM   = Opcode   | Addressing   | Assembler    | B | Cycles
    JSR       = 0x20, // | absolute     | JSR oper     | 3 | 6

    // LDA - Load Accumulator with Memory (M -> A)
    // N Z C I D V
    // + + - - - -
    // ENUM   = Opcode   | Addressing   | Assembler    | B | Cycles
    LDA_IMM   = 0xA9, // | immediate    | LDA #oper    | 2 | 2
    LDA_Z     = 0xA5, // | zeropage     | LDA oper     | 2 | 3
    LDA_ZX    = 0xB5, // | zeropage,X   | LDA oper,X   | 2 | 4
    LDA_ABS   = 0xAD, // | absolute     | LDA oper     | 3 | 4
    LDA_ABSX  = 0xBD, // | absolute,X   | LDA oper,X   | 3 | 4*
    LDA_ABSY  = 0xB9, // | absolute,Y   | LDA oper,Y   | 3 | 4*
    LDA_INDX  = 0xA1, // | (indirect,X) | LDA (oper,X) | 2 | 6
    LDA_INDY  = 0xB1, // | (indirect),Y | LDA (oper),Y | 2 | 5*

    // LDX - Load Index X with Memory (M -> X)
    // N Z C I D V
    // + + - - - -
    // ENUM   = Opcode   | Addressing   | Assembler    | B | Cycles
    LDX_IMM   = 0xA2, // | immediate    | LDX #oper    | 2 | 2
    LDX_Z     = 0xA6, // | zeropage     | LDX oper     | 2 | 3
    LDX_ZY    = 0xB6, // | zeropage,Y   | LDX oper,Y   | 2 | 4
    LDX_ABS   = 0xAE, // | absolute     | LDX oper     | 3 | 4
    LDX_ABSY  = 0xBE, // | absolute,Y   | LDX oper,Y   | 3 | 4*

    // LDY - Load Index Y with Memory (M -> Y)
    // N Z C I D V
    // + + - - - -
    // ENUM   = Opcode   | Addressing   | Assembler    | B | Cycles
    LDY_IMM   = 0xA0, // | immediate    | LDY #oper    | 2 | 2
    LDY_Z     = 0xA4, // | zeropage     | LDY oper     | 2 | 3
    LDY_ZX    = 0xB4, // | zeropage,X   | LDY oper,X   | 2 | 4
    LDY_ABS   = 0xAC, // | absolute     | LDY oper     | 3 | 4
    LDY_ABSX  = 0xBC, // | absolute,X   | LDY oper,X   | 3 | 4*

    // LSR - Shift One Bit Right (Memory or Accumulator) (0 -> [76543210] -> C)
    // N Z C I D V
    // 0 + + - - -
    // ENUM   = Opcode   | Addressing   | Assembler    | B | Cycles
    LSR_A     = 0x4A, // | accumulator  | LSR A        | 1 | 2
    LSR_Z     = 0x46, // | zeropage     | LSR oper     | 2 | 5
    LSR_ZX    = 0x56, // | zeropage,X   | LSR oper,X   | 2 | 6
    LSR_ABS   = 0x4E, // | absolute     | LSR oper     | 3 | 6
    LSR_ABSX  = 0x5E, // | absolute,X   | LSR oper,X   | 3 | 7

    // NOP - No Operation
    // N Z C I D V
    // - - - - - -
    // ENUM   = Opcode   | Addressing   | Assembler    | B | Cycles
    NOP       = 0xEA, // | implied      | NOP          | 1 | 2

    // ORA - OR Memory with Accumulator (A OR M -> A)
    // N Z C I D V
    // + + - - - -
    // ENUM   = Opcode   | Addressing   | Assembler    | B | Cycles
    ORA_IMM   = 0x09, // | immediate    | ORA #oper    | 2 | 2
    ORA_Z     = 0x05, // | zeropage     | ORA oper     | 2 | 3
    ORA_ZX    = 0x15, // | zeropage,X   | ORA oper,X   | 2 | 4
    ORA_ABS   = 0x0D, // | absolute     | ORA oper     | 3 | 4
    ORA_ABSX  = 0x1D, // | absolute,X   | ORA oper,X   | 3 | 4*
    ORA_ABSY  = 0x19, // | absolute,Y   | ORA oper,Y   | 3 | 4*
    ORA_INDX  = 0x01, // | (indirect,X) | ORA (oper,X) | 2 | 6
    ORA_INDY  = 0x11, // | (indirect),Y | ORA (oper),Y | 2 | 5*

    // Stack Instructions
    // ENUM   = Opcode   | Addressing   | Assembler    | B | C | Operation | N Z C I D V
    PHA       = 0x48, // | implied      | PHA          | 1 | 3 | push A    | - - - - - -
    PLA       = 0x68, // | implied      | PLA          | 1 | 4 | pull A    | + + - - - -
    PHP       = 0x08, // | implied      | PHP          | 1 | 3 | push SR   | - - - - - -
    PLP       = 0x28, // | implied      | PLP          | 1 | 4 | pull SR   | + + + + + +

    // ROL - Rotate One Bit Left (Memory or Accumulator) (C <- [76543210] <- C)
    // N Z C I D V
    // + + + - - -
    // ENUM   = Opcode   | Addressing   | Assembler    | B | Cycles
    ROL_A     = 0x2A, // | accumulator  | ROL A        | 1 | 2
    ROL_Z     = 0x26, // | zeropage     | ROL oper     | 2 | 5
    ROL_ZX    = 0x36, // | zeropage,X   | ROL oper,X   | 2 | 6
    ROL_ABS   = 0x2E, // | absolute     | ROL oper     | 3 | 6
    ROL_ABSX  = 0x3E, // | absolute,X   | ROL oper,X   | 3 | 7

    // ROR - Rotate One Bit Right (Memory or Accumulator) (C -> [76543210] -> C)
    // N Z C I D V
    // + + + - - -
    // ENUM   = Opcode   | Addressing   | Assembler    | B | Cycles
    ROR_A     = 0x6A, // | accumulator  | ROR A        | 1 | 2
    ROR_Z     = 0x66, // | zeropage     | ROR oper     | 2 | 5
    ROR_ZX    = 0x76, // | zeropage,X   | ROR oper,X   | 2 | 6
    ROR_ABS   = 0x6E, // | absolute     | ROR oper     | 3 | 6
    ROR_ABSX  = 0x7E, // | absolute,X   | ROR oper,X   | 3 | 7

    // RTI - Return from Interrupt (pull SR, pull PC)
    // N Z C I D V
    // + + + + + +
    // ENUM   = Opcode   | Addressing   | Assembler    | B | Cycles
    RTI       = 0x40, // | implied      | RTI          | 1 | 6

    // RTS - Return from Subroutine (pull PC, PC+1 -> PC)
    // N Z C I D V
    // - - - - - -
    // ENUM   = Opcode   | Addressing   | Assembler    | B | Cycles
    RTS       = 0x60, // | implied      | RTS          | 1 | 6

    // SBC - Subtract Memory from Accumulator with Borrow (A - M - /C -> A)
    // N Z C I D V
    // + + + - - +
    // ENUM   = Opcode   | Addressing   | Assembler    | B | Cycles
    SBC_IMM   = 0xE9, // | immediate    | SBC #oper    | 2 | 2
    SBC_Z     = 0xE5, // | zeropage     | SBC oper     | 2 | 3
    SBC_ZX    = 0xF5, // | zeropage,X   | SBC oper,X   | 2 | 4
    SBC_ABS   = 0xED, // | absolute     | SBC oper     | 3 | 4
    SBC_ABSX  = 0xFD, // | absolute,X   | SBC oper,X   | 3 | 4*
    SBC_ABSY  = 0xF9, // | absolute,Y   | SBC oper,Y   | 3 | 4*
    SBC_INDX  = 0xE1, // | (indirect,X) | SBC (oper,X) | 2 | 6
    SBC_INDY  = 0xF1, // | (indirect),Y | SBC (oper),Y | 2 | 5*

    // STA - Store Accumulator in Memory (A -> M)
    // N Z C I D V
    // - - - - - -
    // ENUM   = Opcode   | Addressing   | Assembler    | B | Cycles
    STA_Z     = 0x85, // | zeropage     | STA oper     | 2 | 3
    STA_ZX    = 0x95, // | zeropage,X   | STA oper,X   | 2 | 4
    STA_ABS   = 0x8D, // | absolute     | STA oper     | 3 | 4
    STA_ABSX  = 0x9D, // | absolute,X   | STA oper,X   | 3 | 5
    STA_ABSY  = 0x99, // | absolute,Y   | STA oper,Y   | 3 | 5
    STA_INDX  = 0x81, // | (indirect,X) | STA (oper,X) | 2 | 6
    STA_INDY  = 0x91, // | (indirect),Y | STA (oper),Y | 2 | 6

    // STX - Store Index X in Memory (X -> M)
    // N Z C I D V
    // - - - - - -
    // ENUM   = Opcode   | Addressing   | Assembler    | B | Cycles
    STX_Z     = 0x86, // | zeropage     | STX oper     | 2 | 3
    STX_ZY    = 0x96, // | zeropage,Y   | STX oper,Y   | 2 | 4
    STX_ABS   = 0x8E, // | absolute     | STX oper     | 3 | 4

    // STY - Sore Index Y in Memory (Y -> M)
    // N Z C I D V
    // - - - - - -
    // ENUM   = Opcode   | Addressing   | Assembler    | B | Cycles
    STY_Z     = 0x84, // | zeropage     | STY oper     | 2 | 3
    STY_ZX    = 0x94, // | zeropage,X   | STY oper,X   | 2 | 4
    STY_ABS   = 0x8C, // | absolute     | STY oper     | 3 | 4

    // Register to register transfer
    // ENUM   = Opcode   | Addressing   | Assembler    | B | C | Operation | N Z C I D V
    TAX       = 0xAA, // | implied      | TAX          | 1 | 2 | A -> X    | + + - - - -
    TXA       = 0x8A, // | implied      | TXA          | 1 | 2 | X -> A    | + + - - - -
    TAY       = 0xA8, // | implied      | TAY          | 1 | 2 | A -> Y    | + + - - - -
    TYA       = 0x98, // | implied      | TYA          | 1 | 2 | Y -> A    | + + - - - -
    TSX       = 0xBA, // | implied      | TSX          | 1 | 2 | SP -> X   | + + - - - -
    TXS       = 0x9A, // | implied      | TXS          | 1 | 2 | X -> SP   | - - - - - -
  };

  /*********************************************************************************************************************
   * ADC - Add Memory to Accumulator with Carry (A + M + C -> A, C)
   *
   * N Z C I D V
   * + + + - - +
   ********************************************************************************************************************/
  // ALU add or subtract with carry flag
#define ALU_ADD_SUB(addr, aluop) \
  /* ACC -> A */ \
  USTEP_STEP(OE_ACC | WR_ALU_A); \
  /* Operand from memory -> B */ \
  USTEP_STEP(OE_RAM | addr | WR_ALU_B); \
  /* Add or subtract with carry */ \
  USTEP_FLST( \
    FLAGIN_C | OE_ALU | aluop | UPD_NZ | UPD_C | UPD_V | WR_ACC, \
    FLAGIN_C | OE_ALU | aluop | UPD_NZ | UPD_C | UPD_V | WR_ACC | CI_ALU \
  )

  // Add with carry and according to decimal flag
#define ADD_ACC(addr) ALU_ADD_SUB(addr, ALU_DADD)

  USTEP_DEFINE(ADC_IMM);
  ADD_ACC(INC_PC);

  USTEP_DEFINE(ADC_Z);
  USTEP_ADDR_Z;
  ADD_ACC(ADDR_ZP);

  USTEP_DEFINE(ADC_ZX);
  USTEP_ADDR_Z_IDX(OE_X);
  ADD_ACC(ADDR_ZP);

  USTEP_DEFINE(ADC_ABS);
  USTEP_ADDR_ABS;
  ADD_ACC(ADDR_FULL);

  USTEP_DEFINE(ADC_ABSX);
  USTEP_ADDR_ABS_IDX(OE_X);
  ADD_ACC(ADDR_FULL);

  USTEP_DEFINE(ADC_ABSY);
  USTEP_ADDR_ABS_IDX(OE_Y);
  ADD_ACC(ADDR_FULL);

  USTEP_DEFINE(ADC_INDX);
  USTEP_ADDR_ZX_IND;
  ADD_ACC(ADDR_FULL);

  USTEP_DEFINE(ADC_INDY);
  USTEP_ADDR_ZY_IND;
  ADD_ACC(ADDR_FULL);

  /*********************************************************************************************************************
   * SBC - Subtract Memory from Accumulator with Borrow (A - M - /C -> A)
   *
   * N Z C I D V
   * + + + - - +
   ********************************************************************************************************************/
#define SUB_ACC(addr) ALU_ADD_SUB(addr, ALU_DSUB)

  USTEP_DEFINE(SBC_IMM);
  SUB_ACC(INC_PC);

  USTEP_DEFINE(SBC_Z);
  USTEP_ADDR_Z;
  SUB_ACC(ADDR_ZP);

  USTEP_DEFINE(SBC_ZX);
  USTEP_ADDR_Z_IDX(OE_X);
  SUB_ACC(ADDR_ZP);

  USTEP_DEFINE(SBC_ABS);
  USTEP_ADDR_ABS;
  SUB_ACC(ADDR_FULL);

  USTEP_DEFINE(SBC_ABSX);
  USTEP_ADDR_ABS_IDX(OE_X);
  SUB_ACC(ADDR_FULL);

  USTEP_DEFINE(SBC_ABSY);
  USTEP_ADDR_ABS_IDX(OE_Y);
  SUB_ACC(ADDR_FULL);

  USTEP_DEFINE(SBC_INDX);
  USTEP_ADDR_ZX_IND;
  SUB_ACC(ADDR_FULL);

  USTEP_DEFINE(SBC_INDY);
  USTEP_ADDR_ZY_IND;
  SUB_ACC(ADDR_FULL);

  /*********************************************************************************************************************
   * AND - AND Memory with Accumulator (A AND M -> A)
   *
   * N Z C I D V
   * + + - - - -
   ********************************************************************************************************************/
  // ALU AND / OR / XOR
#define ALU_LOGIC_OP(addr, aluop) \
  /* ACC -> A */ \
  USTEP_STEP(OE_ACC | WR_ALU_A); \
  /* Operand from memory -> B */ \
  USTEP_STEP(OE_RAM | addr | WR_ALU_B); \
  /* Result of logic operation -> ACC */ \
  USTEP_LAST(OE_ALU | aluop | UPD_NZ | WR_ACC)

  // AND
#define AND_ACC(addr) ALU_LOGIC_OP(addr, ALU_AND)

  USTEP_DEFINE(AND_IMM);
  AND_ACC(INC_PC);

  USTEP_DEFINE(AND_Z);
  USTEP_ADDR_Z;
  AND_ACC(ADDR_ZP);

  USTEP_DEFINE(AND_ZX);
  USTEP_ADDR_Z_IDX(OE_X);
  AND_ACC(ADDR_ZP);

  USTEP_DEFINE(AND_ABS);
  USTEP_ADDR_ABS;
  AND_ACC(ADDR_FULL);

  USTEP_DEFINE(AND_ABSX);
  USTEP_ADDR_ABS_IDX(OE_X);
  AND_ACC(ADDR_FULL);

  USTEP_DEFINE(AND_ABSY);
  USTEP_ADDR_ABS_IDX(OE_Y);
  AND_ACC(ADDR_FULL);

  USTEP_DEFINE(AND_INDX);
  USTEP_ADDR_ZX_IND;
  AND_ACC(ADDR_FULL);

  USTEP_DEFINE(AND_INDY);
  USTEP_ADDR_ZY_IND;
  AND_ACC(ADDR_FULL);

  /*********************************************************************************************************************
   * ORA - OR Memory with Accumulator (A OR M -> A)
   *
   * N Z C I D V
   * + + - - - -
   ********************************************************************************************************************/
#define ORA_ACC(addr) ALU_LOGIC_OP(addr, ALU_OR)

  USTEP_DEFINE(ORA_IMM);
  ORA_ACC(INC_PC);

  USTEP_DEFINE(ORA_Z);
  USTEP_ADDR_Z;
  ORA_ACC(ADDR_ZP);

  USTEP_DEFINE(ORA_ZX);
  USTEP_ADDR_Z_IDX(OE_X);
  ORA_ACC(ADDR_ZP);

  USTEP_DEFINE(ORA_ABS);
  USTEP_ADDR_ABS;
  ORA_ACC(ADDR_FULL);

  USTEP_DEFINE(ORA_ABSX);
  USTEP_ADDR_ABS_IDX(OE_X);
  ORA_ACC(ADDR_FULL);

  USTEP_DEFINE(ORA_ABSY);
  USTEP_ADDR_ABS_IDX(OE_Y);
  ORA_ACC(ADDR_FULL);

  USTEP_DEFINE(ORA_INDX);
  USTEP_ADDR_ZX_IND;
  ORA_ACC(ADDR_FULL);

  USTEP_DEFINE(ORA_INDY);
  USTEP_ADDR_ZY_IND;
  ORA_ACC(ADDR_FULL);

  /*********************************************************************************************************************
   * EOR - Exclusive-OR Memory with Accumulator (A EOR M -> A)
   *
   * N Z C I D V
   * + + - - - -
   ********************************************************************************************************************/
#define EOR_ACC(addr) ALU_LOGIC_OP(addr, ALU_XOR)

  USTEP_DEFINE(EOR_IMM);
  EOR_ACC(INC_PC);

  USTEP_DEFINE(EOR_Z);
  USTEP_ADDR_Z;
  EOR_ACC(ADDR_ZP);

  USTEP_DEFINE(EOR_ZX);
  USTEP_ADDR_Z_IDX(OE_X);
  EOR_ACC(ADDR_ZP);

  USTEP_DEFINE(EOR_ABS);
  USTEP_ADDR_ABS;
  EOR_ACC(ADDR_FULL);

  USTEP_DEFINE(EOR_ABSX);
  USTEP_ADDR_ABS_IDX(OE_X);
  EOR_ACC(ADDR_FULL);

  USTEP_DEFINE(EOR_ABSY);
  USTEP_ADDR_ABS_IDX(OE_Y);
  EOR_ACC(ADDR_FULL);

  USTEP_DEFINE(EOR_INDX);
  USTEP_ADDR_ZX_IND;
  EOR_ACC(ADDR_FULL);

  USTEP_DEFINE(EOR_INDY);
  USTEP_ADDR_ZY_IND;
  EOR_ACC(ADDR_FULL);

  /*********************************************************************************************************************
   * ASL - Shift Left One Bit (Memory or Accumulator) (C <- [76543210] <- 0)
   *
   * N Z C I D V
   * + + + - - -
   ********************************************************************************************************************/
#define ASL(src, dst) \
  /* src -> A */ \
  USTEP_STEP(src | WR_ALU_A); \
  /* A -> B */ \
  USTEP_STEP(OE_ALU_A | WR_ALU_B); \
  /* src + src -> dst */ \
  USTEP_LAST(OE_ALU | ALU_ADD | UPD_NZ | UPD_C | dst)

  USTEP_DEFINE(ASL_A);
  ASL(OE_ACC, WR_ACC);

  USTEP_DEFINE(ASL_Z);
  USTEP_ADDR_Z;
  ASL(OE_RAM | ADDR_ZP, WR_RAM | ADDR_ZP);

  USTEP_DEFINE(ASL_ZX);
  USTEP_ADDR_Z_IDX(OE_X);
  ASL(OE_RAM | ADDR_ZP, WR_RAM | ADDR_ZP);

  USTEP_DEFINE(ASL_ABS);
  USTEP_ADDR_ABS;
  ASL(OE_RAM | ADDR_FULL, WR_RAM | ADDR_FULL);

  USTEP_DEFINE(ASL_ABSX);
  USTEP_ADDR_ABS_IDX(OE_X);
  ASL(OE_RAM | ADDR_FULL, WR_RAM | ADDR_FULL);

  /*********************************************************************************************************************
   * ROL - Rotate One Bit Left (Memory or Accumulator) (C <- [76543210] <- C)
   *
   * N Z C I D V
   * + + + - - -
   ********************************************************************************************************************/
#define ROL(src, dst) \
  /* src -> A */ \
  USTEP_STEP(src | WR_ALU_A); \
  /* A -> B */ \
  USTEP_STEP(OE_ALU_A | WR_ALU_B); \
  /* src + src + C -> dst */ \
  USTEP_FLST( \
    FLAGIN_C | OE_ALU | ALU_ADD | UPD_NZ | UPD_C | dst, \
    FLAGIN_C | OE_ALU | ALU_ADD | UPD_NZ | UPD_C | dst | CI_ALU \
  )

  USTEP_DEFINE(ROL_A);
  ROL(OE_ACC, WR_ACC);

  USTEP_DEFINE(ROL_Z);
  USTEP_ADDR_Z;
  ROL(OE_RAM | ADDR_ZP, WR_RAM | ADDR_ZP);

  USTEP_DEFINE(ROL_ZX);
  USTEP_ADDR_Z_IDX(OE_X);
  ROL(OE_RAM | ADDR_ZP, WR_RAM | ADDR_ZP);

  USTEP_DEFINE(ROL_ABS);
  USTEP_ADDR_ABS;
  ROL(OE_RAM | ADDR_FULL, WR_RAM | ADDR_FULL);

  USTEP_DEFINE(ROL_ABSX);
  USTEP_ADDR_ABS_IDX(OE_X);
  ROL(OE_RAM | ADDR_FULL, WR_RAM | ADDR_FULL);

  /*********************************************************************************************************************
   * LSR - Shift One Bit Right (Memory or Accumulator) (0 -> [76543210] -> C)
   *
   * N Z C I D V
   * 0 + + - - -
   ********************************************************************************************************************/
#define LSR(src, dst) \
  USTEP_STEP(src | WR_ALU_A); \
  USTEP_LAST(OE_ALU | ALU_SHR | UPD_NZ | UPD_C | dst)

  USTEP_DEFINE(LSR_A);
  LSR(OE_ACC, WR_ACC);

  USTEP_DEFINE(LSR_Z);
  LSR(OE_RAM | ADDR_ZP, WR_RAM | ADDR_ZP);

  USTEP_DEFINE(LSR_ZX);
  LSR(OE_RAM | ADDR_ZP, WR_RAM | ADDR_ZP);

  USTEP_DEFINE(LSR_ABS);
  LSR(OE_RAM | ADDR_FULL, WR_RAM | ADDR_FULL);

  USTEP_DEFINE(LSR_ABSX);
  LSR(OE_RAM | ADDR_FULL, WR_RAM | ADDR_FULL);

  /*********************************************************************************************************************
   * ROR - Rotate One Bit Right (Memory or Accumulator) (C -> [76543210] -> C)
   *
   * N Z C I D V
   * + + + - - -
   ********************************************************************************************************************/
#define ROR(src, dst) \
  USTEP_STEP(src | WR_ALU_A); \
  USTEP_FLST( \
    FLAGIN_C | OE_ALU | ALU_SHR | UPD_NZ | UPD_C | dst, \
    FLAGIN_C | OE_ALU | ALU_SHR | UPD_NZ | UPD_C | dst | CI_ALU \
  )

  USTEP_DEFINE(ROR_A);
  ROR(OE_ACC, WR_ACC);

  USTEP_DEFINE(ROR_Z);
  ROR(OE_RAM | ADDR_ZP, WR_RAM | ADDR_ZP);

  USTEP_DEFINE(ROR_ZX);
  ROR(OE_RAM | ADDR_ZP, WR_RAM | ADDR_ZP);

  USTEP_DEFINE(ROR_ABS);
  ROR(OE_RAM | ADDR_FULL, WR_RAM | ADDR_FULL);

  USTEP_DEFINE(ROR_ABSX);
  ROR(OE_RAM | ADDR_FULL, WR_RAM | ADDR_FULL);

  /*********************************************************************************************************************
   * BIT - Test Bits in Memory with Accumulator (A AND M, M7 -> N, M6 -> V)
   *
   *  N Z C I D V
   * M7 + - - - M6
   *
   * bits 7 and 6 of operand are transfered to bit 7 and 6 of SR (N,V);
   * the zero-flag is set according to the result of the operand AND
   * the accumulator (set, if the result is zero, unset otherwise).
   * This allows a quick check of a few bits at once without affecting
   * any of the registers, other than the status register (SR).
   *
   ********************************************************************************************************************/
#define BIT(addr) \
  /* Memory -> A */ \
  USTEP_STEP(OE_RAM | addr | WR_ALU_A); \
  /* Bit 6 -> V, Bit 7 -> N */ \
  USTEP_STEP(OE_ALU_A | WR_FLAGS | UPD_N | UPD_V); \
  /* ACC -> B */ \
  USTEP_STEP(OE_ACC | WR_ALU_B); \
  /* ACC AND Memory -> Z */ \
  USTEP_LAST(OE_ALU | ALU_AND | UPD_Z)

  USTEP_DEFINE(BIT_Z);
  USTEP_ADDR_Z;
  BIT(ADDR_ZP);

  USTEP_DEFINE(BIT_ABS);
  USTEP_ADDR_ABS;
  BIT(ADDR_FULL);

  /*********************************************************************************************************************
   * Comparisons
   *
   * Generally, comparison instructions subtract the operand from the given register without affecting this register.
   * Flags are still set as with a normal subtraction and thus the relation of the two values becomes accessible by the
   * Zero, Carry and Negative flags.
   * (See the branch instructions below for how to evaluate flags.)
   *
   * Relation R - Op    | Z | C | N
   * Register < Operand | 0 | 0 | sign bit of result
   * Register = Operand | 1 | 1 | 0
   * Register > Operand | 0 | 1 | sign bit of result
   ********************************************************************************************************************/
#define CMP(reg, addr) \
  /* Register -> A */ \
  USTEP_STEP(reg | WR_ALU_A); \
  /* Operand -> B */ \
  USTEP_STEP(OE_RAM | addr | WR_ALU_B); \
  /* Operand - Register -> N, Z, C */ \
  USTEP_LAST(OE_ALU | ALU_SUB | CI_ALU | UPD_NZ | UPD_C)

  USTEP_DEFINE(CMP_IMM);
  CMP(OE_ACC, INC_PC);

  USTEP_DEFINE(CMP_Z);
  USTEP_ADDR_Z;
  CMP(OE_ACC, ADDR_ZP);

  USTEP_DEFINE(CMP_ZX);
  USTEP_ADDR_Z_IDX(OE_X);
  CMP(OE_ACC, ADDR_ZP);

  USTEP_DEFINE(CMP_ABS);
  USTEP_ADDR_ABS;
  CMP(OE_ACC, ADDR_FULL);

  USTEP_DEFINE(CMP_ABSX);
  USTEP_ADDR_ABS_IDX(OE_X);
  CMP(OE_ACC, ADDR_FULL);

  USTEP_DEFINE(CMP_ABSY);
  USTEP_ADDR_ABS_IDX(OE_Y);
  CMP(OE_ACC, ADDR_FULL);

  USTEP_DEFINE(CMP_INDX);
  USTEP_ADDR_ZX_IND;
  CMP(OE_ACC, ADDR_FULL);

  USTEP_DEFINE(CMP_INDY);
  USTEP_ADDR_ZY_IND;
  CMP(OE_ACC, ADDR_FULL);

  USTEP_DEFINE(CPX_IMM);
  CMP(OE_X, INC_PC);

  USTEP_DEFINE(CPX_Z);
  USTEP_ADDR_Z;
  CMP(OE_X, ADDR_ZP);

  USTEP_DEFINE(CPX_ABS);
  USTEP_ADDR_ABS;
  CMP(OE_X, ADDR_FULL);

  USTEP_DEFINE(CPY_IMM);
  CMP(OE_Y, INC_PC);

  USTEP_DEFINE(CPY_Z);
  USTEP_ADDR_Z;
  CMP(OE_Y, ADDR_ZP);

  USTEP_DEFINE(CPY_ABS);
  USTEP_ADDR_ABS;
  CMP(OE_Y, ADDR_FULL);

  /*********************************************************************************************************************
   * Increment / Decrement
   *
   * N Z C I D V
   * + + - - - -
   ********************************************************************************************************************/
#define INCDEC(src, dst, const) \
  /* 1 or -1 -> B */ \
  USTEP_STEP(OE_CONST | const | WR_ALU_B); \
  /* Source -> A */ \
  USTEP_STEP(src | WR_ALU_A); \
  /* Increment or decrement */ \
  USTEP_LAST(OE_ALU | ALU_ADD | UPD_NZ | dst)

  // Increment
#define INC(src, dst) INCDEC(src, dst, CONST_1);

  // Decrement
#define DEC(src, dst) INCDEC(src, dst, CONST_FF);

  USTEP_DEFINE(DEC_Z);
  USTEP_ADDR_Z;
  DEC(OE_RAM | ADDR_ZP, WR_RAM | ADDR_ZP);

  USTEP_DEFINE(DEC_ZX);
  USTEP_ADDR_Z_IDX(OE_X);
  DEC(OE_RAM | ADDR_ZP, WR_RAM | ADDR_ZP);

  USTEP_DEFINE(DEC_ABS);
  USTEP_ADDR_ABS;
  DEC(OE_RAM | ADDR_FULL, WR_RAM | ADDR_FULL);

  USTEP_DEFINE(DEC_ABSX);
  USTEP_ADDR_ABS_IDX(OE_X);
  DEC(OE_RAM | ADDR_FULL, WR_RAM | ADDR_FULL);

  USTEP_DEFINE(INC_Z);
  USTEP_ADDR_Z;
  INC(OE_RAM | ADDR_ZP, WR_RAM | ADDR_ZP);

  USTEP_DEFINE(INC_ZX);
  USTEP_ADDR_Z_IDX(OE_X);
  INC(OE_RAM | ADDR_ZP, WR_RAM | ADDR_ZP);

  USTEP_DEFINE(INC_ABS);
  USTEP_ADDR_ABS;
  INC(OE_RAM | ADDR_FULL, WR_RAM | ADDR_FULL);

  USTEP_DEFINE(INC_ABSX);
  USTEP_ADDR_ABS_IDX(OE_X);
  INC(OE_RAM | ADDR_FULL, WR_RAM | ADDR_FULL);

  USTEP_DEFINE(INX);
  INC(OE_X, WR_X);

  USTEP_DEFINE(DEX);
  DEC(OE_X, WR_X);

  USTEP_DEFINE(INY);
  INC(OE_Y, WR_Y);

  USTEP_DEFINE(DEY);
  DEC(OE_Y, WR_Y);

  /*********************************************************************************************************************
   * LDA - Load Accumulator with Memory (M -> A)
   *
   * N Z C I D V
   * + + - - - -
   ********************************************************************************************************************/
  USTEP_DEFINE(LDA_IMM);
  LD_NZ(INC_PC, WR_ACC);

  USTEP_DEFINE(LDA_Z);
  USTEP_ADDR_Z;
  LD_NZ(ADDR_ZP, WR_ACC);

  USTEP_DEFINE(LDA_ZX);
  USTEP_ADDR_Z_IDX(OE_X);
  LD_NZ(ADDR_ZP, WR_ACC);

  USTEP_DEFINE(LDA_ABS);
  USTEP_ADDR_ABS;
  LD_NZ(ADDR_FULL, WR_ACC);

  USTEP_DEFINE(LDA_ABSX);
  USTEP_ADDR_ABS_IDX(OE_X);
  LD_NZ(ADDR_FULL, WR_ACC);

  USTEP_DEFINE(LDA_ABSY);
  USTEP_ADDR_ABS_IDX(OE_Y);
  LD_NZ(ADDR_FULL, WR_ACC);

  USTEP_DEFINE(LDA_INDX);
  USTEP_ADDR_ZX_IND;
  LD_NZ(ADDR_FULL, WR_ACC);

  USTEP_DEFINE(LDA_INDY);
  USTEP_ADDR_ZY_IND;
  LD_NZ(ADDR_FULL, WR_ACC);

  /*********************************************************************************************************************
   * STA - Store Accumulator in Memory (A -> M)
   * N Z C I D V
   * - - - - - -
   ********************************************************************************************************************/
  USTEP_DEFINE(STA_Z);
  USTEP_ADDR_Z;
  USTEP_LAST(OE_ACC | WR_RAM | ADDR_ZP);

  USTEP_DEFINE(STA_ZX);
  USTEP_ADDR_Z_IDX(OE_X);
  USTEP_LAST(OE_ACC | WR_RAM | ADDR_ZP);

  USTEP_DEFINE(STA_ABS);
  USTEP_ADDR_ABS;
  USTEP_LAST(OE_ACC | WR_RAM | ADDR_FULL);

  USTEP_DEFINE(STA_ABSX);
  USTEP_ADDR_ABS_IDX(OE_X);
  USTEP_LAST(OE_ACC | WR_RAM | ADDR_FULL);

  USTEP_DEFINE(STA_ABSY);
  USTEP_ADDR_ABS_IDX(OE_Y);
  USTEP_LAST(OE_ACC | WR_RAM | ADDR_FULL);

  USTEP_DEFINE(STA_INDX);
  USTEP_ADDR_ZX_IND;
  USTEP_LAST(OE_ACC | WR_RAM | ADDR_FULL);

  USTEP_DEFINE(STA_INDY);
  USTEP_ADDR_ZY_IND;
  USTEP_LAST(OE_ACC | WR_RAM | ADDR_FULL);

  /*********************************************************************************************************************
   * STX - Store Index X in Memory (X -> M)
   * N Z C I D V
   * - - - - - -
   ********************************************************************************************************************/
  USTEP_DEFINE(STX_Z);
  USTEP_ADDR_Z;
  USTEP_LAST(OE_X | WR_RAM | ADDR_ZP);

  USTEP_DEFINE(STX_ZY);
  USTEP_ADDR_Z_IDX(OE_Y);
  USTEP_LAST(OE_X | WR_RAM | ADDR_ZP);

  USTEP_DEFINE(STX_ABS);
  USTEP_ADDR_ABS;
  USTEP_LAST(OE_X | WR_RAM | ADDR_FULL);

  /*********************************************************************************************************************
   * STY - Sore Index Y in Memory (Y -> M)
   * N Z C I D V
   * - - - - - -
   ********************************************************************************************************************/
  USTEP_DEFINE(STY_Z);
  USTEP_ADDR_Z;
  USTEP_LAST(OE_Y | WR_RAM | ADDR_ZP);

  USTEP_DEFINE(STY_ZX);
  USTEP_ADDR_Z_IDX(OE_X);
  USTEP_LAST(OE_Y | WR_RAM | ADDR_ZP);

  USTEP_DEFINE(STY_ABS);
  USTEP_ADDR_ABS;
  USTEP_LAST(OE_Y | WR_RAM | ADDR_FULL);

  /*********************************************************************************************************************
   * LDX - Load Index X with Memory (M -> X)
   *
   * N Z C I D V
   * + + - - - -
   ********************************************************************************************************************/
  USTEP_DEFINE(LDX_IMM);
  LD_NZ(INC_PC, WR_X);

  USTEP_DEFINE(LDX_Z);
  USTEP_ADDR_Z;
  LD_NZ(ADDR_ZP, WR_X);

  USTEP_DEFINE(LDX_ZY);
  USTEP_ADDR_Z_IDX(OE_Y);
  LD_NZ(ADDR_ZP, WR_X);

  USTEP_DEFINE(LDX_ABS);
  USTEP_ADDR_ABS;
  LD_NZ(ADDR_FULL, WR_X);

  USTEP_DEFINE(LDX_ABSY);
  USTEP_ADDR_ABS_IDX(OE_Y);
  LD_NZ(ADDR_FULL, WR_X);

  /*********************************************************************************************************************
   * LDY - Load Index Y with Memory (M -> Y)
   *
   * N Z C I D V
   * + + - - - -
   ********************************************************************************************************************/
  USTEP_DEFINE(LDY_IMM);
  LD_NZ(INC_PC, WR_Y);

  USTEP_DEFINE(LDY_Z);
  USTEP_ADDR_Z;
  LD_NZ(ADDR_ZP, WR_Y);

  USTEP_DEFINE(LDY_ZX);
  USTEP_ADDR_Z_IDX(OE_X);
  LD_NZ(ADDR_ZP, WR_Y);

  USTEP_DEFINE(LDY_ABS);
  USTEP_ADDR_ABS;
  LD_NZ(ADDR_FULL, WR_Y);

  USTEP_DEFINE(LDY_ABSX);
  USTEP_ADDR_ABS_IDX(OE_X);
  LD_NZ(ADDR_FULL, WR_Y);

  /*********************************************************************************************************************
   * Flag Instructions
   ********************************************************************************************************************/
  // Clear flag
#define FLAG_CLR(flag) USTEP_LAST(OE_CONST | CONST_0  | WR_FLAGS | flag);
  // Set flag
#define FLAG_SET(flag) USTEP_LAST(OE_CONST | CONST_FF | WR_FLAGS | flag);

  USTEP_DEFINE(CLC);
  FLAG_CLR(UPD_C);

  USTEP_DEFINE(SEC);
  FLAG_SET(UPD_C);

  USTEP_DEFINE(CLD);
  FLAG_CLR(UPD_D);

  USTEP_DEFINE(SED);
  FLAG_SET(UPD_D);

  USTEP_DEFINE(CLI);
  FLAG_CLR(UPD_I);

  USTEP_DEFINE(SEI);
  FLAG_SET(UPD_I);

  USTEP_DEFINE(CLV);
  FLAG_CLR(UPD_V);

  /*********************************************************************************************************************
   * Register To Register Transfer
   ********************************************************************************************************************/
  USTEP_DEFINE(TAX);
  USTEP_LAST(OE_ACC | WR_X | UPD_NZ);

  USTEP_DEFINE(TXA);
  USTEP_LAST(OE_X | WR_ACC  | UPD_NZ);

  USTEP_DEFINE(TAY);
  USTEP_LAST(OE_ACC | WR_Y  | UPD_NZ);

  USTEP_DEFINE(TYA);
  USTEP_LAST(OE_Y | WR_ACC  | UPD_NZ);

  USTEP_DEFINE(TSX);
  USTEP_LAST(OE_SP | WR_X  | UPD_NZ);

  USTEP_DEFINE(TXS);
  USTEP_LAST(OE_X | WR_SP);

  /*********************************************************************************************************************
   * Stack Instructions
   ********************************************************************************************************************/
#define PUSH_REG(reg) \
  USTEP_LAST(reg | ADDR_SP | WR_RAM | DEC_SP)
#define PULL_REG(reg) \
  USTEP_STEP(INC_SP); \
  USTEP_LAST(OE_RAM | ADDR_SP | reg | UPD_NZ)

  USTEP_DEFINE(PHA);
  PUSH_REG(OE_ACC);

  USTEP_DEFINE(PLA);
  PULL_REG(WR_ACC);

  // The status register will be pushed with the break flag and bit 5 set to 1.
  USTEP_DEFINE(PHP);
  PUSH_REG(OE_FLAGS);

  // The status register will be pulled with the break flag and bit 5 ignored.
  USTEP_DEFINE(PLP);
  PULL_REG(WR_FLAGS | UPD_C | UPD_Z | UPD_I | UPD_D | UPD_V | UPD_N);

  /*********************************************************************************************************************
   * Jumps & Subroutines
   ********************************************************************************************************************/
  USTEP_DEFINE(JMP_ABS);
  // Jump address low byte -> ADDRL (PCL is needed to read next byte)
  USTEP_STEP(OE_RAM | ADDR_PC | INC_PC | WR_ADDRL);
  // Jump address low byte -> PCH
  USTEP_STEP(OE_RAM | ADDR_PC | WR_PCH);
  // ADDRL -> PCL
  USTEP_LAST(OE_ADDRL | WR_PCL);

  USTEP_DEFINE(JMP_IND);
  // Indirect address -> ADDRL and ADDRH
  USTEP_STEP(OE_RAM | ADDR_PC | INC_PC | WR_ADDRL);
  USTEP_STEP(OE_RAM | ADDR_PC | WR_ADDRH);
  // Jump address -> PCL and PCH (6502 page boundary bug here, ADDRH does not increment!)
  if(cpuType == CPU_6502) {
    USTEP_STEP(OE_RAM | ADDR_FULL | WR_PCL | INC_ADDRL);
  }
  // Fixed in 65C02
  else {
    USTEP_FLAG(
      FLAGIN_ALOV | OE_RAM | ADDR_FULL | WR_PCL | INC_ADDRL,
      FLAGIN_ALOV | OE_RAM | ADDR_FULL | WR_PCL | INC_ADDRL | INC_ADDRH
    );
  }
  USTEP_LAST(OE_RAM | ADDR_FULL | WR_PCH);

  USTEP_DEFINE(JSR);
  // Subroutine address low byte -> A
  USTEP_STEP(OE_RAM | ADDR_PC | INC_PC | WR_ALU_A);
  // Push PCH
  USTEP_STEP(OE_PCH | ADDR_SP | WR_RAM | DEC_SP);
  // Push PCL
  USTEP_STEP(OE_PCL | ADDR_SP | WR_RAM | DEC_SP);
  // Subroutine address high byte -> PCH
  USTEP_STEP(OE_RAM | ADDR_PC | WR_PCH );
  // A -> PCL
  USTEP_LAST(OE_ALU_A | WR_PCL );

  USTEP_DEFINE(RTS);
  // SP++
  USTEP_STEP(INC_SP);
  // Pop PCL, SP++
  USTEP_STEP(OE_RAM | ADDR_SP | WR_PCL | INC_SP);
  // Pop PCH
  USTEP_STEP(OE_RAM | ADDR_SP | WR_PCH);
  // PC++
  USTEP_LAST(INC_PC);

  // The status register is pulled with the break flag and bit 5 ignored. Then PC is pulled from the stack.
  USTEP_DEFINE(RTI);
  // SP++
  USTEP_STEP(INC_SP);
  // Pop flags, SP++
  USTEP_STEP(OE_RAM | ADDR_SP | WR_FLAGS | UPD_C | UPD_Z | UPD_I | UPD_D | UPD_V | UPD_N | INC_SP);
  // Pop PCL, SP++
  USTEP_STEP(OE_RAM | ADDR_SP | WR_PCL | INC_SP);
  // Pop PCH
  USTEP_LAST(OE_RAM | ADDR_SP | WR_PCH);

  /*********************************************************************************************************************
   * Conditional Branch Instructions
   ********************************************************************************************************************/
  // Branch offset -> B and PC++
#define BRANCH_LOAD_B (OE_RAM | ADDR_PC | INC_PC | WR_ALU_B)
  // Do the relative branching
#define BRANCH_DO_BRANCH \
  /* PCL -> A */ \
  USTEP_STEP(OE_PCL | WR_ALU_A); \
  /* offset + PCL -> PCL */ \
  USTEP_STEP(OE_ALU | ALU_ADD | WR_PCL); \
  /* Sign extend offset into B */ \
  USTEP_FLAG( \
    /* If B positive,  0 -> B */ \
    FLAGIN_NB | OE_CONST | CONST_0  | WR_ALU_B, \
    /* If B negative, FF -> B */ \
    FLAGIN_NB | OE_CONST | CONST_FF | WR_ALU_B \
  ); \
  /* PCH -> A */ \
  USTEP_STEP(OE_PCH | WR_ALU_A); \
  /* Add PCH with sign extension in B and carry */ \
  USTEP_FLST( \
    FLAGIN_CALU | OE_ALU | ALU_ADD | WR_PCH, \
    FLAGIN_CALU | OE_ALU | ALU_ADD | WR_PCH | CI_ALU \
  )
  // Take the branch if flag is clear
#define BRANCH_ON_FLAG_0(flag) \
  USTEP_FLAG( \
    /* If flag clear, branch offset -> B and PC++ */ \
    flag | BRANCH_LOAD_B, \
    /* If flag set, PC++ and end of uCode */ \
    flag | INC_PC | RST_USTEP \
  )
  // Take the branch if flag is set
#define BRANCH_ON_FLAG_1(flag) \
  USTEP_FLAG( \
    /* If flag clear, PC++ and end of uCode */ \
    flag | INC_PC | RST_USTEP, \
    /* If flag set, branch offset -> B and PC++ */ \
    flag | BRANCH_LOAD_B \
  )
  // Branch depending of flag value
#define BRANCH_ON_FLAG(flag, onvalue) \
  BRANCH_ON_FLAG_##onvalue(flag); \
  BRANCH_DO_BRANCH

  USTEP_DEFINE(BCC);
  BRANCH_ON_FLAG(FLAGIN_C, 0);

  USTEP_DEFINE(BCS);
  BRANCH_ON_FLAG(FLAGIN_C, 1);

  USTEP_DEFINE(BNE);
  BRANCH_ON_FLAG(FLAGIN_Z, 0);

  USTEP_DEFINE(BEQ);
  BRANCH_ON_FLAG(FLAGIN_Z, 1);

  USTEP_DEFINE(BPL);
  BRANCH_ON_FLAG(FLAGIN_N, 0);

  USTEP_DEFINE(BMI);
  BRANCH_ON_FLAG(FLAGIN_N, 1);

  USTEP_DEFINE(BVC);
  BRANCH_ON_FLAG(FLAGIN_V, 0);

  USTEP_DEFINE(BVS);
  BRANCH_ON_FLAG(FLAGIN_V, 1);

  /*********************************************************************************************************************
   * BRK
   *********************************************************************************************************************
   * BRK initiates a software interrupt similar to a hardware interrupt (IRQ). The return address pushed to the stack is
   * PC+2, providing an extra byte of spacing for a break mark (identifying a reason for the break.) The status register
   * will be pushed to the stack with the break flag set to 1. However, when retrieved during RTI or by a PLP
   * instruction, the break flag will be ignored. The interrupt disable flag is not set automatically.
   *********************************************************************************************************************
   * Interrupts
   *********************************************************************************************************************
   * A hardware interrupt (maskable IRQ and non-maskable NMI), will cause the processor to put first the address
   * currently in the program counter onto the stack (in HB-LB order), followed by the value of the status register.
   * (The stack will now contain, seen from the bottom or from the most recently added byte, SR PC-L PC-H with the stack
   * pointer pointing to the address below the stored contents of status register.) Then, the processor will divert its
   * control flow to the address provided in the two word-size interrupt vectors at $FFFA (IRQ) and $FFFE (NMI).
   * A set interrupt disable flag will inhibit the execution of an IRQ, but not of a NMI, which will be executed
   * anyways.
   * The break instruction (BRK) behaves like a NMI, but will push the value of PC+2 onto the stack to be used as the
   * return address. Also, as with any software initiated transfer of the status register to the stack, the break flag
   * will be found set on the respective value pushed onto the stack. Then, control is transferred to the address in the
   * NMI-vector at $FFFE.
   * In any way, the interrupt disable flag is set to inhibit any further IRQ as control is transferred to the interrupt
   * handler specified by the respective interrupt vector.
   * The RTI instruction restores the status register from the stack and behaves otherwise like the JSR instruction.
   * (The break flag is always ignored as the status is read from the stack, as it isn't a real processor flag anyway.)
   ********************************************************************************************************************/
  USTEP_DEFINE(BRK);
  // Increment PC if software interrupt
  USTEP_FLAG(
    INT_HOLD | FLAGIN_INTR | INC_PC,
    INT_HOLD | FLAGIN_INTR
  );
  // Push PCH if not reset
  USTEP_FLAG(
    INT_HOLD | FLAGIN_RST | OE_PCH | ADDR_SP | WR_RAM | DEC_SP,
    INT_HOLD | FLAGIN_RST
  );
  // Push PCL if not reset
  USTEP_FLAG(
    INT_HOLD | FLAGIN_RST | OE_PCL | ADDR_SP | WR_RAM | DEC_SP,
    INT_HOLD | FLAGIN_RST
  );
  // Push flags if not reset
  USTEP_FLAG(
    INT_HOLD | FLAGIN_RST | OE_FLAGS | WR_RAM | ADDR_SP | DEC_SP,
    INT_HOLD | FLAGIN_RST
  );

  // 6502: Set interrupt disable flag only
  if(cpuType == CPU_6502) {
    USTEP_STEP(INT_HOLD | OE_CONST | CONST_FF | WR_FLAGS | UPD_I);
  }
  // 65C02: Set interrupt disable flag and clear decimal flag
  else {
    USTEP_STEP(INT_HOLD | OE_CONST | CONST_4  | WR_FLAGS | UPD_I | UPD_D);
  }

  // Fetch interrupt vector
  USTEP_STEP(INT_HOLD | OE_CONST| CONST_FF | WR_ADDRH);
  // Interrupt or reset?
  USTEP_FLAG(
    // Interrupt vector
    INT_HOLD | FLAGIN_RST | OE_CONST| CONST_FE | WR_ADDRL,
    // Reset vector
    INT_HOLD | FLAGIN_RST | OE_CONST| CONST_FC | WR_ADDRL
  );
  // NMI
  USTEP_FLAG(
    INT_HOLD | FLAGIN_NMI,
    INT_HOLD | FLAGIN_NMI | OE_CONST| CONST_FA | WR_ADDRL
  );
  USTEP_STEP(OE_RAM | ADDR_FULL | WR_PCL | INC_ADDRL);
  USTEP_LAST(OE_RAM | ADDR_FULL | WR_PCH);

  /*********************************************************************************************************************
   * NOP
   ********************************************************************************************************************/
  USTEP_DEFINE(NOP);
  USTEP_LAST(0);
}

/***********************************************************************************************************************
 * Generate 65C02 microcode
 **********************************************************************************************************************/
static void GenerateUcode65C02(void)
{
  // 65C02 extra instruction opcodes
  enum {
    // 6502 instructions with zero page indirect addressing, INSTR (zpaddr), 2 bytes, 5 cycles
    ADC_INDZ  = 0x72,
    AND_INDZ  = 0x32,
    CMP_INDZ  = 0xD2,
    EOR_INDZ  = 0x52,
    LDA_INDZ  = 0xB2,
    ORA_INDZ  = 0x12,
    SBC_INDZ  = 0xF2,
    STA_INDZ  = 0x92,

    // BIT - Test Bits in Memory with Accumulator, other addressing modes (A AND M, M7 -> N, M6 -> V)
    //  N Z C I D V
    // M7 + - - - M6
    // ENUM   = Opcode   | Addressing   | Assembler    | B | Cycles
    BIT_IMM   = 0x89, // | immediate    | BIT #oper    | 2 | 2      Does not affect N and V flags!
    BIT_ZX    = 0x34, // | zeropage,X   | BIT oper,X   | 2 | 4
    BIT_ABSX  = 0x3C, // | absolute,X   | BIT oper,X   | 3 | 4*

    // ACC increment / decrement
    // N Z C I D V
    // + + - - - -
    // ENUM   = Opcode   | Addressing   | Assembler    | B | C | Operation
    DEC_A     = 0x3A, // | implied      | DEC A / DEA  | 1 | 2 | A - 1 -> A
    INC_A     = 0x1A, // | implied      | INC A / INA  | 1 | 2 | A + 1 -> A

    // JMP with absolute indexed indirect addressing, JMP (addr,x), 3 bytes, 6 cycles
    JMP_INDX  = 0x7C,

    // Branch Relative Always, 2 bytes, 3 - 4 cycles
    BRA       = 0x80,

    // Stack Instructions
    // ENUM   = Opcode   | Addressing   | Assembler    | B | C | Operation | N Z C I D V
    PHX       = 0xDA, // | implied      | PHX          | 1 | 3 | push X    | - - - - - -
    PLX       = 0xFA, // | implied      | PLX          | 1 | 4 | pull X    | + + - - - -
    PHY       = 0x5A, // | implied      | PHY          | 1 | 3 | push Y    | - - - - - -
    PLY       = 0x7A, // | implied      | PLY          | 1 | 4 | pull Y    | + + - - - -

    // STZ - Store Zero in Memory (0 -> M)
    // N Z C I D V
    // - - - - - -
    // ENUM   = Opcode   | Addressing   | Assembler    | B | Cycles
    STZ_Z     = 0x64, // | zeropage     | STZ oper     | 2 | 3
    STZ_ZX    = 0x74, // | zeropage,X   | STZ oper,X   | 2 | 4
    STZ_ABS   = 0x9C, // | absolute     | STZ oper     | 3 | 4
    STZ_ABSX  = 0x9E, // | absolute,X   | STZ oper,X   | 3 | 5

    // TSB - Test & Set memory Bits with A. (M | A -> M, M & A -> Z)
    // N Z C I D V
    // - + - - - -
    // ENUM   = Opcode   | Addressing   | Assembler    | B | Cycles
    TSB_Z     = 0x04, // | zeropage     | TSB oper     | 2 | 5
    TSB_ABS   = 0x0C, // | absolute     | TSB oper     | 3 | 6

    // TRB - Test & Reset memory Bits with A. (M & ~A -> M, M & A -> Z)
    // N Z C I D V
    // - + - - - -
    // ENUM   = Opcode   | Addressing   | Assembler    | B | Cycles
    TRB_Z     = 0x14, // | zeropage     | TRB oper     | 2 | 5
    TRB_ABS   = 0x1C, // | absolute     | TRB oper     | 3 | 6

    // RMB - Reset Zero Page Memory Bit, 2 bytes, 5 cycles
    RMB0      = 0x07,
    RMB1      = 0x17,
    RMB2      = 0x27,
    RMB3      = 0x37,
    RMB4      = 0x47,
    RMB5      = 0x57,
    RMB6      = 0x67,
    RMB7      = 0x77,

    // SMB - Set Zero Page Memory Bit, 2 bytes, 5 cycles
    SMB0      = 0x87,
    SMB1      = 0x97,
    SMB2      = 0xA7,
    SMB3      = 0xB7,
    SMB4      = 0xC7,
    SMB5      = 0xD7,
    SMB6      = 0xE7,
    SMB7      = 0xF7,

    // BBR - Branch if zero page memory bit is reset, 3 bytes, 5 - 7 cycles
    BBR0      = 0x0F,
    BBR1      = 0x1F,
    BBR2      = 0x2F,
    BBR3      = 0x3F,
    BBR4      = 0x4F,
    BBR5      = 0x5F,
    BBR6      = 0x6F,
    BBR7      = 0x7F,

    // BBS - Branch if zero page memory bit is set, 3 bytes, 5 - 7 cycles
    BBS0      = 0x8F,
    BBS1      = 0x9F,
    BBS2      = 0xAF,
    BBS3      = 0xBF,
    BBS4      = 0xCF,
    BBS5      = 0xDF,
    BBS6      = 0xEF,
    BBS7      = 0xFF,

    // SToP the processor until the next RST.
    STP       = 0xDB,

    // WAIt for interrupt
    WAI       = 0xCB,
  };

  /*********************************************************************************************************************
   * Zero page indirect addressing
   ********************************************************************************************************************/
  USTEP_DEFINE(ADC_INDZ);
  USTEP_ADDR_Z_IND;
  ADD_ACC(ADDR_FULL);

  USTEP_DEFINE(AND_INDZ);
  USTEP_ADDR_Z_IND;
  AND_ACC(ADDR_FULL);

  USTEP_DEFINE(CMP_INDZ);
  USTEP_ADDR_Z_IND;
  CMP(OE_ACC, ADDR_FULL);

  USTEP_DEFINE(EOR_INDZ);
  USTEP_ADDR_Z_IND;
  EOR_ACC(ADDR_FULL);

  USTEP_DEFINE(LDA_INDZ);
  USTEP_ADDR_Z_IND;
  LD_NZ(ADDR_FULL, WR_ACC);

  USTEP_DEFINE(ORA_INDZ);
  USTEP_ADDR_Z_IND;
  ORA_ACC(ADDR_FULL);

  USTEP_DEFINE(SBC_INDZ);
  USTEP_ADDR_Z_IND;
  SUB_ACC(ADDR_FULL);

  USTEP_DEFINE(STA_INDZ);
  USTEP_ADDR_Z_IND;
  USTEP_LAST(OE_ACC | WR_RAM | ADDR_FULL);

  /*********************************************************************************************************************
   * BIT instruction with other addressing modes
   ********************************************************************************************************************/
  // Attention! BIT #IMM does not affect N and V flags!
  USTEP_DEFINE(BIT_IMM);
  // Memory -> A
  USTEP_STEP(OE_RAM | ADDR_PC | INC_PC | WR_ALU_A);
  // ACC -> B
  USTEP_STEP(OE_ACC | WR_ALU_B);
  // ACC AND Memory -> Z
  USTEP_LAST(OE_ALU | ALU_AND | UPD_Z);

  USTEP_DEFINE(BIT_ZX);
  USTEP_ADDR_Z_IDX(OE_X);
  BIT(ADDR_ZP);

  USTEP_DEFINE(BIT_ABSX);
  USTEP_ADDR_ABS_IDX(OE_X);
  BIT(ADDR_FULL);

  /*********************************************************************************************************************
   * ACC increment / decrement
   * N Z C I D V
   * + + - - - -
   ********************************************************************************************************************/
  USTEP_DEFINE(INC_A);
  INC(OE_ACC, WR_ACC);

  USTEP_DEFINE(DEC_A);
  DEC(OE_ACC, WR_ACC);

  /*********************************************************************************************************************
   * JMP (oper,x)
   ********************************************************************************************************************/
  USTEP_DEFINE(JMP_INDX);
  // Indirect address -> ADDRL and ADDRH
  USTEP_ADDR_ABS_IDX(OE_X);
  // Jump address -> PCL and PCH
  USTEP_FLAG(
    FLAGIN_ALOV | OE_RAM | ADDR_FULL | WR_PCL | INC_ADDRL,
    FLAGIN_ALOV | OE_RAM | ADDR_FULL | WR_PCL | INC_ADDRL | INC_ADDRH
  );
  USTEP_LAST(OE_RAM | ADDR_FULL | WR_PCH);

  /*********************************************************************************************************************
   * Branch Relative Always
   ********************************************************************************************************************/
  USTEP_DEFINE(BRA);
  USTEP_STEP(BRANCH_LOAD_B);
  BRANCH_DO_BRANCH;

  /*********************************************************************************************************************
   * Stack Instructions
   ********************************************************************************************************************/
  USTEP_DEFINE(PHX);
  PUSH_REG(OE_X);

  USTEP_DEFINE(PLX);
  PULL_REG(WR_X);

  USTEP_DEFINE(PHY);
  PUSH_REG(OE_Y);

  USTEP_DEFINE(PLY);
  PULL_REG(WR_Y);

  /*********************************************************************************************************************
   * STZ - Store Zero in Memory (0 -> M)
   * N Z C I D V
   * - - - - - -
   ********************************************************************************************************************/
  USTEP_DEFINE(STZ_Z);
  USTEP_ADDR_Z;
  USTEP_LAST(OE_CONST | CONST_0 | WR_RAM | ADDR_ZP);

  USTEP_DEFINE(STZ_ZX);
  USTEP_ADDR_Z_IDX(OE_X);
  USTEP_LAST(OE_CONST | CONST_0 | WR_RAM | ADDR_ZP);

  USTEP_DEFINE(STZ_ABS);
  USTEP_ADDR_ABS;
  USTEP_LAST(OE_CONST | CONST_0 | WR_RAM | ADDR_FULL);

  USTEP_DEFINE(STZ_ABSX);
  USTEP_ADDR_ABS_IDX(OE_X);
  USTEP_LAST(OE_CONST | CONST_0 | WR_RAM | ADDR_FULL);

  /*********************************************************************************************************************
   * TRB - Test & Reset memory Bits with A. (M & ~A -> M, M & A -> Z)
   * N Z C I D V
   * - + - - - -
   ********************************************************************************************************************/
#define TRB(src, dst) \
  /* ACC -> A */ \
  USTEP_STEP(OE_ACC | WR_ALU_A); \
  /* MEM -> B */ \
  USTEP_STEP(src | WR_ALU_B); \
  /* ACC & MEM -> Z */ \
  USTEP_STEP(OE_ALU | ALU_AND | UPD_Z); \
  /* ~A -> A */ \
  USTEP_STEP(OE_ALU | ALU_INVA | WR_ALU_A); \
  /* Clear bits in memory */ \
  USTEP_LAST(OE_ALU | ALU_AND | dst)

  USTEP_DEFINE(TRB_Z);
  USTEP_ADDR_Z;
  TRB(OE_RAM | ADDR_ZP, WR_RAM | ADDR_ZP);

  USTEP_DEFINE(TRB_ABS);
  USTEP_ADDR_ABS;
  TRB(OE_RAM | ADDR_FULL, WR_RAM | ADDR_FULL);

  /*********************************************************************************************************************
   * TSB - Test & Set memory Bits with A. (M | A -> M, M & A -> Z)
   * N Z C I D V
   * - + - - - -
   ********************************************************************************************************************/
#define TSB(src, dst) \
  /* ACC -> A */ \
  USTEP_STEP(OE_ACC | WR_ALU_A); \
  /* MEM -> B */ \
  USTEP_STEP(src | WR_ALU_B); \
  /* ACC & MEM -> Z */ \
  USTEP_STEP(OE_ALU | ALU_AND | UPD_Z); \
  /* Set bits in memory */ \
  USTEP_LAST(OE_ALU | ALU_OR | dst)

  USTEP_DEFINE(TSB_Z);
  USTEP_ADDR_Z;
  TSB(OE_RAM | ADDR_ZP, WR_RAM | ADDR_ZP);

  USTEP_DEFINE(TSB_ABS);
  USTEP_ADDR_ABS;
  TSB(OE_RAM | ADDR_FULL, WR_RAM | ADDR_FULL);

  /*********************************************************************************************************************
   * RMB - Reset Memory Bit
   ********************************************************************************************************************/
#define SRMB(const, op) \
  USTEP_ADDR_Z; \
  /* Bit number to manipulate -> B */ \
  USTEP_STEP(OE_CONST | const | WR_ALU_B); \
  /* MEM -> A */ \
  USTEP_STEP(OE_RAM | ADDR_ZP | WR_ALU_A); \
  /* Write back changed bits */ \
  USTEP_LAST(OE_ALU | op | WR_RAM | ADDR_ZP)

#define RMB(bit) \
  USTEP_DEFINE(RMB##bit); \
  SRMB(CONST_##bit, ALU_CLRBIT)

  RMB(0);
  RMB(1);
  RMB(2);
  RMB(3);
  RMB(4);
  RMB(5);
  RMB(6);
  RMB(7);

  /*********************************************************************************************************************
   * SMB - Set Memory Bit
   ********************************************************************************************************************/
#define SMB(bit) \
  USTEP_DEFINE(SMB##bit); \
  SRMB(CONST_##bit, ALU_SETBIT)

  SMB(0);
  SMB(1);
  SMB(2);
  SMB(3);
  SMB(4);
  SMB(5);
  SMB(6);
  SMB(7);

  /*********************************************************************************************************************
   * BBR - Branch if zero page memory bit is reset
   ********************************************************************************************************************/
#define TEST_BIT(const) \
    USTEP_ADDR_Z; \
    /* Bit number to test into B */ \
    USTEP_STEP(OE_CONST | const | WR_ALU_B); \
    /* MEM -> A */ \
    USTEP_STEP(OE_RAM | ADDR_ZP | WR_ALU_A); \
    /* Test bit and update internal zero bit (ZALU) */ \
    USTEP_STEP(OE_ALU | ALU_TSTBIT)

#define BBR(bit) \
  USTEP_DEFINE(BBR##bit); \
  TEST_BIT(CONST_##bit); \
  BRANCH_ON_FLAG(FLAGIN_ZALU, 1)

  BBR(0);
  BBR(1);
  BBR(2);
  BBR(3);
  BBR(4);
  BBR(5);
  BBR(6);
  BBR(7);

  /*********************************************************************************************************************
   * BBS - Branch if zero page memory bit is set
   ********************************************************************************************************************/
#define BBS(bit) \
  USTEP_DEFINE(BBS##bit); \
  TEST_BIT(CONST_##bit); \
  BRANCH_ON_FLAG(FLAGIN_ZALU, 0)

  BBS(0);
  BBS(1);
  BBS(2);
  BBS(3);
  BBS(4);
  BBS(5);
  BBS(6);
  BBS(7);

  /*********************************************************************************************************************
   * SToP the processor until the next RST.
   ********************************************************************************************************************/
  USTEP_DEFINE(STP);
  uStep[_inst] = 0;
  USTEP_FLST(
    FLAGIN_RST,
    FLAGIN_RST | OE_CONST | CONST_BRK | WR_INST
  );

  /*********************************************************************************************************************
   * WAIt for interrupt
   ********************************************************************************************************************/
  USTEP_DEFINE(WAI);
  uStep[_inst] = 0;
  USTEP_FLST(
    FLAGIN_INTR,
    FLAGIN_INTR | OE_RAM | WR_INST
  );
}

/***********************************************************************************************************************
 * Print one uCode step
 **********************************************************************************************************************/
static void UcodePrintStep(unsigned int same, unsigned int last, unsigned int lasti)
{
  // Print instruction opcode and mnemonic at the beginning of each opcode
  if((lasti % USTEP_MAX) == 0) {
    unsigned i = (lasti / USTEP_MAX) % INSTR_MAX;
    char *instName = instr[i];
    printf("\n");
    if(instName != NULL) {
      printf("# %02X - %s\n", i, instName);
    }
  }
  else {
    printf(" ");
  }

  // Print one or more same steps
  if(same == 1) {
    printf("%X", last);
  }
  else {
    printf("%u*%X", same, last);
  }
}

/***********************************************************************************************************************
 * Print uCode as RLE encoded hex file
 **********************************************************************************************************************/
static void UcodePrintRLE(void)
{
  unsigned int n = 0;
  printf("v2.0 raw\n\n");

  // Instruction Matrix
#if 0
  printf("#   |");
  for(unsigned int i = 0; i < 16; i++) {
    printf("%8X | ", i);
  }
  for(unsigned int i = 0; i < INSTR_MAX; i++) {
    if((i % 16) == 0) {
      printf("\n# %X |", i / 16);
    }
    printf("%-8s | ", (instr[i] != NULL) ? instr[i] : "");
  }
  printf("\n\n");
#endif

  // Instruction statistics
  printf("# CPU: %s\n# Instr.   | Op | Cycles\n", (cpuType == CPU_6502) ? "6502" : "65C02");
  for(unsigned int i = 0; i < INSTR_MAX; i++) {
    if(instr[i] != NULL) {
      n++;
      printf("# %-8s | ", instr[i]);
      printf("%02X | ", i);
      printf("%2u\n", uStep[i]);
    }
  }
  printf("# Total: %u\n", n);

  // Print all uSteps for all opcodes
  uint32_t last = rom[0];
  unsigned int same = 0, lasti = 0;
  for(unsigned int i = 0; i < UCODE_SIZE; i++) {
    if(rom[i] == last) {
      same++;
    }
    else {
      UcodePrintStep(same, last, lasti);
      same = 1;
    }
    last = rom[i];
    lasti = i;
  }
  UcodePrintStep(same, last, lasti);
  printf("\n");
}

/***********************************************************************************************************************
 * Generate proper microcode for all supported CPUs to stdout
 **********************************************************************************************************************/
int main(int argv, char *argc[])
{
  // Determine which CPU type we generate
  if(argv == 2) {
    if(strcmp(argc[1], "--65c02") == 0) {
      cpuType = CPU_65C02;
    }
  }

  // Fill with end ucode
  for(unsigned int i = 0; i < UCODE_SIZE; i++) {
    rom[i] = RST_USTEP;
  }

  // Fill each uStep 0 with Fetch and interrupt detection
  for(unsigned int i = 0; i < INSTR_MAX; i ++) {
    // No interrupt
    rom[(0 << ROM_FLAGS_POS) | (i << ROM_INSTR_POS)] = FLAGIN_INTR | OE_RAM | WR_INST | INC_PC;
    // Interrupt asserted, inject BRK opcode
    rom[(1 << ROM_FLAGS_POS) | (i << ROM_INSTR_POS)] = FLAGIN_INTR | OE_CONST | CONST_BRK | WR_INST | INT_HOLD;
  }

  // Generate 6502 uCode
  GenerateUcode6502();

  // Generate 65C02 uCode
  if(cpuType == CPU_65C02) {
    GenerateUcode65C02();
  }

  // Dump uCode to stdout
  UcodePrintRLE();

  return 0;
}
