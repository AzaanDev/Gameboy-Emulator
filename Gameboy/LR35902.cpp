#include "LR35902.h"
#include "Bus.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
LR35902::LR35902()
{
	Regs.pc = 0x100;
	AF = 0x01B0;
	BC = 0x0013;
	DE = 0x00D8;
	HL = 0x014D;
	Regs.sp = 0xFFFE;
	IE_Register = 0;
	interrupt_flags = 0;
	IME = false;
	master_interrupt = false;
}

LR35902::~LR35902()
{
}

uint8_t LR35902::FetchByte()
{
	return read(Regs.pc++);
}

uint16_t LR35902::FetchWord()
{
	uint16_t low = FetchByte();
	uint16_t high = FetchByte();
	return low | (high << 8);
}

void LR35902::Step()
{
	if (!halt)
	{
		Execute();
	}
	else {
		Cycles(1);

		if (interrupt_flags) {
			halt = false;
		}
	}

	if (master_interrupt) {
		HandleInterrupt();
		IME = false;
	}

	if (IME) {
		master_interrupt = true;
	}

}

void LR35902::Run()
{
	while (true)
	{
		Step();
	}
}

void LR35902::Clock()
{
	for (int i = 0; i < (cycles * 4); i++) {
		Tick();
		bus->ppu.Tick();
	}
	cycles = 0;
}

void LR35902::Cycles(int i)
{
	cycles += i;
}

void LR35902::Tick()
{
	uint16_t prev = bus->div;
	bus->div++;
	bool update = false;
	switch (bus->tac & (0b11))
	{
	case 0b00:
		update = (prev & (1 << 9)) && (!(bus->div & (1 << 9)));
		break;
	case 0b01:
		update = (prev & (1 << 3)) && (!(bus->div & (1 << 3)));
		break;
	case 0b10:
		update = (prev & (1 << 5)) && (!(bus->div & (1 << 5)));
		break;
	case 0b11:
		update = (prev & (1 << 7)) && (!(bus->div & (1 << 7)));
		break;
	}
	if (update && bus->tac & (1 << 2))
	{
		bus->tima++;
		if (bus->tima == 0xFF)
		{
			bus->tima = bus->tma;
			RequestInterrupt(I_TIMER);
		}
	}
}

void LR35902::HandleInterrupt()
{
	if (CheckInterruptType(0x40, I_VBLANK)) {}
	else if (CheckInterruptType(0x48, I_LCD_STAT)) {}
	else if (CheckInterruptType(0x50, I_TIMER)) {}
	else if (CheckInterruptType(0x58, I_SERIAL)) {}
	else if (CheckInterruptType(0x60, I_JOYPAD)) {}
}

void LR35902::RequestInterrupt(I_Types t)
{
	interrupt_flags |= t;
}

void LR35902::DebugFlags()
{
	std::cout << "Z: " << (int)GetFlag(Flags::Z) << "  "
			  << "N: " << (int)GetFlag(Flags::N) << "  "
			  << "H: " << (int)GetFlag(Flags::H) << "  "
			  << "C: " << (int)GetFlag(Flags::C) << "  ";
	std::cout << std::endl;
}

void LR35902::Step(int counter)
{
	while (counter > 0)
	{
		Step();
		counter--;
	}
}

void LR35902::DebugRegisters16()
{
	std::cout << "A: " <<  std::hex << (int)A << "  "
			  << "F: " <<  std::hex << (int)F << "  "
			  << "BC: " <<  std::hex << (int)BC << "  "
			  << "DE: " <<  std::hex << (int)DE << "  "
			  << "HL: " << std::hex << (int)HL << "  "
			  << "PC: " << std::hex << (int)Regs.pc << "  ";
	std::cout << std::endl;
}

#include <stdio.h>

void LR35902::DebugRegisters15()
{
	FILE* fp;
	fp = fopen("file.txt", "a");
	fprintf(fp ,"A: %02X F: %c%c%c%c BC: %02X%02X DE: %02X%02X HL: %02X%02X PC:%04X\n",
		A, F & (1 << 7) ? 'Z' : '-',
		F & (1 << 6) ? 'N' : '-',
		F & (1 << 5) ? 'H' : '-',
		F & (1 << 4) ? 'C' : '-', B, C, D, E, H, L, Regs.pc);
	fclose(fp);
}

bool LR35902::CheckInterruptType(uint16_t addr, I_Types I)
{
	if (interrupt_flags & I && IE_Register & I)
	{
		Handle(addr);
		interrupt_flags &= ~I;
		halt = false;
		master_interrupt = false;

		return true;
	}
	return false;
}

void LR35902::Handle(uint16_t address)
{
	StackPush16(Regs.pc);
	Regs.pc = address;
}

