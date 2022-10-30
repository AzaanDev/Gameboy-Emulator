#pragma once

#include "Common.h"

class Bus;

enum Flags
{
	Z = (1 << 7), // Zero Flag
	N = (1 << 6), // Subtraction Flag
	H = (1 << 5), // Half Carry Flag
	C = (1 << 4)  // Carry Flag
};
//Conditions
enum Conditions
{
	CT_NONE, CT_C, CT_NC, CT_Z, CT_NZ
};

enum I_Types {
	I_VBLANK = 1, I_LCD_STAT = 2, I_TIMER = 4, I_SERIAL = 8, I_JOYPAD = 16
};

class LR35902
{
public:
	LR35902();
	~LR35902();

	void ConnectBus(Bus* n) { bus = n; }
	uint8_t FetchByte();
	uint16_t FetchWord();
	void Step();
	void Run();
	void Execute();
	void Clock();
	void Cycles(int i);
	void Tick();
	void HandleInterrupt();
	void RequestInterrupt(I_Types t);
	void DebugFlags();
	void Step(int counter);
	void DebugRegisters16();
	void DebugRegisters15();
	int step_counter = 0;
public:

	union RegU
	{
		uint16_t u;
		struct {
			uint8_t lo;
			uint8_t hi;
		};
	};

	struct Registers {
		RegU AF;
		RegU BC;
		RegU DE;
		RegU HL;
		uint16_t pc;
		uint16_t sp;
	} Regs;

	uint16_t& AF = Regs.AF.u;
	uint16_t& BC = Regs.BC.u;
	uint16_t& DE = Regs.DE.u;
	uint16_t& HL = Regs.HL.u;

	uint8_t&  A = Regs.AF.hi;
	uint8_t&  F = Regs.AF.lo;
	uint8_t&  B = Regs.BC.hi;
	uint8_t&  C = Regs.BC.lo;
	uint8_t&  D = Regs.DE.hi;
	uint8_t&  E = Regs.DE.lo;
	uint8_t&  H = Regs.HL.hi;
	uint8_t&  L = Regs.HL.lo;
	
	uint8_t opcode = 0;
	int cycles = 0;
	uint8_t interrupt_flags;
	uint8_t IE_Register;
	bool halt = false;
	bool master_interrupt;
	bool IME;

public:
	Bus* bus = nullptr;

	uint8_t read(uint16_t addr);
	void write(uint16_t addr, uint8_t data);

	uint8_t GetFlag(Flags f);
	void SetFlag(Flags f, bool v);
	bool CheckFlagCondition(Conditions c);
	void Handle(uint16_t address);
	bool CheckInterruptType(uint16_t addr, I_Types I);

public:
	// OPCODES
	void NOP();
	void STOP();
	void HALT();
	void EI();
	void DI();


	void ADD(const uint16_t v);
	void ADD_SP(const int8_t v);
	void ADD(uint8_t& A, const uint8_t v);
	void ADC(uint8_t& A, const uint8_t v);
	void AND(uint8_t& A, const uint8_t v);
	void SUB(uint8_t& A, const uint8_t v);
	void SBC(uint8_t& A, const uint8_t v);
	void XOR(uint8_t& A, const uint8_t v);
	void  OR(uint8_t& A, const uint8_t v);
	void  CP(const uint8_t reg);

	void INC(uint8_t& reg);
	void INC(uint16_t& reg);
	void DEC(uint8_t& reg);
	void DEC(uint16_t& reg);
	void SWAP(uint8_t& r);
	void DAA();

	void LD(uint8_t& r1, const uint8_t r2);
	void LD(uint8_t& r);
	void LD(uint16_t& r);
	void LD(uint16_t addr, uint8_t r2);


	void BIT(uint8_t bit,const uint8_t& reg);
	void RES(uint8_t bit, uint8_t& reg);
	void SET(uint8_t bit, uint8_t& reg);

	void JP(bool cond);
	void JP();
	void JR(bool cond);


	void StackPush(uint8_t data);
	void StackPush16(uint16_t data);
	uint8_t StackPop();
	uint16_t StackPop16();
	void CALL(bool cond);
	void RST(uint8_t v);
	void RET(bool cond);

	//Rotate
	void RLC(uint8_t& reg);
	void RRC(uint8_t& reg);
	void RL(uint8_t& reg);
	void RR(uint8_t& reg);
	void SLA(uint8_t& reg);
	void SRA(uint8_t& reg);
	void SRL(uint8_t& reg);
};

