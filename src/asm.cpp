/// \file
/// \brief 6502 assembler and disassembler

#include "types.h"
#include "utils/xstring.h"
#include "utils/StringBuilder.h"
#include "debug.h"
#include "asm.h"
#include "x6502.h"

#include <cstring>
#include <cstdlib>
#include <cstdio>
///assembles the string to an instruction located at addr, storing opcodes in output buffer
int Assemble(unsigned char *output, int addr, char *str) {
	//unsigned char opcode[3] = { 0,0,0 };
	output[0] = output[1] = output[2] = 0;
	char astr[128],ins[4];
	int len = strlen(str);
	if ((!len) || (len > 127)) return 1;

	strcpy(astr,str);
	str_ucase(astr);
	sscanf(astr,"%3s",ins); //get instruction
	if (strlen(ins) != 3) return 1;
	strcpy(astr,strstr(astr,ins)+3); //heheh, this is probably a bad idea, but let's do it anyway!
	if ((astr[0] != ' ') && (astr[0] != 0)) return 1;

	//remove all whitespace
	str_strip(astr,STRIP_SP|STRIP_TAB|STRIP_CR|STRIP_LF);

	//repair syntax
	chr_replace(astr,'[','(');	//brackets
	chr_replace(astr,']',')');
	chr_replace(astr,'{','(');
	chr_replace(astr,'}',')');
	chr_replace(astr,';',0);	//comments
	str_replace(astr,"0X","$");	//miscellaneous

	//This does the following:
	// 1) Sets opcode[0] on success, else returns 1.
	// 2) Parses text in *astr to build the rest of the assembled
	//    data in 'opcode', else returns 1 on error.

	if (!strlen(astr)) {
		//Implied instructions
			 if (!strcmp(ins,"BRK")) output[0] = 0x00;
		else if (!strcmp(ins,"PHP")) output[0] = 0x08;
		else if (!strcmp(ins,"ASL")) output[0] = 0x0A;
		else if (!strcmp(ins,"CLC")) output[0] = 0x18;
		else if (!strcmp(ins,"PLP")) output[0] = 0x28;
		else if (!strcmp(ins,"ROL")) output[0] = 0x2A;
		else if (!strcmp(ins,"SEC")) output[0] = 0x38;
		else if (!strcmp(ins,"RTI")) output[0] = 0x40;
		else if (!strcmp(ins,"PHA")) output[0] = 0x48;
		else if (!strcmp(ins,"LSR")) output[0] = 0x4A;
		else if (!strcmp(ins,"CLI")) output[0] = 0x58;
		else if (!strcmp(ins,"RTS")) output[0] = 0x60;
		else if (!strcmp(ins,"PLA")) output[0] = 0x68;
		else if (!strcmp(ins,"ROR")) output[0] = 0x6A;
		else if (!strcmp(ins,"SEI")) output[0] = 0x78;
		else if (!strcmp(ins,"DEY")) output[0] = 0x88;
		else if (!strcmp(ins,"TXA")) output[0] = 0x8A;
		else if (!strcmp(ins,"TYA")) output[0] = 0x98;
		else if (!strcmp(ins,"TXS")) output[0] = 0x9A;
		else if (!strcmp(ins,"TAY")) output[0] = 0xA8;
		else if (!strcmp(ins,"TAX")) output[0] = 0xAA;
		else if (!strcmp(ins,"CLV")) output[0] = 0xB8;
		else if (!strcmp(ins,"TSX")) output[0] = 0xBA;
		else if (!strcmp(ins,"INY")) output[0] = 0xC8;
		else if (!strcmp(ins,"DEX")) output[0] = 0xCA;
		else if (!strcmp(ins,"CLD")) output[0] = 0xD8;
		else if (!strcmp(ins,"INX")) output[0] = 0xE8;
		else if (!strcmp(ins,"NOP")) output[0] = 0xEA;
		else if (!strcmp(ins,"SED")) output[0] = 0xF8;
		else return 1;
	}
	else {
		//Instructions with Operands
			 if (!strcmp(ins,"ORA")) output[0] = 0x01;
		else if (!strcmp(ins,"ASL")) output[0] = 0x06;
		else if (!strcmp(ins,"BPL")) output[0] = 0x10;
		else if (!strcmp(ins,"JSR")) output[0] = 0x20;
		else if (!strcmp(ins,"AND")) output[0] = 0x21;
		else if (!strcmp(ins,"BIT")) output[0] = 0x24;
		else if (!strcmp(ins,"ROL")) output[0] = 0x26;
		else if (!strcmp(ins,"BMI")) output[0] = 0x30;
		else if (!strcmp(ins,"EOR")) output[0] = 0x41;
		else if (!strcmp(ins,"LSR")) output[0] = 0x46;
		else if (!strcmp(ins,"JMP")) output[0] = 0x4C;
		else if (!strcmp(ins,"BVC")) output[0] = 0x50;
		else if (!strcmp(ins,"ADC")) output[0] = 0x61;
		else if (!strcmp(ins,"ROR")) output[0] = 0x66;
		else if (!strcmp(ins,"BVS")) output[0] = 0x70;
		else if (!strcmp(ins,"STA")) output[0] = 0x81;
		else if (!strcmp(ins,"STY")) output[0] = 0x84;
		else if (!strcmp(ins,"STX")) output[0] = 0x86;
		else if (!strcmp(ins,"BCC")) output[0] = 0x90;
		else if (!strcmp(ins,"LDY")) output[0] = 0xA0;
		else if (!strcmp(ins,"LDA")) output[0] = 0xA1;
		else if (!strcmp(ins,"LDX")) output[0] = 0xA2;
		else if (!strcmp(ins,"BCS")) output[0] = 0xB0;
		else if (!strcmp(ins,"CPY")) output[0] = 0xC0;
		else if (!strcmp(ins,"CMP")) output[0] = 0xC1;
		else if (!strcmp(ins,"DEC")) output[0] = 0xC6;
		else if (!strcmp(ins,"BNE")) output[0] = 0xD0;
		else if (!strcmp(ins,"CPX")) output[0] = 0xE0;
		else if (!strcmp(ins,"SBC")) output[0] = 0xE1;
		else if (!strcmp(ins,"INC")) output[0] = 0xE6;
		else if (!strcmp(ins,"BEQ")) output[0] = 0xF0;
		else return 1;

		{
			//Parse Operands
			// It's not the sexiest thing ever, but it works well enough!

			//TODO:
			// Add branches.
			// Fix certain instructions. (Setting bits is not 100% perfect.)
			// Fix instruction/operand matching. (Instructions like "jmp ($94),Y" are no good!)
			// Optimizations?
			int tmpint;
			char tmpchr,tmpstr[20];

			if (sscanf(astr,"#$%2X%c",&tmpint,&tmpchr) == 1) { //#Immediate
				switch (output[0]) {
					case 0x20: case 0x4C: //Jumps
					case 0x10: case 0x30: case 0x50: case 0x70: //Branches
					case 0x90: case 0xB0: case 0xD0: case 0xF0:
					case 0x06: case 0x24: case 0x26: case 0x46: //Other instructions incapable of #Immediate
					case 0x66: case 0x81: case 0x84: case 0x86:
					case 0xC6: case 0xE6:
						return 1;
					default:
						//cheap hack for certain instructions
						switch (output[0]) {
							case 0xA0: case 0xA2: case 0xC0: case 0xE0:
								break;
							default:
								output[0] |= 0x08;
								break;
						}
						output[1] = tmpint;
						break;
				}
			}
			else if (sscanf(astr,"$%4X%c",&tmpint,&tmpchr) == 1) { //Absolute, Zero Page, Branch, or Jump
				switch (output[0]) {
					case 0x20: case 0x4C: //Jumps
						output[1] = (tmpint & 0xFF);
						output[2] = (tmpint >> 8);
						break;
					case 0x10: case 0x30: case 0x50: case 0x70: //Branches
					case 0x90: case 0xB0: case 0xD0: case 0xF0:
						tmpint -= (addr+2);
						if ((tmpint < -128) || (tmpint > 127)) return 1;
						output[1] = (tmpint & 0xFF);
						break;
						//return 1; //FIX ME
					default:
						if (tmpint > 0xFF) { //Absolute
							output[0] |= 0x0C;
							output[1] = (tmpint & 0xFF);
							output[2] = (tmpint >> 8);
						}
						else { //Zero Page
							output[0] |= 0x04;
							output[1] = (tmpint & 0xFF);
						}
						break;
				}
			}
			else if (sscanf(astr,"$%4X%s",&tmpint,tmpstr) == 2) { //Absolute,X, Zero Page,X, Absolute,Y or Zero Page,Y
				if (!strcmp(tmpstr,",X")) { //Absolute,X or Zero Page,X
					switch (output[0]) {
						case 0x20: case 0x4C: //Jumps
						case 0x10: case 0x30: case 0x50: case 0x70: //Branches
						case 0x90: case 0xB0: case 0xD0: case 0xF0:
						case 0x24: case 0x86: case 0xA2: case 0xC0: //Other instructions incapable of Absolute,X or Zero Page,X
						case 0xE0:
							return 1;
						default:
							if (tmpint > 0xFF) { //Absolute
								if (output[0] == 0x84) return 1; //No STY Absolute,X!
								output[0] |= 0x1C;
								output[1] = (tmpint & 0xFF);
								output[2] = (tmpint >> 8);
							}
							else { //Zero Page
								output[0] |= 0x14;
								output[1] = (tmpint & 0xFF);
							}
							break;
					}
				}
				else if (!strcmp(tmpstr,",Y")) { //Absolute,Y or Zero Page,Y
					switch (output[0]) {
						case 0x20: case 0x4C: //Jumps
						case 0x10: case 0x30: case 0x50: case 0x70: //Branches
						case 0x90: case 0xB0: case 0xD0: case 0xF0:
						case 0x06: case 0x24: case 0x26: case 0x46: //Other instructions incapable of Absolute,Y or Zero Page,Y
						case 0x66: case 0x84: case 0x86: case 0xA0:
						case 0xC0: case 0xC6: case 0xE0: case 0xE6:
							return 1;
						case 0xA2: //cheap hack for LDX
							output[0] |= 0x04;
						default:
							if (tmpint > 0xFF) { //Absolute
								if (output[0] == 0x86) return 1; //No STX Absolute,Y!
								output[0] |= 0x18;
								output[1] = (tmpint & 0xFF);
								output[2] = (tmpint >> 8);
							}
							else { //Zero Page
								if ((output[0] != 0x86) && (output[0] != 0xA2)) return 1; //only STX and LDX Absolute,Y!
								output[0] |= 0x10;
								output[1] = (tmpint & 0xFF);
							}
							break;
					}
				}
				else return 1;
			}
			else if (sscanf(astr,"($%4X%s",&tmpint,tmpstr) == 2) { //Jump (Indirect), (Indirect,X) or (Indirect),Y
				switch (output[0]) {
					case 0x20: //Jumps
					case 0x10: case 0x30: case 0x50: case 0x70: //Branches
					case 0x90: case 0xB0: case 0xD0: case 0xF0:
					case 0x06: case 0x24: case 0x26: case 0x46: //Other instructions incapable of Jump (Indirect), (Indirect,X) or (Indirect),Y
					case 0x66: case 0x84: case 0x86: case 0xA0:
					case 0xA2: case 0xC0: case 0xC6: case 0xE0:
					case 0xE6:
						return 1;
					default:
						if ((!strcmp(tmpstr,")")) && (output[0] == 0x4C)) { //Jump (Indirect)
							output[0] = 0x6C;
							output[1] = (tmpint & 0xFF);
							output[2] = (tmpint >> 8);
						}
						else if ((!strcmp(tmpstr,",X)")) && (tmpint <= 0xFF) && (output[0] != 0x4C)) { //(Indirect,X)
							output[1] = (tmpint & 0xFF);
						}
						else if ((!strcmp(tmpstr,"),Y")) && (tmpint <= 0xFF) && (output[0] != 0x4C)) { //(Indirect),Y
							output[0] |= 0x10;
							output[1] = (tmpint & 0xFF);
						}
						else return 1;
						break;
				}
			}
			else return 1;
		}
	}

	return 0;
}