void LR35902::Execute()
{
	opcode = FetchByte();
	Cycles(1);
	switch (opcode)
	{
	case 0x00:
		NOP();
		Cycles(1);
		break;
	case 0x01:
		LD(BC);
		break;
	case 0x02:
		LD(BC, A);
		break;
	case 0x03:
		INC(BC);
		Cycles(2);
		break;
	case 0x04:
		INC(B);
		Cycles(1);
		break;
	case 0x05:
		DEC(B);
		Cycles(1);
		break;
	case 0x06:
		LD(B);
		break;
	case 0x07:
		RLC(A);
		cycles--;
		SetFlag(Flags::Z, 0);
		break;
	case 0x08:
	{
		uint16_t addr = FetchWord();
		write(addr, Regs.sp & 0xFF);
		write(addr + 1, (Regs.sp & 0xFF00) >> 8);
		Cycles(5);
	}	break;
	case 0x09:
		ADD(BC);
		break;
	case 0x0A:
		LD(A, read(BC));
		Cycles(1);
		break;
	case 0x0B:
		DEC(BC);
		break;
	case 0x0C:
		INC(C);
		Cycles(1);
		break;
	case 0x0D:
		DEC(C);
		Cycles(1);
		break;
	case 0x0E:
		LD(C);
		break;
	case 0x0F:
		RRC(A);
		cycles--;
		SetFlag(Flags::Z, 0);
		break;
	case 0x10:
		STOP();
		break;
	case 0x11:
		LD(DE);
		break;
	case 0x12:
		LD(DE, A);
		break;
	case 0x13:
		INC(DE);
		Cycles(2);
		break;
	case 0x14:
		INC(D);
		Cycles(1);
		break;
	case 0x15:
		DEC(D);
		Cycles(1);
		break;
	case 0x16:
		LD(D);
		break;
	case 0x17:
		RL(A);
		cycles--;
		SetFlag(Flags::Z, 0);
		break;
	case 0x18:
		JR(true);
		break;
	case 0x19:
		ADD(DE);
		break;
	case 0x1A:
		LD(A, read(DE));
		Cycles(1);
		break;
	case 0x1B:
		DEC(DE);
		break;
	case 0x1C:
		INC(E);
		Cycles(1);
		break;
	case 0x1D:
		DEC(E);
		Cycles(1);
		break;
	case 0x1E:
		LD(E);
		break;
	case 0x1F:
		RR(A);
		cycles--;
		SetFlag(Flags::Z, 0);
		break;
	case 0x20:
		JR(CheckFlagCondition(CT_NZ));
		break;
	case 0x21:
		LD(HL);
		break;
	case 0x22:
		LD(HL, A);
		HL++;
		break;
	case 0x23:
		INC(HL);
		Cycles(2);
		break;
	case 0x24:
		INC(H);
		Cycles(1);
		break;
	case 0x25:
		DEC(H);
		Cycles(1);
		break;
	case 0x26:
		LD(H);
		break;
	case 0x27:
		DAA();
		break;
	case 0x28:
		JR(CheckFlagCondition(CT_Z));
		break;
	case 0x29:
		ADD(HL);
		break;
	case 0x2A:
		LD(A, read(HL));
		HL++;
		Cycles(1);
		break;
	case 0x2B:
		DEC(HL);
		break;
	case 0x2C:
		INC(L);
		Cycles(1);
		break;
	case 0x2D:
		DEC(L);
		Cycles(1);
		break;
	case 0x2E:
		LD(L);
		break;
	case 0x2F:
		A = ~A;
		SetFlag(Flags::N, 1);
		SetFlag(Flags::H, 1);
		Cycles(1);
		break;
	case 0x30:
		JR(CheckFlagCondition(CT_NC));
		break;
	case 0x31:
		LD(Regs.sp);
		break;
	case 0x32:
		LD(HL, A);
		HL--;
		break;
	case 0x33:
		INC(Regs.sp);
		Cycles(2);
		break;
	case 0x34:
	{
		uint8_t n8 = read(HL);
		INC(n8);
		write(HL, n8);
		Cycles(3);
	}	break;
	case 0x35:
	{
		uint8_t n8 = read(HL);
		DEC(n8);
		write(HL, n8);
		Cycles(3);
	}	break;
	case 0x36:
		write(HL, FetchByte());
		Cycles(3);
		break;
	case 0x37:
		SetFlag(Flags::N, 0);
		SetFlag(Flags::H, 0);
		SetFlag(Flags::C, 1);
		Cycles(1);
		break;
	case 0x38:
		JR(CheckFlagCondition(CT_C));
		break;
	case 0x39:
		ADD(Regs.sp);
		break;
	case 0x3A:
		LD(A, read(HL));
		HL--;
		Cycles(1);
		break;
	case 0x3B:
		DEC(Regs.sp);
		break;
	case 0x3C:
		INC(A);
		Cycles(1);
		break;
	case 0x3D:
		DEC(A);
		Cycles(1);
		break;
	case 0x3E:
		LD(A);
		break;
	case 0x3F:
		SetFlag(Flags::N, 0);
		SetFlag(Flags::H, 0);
		SetFlag(Flags::C, !(GetFlag(Flags::C)));
		Cycles(1);
		break;
	case 0x40:
		LD(B, B);
		break;
	case 0x41:
		LD(B, C);
		break;
	case 0x42:
		LD(B, D);
		break;
	case 0x43:
		LD(B, E);
		break;
	case 0x44:
		LD(B, H);
		break;
	case 0x45:
		LD(B, L);
		break;
	case 0x46:
		LD(B, read(HL));
		Cycles(1);
		break;
	case 0x47:
		LD(B, A);
		break;
	case 0x48:
		LD(C, B);
		break;
	case 0x49:
		LD(C, C);
		break;
	case 0x4A:
		LD(C, D);
		break;
	case 0x4B:
		LD(C, E);
		break;
	case 0x4C:
		LD(C, H);
		break;
	case 0x4D:
		LD(C, L);
		break;
	case 0x4E:
		LD(C, read(HL));
		Cycles(1);
		break;
	case 0x4F:
		LD(C, A);
		break;
	case 0x50:
		LD(D, B);
		break;
	case 0x51:
		LD(D, C);
		break;
	case 0x52:
		LD(D, D);
		break;
	case 0x53:
		LD(D, E);
		break;
	case 0x54:
		LD(D, H);
		break;
	case 0x55:
		LD(D, L);
		break;
	case 0x56:
		LD(D, read(HL));
		Cycles(1);
		break;
	case 0x57:
		LD(D, A);
		break;
	case 0x58:
		LD(E, B);
		break;
	case 0x59:
		LD(E, C);
		break;
	case 0x5A:
		LD(E, D);
		break;
	case 0x5B:
		LD(E, E);
		break;
	case 0x5C:
		LD(E, H);
		break;
	case 0x5D:
		LD(E, L);
		break;
	case 0x5E:
		LD(E, read(HL));
		Cycles(1);
		break;
	case 0x5F:
		LD(E, A);
		break;
	case 0x60:
		LD(H, B);
		break;
	case 0x61:
		LD(H, C);
		break;
	case 0x62:
		LD(H, D);
		break;
	case 0x63:
		LD(H, E);
		break;
	case 0x64:
		LD(H, H);
		break;
	case 0x65:
		LD(H, L);
		break;
	case 0x66:
		LD(H, read(HL));
		Cycles(1);
		break;
	case 0x67:
		LD(H, A);
		break;
	case 0x68:
		LD(L, B);
		break;
	case 0x69:
		LD(L, C);
		break;
	case 0x6A:
		LD(L, D);
		break;
	case 0x6B:
		LD(L, E);
		break;
	case 0x6C:
		LD(L, H);
		break;
	case 0x6D:
		LD(L, L);
		break;
	case 0x6E:
		LD(L, read(HL));
		Cycles(1);
		break;
	case 0x6F:
		LD(L, A);
		break;
	case 0x70:
		write(HL, B);
		Cycles(2);
		break;
	case 0x71:
		write(HL, C);
		Cycles(2);
		break;
	case 0x72:
		write(HL, D);
		Cycles(2);
		break;
	case 0x73:
		write(HL, E);
		Cycles(2);
		break;
	case 0x74:
		write(HL, H);
		Cycles(2);
		break;
	case 0x75:
		write(HL, L);
		Cycles(2);
		break;
	case 0x76:
		HALT();
		break;
	case 0x77:
		LD(HL, A);
		break;
	case 0x78:
		LD(A, B);
		break;
	case 0x79:
		LD(A, C);
		break;
	case 0x7A:
		LD(A, D);
		break;
	case 0x7B:
		LD(A, E);
		break;
	case 0x7C:
		LD(A, H);
		break;
	case 0x7D:
		LD(A, L);
		break;
	case 0x7E:
		LD(A, read(HL));
		Cycles(1);
		break;
	case 0x7F:
		LD(A, A);
		break;
	case 0x80:
		ADD(A, B);
		break;
	case 0x81:
		ADD(A, C);
		break;
	case 0x82:
		ADD(A, D);
		break;
	case 0x83:
		ADD(A, E);
		break;
	case 0x84:
		ADD(A, H);
		break;
	case 0x85:
		ADD(A, L);
		break;
	case 0x86:
		ADD(A, read(HL));
		Cycles(1);
		break;
	case 0x87:
		ADD(A, A);
		break;
	case 0x88:
		ADC(A, B);
		break;
	case 0x89:
		ADC(A, C);
		break;
	case 0x8A:
		ADC(A, D);
		break;
	case 0x8B:
		ADC(A, E);
		break;
	case 0x8C:
		ADC(A, H);
		break;
	case 0x8D:
		ADC(A, L);
		break;
	case 0x8E:
		ADC(A, read(HL));
		Cycles(1);
		break;
	case 0x8F:
		ADC(A, A);
		break;
	case 0x90:
		SUB(A, B);
		break;
	case 0x91:
		SUB(A, C);
		break;
	case 0x92:
		SUB(A, D);
		break;
	case 0x93:
		SUB(A, E);
		break;
	case 0x94:
		SUB(A, H);
		break;
	case 0x95:
		SUB(A, L);
		break;
	case 0x96:
		SUB(A, read(HL));
		Cycles(1);
		break;
	case 0x97:
		SUB(A, A);
		break;
	case 0x98:
		SBC(A, B);
		break;
	case 0x99:
		SBC(A, C);
		break;
	case 0x9A:
		SBC(A, D);
		break;
	case 0x9B:
		SBC(A, E);
		break;
	case 0x9C:
		SBC(A, H);
		break;
	case 0x9D:
		SBC(A, L);
		break;
	case 0x9E:
		SBC(A, read(HL));
		Cycles(1);
		break;
	case 0x9F:
		SBC(A, A);
		break;
	case 0xA0:
		AND(A, B);
		break;
	case 0xA1:
		AND(A, C);
		break;
	case 0xA2:
		AND(A, D);
		break;
	case 0xA3:
		AND(A, E);
		break;
	case 0xA4:
		AND(A, H);
		break;
	case 0xA5:
		AND(A, L);
		break;
	case 0xA6:
		AND(A, read(HL));
		Cycles(1);
		break;
	case 0xA7:
		AND(A, A);
		break;
	case 0xA8:
		XOR(A, B);
		break;
	case 0xA9:
		XOR(A, C);
		break;
	case 0xAA:
		XOR(A, D);
		break;
	case 0xAB:
		XOR(A, E);
		break;
	case 0xAC:
		XOR(A, H);
		break;
	case 0xAD:
		XOR(A, L);
		break;
	case 0xAE:
		XOR(A, read(HL));
		Cycles(1);
		break;
	case 0xAF:
		XOR(A, A);
		break;
	case 0xB0:
		OR(A, B);
		break;
	case 0xB1:
		OR(A, C);
		break;
	case 0xB2:
		OR(A, D);
		break;
	case 0xB3:
		OR(A, E);
		break;
	case 0xB4:
		OR(A, H);
		break;
	case 0xB5:
		OR(A, L);
		break;
	case 0xB6:
		OR(A, read(HL));
		Cycles(1);
		break;
	case 0xB7:
		OR(A, A);
		break;
	case 0xB8:
		CP(B);
		break;
	case 0xB9:
		CP(C);
		break;
	case 0xBA:
		CP(D);
		break;
	case 0xBB:
		CP(E);
		break;
	case 0xBC:
		CP(H);
		break;
	case 0xBD:
		CP(L);
		break;
	case 0xBE:
		CP(read(HL));
		Cycles(1);
		break;
	case 0xBF:
		CP(A);
		break;
	case 0xC0:
		RET(CheckFlagCondition(CT_NZ));
		break;
	case 0xC1:
		BC = StackPop16();
		Cycles(1);
		break;
	case 0xC2:
		JP(CheckFlagCondition(CT_NZ));
		break;
	case 0xC3:
		JP(true);
		break;
	case 0xC4:
		CALL(CheckFlagCondition(CT_NZ));
		break;
	case 0xC5:
		StackPush16(BC);
		break;
	case 0xC6:
		ADD(A, FetchByte());
		Cycles(1);
		break;
	case 0xC7:
		RST(0x0);
		break;
	case 0xC8:
		RET(CheckFlagCondition(CT_Z));
		break;
	case 0xC9:
		RET(true);
		cycles--;
		break;
	case 0xCA:
		JP(CheckFlagCondition(CT_Z));
		break;
	case 0xCC:
		CALL(CheckFlagCondition(CT_Z));
		break;
	case 0xCD:
		CALL(true);
		break;
	case 0xCE:
		ADC(A, FetchByte());
		Cycles(1);
		break;
	case 0xCF:
		RST(0x08);
		break;
	case 0xD0:
		RET(CheckFlagCondition(CT_NC));
		break;
	case 0xD1:
		DE = StackPop16();
		Cycles(1);
		break;
	case 0xD2:
		JP(CheckFlagCondition(CT_NC));
		break;
	case 0xD3:
		break;
	case 0xD4:
		CALL(CheckFlagCondition(CT_NC));
		break;
	case 0xD5:
		StackPush16(DE);
		break;
	case 0xD6:
		SUB(A, FetchByte());
		Cycles(1);
		break;
	case 0xD7:
		RST(0x10);
		break;
	case 0xD8:
		RET(CheckFlagCondition(CT_C));
		break;
	case 0xD9:
		master_interrupt = true;
		RET(true);
		break;
	case 0xDA:
		JP(CheckFlagCondition(CT_C));
		break;
	case 0xDB:
		break;
	case 0xDC:
		CALL(CheckFlagCondition(CT_C));
		break;
	case 0xDE:
		SBC(A, FetchByte());
		Cycles(1);
		break;
	case 0xDF:
		RST(0x18);
		break;
	case 0xE0:
		LD(0xFF00+FetchByte(), A);
		Cycles(1);
		break;
	case 0xE1:
		HL = StackPop16();
		Cycles(1);
		break;
	case 0xE2:
		LD(0xFF00 | C, A);
		break;
	case 0xE3:
		break;
	case 0xE4:
		break;
	case 0xE5:
		StackPush16(HL);
		break;
	case 0xE6:
		AND(A, FetchByte());
		Cycles(1);
		break;
	case 0xE7:
		RST(0x20);
		break;
	case 0xE8:
		ADD_SP((int8_t)FetchByte());
		break;
	case 0xE9:
		JP();
		break;
	case 0xEA:
		LD(FetchWord(), A);
		Cycles(2);
		break;
	case 0xEB:
		break;
	case 0xEC:
		break;
	case 0xED:
		break;
	case 0xEE:
		XOR(A, FetchByte());
		Cycles(1);
		break;
	case 0xEF:
		RST(0x28);
		break;
	case 0xF0:
		LD(A, read(0xFF00 + FetchByte()));
		Cycles(1);
		break;
	case 0xF1:
		AF = StackPop16() & 0xFFF0;
		Cycles(1);
		break;
	case 0xF2:
		LD(A, read(0xFF00 + C));
		Cycles(1);
		break;
	case 0xF3:
		DI();
		break;
	case 0xF4:
		break;
	case 0xF5:
		StackPush16(AF);
		break;
	case 0xF6:
		OR(A, FetchByte());
		Cycles(1);
		break;
	case 0xF7:
		RST(0x30);
		break;
	case 0xF8:
	{
		uint16_t i = Regs.sp;
		int8_t v = (int8_t)FetchByte();

		int r = (int)(i + v);
		SetFlag(Flags::Z, 0);
		SetFlag(Flags::N, 0);
		SetFlag(Flags::H, ((i ^ v ^ (r & 0xFFFF)) & 0x10) == 0x10);
		SetFlag(Flags::C, ((i ^ v ^ (r & 0xFFFF)) & 0x100) == 0x100);
		HL = (uint16_t)r;
	} break;
	case 0xF9:
		Regs.sp = HL;
		Cycles(2);
		break;
	case 0xFA:
		LD(A, read(FetchWord()));
		Cycles(3);
		break;
	case 0xFB:
		IME = true;
		Cycles(1);
		break;
	case 0xFC:
		break;
	case 0xFD:
		break;
	case 0xFE:
		CP(FetchByte());
		Cycles(1);
		break;
	case 0xFF:
		RST(0x38);
		break;
	case 0xCB:
		opcode = FetchByte();
		switch (opcode)
		{
		/*ROTATE OPCODES*/
		case 0x00:
			RLC(B);
			break;
		case 0x01:
			RLC(C);
			break;
		case 0x02:
			RLC(D);
			break;
		case 0x03:
			RLC(E);
			break;
		case 0x04:
			RLC(H);
			break;
		case 0x05:
			RLC(L);
			break;
		case 0x06:
		{
			uint8_t v = read(HL);
			RLC(v);
			write(HL, v);
			Cycles(2);
		} break;
		case 0x07:
			RLC(A);
			break;
		case 0x08:
			RRC(B);
			break;
		case 0x09:
			RRC(C);
			break;
		case 0x0A:
			RRC(D);
			break;
		case 0x0B:
			RRC(E);
			break;
		case 0x0C:
			RRC(H);
			break;
		case 0x0D:
			RRC(L);
			break;
		case 0x0E:
		{
			uint8_t v = read(HL);
			RRC(v);
			write(HL, v);
			Cycles(2);
		} break;
		case 0x0F:
			RRC(A);
			break;

		case 0x10:
			RL(B);
			break;
		case 0x11:
			RL(C);
			break;
		case 0x12:
			RL(D);
			break;
		case 0x13:
			RL(E);
			break;
		case 0x14:
			RL(H);
			break;
		case 0x15:
			RL(L);
			break;
		case 0x16:
		{
			uint8_t v = read(HL);
			RL(v);
			write(HL, v);
			Cycles(2);
		} break;
		case 0x17:
			RL(A);
			break;
		case 0x18:
			RR(B);
			break;
		case 0x19:
			RR(C);
			break;
		case 0x1A:
			RR(D);
			break;
		case 0x1B:
			RR(E);
			break;
		case 0x1C:
			RR(H);
			break;
		case 0x1D:
			RR(L);
			break;
		case 0x1E:
		{
			uint8_t v = read(HL);
			RR(v);
			write(HL, v);
			Cycles(2);
		} break;
		case 0x1F:
			RR(A);
			break;

		case 0x20:
			SLA(B);
			break;
		case 0x21:
			SLA(C);
			break;
		case 0x22:
			SLA(D);
			break;
		case 0x23:
			SLA(E);
			break;
		case 0x24:
			SLA(H);
			break;
		case 0x25:
			SLA(L);
			break;
		case 0x26:
		{
			uint8_t v = read(HL);
			SLA(v);
			write(HL, v);
			Cycles(2);
		} break;
		case 0x27:
			SLA(A);
			break;
		case 0x28:
			SRA(B);
			break;
		case 0x29:
			SRA(C);
			break;
		case 0x2A:
			SRA(D);
			break;
		case 0x2B:
			SRA(E);
			break;
		case 0x2C:
			SRA(H);
			break;
		case 0x2D:
			SRA(L);
			break;
		case 0x2E:
		{
			uint8_t v = read(HL);
			SRA(v);
			write(HL, v);
			Cycles(2);
		} break;
		case 0x2F:
			SRA(A);
			break;
		case 0x30:
			SWAP(B);
			break;
		case 0x31:
			SWAP(C);
			break;
		case 0x32:
			SWAP(D);
			break;
		case 0x33:
			SWAP(E);
			break;
		case 0x34:
			SWAP(H);
			break;
		case 0x35:
			SWAP(L);
			break;
		case 0x36:
		{
			uint8_t v = read(HL);
			SWAP(v);
			write(HL, v);
			Cycles(2);
		} break;
		case 0x37:
			SWAP(A);
			break;
		case 0x38:
			SRL(B);
			break;
		case 0x39:
			SRL(C);
			break;
		case 0x3A:
			SRL(D);
			break;
		case 0x3B:
			SRL(E);
			break;
		case 0x3C:
			SRL(H);
			break;
		case 0x3D:
			SRL(L);
			break;
		case 0x3E:
		{
			uint8_t v = read(HL);
			SRL(v);
			write(HL, v);
			Cycles(2);
		} break;
		case 0x3F:
			SRL(A);
			break;
		/*BIT OPCODES*/
		case 0x40:
			BIT(0, B);
			break;
		case 0x41:
			BIT(0, C);
			break;
		case 0x42:
			BIT(0, D);
			break;
		case 0x43:
			BIT(0, E);
			break;
		case 0x44:
			BIT(0, H);
			break;
		case 0x45:
			BIT(0, L);
			break;
		case 0x46:
			BIT(0, read(HL));
			Cycles(1);
			break;
		case 0x47:
			BIT(0, A);
			break;
		case 0x48:
			BIT(1, B);
			break;
		case 0x49:
			BIT(1, C);
			break;
		case 0x4A:
			BIT(1, D);
			break;
		case 0x4B:
			BIT(1, E);
			break;
		case 0x4C:
			BIT(1, H);
			break;
		case 0x4D:
			BIT(1, L);
			break;
		case 0x4E:
			BIT(1, read(HL));
			Cycles(1);
			break;
		case 0x4F:
			BIT(1, A);
			break;
		case 0x50:
			BIT(2, B);
			break;
		case 0x51:
			BIT(2, C);
			break;
		case 0x52:
			BIT(2, D);
			break;
		case 0x53:
			BIT(2, E);
			break;
		case 0x54:
			BIT(2, H);
			break;
		case 0x55:
			BIT(2, L);
			break;
		case 0x56:
			BIT(2, read(HL));
			Cycles(1);
			break;
		case 0x57:
			BIT(2, A);
			break;
		case 0x58:
			BIT(3, B);
			break;
		case 0x59:
			BIT(3, C);
			break;
		case 0x5A:
			BIT(3, D);
			break;
		case 0x5B:
			BIT(3, E);
			break;
		case 0x5C:
			BIT(3, H);
			break;
		case 0x5D:
			BIT(3, L);
			break;
		case 0x5E:
			BIT(3, read(HL));
			Cycles(1);
			break;
		case 0x5F:
			BIT(3, A);
			break;
		case 0x60:
			BIT(4, B);
			break;
		case 0x61:
			BIT(4, C);
			break;
		case 0x62:
			BIT(4, D);
			break;
		case 0x63:
			BIT(4, E);
			break;
		case 0x64:
			BIT(4, H);
			break;
		case 0x65:
			BIT(4, L);
			break;
		case 0x66:
			BIT(4, read(HL));
			Cycles(1);
			break;
		case 0x67:
			BIT(4, A);
			break;
		case 0x68:
			BIT(5, B);
			break;
		case 0x69:
			BIT(5, C);
			break;
		case 0x6A:
			BIT(5, D);
			break;
		case 0x6B:
			BIT(5, E);
			break;
		case 0x6C:
			BIT(5, H);
			break;
		case 0x6D:
			BIT(5, L);
			break;
		case 0x6E:
			BIT(5, read(HL));
			Cycles(1);
			break;
		case 0x6F:
			BIT(5, A);
			break;
		case 0x70:
			BIT(6, B);
			break;
		case 0x71:
			BIT(6, C);
			break;
		case 0x72:
			BIT(6, D);
			break;
		case 0x73:
			BIT(6, E);
			break;
		case 0x74:
			BIT(6, H);
			break;
		case 0x75:
			BIT(6, L);
			break;
		case 0x76:
			BIT(6, read(HL));
			Cycles(1);
			break;
		case 0x77:
			BIT(6, A);
			break;
		case 0x78:
			BIT(7, B);
			break;
		case 0x79:
			BIT(7, C);
			break;
		case 0x7A:
			BIT(7, D);
			break;
		case 0x7B:
			BIT(7, E);
			break;
		case 0x7C:
			BIT(7, H);
			break;
		case 0x7D:
			BIT(7, L);
			break;
		case 0x7E:
			BIT(7, read(HL));
			Cycles(1);
			break;
		case 0x7F:
			BIT(7, A);
			break;

		/*RES OPCODES*/
		case 0x80:
			RES(0, B);
			break;
		case 0x81:
			RES(0, C);
			break;
		case 0x82:
			RES(0, D);
			break;
		case 0x83:
			RES(0, E);
			break;
		case 0x84:
			RES(0, H);
			break;
		case 0x85:
			RES(0, L);
			break;
		case 0x86:
		{
			uint8_t v = read(HL);
			RES(0, v);
			write(HL, v);
			Cycles(2);
		} break;
		case 0x87:
			RES(0, A);
			break;
		case 0x88:
			RES(1, B);
			break;
		case 0x89:
			RES(1, C);
			break;
		case 0x8A:
			RES(1, D);
			break;
		case 0x8B:
			RES(1, E);
			break;
		case 0x8C:
			RES(1, H);
			break;
		case 0x8D:
			RES(1, L);
			break;
		case 0x8E:
		{
			uint8_t v = read(HL);
			RES(1, v);
			write(HL, v);
			Cycles(2);
		} break;
		case 0x8F:
			RES(1, A);
			break;
		case 0x90:
			RES(2, B);
			break;
		case 0x91:
			RES(2, C);
			break;
		case 0x92:
			RES(2, D);
			break;
		case 0x93:
			RES(2, E);
			break;
		case 0x94:
			RES(2, H);
			break;
		case 0x95:
			RES(2, L);
			break;
		case 0x96:
		{
			uint8_t v = read(HL);
			RES(2, v);
			write(HL, v);
			Cycles(2);
		} break;
		case 0x97:
			RES(2, A);
			break;
		case 0x98:
			RES(3, B);
			break;
		case 0x99:
			RES(3, C);
			break;
		case 0x9A:
			RES(3, D);
			break;
		case 0x9B:
			RES(3, E);
			break;
		case 0x9C:
			RES(3, H);
			break;
		case 0x9D:
			RES(3, L);
			break;
		case 0x9E:
		{
			uint8_t v = read(HL);
			RES(3, v);
			write(HL, v);
			Cycles(2);
		} break;
		case 0x9F:
			RES(3, A);
			break;
		case 0xA0:
			RES(4, B);
			break;
		case 0xA1:
			RES(4, C);
			break;
		case 0xA2:
			RES(4, D);
			break;
		case 0xA3:
			RES(4, E);
			break;
		case 0xA4:
			RES(4, H);
			break;
		case 0xA5:
			RES(4, L);
			break;
		case 0xA6:
		{
			uint8_t v = read(HL);
			RES(4, v);
			write(HL, v);
			Cycles(2);
		} break;
		case 0xA7:
			RES(4, A);
			break;
		case 0xA8:
			RES(5, B);
			break;
		case 0xA9:
			RES(5, C);
			break;
		case 0xAA:
			RES(5, D);
			break;
		case 0xAB:
			RES(5, E);
			break;
		case 0xAC:
			RES(5, H);
			break;
		case 0xAD:
			RES(5, L);
			break;
		case 0xAE:
		{
			uint8_t v = read(HL);
			RES(5, v);
			write(HL, v);
			Cycles(2);
		} break;
		case 0xAF:
			RES(5, A);
			break;
		case 0xB0:
			RES(6, B);
			break;
		case 0xB1:
			RES(6, C);
			break;
		case 0xB2:
			RES(6, D);
			break;
		case 0xB3:
			RES(6, E);
			break;
		case 0xB4:
			RES(6, H);
			break;
		case 0xB5:
			RES(6, L);
			break;
		case 0xB6:
		{
			uint8_t v = read(HL);
			RES(6, v);
			write(HL, v);
			Cycles(2);
		} break;
		case 0xB7:
			RES(6, A);
			break;
		case 0xB8:
			RES(7, B);
			break;
		case 0xB9:
			RES(7, C);
			break;
		case 0xBA:
			RES(7, D);
			break;
		case 0xBB:
			RES(7, E);
			break;
		case 0xBC:
			RES(7, H);
			break;
		case 0xBD:
			RES(7, L);
			break;
		case 0xBE:
		{
			uint8_t v = read(HL);
			RES(7, v);
			write(HL, v);
			Cycles(2);
		} break;
		case 0xBF:
			RES(7, A);
			break;

		/*SET OPCODES*/
		case 0xC0:
			SET(0, B);
			break;
		case 0xC1:
			SET(0, C);
			break;
		case 0xC2:
			SET(0, D);
			break;
		case 0xC3:
			SET(0, E);
			break;
		case 0xC4:
			SET(0, H);
			break;
		case 0xC5:
			SET(0, L);
			break;
		case 0xC6:
		{
			uint8_t v = read(HL);
			SET(0, v);
			write(HL, v);
			Cycles(2);
		} break;
		case 0xC7:
			SET(0, A);
			break;
		case 0xC8:
			SET(1, B);
			break;
		case 0xC9:
			SET(1, C);
			break;
		case 0xCA:
			SET(1, D);
			break;
		case 0xCB:
			SET(1, E);
			break;
		case 0xCC:
			SET(1, H);
			break;
		case 0xCD:
			SET(1, L);
			break;
		case 0xCE:
		{
			uint8_t v = read(HL);
			SET(1, v);
			write(HL, v);
			Cycles(2);
		} break;
		case 0xCF:
			SET(1, A);
			break;
		case 0xD0:
			SET(2, B);
			break;
		case 0xD1:
			SET(2, C);
			break;
		case 0xD2:
			SET(2, D);
			break;
		case 0xD3:
			SET(2, E);
			break;
		case 0xD4:
			SET(2, H);
			break;
		case 0xD5:
			SET(2, L);
			break;
		case 0xD6:
		{
			uint8_t v = read(HL);
			SET(2, v);
			write(HL, v);
			Cycles(2);
		} break;
		case 0xD7:
			SET(2, A);
			break;
		case 0xD8:
			SET(3, B);
			break;
		case 0xD9:
			SET(3, C);
			break;
		case 0xDA:
			SET(3, D);
			break;
		case 0xDB:
			SET(3, E);
			break;
		case 0xDC:
			SET(3, H);
			break;
		case 0xDD:
			SET(3, L);
			break;
		case 0xDE:
		{
			uint8_t v = read(HL);
			SET(3, v);
			write(HL, v);
			Cycles(2);
		} break;
		case 0xDF:
			SET(3, A);
			break;
		case 0xE0:
			SET(4, B);
			break;
		case 0xE1:
			SET(4, C);
			break;
		case 0xE2:
			SET(4, D);
			break;
		case 0xE3:
			SET(4, E);
			break;
		case 0xE4:
			SET(4, H);
			break;
		case 0xE5:
			SET(4, L);
			break;
		case 0xE6:
		{
			uint8_t v = read(HL);
			SET(4, v);
			write(HL, v);
			Cycles(2);
		} break;
		case 0xE7:
			SET(4, A);
			break;
		case 0xE8:
			SET(5, B);
			break;
		case 0xE9:
			SET(5, C);
			break;
		case 0xEA:
			SET(5, D);
			break;
		case 0xEB:
			SET(5, E);
			break;
		case 0xEC:
			SET(5, H);
			break;
		case 0xED:
			SET(5, L);
			break;
		case 0xEE:
		{
			uint8_t v = read(HL);
			SET(5, v);
			write(HL, v);
			Cycles(2);
		} break;
		case 0xEF:
			SET(5, A);
			break;
		case 0xF0:
			SET(6, B);
			break;
		case 0xF1:
			SET(6, C);
			break;
		case 0xF2:
			SET(6, D);
			break;
		case 0xF3:
			SET(6, E);
			break;
		case 0xF4:
			SET(6, H);
			break;
		case 0xF5:
			SET(6, L);
			break;
		case 0xF6:
		{
			uint8_t v = read(HL);
			SET(6, v);
			write(HL, v);
			Cycles(2);
		} break;
		case 0xF7:
			SET(6, A);
			break;
		case 0xF8:
			SET(7, B);
			break;
		case 0xF9:
			SET(7, C);
			break;
		case 0xFA:
			SET(7, D);
			break;
		case 0xFB:
			SET(7, E);
			break;
		case 0xFC:
			SET(7, H);
			break;
		case 0xFD:
			SET(7, L);
			break;
		case 0xFE:
		{
			uint8_t v = read(HL);
			SET(7, v);
			write(HL, v);
			Cycles(2);
		} break;
		case 0xFF:
			SET(7, A);
			break;

		}
		break;
	}
}

