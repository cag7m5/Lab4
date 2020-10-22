#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "mu-mips.h"

/***************************************************************/
/* Print out a list of commands available                                                                  */
/***************************************************************/
void help() {        
	printf("------------------------------------------------------------------\n\n");
	printf("\t**********MU-MIPS Help MENU**********\n\n");
	printf("sim\t-- simulate program to completion \n");
	printf("run <n>\t-- simulate program for <n> instructions\n");
	printf("rdump\t-- dump register values\n");
	printf("reset\t-- clears all registers/memory and re-loads the program\n");
	printf("input <reg> <val>\t-- set GPR <reg> to <val>\n");
	printf("mdump <start> <stop>\t-- dump memory from <start> to <stop> address\n");
	printf("high <val>\t-- set the HI register to <val>\n");
	printf("low <val>\t-- set the LO register to <val>\n");
	printf("print\t-- print the program loaded into memory\n");
	printf("show\t-- print the current content of the pipeline registers\n");
	printf("?\t-- display help menu\n");
	printf("quit\t-- exit the simulator\n\n");
	printf("------------------------------------------------------------------\n\n");
}

/***************************************************************/
/* Read a 32-bit word from memory                                                                            */
/***************************************************************/
uint32_t mem_read_32(uint32_t address)
{
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) &&  ( address <= MEM_REGIONS[i].end) ) {
			uint32_t offset = address - MEM_REGIONS[i].begin;
			return (MEM_REGIONS[i].mem[offset+3] << 24) |
					(MEM_REGIONS[i].mem[offset+2] << 16) |
					(MEM_REGIONS[i].mem[offset+1] <<  8) |
					(MEM_REGIONS[i].mem[offset+0] <<  0);
		}
	}
	return 0;
}

/***************************************************************/
/* Write a 32-bit word to memory                                                                                */
/***************************************************************/
void mem_write_32(uint32_t address, uint32_t value)
{
	int i;
	uint32_t offset;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) && (address <= MEM_REGIONS[i].end) ) {
			offset = address - MEM_REGIONS[i].begin;

			MEM_REGIONS[i].mem[offset+3] = (value >> 24) & 0xFF;
			MEM_REGIONS[i].mem[offset+2] = (value >> 16) & 0xFF;
			MEM_REGIONS[i].mem[offset+1] = (value >>  8) & 0xFF;
			MEM_REGIONS[i].mem[offset+0] = (value >>  0) & 0xFF;
		}
	}
}

/***************************************************************/
/* Execute one cycle                                                                                                              */
/***************************************************************/
void cycle() {                                                
	handle_pipeline();
	CURRENT_STATE = NEXT_STATE;
	CYCLE_COUNT++;
}

/***************************************************************/
/* Simulate MIPS for n cycles                                                                                       */
/***************************************************************/
void run(int num_cycles) {                                      
	
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped\n\n");
		return;
	}

	printf("Running simulator for %d cycles...\n\n", num_cycles);
	int i;
	for (i = 0; i < num_cycles; i++) {
		if (RUN_FLAG == FALSE) {
			printf("Simulation Stopped.\n\n");
			break;
		}
		cycle();
	}
}

/***************************************************************/
/* simulate to completion                                                                                               */
/***************************************************************/
void runAll() {                                                     
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped.\n\n");
		return;
	}

	printf("Simulation Started...\n\n");
	while (RUN_FLAG){
		cycle();
	}
	printf("Simulation Finished.\n\n");
}

/***************************************************************/ 
/* Dump a word-aligned region of memory to the terminal                              */
/***************************************************************/
void mdump(uint32_t start, uint32_t stop) {          
	uint32_t address;

	printf("-------------------------------------------------------------\n");
	printf("Memory content [0x%08x..0x%08x] :\n", start, stop);
	printf("-------------------------------------------------------------\n");
	printf("\t[Address in Hex (Dec) ]\t[Value]\n");
	for (address = start; address <= stop; address += 4){
		printf("\t0x%08x (%d) :\t0x%08x\n", address, address, mem_read_32(address));
	}
	printf("\n");
}