///disassembles the opcodes in the buffer assuming the provided address. Uses GetMem() and 6502 current registers to query referenced values. returns a static string buffer.
char *Disassemble(int addr, uint8 *opcode) {
	static char str[64]={0};
	const char *chr;
	char indReg;
	uint16 tmp,tmp2;
	StringBuilder sb(str);

	//these may be replaced later with passed-in values to make a lighter-weight disassembly mode that may not query the referenced values
	#define RX (X.X)
	#define RY (X.Y)

	switch (opcode[0]) {
		#define relative(a) { \
			if (((a)=opcode[1])&0x80) (a) = addr-(((a)-1)^0xFF); \
			else (a)+=addr; \
		}
		#define absolute(a) { \
			(a) = opcode[1] | opcode[2]<<8; \
		}
		#define zpIndex(a,i) { \
			(a) = (opcode[1]+(i))&0xFF; \
		}
		#define indirectX(a) { \
			(a) = (opcode[1]+RX)&0xFF; \
			(a) = GetMem((a)) | (GetMem(((a)+1)&0xff))<<8; \
		}
		#define indirectY(a) { \
			(a) = GetMem(opcode[1]) | (GetMem((opcode[1]+1)&0xff))<<8; \
			(a) += RY; \
		}


		#ifdef BRK_3BYTE_HACK
			case 0x00:
			sb << "BRK " << sb_hex(opcode[1], 2) << ' ' << sb_hex(opcode[2], 2);
			break;
		#else
			case 0x00: strcpy(str,"BRK"); break;
		#endif

		//odd, 1-byte opcodes
		case 0x08: strcpy(str,"PHP"); break;
		case 0x0A: strcpy(str,"ASL"); break;
		case 0x18: strcpy(str,"CLC"); break;
		case 0x28: strcpy(str,"PLP"); break;
		case 0x2A: strcpy(str,"ROL"); break;
		case 0x38: strcpy(str,"SEC"); break;
		case 0x40: strcpy(str,"RTI"); break;
		case 0x48: strcpy(str,"PHA"); break;
		case 0x4A: strcpy(str,"LSR"); break;
		case 0x58: strcpy(str,"CLI"); break;
		case 0x60: strcpy(str,"RTS"); break;
		case 0x68: strcpy(str,"PLA"); break;
		case 0x6A: strcpy(str,"ROR"); break;
		case 0x78: strcpy(str,"SEI"); break;
		case 0x88: strcpy(str,"DEY"); break;
		case 0x8A: strcpy(str,"TXA"); break;
		case 0x98: strcpy(str,"TYA"); break;
		case 0x9A: strcpy(str,"TXS"); break;
		case 0xA8: strcpy(str,"TAY"); break;
		case 0xAA: strcpy(str,"TAX"); break;
		case 0xB8: strcpy(str,"CLV"); break;
		case 0xBA: strcpy(str,"TSX"); break;
		case 0xC8: strcpy(str,"INY"); break;
		case 0xCA: strcpy(str,"DEX"); break;
		case 0xD8: strcpy(str,"CLD"); break;
		case 0xE8: strcpy(str,"INX"); break;
		case 0xEA: strcpy(str,"NOP"); break;
		case 0xF8: strcpy(str,"SED"); break;

		//(Indirect,X)
		case 0x01: chr = "ORA"; goto _indirectx;
		case 0x21: chr = "AND"; goto _indirectx;
		case 0x41: chr = "EOR"; goto _indirectx;
		case 0x61: chr = "ADC"; goto _indirectx;
		case 0x81: chr = "STA"; goto _indirectx;
		case 0xA1: chr = "LDA"; goto _indirectx;
		case 0xC1: chr = "CMP"; goto _indirectx;
		case 0xE1: chr = "SBC"; goto _indirectx;
		_indirectx:
			indirectX(tmp);
			indReg = 'X';

		_indirect:
			sb << chr << " (" << sb_addr(opcode[1], 2) << ',' << indReg << ") @ " << sb_addr(tmp) << " = " << sb_lit(GetMem(tmp));
			break;

		//Zero Page
		case 0x05: chr = "ORA"; goto _zeropage;
		case 0x06: chr = "ASL"; goto _zeropage;
		case 0x24: chr = "BIT"; goto _zeropage;
		case 0x25: chr = "AND"; goto _zeropage;
		case 0x26: chr = "ROL"; goto _zeropage;
		case 0x45: chr = "EOR"; goto _zeropage;
		case 0x46: chr = "LSR"; goto _zeropage;
		case 0x65: chr = "ADC"; goto _zeropage;
		case 0x66: chr = "ROR"; goto _zeropage;
		case 0x84: chr = "STY"; goto _zeropage;
		case 0x85: chr = "STA"; goto _zeropage;
		case 0x86: chr = "STX"; goto _zeropage;
		case 0xA4: chr = "LDY"; goto _zeropage;
		case 0xA5: chr = "LDA"; goto _zeropage;
		case 0xA6: chr = "LDX"; goto _zeropage;
		case 0xC4: chr = "CPY"; goto _zeropage;
		case 0xC5: chr = "CMP"; goto _zeropage;
		case 0xC6: chr = "DEC"; goto _zeropage;
		case 0xE4: chr = "CPX"; goto _zeropage;
		case 0xE5: chr = "SBC"; goto _zeropage;
		case 0xE6: chr = "INC"; goto _zeropage;
		_zeropage:
		// ################################## Start of SP CODE ###########################
		// Change width to %04X // don't!
			sb << chr << ' ' << sb_addr(opcode[1], 2) << " = " << sb_lit(GetMem(opcode[1]));
		// ################################## End of SP CODE ###########################
			break;

		//#Immediate
		case 0x09: chr = "ORA"; goto _immediate;
		case 0x29: chr = "AND"; goto _immediate;
		case 0x49: chr = "EOR"; goto _immediate;
		case 0x69: chr = "ADC"; goto _immediate;
		//case 0x89: chr = "STA"; goto _immediate;  //baka, no STA #imm!!
		case 0xA0: chr = "LDY"; goto _immediate;
		case 0xA2: chr = "LDX"; goto _immediate;
		case 0xA9: chr = "LDA"; goto _immediate;
		case 0xC0: chr = "CPY"; goto _immediate;
		case 0xC9: chr = "CMP"; goto _immediate;
		case 0xE0: chr = "CPX"; goto _immediate;
		case 0xE9: chr = "SBC"; goto _immediate;
		_immediate:
			sb << chr << ' ' << sb_lit(opcode[1]);
			break;

		//Absolute
		case 0x0D: chr = "ORA"; goto _absolute;
		case 0x0E: chr = "ASL"; goto _absolute;
		case 0x2C: chr = "BIT"; goto _absolute;
		case 0x2D: chr = "AND"; goto _absolute;
		case 0x2E: chr = "ROL"; goto _absolute;
		case 0x4D: chr = "EOR"; goto _absolute;
		case 0x4E: chr = "LSR"; goto _absolute;
		case 0x6D: chr = "ADC"; goto _absolute;
		case 0x6E: chr = "ROR"; goto _absolute;
		case 0x8C: chr = "STY"; goto _absolute;
		case 0x8D: chr = "STA"; goto _absolute;
		case 0x8E: chr = "STX"; goto _absolute;
		case 0xAC: chr = "LDY"; goto _absolute;
		case 0xAD: chr = "LDA"; goto _absolute;
		case 0xAE: chr = "LDX"; goto _absolute;
		case 0xCC: chr = "CPY"; goto _absolute;
		case 0xCD: chr = "CMP"; goto _absolute;
		case 0xCE: chr = "DEC"; goto _absolute;
		case 0xEC: chr = "CPX"; goto _absolute;
		case 0xED: chr = "SBC"; goto _absolute;
		case 0xEE: chr = "INC"; goto _absolute;
		_absolute:
			absolute(tmp);

			sb << chr << ' ' << sb_addr(tmp) << " = " << sb_lit(GetMem(tmp));

			break;

		//branches
		case 0x10: chr = "BPL"; goto _branch;
		case 0x30: chr = "BMI"; goto _branch;
		case 0x50: chr = "BVC"; goto _branch;
		case 0x70: chr = "BVS"; goto _branch;
		case 0x90: chr = "BCC"; goto _branch;
		case 0xB0: chr = "BCS"; goto _branch;
		case 0xD0: chr = "BNE"; goto _branch;
		case 0xF0: chr = "BEQ"; goto _branch;
		_branch:
			relative(tmp);

			sb << chr << ' ' << sb_addr(tmp);

			break;

		//(Indirect),Y
		case 0x11: chr = "ORA"; goto _indirecty;
		case 0x31: chr = "AND"; goto _indirecty;
		case 0x51: chr = "EOR"; goto _indirecty;
		case 0x71: chr = "ADC"; goto _indirecty;
		case 0x91: chr = "STA"; goto _indirecty;
		case 0xB1: chr = "LDA"; goto _indirecty;
		case 0xD1: chr = "CMP"; goto _indirecty;
		case 0xF1: chr = "SBC"; goto _indirecty;
		_indirecty:
			indirectY(tmp);
			indReg = 'Y';

			goto _indirect;

		//Zero Page,X
		case 0x15: chr = "ORA"; goto _zeropagex;
		case 0x16: chr = "ASL"; goto _zeropagex;
		case 0x35: chr = "AND"; goto _zeropagex;
		case 0x36: chr = "ROL"; goto _zeropagex;
		case 0x55: chr = "EOR"; goto _zeropagex;
		case 0x56: chr = "LSR"; goto _zeropagex;
		case 0x75: chr = "ADC"; goto _zeropagex;
		case 0x76: chr = "ROR"; goto _zeropagex;
		case 0x94: chr = "STY"; goto _zeropagex;
		case 0x95: chr = "STA"; goto _zeropagex;
		case 0xB4: chr = "LDY"; goto _zeropagex;
		case 0xB5: chr = "LDA"; goto _zeropagex;
		case 0xD5: chr = "CMP"; goto _zeropagex;
		case 0xD6: chr = "DEC"; goto _zeropagex;
		case 0xF5: chr = "SBC"; goto _zeropagex;
		case 0xF6: chr = "INC"; goto _zeropagex;
		_zeropagex:
			zpIndex(tmp, RX);
			indReg = 'X';

		_indexed:
		// ################################## Start of SP CODE ###########################
		// Change width to %04X // don't!
			sb << chr << ' ' << sb_addr(opcode[1], 2) << ',' << indReg << " @ " << sb_addr(tmp) << " = " << sb_lit(GetMem(tmp));
		// ################################## End of SP CODE ###########################
			break;

		//Absolute,Y
		case 0x19: chr = "ORA"; goto _absolutey;
		case 0x39: chr = "AND"; goto _absolutey;
		case 0x59: chr = "EOR"; goto _absolutey;
		case 0x79: chr = "ADC"; goto _absolutey;
		case 0x99: chr = "STA"; goto _absolutey;
		case 0xB9: chr = "LDA"; goto _absolutey;
		case 0xBE: chr = "LDX"; goto _absolutey;
		case 0xD9: chr = "CMP"; goto _absolutey;
		case 0xF9: chr = "SBC"; goto _absolutey;
		_absolutey:
			absolute(tmp);
			tmp2 = (tmp + RY);
			indReg = 'Y';

		_absindexed:
			sb << chr << ' ' << sb_addr(tmp) << ',' << indReg << " @ " << sb_addr(tmp2) << " = " << sb_lit(GetMem(tmp2));

			break;

		//Absolute,X
		case 0x1D: chr = "ORA"; goto _absolutex;
		case 0x1E: chr = "ASL"; goto _absolutex;
		case 0x3D: chr = "AND"; goto _absolutex;
		case 0x3E: chr = "ROL"; goto _absolutex;
		case 0x5D: chr = "EOR"; goto _absolutex;
		case 0x5E: chr = "LSR"; goto _absolutex;
		case 0x7D: chr = "ADC"; goto _absolutex;
		case 0x7E: chr = "ROR"; goto _absolutex;
		case 0x9D: chr = "STA"; goto _absolutex;
		case 0xBC: chr = "LDY"; goto _absolutex;
		case 0xBD: chr = "LDA"; goto _absolutex;
		case 0xDD: chr = "CMP"; goto _absolutex;
		case 0xDE: chr = "DEC"; goto _absolutex;
		case 0xFD: chr = "SBC"; goto _absolutex;
		case 0xFE: chr = "INC"; goto _absolutex;
		_absolutex:
			absolute(tmp);
			tmp2 = (tmp + RX);
			indReg = 'X';

			goto _absindexed;

		//jumps
		case 0x20: chr = "JSR"; goto _jump;
		case 0x4C: chr = "JMP"; goto _jump;
		_jump:
			absolute(tmp);

			sb << chr << ' ' << sb_addr(tmp);

			break;

		case 0x6C:
			absolute(tmp);

			sb << "JMP (" << sb_addr(tmp);
			sb << ") = " << sb_addr(GetMem(tmp) | GetMem(tmp + 1) << 8);

			break;

		//Zero Page,Y
		case 0x96: chr = "STX"; goto _zeropagey;
		case 0xB6: chr = "LDX"; goto _zeropagey;
		_zeropagey:
			zpIndex(tmp, RY);
			indReg = 'Y';

			goto _indexed;

		//UNDEFINED
		default: strcpy(str, "ERROR"); break;
	}
	return str;
}