uint8_t LR35902::GetFlag(Flags f)
{
	return ((F & f) > 0) ? 1 : 0;
}

void LR35902::SetFlag(Flags f, bool v)
{
	if (v) // Sets
		F |= f; 
	else // Resets
		F &= ~f;
}

bool LR35902::CheckFlagCondition(Conditions c)
{
	bool Z = GetFlag(Flags::Z);
	bool C = GetFlag(Flags::C);
	switch (c)
	{
	case CT_NONE:
		return true;
	case CT_C:
		return C;
	case CT_NC:
		return !C;
	case CT_Z:
		return Z;
	case CT_NZ:
		return !Z;
	}
	return false;
}


uint8_t LR35902::read(uint16_t addr)
{
	return bus->read(addr);
}

void LR35902::write(uint16_t addr, uint8_t data)
{
	bus->write(addr, data);
}

void LR35902::NOP()
{
}

void LR35902::STOP()
{
	std::cout << "Stopping " << std::endl;
	Cycles(1);
}

void LR35902::HALT()
{
	halt = true;
	Cycles(1);
}

void LR35902::EI()
{
	IME = true;
	Cycles(1);
}

void LR35902::DI()
{
	master_interrupt = false;
	Cycles(1);
}

void LR35902::LD(uint8_t& r1, const uint8_t r2)
{
	r1 = r2;
	Cycles(1);
}

