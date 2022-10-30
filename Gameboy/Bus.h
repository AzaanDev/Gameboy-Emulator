#pragma once

#include "LR35902.h"
#include "cartridge.h"
#include "PPU.h"
#include <vector>


class Bus
{
public:
	Bus();
	~Bus();

	uint8_t read(uint16_t addr);
	void write(uint16_t addr, uint8_t data);
	void ConnectCart(const std::shared_ptr<Cartridge>& c);
	void DMA_Transfer(uint8_t data);

public:
	LR35902 cpu;
	PPU ppu;
	std::shared_ptr<Cartridge> cartridge;
	uint8_t wram[0x2000] = {0x0}; //Work Ram
	uint8_t hram[0x80] = {0x0};	  //High Ram
	uint8_t ioram[0x80] = {0x0};  //I/O Registers
	uint8_t vram[0x2000] = {0x0}; //VRAM
	struct OAM	oam[40] = {0x0};  //Object Attribute Memory


	uint16_t div;                 //Div Register
	uint8_t& tima = ioram[0x05];
	uint8_t& tma  = ioram[0x06];
	uint8_t& tac  = ioram[0x07];
	uint8_t& scy  = ioram[0x42];
	uint8_t& scx  = ioram[0x43];
	uint8_t& ly   = ioram[0x44];
	uint8_t& lyc  = ioram[0x45];
	uint8_t& bgp  = ioram[0x47];
	uint8_t& obp0 = ioram[0x48];
	uint8_t& obp1 = ioram[0x49];
	uint8_t& wy   = ioram[0x4A];
	uint8_t& wx   = ioram[0x4B];

	union {
		struct {
			uint8_t ModeFlag : 2; 
			uint8_t LYCFlag : 1; 
			uint8_t I_HBlank : 1; 
			uint8_t I_VBlank : 1; 
			uint8_t I_OAM : 1; 
			uint8_t I_LYC : 1; 
			uint8_t Empty : 1; 
		};
		uint8_t reg;
	} LCDStatus;

	union {
		struct {
			uint8_t BGEnable : 1; 
			uint8_t OBJEnable : 1; 
			uint8_t OBJSize : 1; 
			uint8_t BGTileMapArea : 1; 
			uint8_t BGWinTileDataArea : 1; 
			uint8_t WinEnable : 1;
			uint8_t WinTileMapArea : 1; 
			uint8_t LCDEnable : 1; 
		};
		uint8_t reg;
	} LCDControl;
};

/*
0000 - 7FFF  Cartridge     - 32KB
8000 - 9FFF  VRAM          - 8KB
A000 - BFFF  External RAM  - 8KB
C000 - DFFF  WRAM          - 8KB 
E000 - FDFF  ECHO RAM
FE00 - FE9F  OAM           - 160 or 0xA0
FF00 - FF7F  I/O Registers - 128 or 0x80
FF80 - FFFE  HRAM
FFFF - FFFF  IE
*/