///disassembles the opcodes in the buffer assuming the provided address. Uses GetMem() and 6502 current registers to query referenced values. returns a static string buffer.
char *bzk_Disassemble(int addr, uint8 *opcode) {
	static char str[64] = {0};
	uint16 tmp;
    
	switch (opcode[0]) {
		//Zero Page
		case 0x05: goto _zeropage; //ORA
		case 0x06: goto _zeropage; //ASL
		case 0x24: goto _zeropage; //BIT
		case 0x25: goto _zeropage; //AND
		case 0x26: goto _zeropage; //ROL
		case 0x45: goto _zeropage; //EOR
		case 0x46: goto _zeropage; //LSR
		case 0x65: goto _zeropage; //ADC
		case 0x66: goto _zeropage; //ROR
		case 0x84: goto _zeropage; //STY
		case 0x85: goto _zeropage; //STA
		case 0x86: goto _zeropage; //STX
		case 0xA4: goto _zeropage; //LDY
		case 0xA5: goto _zeropage; //LDA
		case 0xA6: goto _zeropage; //LDX
		case 0xC4: goto _zeropage; //CPY
		case 0xC5: goto _zeropage; //CMP
		case 0xC6: goto _zeropage; //DEC
		case 0xE4: goto _zeropage; //CPX
		case 0xE5: goto _zeropage; //SBC
		case 0xE6: goto _zeropage; //INC
		_zeropage:
            tmp = opcode[1];
			sprintf(str, "%u|%u|%u", bzk_GetNesFileAddress(tmp), bzk_getBank(tmp), GetMem(tmp));
			break;
        
		//Zero Page,X
		case 0x15: goto _zeropagex; //ORA
		case 0x16: goto _zeropagex; //ASL
		case 0x35: goto _zeropagex; //AND
		case 0x36: goto _zeropagex; //ROL
		case 0x55: goto _zeropagex; //EOR
		case 0x56: goto _zeropagex; //LSR
		case 0x75: goto _zeropagex; //ADC
		case 0x76: goto _zeropagex; //ROR
		case 0x94: goto _zeropagex; //STY
		case 0x95: goto _zeropagex; //STA
		case 0xB4: goto _zeropagex; //LDY
		case 0xB5: goto _zeropagex; //LDA
		case 0xD5: goto _zeropagex; //CMP
		case 0xD6: goto _zeropagex; //DEC
		case 0xF5: goto _zeropagex; //SBC
		case 0xF6: goto _zeropagex; //INC
		_zeropagex:
			tmp = (opcode[1] + X.X) & 0xFF;
            sprintf(str, "%u|%u|%u", bzk_GetNesFileAddress(tmp), bzk_getBank(tmp), GetMem(tmp));
			break;
        
		//Zero Page,Y
		case 0x96: goto _zeropagey; //STX
		case 0xB6: goto _zeropagey; //LDX
		_zeropagey:
			tmp = (opcode[1] + X.Y) & 0xFF;
            sprintf(str, "%u|%u|%u", bzk_GetNesFileAddress(tmp), bzk_getBank(tmp), GetMem(tmp));
			break;
        
		//Absolute
		case 0x0D: goto _absolute; //ORA
		case 0x0E: goto _absolute; //ASL
		case 0x2C: goto _absolute; //BIT
		case 0x2D: goto _absolute; //AND
		case 0x2E: goto _absolute; //ROL
		case 0x4D: goto _absolute; //EOR
		case 0x4E: goto _absolute; //LSR
		case 0x6D: goto _absolute; //ADC
		case 0x6E: goto _absolute; //ROR
		case 0x8C: goto _absolute; //STY
		case 0x8D: goto _absolute; //STA
		case 0x8E: goto _absolute; //STX
		case 0xAC: goto _absolute; //LDY
		case 0xAD: goto _absolute; //LDA
		case 0xAE: goto _absolute; //LDX
		case 0xCC: goto _absolute; //CPY
		case 0xCD: goto _absolute; //CMP
		case 0xCE: goto _absolute; //DEC
		case 0xEC: goto _absolute; //CPX
		case 0xED: goto _absolute; //SBC
		case 0xEE: goto _absolute; //INC
		_absolute:
			tmp = opcode[1] | opcode[2] << 8;
			sprintf(str, "%u|%u|%u", bzk_GetNesFileAddress(tmp), bzk_getBank(tmp), GetMem(tmp));
			break;
        
		//Absolute,X
		case 0x1D: goto _absolutex; //ORA
		case 0x1E: goto _absolutex; //ASL
		case 0x3D: goto _absolutex; //AND
		case 0x3E: goto _absolutex; //ROL
		case 0x5D: goto _absolutex; //EOR
		case 0x5E: goto _absolutex; //LSR
		case 0x7D: goto _absolutex; //ADC
		case 0x7E: goto _absolutex; //ROR
		case 0x9D: goto _absolutex; //STA
		case 0xBC: goto _absolutex; //LDY
		case 0xBD: goto _absolutex; //LDA
		case 0xDD: goto _absolutex; //CMP
		case 0xDE: goto _absolutex; //DEC
		case 0xFD: goto _absolutex; //SBC
		case 0xFE: goto _absolutex; //INC
		_absolutex:
			tmp = (opcode[1] | opcode[2] << 8) + X.X;
			sprintf(str, "%u|%u|%u", bzk_GetNesFileAddress(tmp), bzk_getBank(tmp), GetMem(tmp));
			break;
        
		//Absolute,Y
		case 0x19: goto _absolutey; //ORA
		case 0x39: goto _absolutey; //AND
		case 0x59: goto _absolutey; //EOR
		case 0x79: goto _absolutey; //ADC
		case 0x99: goto _absolutey; //STA
		case 0xB9: goto _absolutey; //LDA
		case 0xBE: goto _absolutey; //LDX
		case 0xD9: goto _absolutey; //CMP
		case 0xF9: goto _absolutey; //SBC
		_absolutey:
			tmp = (opcode[1] | opcode[2] << 8) + X.Y;
			sprintf(str, "%u|%u|%u", bzk_GetNesFileAddress(tmp), bzk_getBank(tmp), GetMem(tmp));
			break;
        
		//branches
		case 0x10: goto _branch; //BPL
		case 0x30: goto _branch; //BMI
		case 0x50: goto _branch; //BVC
		case 0x70: goto _branch; //BVS
		case 0x90: goto _branch; //BCC
		case 0xB0: goto _branch; //BCS
		case 0xD0: goto _branch; //BNE
		case 0xF0: goto _branch; //BEQ
		_branch:
            tmp = addr + opcode[1] + 0x02;
			if (opcode[1] >= 0x80) tmp -= 0x100;
			sprintf(str, "%u|%u|%u", bzk_GetNesFileAddress(tmp), bzk_getBank(tmp), GetMem(tmp));
			break;
        
		//(Indirect,X)
		case 0x01: goto _indirectx; //ORA
		case 0x21: goto _indirectx; //AND
		case 0x41: goto _indirectx; //EOR
		case 0x61: goto _indirectx; //ADC
		case 0x81: goto _indirectx; //STA
		case 0xA1: goto _indirectx; //LDA
		case 0xC1: goto _indirectx; //CMP
		case 0xE1: goto _indirectx; //SBC
		_indirectx:
            tmp = (opcode[1] + X.X) & 0xFF;
            tmp = GetMem((tmp)) | (GetMem(((tmp) + 1) & 0xFF)) << 8;
			sprintf(str, "%u|%u|%u", bzk_GetNesFileAddress(tmp), bzk_getBank(tmp), GetMem(tmp));
			break;
        
		//(Indirect),Y
		case 0x11: goto _indirecty; //ORA
		case 0x31: goto _indirecty; //AND
		case 0x51: goto _indirecty; //EOR
		case 0x71: goto _indirecty; //ADC
		case 0x91: goto _indirecty; //STA
		case 0xB1: goto _indirecty; //LDA
		case 0xD1: goto _indirecty; //CMP
		case 0xF1: goto _indirecty; //SBC
		_indirecty:
            tmp = (GetMem(opcode[1]) | (GetMem((opcode[1] + 1) & 0xFF)) << 8) + X.Y;
			sprintf(str, "%u|%u|%u", bzk_GetNesFileAddress(tmp), bzk_getBank(tmp), GetMem(tmp));
			break;
        
		//absolute jumps
		case 0x20: goto _jump; //JSR
		case 0x4C: goto _jump; //JMP
		_jump:
            tmp = opcode[1] | opcode[2] << 8;
            sprintf(str, "%u|%u|%u", bzk_GetNesFileAddress(tmp), bzk_getBank(tmp), GetMem(tmp));
			break;
        
        //indirect jump
		case 0x6C: //JMP
            tmp = opcode[1] | opcode[2] << 8;
            tmp = GetMem(tmp) | GetMem(tmp + 1) << 8;
            sprintf(str, "%u|%u|%u", bzk_GetNesFileAddress(tmp), bzk_getBank(tmp), GetMem(tmp));
            break;
        
        //return from subroutine
        case 0x60: //RTS
            tmp = GetMem(((X.S) + 1)|0x0100) + (GetMem(((X.S) + 2)|0x0100) << 8) + 0x01;
            sprintf(str, "%u|%u|%u", bzk_GetNesFileAddress(tmp), bzk_getBank(tmp), GetMem(tmp));
            break;
        
        //return from interrupt
        case 0x40: //RTI
            tmp = GetMem(((X.S) + 2)|0x0100) + (GetMem(((X.S) + 3)|0x0100) << 8);
            sprintf(str, "%u|%u|%u", bzk_GetNesFileAddress(tmp), bzk_getBank(tmp), GetMem(tmp));
            break;
        
        //for other opcodes, which are immediate and 1-byte instructions
		default:
            strcpy(str, "?");
            break;
	}

	return str;
}