void LR35902::LD(uint8_t& r)
{
	r = FetchByte();
	Cycles(2);
}

void LR35902::LD(uint16_t& r)
{
	r = FetchWord();
	Cycles(3);
}

void LR35902::LD(uint16_t addr, uint8_t r2)
{
	write(addr, r2);
	Cycles(2);
}

void LR35902::DAA()
{
	uint8_t u = 0;
	int fc = 0;

	if (GetFlag(Flags::H) || (!GetFlag(Flags::N) && (A & 0xF) > 9)) {
		u = 6;
	}

	if (GetFlag(Flags::C) || (!GetFlag(Flags::N) && A > 0x99)) {
		u |= 0x60;
		fc = 1;
	}

	A += GetFlag(Flags::N) ? -u : u;

	SetFlag(Flags::Z, A == 0);
	SetFlag(Flags::H, 0);
	SetFlag(Flags::C, fc);
	Cycles(1);
}

//ADD HL
void LR35902::ADD(const uint16_t v)
{
	unsigned int r = HL + v;
	SetFlag(Flags::N, 0);
	SetFlag(Flags::H, ((HL & 0xFFF) + (v & 0xFFF) > 0xFFF));
	SetFlag(Flags::C, r > 0xFFFF);
	HL = (uint16_t)r;
	Cycles(2);
}