/***************************************************************/
/* Dump current values of registers to the teminal                                              */   
/***************************************************************/
void rdump() {                               
	int i; 
	printf("-------------------------------------\n");
	printf("Dumping Register Content\n");
	printf("-------------------------------------\n");
	printf("# Instructions Executed\t: %u\n", INSTRUCTION_COUNT);
	printf("# Cycles Executed\t: %u\n", CYCLE_COUNT);
	printf("PC\t: 0x%08x\n", CURRENT_STATE.PC);
	printf("-------------------------------------\n");
	printf("[Register]\t[Value]\n");
	printf("-------------------------------------\n");
	for (i = 0; i < MIPS_REGS; i++){
		printf("[R%d]\t: 0x%08x\n", i, CURRENT_STATE.REGS[i]);
	}
	printf("-------------------------------------\n");
	printf("[HI]\t: 0x%08x\n", CURRENT_STATE.HI);
	printf("[LO]\t: 0x%08x\n", CURRENT_STATE.LO);
	printf("-------------------------------------\n");
}

/***************************************************************/
/* Read a command from standard input.                                                               */  
/***************************************************************/
void handle_command() {                         
	char buffer[20];
	uint32_t start, stop, cycles;
	uint32_t register_no;
	int register_value;
	int hi_reg_value, lo_reg_value;

	printf("MU-MIPS SIM:> ");

	if (scanf("%s", buffer) == EOF){
		exit(0);
	}

	switch(buffer[0]) {
		case 'S':
		case 's':
			if (buffer[1] == 'h' || buffer[1] == 'H'){
				show_pipeline();
			}else {
				runAll(); 
			}
			break;
		case 'M':
		case 'm':
			if (scanf("%x %x", &start, &stop) != 2){
				break;
			}
			mdump(start, stop);
			break;
		case '?':
			help();
			break;
		case 'Q':
		case 'q':
			printf("**************************\n");
			printf("Exiting MU-MIPS! Good Bye...\n");
			printf("**************************\n");
			exit(0);
		case 'R':
		case 'r':
			if (buffer[1] == 'd' || buffer[1] == 'D'){
				rdump();
			}else if(buffer[1] == 'e' || buffer[1] == 'E'){
				reset();
			}
			else {
				if (scanf("%d", &cycles) != 1) {
					break;
				}
				run(cycles);
			}
			break;
		case 'I':
		case 'i':
			if (scanf("%u %i", &register_no, &register_value) != 2){
				break;
			}
			CURRENT_STATE.REGS[register_no] = register_value;
			NEXT_STATE.REGS[register_no] = register_value;
			break;
		case 'H':
		case 'h':
			if (scanf("%i", &hi_reg_value) != 1){
				break;
			}
			CURRENT_STATE.HI = hi_reg_value; 
			NEXT_STATE.HI = hi_reg_value; 
			break;
		case 'L':
		case 'l':
			if (scanf("%i", &lo_reg_value) != 1){
				break;
			}
			CURRENT_STATE.LO = lo_reg_value;
			NEXT_STATE.LO = lo_reg_value;
			break;
		case 'P':
		case 'p':
			print_program(); 
			break;
		default:
			printf("Invalid Command.\n");
			break;
	}
}