//ADD SP
void LR35902::ADD_SP(const int8_t v)
{
	uint16_t i = Regs.sp;
	int r = (int)(i + v);
	SetFlag(Flags::Z, 0);
	SetFlag(Flags::N, 0);
	SetFlag(Flags::H, ((i ^ v ^ (r & 0xFFFF)) & 0x10) == 0x10);
	SetFlag(Flags::C, ((i ^ v ^ (r & 0xFFFF)) & 0x100) == 0x100);
	Regs.sp = (uint16_t)r;
	Cycles(4);
}

//8 bit ADD 
void LR35902::ADD(uint8_t& reg, const uint8_t v)
{
	unsigned int r = reg + v;
	SetFlag(Flags::Z, (uint8_t)r == 0);
	SetFlag(Flags::N, 0);
	SetFlag(Flags::H, ((reg & 0xF) + (v & 0xF) > 0xF));
	SetFlag(Flags::C, r > 0xFF);

	reg = (uint8_t)r;
	Cycles(1);
}

void LR35902::ADC(uint8_t& reg, const uint8_t v)
{
	unsigned int r = reg + v + GetFlag(Flags::C);
	
	SetFlag(Flags::Z, (uint8_t)r == 0);
	SetFlag(Flags::N, 0);
	SetFlag(Flags::H, ((reg & 0xF) + (v & 0xF) + GetFlag(Flags::C)) > 0xF);
	SetFlag(Flags::C,  r > 0xFF);

	reg = (uint8_t)r;
	Cycles(1);
}

void LR35902::AND(uint8_t& A, const uint8_t v)
{
	A &= v;
	
	SetFlag(Flags::Z, A == 0);
	SetFlag(Flags::N, 0);
	SetFlag(Flags::H, 1);
	SetFlag(Flags::C, 0);
	Cycles(1);
}

void LR35902::SUB(uint8_t& reg, const uint8_t v)
{
	uint8_t r = reg - v;
	SetFlag(Flags::Z, r == 0);
	SetFlag(Flags::N, 1);
	SetFlag(Flags::H, ((reg & 0xF) - (v & 0xF)) < 0);
	SetFlag(Flags::C, reg < v);
	reg = r;
	Cycles(1);
}

void LR35902::SBC(uint8_t& reg, const uint8_t v)
{
	int r = reg - v - GetFlag(Flags::C);
	
	SetFlag(Flags::Z, (uint8_t)r == 0);
	SetFlag(Flags::N, 1);
	SetFlag(Flags::H, ((reg & 0xF) - (v & 0xF) - GetFlag(Flags::C)) < 0);
	SetFlag(Flags::C, r < 0);
	reg = (uint8_t)r;
	Cycles(1);
}

void LR35902::XOR(uint8_t& A, const uint8_t v)
{
	A ^= v;
	F = 0;
	SetFlag(Flags::Z, A == 0);
	Cycles(1);
}