/***************************************************************/
/* reset registers/memory and reload program                                                    */
/***************************************************************/
void reset() {   
	int i;
	/*reset registers*/
	for (i = 0; i < MIPS_REGS; i++){
		CURRENT_STATE.REGS[i] = 0;
	}
	CURRENT_STATE.HI = 0;
	CURRENT_STATE.LO = 0;
	
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
	
	/*load program*/
	load_program();
	
	/*reset PC*/
	INSTRUCTION_COUNT = 0;
	CURRENT_STATE.PC =  MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/***************************************************************/
/* Allocate and set memory to zero                                                                            */
/***************************************************************/
void init_memory() {                                           
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		MEM_REGIONS[i].mem = malloc(region_size);
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
}

/**************************************************************/
/* load program into memory                                                                                      */
/**************************************************************/
void load_program() {                   
	FILE * fp;
	int i, word;
	uint32_t address;

	/* Open program file. */
	fp = fopen(prog_file, "r");
	if (fp == NULL) {
		printf("Error: Can't open program file %s\n", prog_file);
		exit(-1);
	}

	/* Read in the program. */

	i = 0;
	while( fscanf(fp, "%x\n", &word) != EOF ) {
		address = MEM_TEXT_BEGIN + i;
		mem_write_32(address, word);
		printf("writing 0x%08x into address 0x%08x (%d)\n", word, address, address);
		i += 4;
	}
	PROGRAM_SIZE = i/4;
	printf("Program loaded into memory.\n%d words written into memory.\n\n", PROGRAM_SIZE);
	fclose(fp);
}

/************************************************************/
/* maintain the pipeline                                                                                           */ 
/************************************************************/
void handle_pipeline()
{
	/*INSTRUCTION_COUNT should be incremented when instruction is done*/
	/*Since we do not have branch/jump instructions, INSTRUCTION_COUNT should be incremented in WB stage */
	
	WB();
	MEM();
	EX();
	ID();
	IF();
}

/************************************************************/
/* writeback (WB) pipeline stage:                                                                          */ 
/************************************************************/
void WB()
{
	/*IMPLEMENT THIS*/
	//Fifth stage
	
	MEM_WB.IR = EX_MEM.IR;
	MEM_WB.ALUOutput = EX_MEM.ALUOutput;
	MEM_WB.LMD = 0;
	
	int opcode = (MEM_WB.IR & 0xFC000000) >> 26;	//Shift left to get opcode bits 26-31
	int funct = MEM_WB.IR & 0x0000003F;	//Get first 6 bits for function code
	int rt = (instruction & 0x001F0000) >> 16;
	int rd = (instruction & 0x0000F800) >> 11;

	if (opcode == 0x00) {	 //R-type instruction
		switch(funct) {
			case 0x00:	//SLL
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x02:	//SRL
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput; 
				INSTRUCTION_COUNT++;
				break;
				
			case 0x03:	//SRA
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x08:	//JR
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x09:	//JALR
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x0C:	//SYSCALL
				break;
				
			case 0x10:	//MFHI
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;  
				INSTRUCTION_COUNT++;
				break;
				
			case 0x11:	//MTHI
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x12:	//MFLO
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;	
				INSTRUCTION_COUNT++;
				break;
				
			case 0x13:	//MTLO
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x18:	//MULT
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x19:	//MULTU
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x1A:	//DIV
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x1B:	//DIVU
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x20:	//ADD
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x21:	//ADDU
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x22:	//SUB
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x23:	//SUBU
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x24:	//AND
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x25:	//OR
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x26:	//XOR
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x27:	//NOR
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x2A:	//SLT
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			default:
				printf("R-type instruction not handled\n");
				break;
		}
	}
	else{
		switch(opcode){
			case 0x01:	//BLTZ OR BGEZ
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x02:	//J
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x03:	//JAL
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x04:	//BEQ
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x05:	//BNE
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x06:	//BLEZ
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x07:	//BGTZ
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x08:	//ADDI
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x09:	//ADDIU
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x0A:	//SLTI
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x0C:	//ANDI
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x0D:	//ORI
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x0E:	//XORI
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x0F:	//LUI
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x20:	//LB
				NEXT_STATE.REGS[rt] = MEM_WB.LMD;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x21:	//LH
				NEXT_STATE.REGS[rt] = MEM_WB.LMD;
				INSTRUCTION_COUNT++;
				break;
					
			case 0x23:	//LW
				NEXT_STATE.REGS[rt] = MEM_WB.LMD;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x28:	//SB
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x29:	//SH
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			case 0x2B:	//SW
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				INSTRUCTION_COUNT++;
				break;
				
			default:
				break;
		}
	}
}

/************************************************************/
/* memory access (MEM) pipeline stage:                                                          */ 
/************************************************************/
void MEM()
{
	/*IMPLEMENT THIS*/
	//Fourth stage
	//Load/Store only?
	MEM_WB.IR = EX_MEM.IR;
	MEM_WB.PC = EX_MEM.PC;
	MEM_WB.A = EX_MEM.A;
	MEM_WB.B = EX_MEM.B;
	MEM_WB.imm = EX_MEM.imm;
	MEM_WB.ALUOutput = EX_MEM.ALUOutput;
	MEM_WB.LMD = 0;
	
	uint32_t opcode;
	
	opcode = (MEM_WB.IR & 0xFC000000) >> 26;	//Shift to get opcode bits 26-31
	
	if (opcode == 0x00){
		return;	//Don't need r type	
	}
	
	else{
		switch(opcode){
			case 0x20:	//LB
				MEM_WB.LMD = 0x000000FF & mem_read_32(MEM_WB.ALUOutput);	//Get first 8 bits from memory and place in lmd
				break;
				
			case 0x21:	//LH
				MEM_WB.LMD = 0x0000FFFF & mem_read_32(MEM_WB.ALUOutput);	//Get first 16 bits from memory and place in lmd
				break;
				
			case 0x23:	//LW
				MEM_WB.LMD = 0xFFFFFFFF & mem_read_32(MEM_WB.ALUOutput);	//Get first 32 bits from memory and place in lmd
				break;
				
			case 0x28:	//SB
				mem_write_32(MEM_WB.ALUOutput, MEM_WB.B);	//Write B into ALUOutput memory
				break;
				
			case 0x29:	//SH
				mem_write_32(MEM_WB.ALUOutput, MEM_WB.B);	//Write B into ALUOutput memory
				break;
				
			case 0x2B:	//SW
				mem_write_32(MEM_WB.ALUOutput, MEM_WB.B);	//Write B into ALUOutput memory
				break;
				
			default:
				break;
		}
	}
	
}

/************************************************************/
/* execution (EX) pipeline stage:                                                                          */ 
/************************************************************/
void EX()
{
	/*IMPLEMENT THIS*/
	//Third stage
	//Initialize EX pipeline registers
	EX_MEM.IR = ID_EX.IR;
	EX_MEM.PC = ID_EX.PC;
	EX_MEM.A = ID_EX.A;
	EX_MEM.B = ID_EX.B;
	EX_MEM.imm = ID_EX.imm;
	EX_MEM.ALUOutput = 0;
	
	uint32_t opcode, funct, sa;
	uint64_t multiply;
	
	opcode = (EX_MEM.IR & 0xFC000000) >> 26;	//Shift left to get opcode bits 26-31
	funct = EX_MEM.IR & 0x0000003F;	//Get first 6 bits for function code
	sa = (EX_MEM.IR & 0x000007C0) >> 6;	//Get shift amount
	
	if (opcode == 0x00){	//R-type instruction
		switch(funct){
			case 0x00:	//SLL
				EX_MEM.ALUOutput = EX_MEM.B << sa;	//SLL, rd(aluoutput), rt(EX_MEM.B), sa
				break;
				
			case 0x02:	//SRL
				EX_MEM.ALUOutput = EX_MEM.B >> sa;	//SRL, rd(aluoutput), rt(EX_MEM.B), sa
				break;
				
			case 0x03:	//SRA
				EX_MEM.ALUOutput = EX_MEM.B >> sa;	//Same as SRL
				break;
				
			case 0x08:	//JR
				break;
				
			case 0x09:	//JALR
				break;
				
			case 0x0C:	//SYSCALL
				break;
				
			case 0x10:	//MFHI
				EX_MEM.ALUOutput = CURRENT_STATE.HI;	//Contents of HI are loaded into rd(aluoutput)
				break;
				
			case 0x11:	//MTHI
				NEXT_STATE.HI = EX_MEM.A;	//Contents of rs(A) are loaded into HI
				break;
				
			case 0x12:	//MFLO
				EX_MEM.ALUOutput = CURRENT_STATE.LO;	//Contents of LO are loaded into rd(aluoutput)
				break;
				
			case 0x13:	//MTLO
				NEXT_STATE.LO = EX_MEM.A;	//Contents of rs(A) are loaded into LO
				break;
				
			case 0x18:	//MULT
				multiply = EX_MEM.A * EX_MEM.B;	//multiply rs and rt, store low order into LO and high order into HI
				NEXT_STATE.LO = 0x00000000FFFFFFFF & multiply;
				NEXT_STATE.HI = (0xFFFFFFFF00000000 & multiply) >> 32;
				break;
				
			case 0x19:	//MULTU
				multiply = EX_MEM.A * EX_MEM.B;	//multiply rs and rt, store low order into LO and high order into HI
				NEXT_STATE.LO = 0x00000000FFFFFFFF & multiply;
				NEXT_STATE.HI = (0xFFFFFFFF00000000 & multiply) >> 32;
				break;
				
			case 0x1A:	//DIV
				if (EX_MEM.B == 0){
					printf("Cannot divide by 0\n");
				}
				else{
					NEXT_STATE.LO = EX_MEM.A / EX_MEM.B;	//same as lab 1
					NEXT_STATE.HI = EX_MEM.A % EX_MEM.B;
				}
				break;
				
			case 0x1B:	//DIVU
				if (EX_MEM.B == 0){
					printf("Cannot divide by 0\n");
				}
				else{
					NEXT_STATE.LO = EX_MEM.A / EX_MEM.B;	//same as lab 1
					NEXT_STATE.HI = EX_MEM.A % EX_MEM.B;
				}
				break;
				
			case 0x20:	//ADD
				EX_MEM.ALUOutput = EX_MEM.A + EX_MEM.B;	//ADD rd(ALUOutput), rs(A), rt(B)
				break;
				
			case 0x21:	//ADDU
				EX_MEM.ALUOutput = EX_MEM.A + EX_MEM.B;	//ADDU rd(ALUOutput), rs(A), rt(B)
				break;
				
			case 0x22:	//SUB
				EX_MEM.ALUOutput = EX_MEM.A - EX_MEM.B;	//SUB rd(ALUOutput), rs(A), rt(B)
				break;
				
			case 0x23:	//SUBU
				EX_MEM.ALUOutput = EX_MEM.A - EX_MEM.B;	//SUBU rd(ALUOutput), rs(A), rt(B)
				break;
				
			case 0x24:	//AND
				EX_MEM.ALUOutput = EX_MEM.A & EX_MEM.B;	//AND rd(ALUOutput), rs(A), rt(B)
				break;
				
			case 0x25:	//OR
				EX_MEM.ALUOutput = EX_MEM.A | EX_MEM.B;	//OR rd(ALUOutput), rs(A), rt(B)
				break;
				
			case 0x26:	//XOR
				EX_MEM.ALUOutput = EX_MEM.A ^ EX_MEM.B;	//XOR rd(ALUOutput), rs(A), rt(B)
				break;
				
			case 0x27:	//NOR
				EX_MEM.ALUOutput = ~(EX_MEM.A | EX_MEM.B);	//NOR rd(ALUOutput), rs(A), rt(B)
				break;
				
			case 0x2A:	//SLT
				if(EX_MEM.A < EX_MEM.B){
					EX_MEM.ALUOutput = 1;	//If rs(A) is less than rt(B), result = 1
				}
				else{
					EX_MEM.ALUOutput = 0;	//Else, result = 0
				}
				break;
				
			default:
				printf("R-type instruction not handled\n");
				break;
		}
	}
	else{
		switch(opcode){
			case 0x01:	//BLTZ OR BGEZ
				break;
				
			case 0x02:	//J
				break;
				
			case 0x03:	//JAL
				break;
				
			case 0x04:	//BEQ
				break;
				
			case 0x05:	//BNE
				break;
				
			case 0x06:	//BLEZ
				break;
				
			case 0x07:	//BGTZ
				break;
				
			case 0x08:	//ADDI
				if (EX_MEM.imm >> 15){	//negative
					EX_MEM.imm = 0xFFFF0000 | EX_MEM.imm;	//sign extended
				}
				EX_MEM.ALUOutput = EX_MEM.imm + EX_MEM.A;
				break;
				
			case 0x09:	//ADDIU
				EX_MEM.ALUOutput = EX_MEM.A + EX_MEM.imm;	//ADDIU rt(aluoutput), rs(A), immediate
				break;
				
			case 0x0A:	//SLTI
				if (EX_MEM.A < EX_MEM.imm){
					EX_MEM.ALUOutput = 1;	//If rs(A) < immediate, rt(aluoutput) = 1	
				}
				else{
					EX_MEM.ALUOutput = 0;	//If rs(A) > immediate, rt(aluoutput) = 0	
				}
				break;
				
			case 0x0C:	//ANDI
				EX_MEM.ALUOutput = EX_MEM.A & EX_MEM.imm;	//ANDI rt(aluotput), rs(A), immediate
				break;
				
			case 0x0D:	//ORI
				EX_MEM.ALUOutput = EX_MEM.A | EX_MEM.imm;	//ORI rt(aluotput), rs(A), immediate
				break;
				
			case 0x0E:	//XORI
				EX_MEM.ALUOutput = EX_MEM.A ^ EX_MEM.imm;	//XORI rt(aluotput), rs(A), immediate
				break;
				
			case 0x0F:	//LUI
				EX_MEM.ALUOutput = EX_MEM.imm << 16;	//Shift immediate left 16 bits and place in ALUOutput
				break;
				
			case 0x20:	//LB
				EX_MEM.ALUOutput = ID_EX.A + ID_EX.imm;	//aluoutput = a + immediate
				EX_MEM.A = ID_EX.A;	//Update Memory locations with current values
				EX_MEM.B = ID_EX.B;
				EX_MEM.imm = ID_EX.imm;
				break;
				
			case 0x21:	//LH
				EX_MEM.ALUOutput = ID_EX.A + ID_EX.imm;	//aluoutput = a + immediate
				EX_MEM.A = ID_EX.A;	//Update Memory locations with current values
				EX_MEM.B = ID_EX.B;
				EX_MEM.imm = ID_EX.imm;
				break;
					
			case 0x23:	//LW
				EX_MEM.ALUOutput = ID_EX.A + ID_EX.imm;	//aluoutput = a + immediate
				EX_MEM.A = ID_EX.A;	//Update Memory locations with current values
				EX_MEM.B = ID_EX.B;
				EX_MEM.imm = ID_EX.imm;
				break;
				
			case 0x28:	//SB
				EX_MEM.ALUOutput = ID_EX.A + ID_EX.imm;	//aluoutput = a + immediate
				EX_MEM.A = ID_EX.A;	//Update Memory locations with current values
				EX_MEM.B = ID_EX.B;
				EX_MEM.imm = ID_EX.imm;
				break;
				
			case 0x29:	//SH
				EX_MEM.ALUOutput = ID_EX.A + ID_EX.imm;	//aluoutput = a + immediate
				EX_MEM.A = ID_EX.A;	//Update Memory locations with current values
				EX_MEM.B = ID_EX.B;
				EX_MEM.imm = ID_EX.imm;
				break;
				
			case 0x2B:	//SW
				EX_MEM.ALUOutput = ID_EX.A + ID_EX.imm;	//aluoutput = a + immediate
				EX_MEM.A = ID_EX.A;	//Update Memory locations with current values
				EX_MEM.B = ID_EX.B;
				EX_MEM.imm = ID_EX.imm;
				break;
				
			default:
				break;
		}
	}
}

/************************************************************/
/* instruction decode (ID) pipeline stage:                                                         */ 
/************************************************************/
void ID()
{
	/*IMPLEMENT THIS*/
	//Second stage
	//Initialize ID pipeline registers
	ID_EX.IR = IF_ID.IR;
	ID_EX.PC = IF_ID.PC;
	ID_EX.A = 0;
	ID_EX.B = 0;
	ID_EX.imm = 0;
	
	uint32_t rs, rt, immediate;
	
	rs = (IF_ID.IR & 0x03E00000) >> 21;	//Shift left to get rs bits 21-25
	rt = (IF_ID.IR & 0x001F0000) >> 16;	//Shift left to get rt bits 16-20
	immediate = IF_ID.IR & 0x0000FFFF;	//Get first 16 bits of instruction
	
	ID_EX.A = CURRENT_STATE.REGS[rs];	//Set A to value in current state of rs
	ID_EX.B = CURRENT_STATE.REGS[rt];	//Set B to value in current state of rt
	
	if ((immediate >> 15) == 1){	//If negative
		ID_EX.imm = immediate | 0xFFFF0000;	//Sign extend and store in imm		
	}
	else{
		ID_EX.imm = immediate & 0x0000FFFF;	//Else it's positive, store in imm	
	}
	
}

/************************************************************/
/* instruction fetch (IF) pipeline stage:                                                              */ 
/************************************************************/
void IF()
{	//something with memread
	/*IMPLEMENT THIS*/
	//First stage
	
	IF_ID.IR = mem_read_32(CURRENT_STATE.PC);	//Get current value in memory
	IF_ID.PC = CURRENT_STATE.PC + 4;	//Increment counter
	NEXT_STATE.PC = IF_ID.PC;	//Store incremented counter into pc's next state
}


/************************************************************/
/* Initialize Memory                                                                                                    */ 
/************************************************************/
void initialize() { 
	init_memory();
	CURRENT_STATE.PC = MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/************************************************************/
/* Print the program loaded into memory (in MIPS assembly format)    */ 
/************************************************************/
void print_program(){
	/*IMPLEMENT THIS*/
}

/************************************************************/
/* Print the current pipeline                                                                                    */ 
/************************************************************/
void show_pipeline(){
	/*IMPLEMENT THIS*/
}

/***************************************************************/
/* main                                                                                                                                   */
/***************************************************************/
int main(int argc, char *argv[]) {                              
	printf("\n**************************\n");
	printf("Welcome to MU-MIPS SIM...\n");
	printf("**************************\n\n");
	
	if (argc < 2) {
		printf("Error: You should provide input file.\nUsage: %s <input program> \n\n",  argv[0]);
		exit(1);
	}

	strcpy(prog_file, argv[1]);
	initialize();
	load_program();
	help();
	while (1){
		handle_command();
	}
	return 0;
}