void LR35902::OR(uint8_t& A, const uint8_t v)
{
	A |= v;
	F = 0;
	SetFlag(Flags::Z, A == 0);
	Cycles(1);
}

void LR35902::CP(const uint8_t reg)
{
	uint8_t r = (uint8_t)(A - reg);
	SetFlag(Flags::Z, r == 0);
	SetFlag(Flags::N, 1);
	SetFlag(Flags::H, (((A & 0xF) - (reg & 0xF)) < 0));
	SetFlag(Flags::C, A < reg);
	Cycles(1);
}

void LR35902::INC(uint16_t& reg)
{
	reg++;
}

void LR35902::DEC(uint8_t& reg)
{
	reg--;
	SetFlag(Flags::Z, reg == 0);
	SetFlag(Flags::N, 1);
	SetFlag(Flags::H, (reg & 0x0F) == 0x0F);
}

void LR35902::DEC(uint16_t& reg)
{
	reg--;
	Cycles(2);
}

void LR35902::SWAP(uint8_t& r)
{
	r = ((r & 0x0F) << 4 | (r & 0xF0) >> 4);
	SetFlag(Flags::Z, r == 0);
	SetFlag(Flags::N, 0);
	SetFlag(Flags::H, 0);
	SetFlag(Flags::C, 0);
	Cycles(2);
}

void LR35902::StackPush(uint8_t data)
{
	Regs.sp--;
	write(Regs.sp, data);
	Cycles(2);
}

void LR35902::StackPush16(uint16_t data)
{
	StackPush((data >> 8) & 0xFF);
	StackPush(data & 0xFF);
}

uint8_t LR35902::StackPop()
{
	Cycles(1);
	return read(Regs.sp++);
}

uint16_t LR35902::StackPop16()
{
	uint16_t lo = StackPop();
	uint16_t hi = StackPop();
	return (hi << 8) | lo;
}

void LR35902::CALL(bool cond)
{
	uint16_t nn = FetchWord();
	if (cond)
	{
		StackPush16(Regs.pc);
		Regs.pc = nn;
		Cycles(4);
	}
	else {
		Cycles(3);
	}
}

void LR35902::RST(uint8_t v)
{
	StackPush16(Regs.pc);
	Regs.pc = v;
}

void LR35902::RET(bool cond)
{
	if (cond)
	{
		Regs.pc = StackPop16();
		Cycles(3);
	}
	else {
		Cycles(2);
	}
}

void LR35902::RLC(uint8_t& reg)
{
	reg = (reg << 1) | (reg >> (8 - 1));
	SetFlag(Flags::Z, reg == 0);
	SetFlag(Flags::N, 0);
	SetFlag(Flags::H, 0);
	SetFlag(Flags::C, (reg & 1));
	Cycles(2);
}

void LR35902::RRC(uint8_t& reg)
{
	reg = (reg >> 1) | (reg << (8 - 1));
	SetFlag(Flags::Z, reg == 0);
	SetFlag(Flags::N, 0);
	SetFlag(Flags::H, 0);
	SetFlag(Flags::C, (reg >> 7) & 1);
	Cycles(2);
}

void LR35902::RL(uint8_t& reg)
{
	uint8_t bit7 = (reg >> 7) & 1;
	reg = (reg << 1);
	reg |= GetFlag(Flags::C);

	SetFlag(Flags::Z, reg == 0);
	SetFlag(Flags::N, 0);
	SetFlag(Flags::H, 0);
	SetFlag(Flags::C, bit7);
	Cycles(2);
}

void LR35902::RR(uint8_t& reg)
{
	uint8_t bit0 = reg & 1;
	reg = (reg >> 1);
	
	reg |= (GetFlag(Flags::C) << 7);

	SetFlag(Flags::Z, reg == 0);
	SetFlag(Flags::N, 0);
	SetFlag(Flags::H, 0);
	SetFlag(Flags::C, bit0);
	Cycles(2);
}

void LR35902::SLA(uint8_t& reg)
{
	uint8_t bit7 = (reg >> 7) & 1;
	reg = (reg << 1);
	SetFlag(Flags::Z, reg == 0);
	SetFlag(Flags::N, 0);
	SetFlag(Flags::H, 0);
	SetFlag(Flags::C, bit7);
	Cycles(2);
}

void LR35902::SRA(uint8_t& reg)
{
	uint8_t bit0 = reg & 1;
	uint8_t bit7 = (reg >> 7) & 1;
	reg = (reg >> 1);
	reg |= (bit7 << 7);

	SetFlag(Flags::Z, reg == 0);
	SetFlag(Flags::N, 0);
	SetFlag(Flags::H, 0);
	SetFlag(Flags::C, bit0);
	Cycles(2);
}

void LR35902::SRL(uint8_t& reg)
{
	uint8_t bit0 = reg & 1;
	reg = (reg >> 1);

	SetFlag(Flags::Z, reg == 0);
	SetFlag(Flags::N, 0);
	SetFlag(Flags::H, 0);
	SetFlag(Flags::C, bit0);
	Cycles(2);
}

void LR35902::INC(uint8_t& reg)
{
	reg++;
	SetFlag(Flags::Z, reg == 0);
	SetFlag(Flags::N, 0);
	SetFlag(Flags::H, (reg & 0x0F) == 0x0);
}

void LR35902::BIT(uint8_t bit, const uint8_t& reg)
{
	SetFlag(Flags::Z, !((reg >> bit) & 1));
	SetFlag(Flags::N, 0);
	SetFlag(Flags::H, 1);
	Cycles(2);
}

void LR35902::RES(uint8_t bit, uint8_t& reg)
{
	reg &= ~(1 << bit);
	Cycles(2);
}

void LR35902::SET(uint8_t bit, uint8_t& reg)
{
	reg |= 1 << bit;
	Cycles(2);
}

void LR35902::JP(bool cond)
{
	uint16_t n16 = FetchWord();
	if (cond)
	{
		Regs.pc = n16;
		Cycles(4);
	}
	else
	{
		Cycles(3);
	}
}

void LR35902::JP()
{
	Regs.pc = HL;
	Cycles(1);
}

void LR35902::JR(bool cond)
{
	int8_t s8 = (int8_t)FetchByte();
	if (cond)
	{
		Regs.pc = (Regs.pc + s8);
		Cycles(3);
	}
	else
	{
		Cycles(2);
	}
}
