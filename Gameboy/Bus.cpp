#include "Bus.h"
#include <iostream>
#include <fstream>
#include <iostream>


Bus::Bus()
{
    div = 0xAC00;
    write(0xFF05, 0x00);  // TIMA
    write(0xFF06, 0x00);  // TMA
    write(0xFF07, 0x00);  // TAC
    write(0xFF10, 0x80);  // NR10
    write(0xFF11, 0xBF);  // NR11
    write(0xFF12, 0xF3);  // NR12
    write(0xFF14, 0xBF);  // NR14
    write(0xFF16, 0x3F);  // NR21
    write(0xFF17, 0x00);  // NR22
    write(0xFF19, 0xBF);  // NR24
    write(0xFF1A, 0x7F);  // NR30
    write(0xFF1B, 0xFF);  // NR31
    write(0xFF1C, 0x9F);  // NR32
    write(0xFF1E, 0xBF);  // NR33
    write(0xFF20, 0xFF);  // NR41
    write(0xFF21, 0x00);  // NR42
    write(0xFF22, 0x00);  // NR43
    write(0xFF23, 0xBF);  // NR30
    write(0xFF24, 0x77);  // NR50
    write(0xFF25, 0xF3);  // NR51
    write(0xFF26, 0xF1);  // NR52
    write(0xFF40, 0x91);  // LCDC
    write(0xFF42, 0x00);  // SCY
    write(0xFF43, 0x00);  // SCX
    write(0xFF45, 0x00);  // LYC
    write(0xFF47, 0xFC);  // BGP
    write(0xFF48, 0xFF);  // OBP0
    write(0xFF49, 0xFF);  // OBP1
    write(0xFF4A, 0x00);  // WY
    write(0xFF4B, 0x00);  // WX
    write(0xFFFF, 0x00);  // IE

	cpu.ConnectBus(this);
    ppu.ConnectBus(this);

    LCDStatus.ModeFlag = LCD_MODE::OAM;
}

Bus::~Bus()
{
}

void Bus::ConnectCart(const std::shared_ptr<Cartridge>& c)
{
	this->cartridge = c;
}

void Bus::DMA_Transfer(uint8_t data)
{
    uint16_t addr = data << 8;
    for (int i = 0; i < 0xA0; i++)
    {
        write(0xFE00 + i, read(addr + i));
    }
}

uint8_t Bus::read(uint16_t address)
{
    if (address < 0x8000)
    {
        return (cartridge->read(address));
    }
    else if (address < 0xA000) //VRAM
    {
        return vram[address - 0x8000];
    }
    else if (address < 0xC000)//Cartridge Ram
    {
        return (cartridge->read(address));
    }
    else if (address < 0xE000)//WRAM
    {
        return wram[address - 0xC000];
    }
    else if (address < 0xFE00) //Reserved Echo Ram
    {
        return 0;
    }
    else if (address < 0xFEA0) //OAM
    {
        uint8_t* bytes = (uint8_t*)oam;
        return bytes[address - 0xFE00];
    }
    else if (address == 0xFF00)
    {
        return ppu.JoyPadOut();
    }
    else if (address < 0xFF00) //Not Usable/ Reserved Ram
    {
        return 0;
    }
    else if (address == 0xFF04) // Div Register
    {
        return (div >> 8);
    }
    else if (address == 0xFF0F) // Interrupt Flag Register
    {
        return cpu.interrupt_flags;
    }
    else if (address == 0xFF40) // LCDC Register
    {
        return LCDControl.reg;
    }
    else if (address == 0xFF41) // LCDS Register
    {
        return LCDStatus.reg;
    }
    else if (address < 0xFF80) // I/O Registers
    {
        return ioram[address - 0xFF00];
    }
    else if (address == 0xFFFF) //Interrupt Enable Register
    {
        return cpu.IE_Register;
    }

    return hram[address - 0xFF80];
}

void Bus::write(uint16_t address, uint8_t data)
{
    if (address == 0xFF02 && data == 0x81) {
        std::cout << read(0xFF01);
    }
    if (address < 0x8000) 
    {
        cartridge->write(address, data);
    } //ROM BANKS
    else if (address < 0xA000) //VRAM
    {
        vram[address - 0x8000] = data;
    }
    else if (address < 0xC000)//Cartridge Ram
    {
        (cartridge->write(address, data));
    }
    else if (address < 0xE000)//WRAM
    {
        wram[address - 0xC000] = data;
    }
    else if (address < 0xFE00) //Reserved Echo Ram
    {
        std::cout << "Reserved Echo Ram" << std::endl;
    }
    else if (address < 0xFEA0) //OAM
    {
        uint8_t* bytes = (uint8_t*)oam;
        bytes[address - 0xFE00] = data;
    }
    else if (address == 0xFF00)
    {
        ppu.JoyPadSetSelected(data);
        return;
    }
    else if (address < 0xFF00) //Not Usable/ Reserved Ram
    {
        std::cout << "Unusable Ram" << std::endl;
    }
    else if (address == 0xFF04) // Div Register
    {
        div = 0;
        return;
    }
    else if (address == 0xFF0F) // Interrupt Flag Register
    {
        cpu.interrupt_flags = data;
        return;
    }
    else if (address == 0xFF40) // LCDC Register
    {
        LCDControl.reg = data;
        return;
    }
    else if (address == 0xFF41) // LCDS Register
    {
        LCDStatus.reg = data;
        return;
    }
    else if (address == 0xFF44)  // lY Register
    {
        ly = 0;
        return;
    }
    else if (address == 0xFF46) // DMA TRANSFER
    {
        std::cout << "DMA Start" << std::endl;
        DMA_Transfer(data);
        return;
    }
    else if (address < 0xFF80) // I/O Registers
    {
        ioram[address - 0xFF00] = data;
       
        if (address == 0xFF47)
            ppu.UpdatePalette(data, 0);
        else if (address == 0xFF48)
            ppu.UpdatePalette(data & 0b11111100, 1);
        else if (address == 0xFF49)
            ppu.UpdatePalette(data & 0b11111100, 2);
            
    }
    else if (address == 0xFFFF) //Interrupt Enable Register
    {
         cpu.IE_Register = data;
    }
    else
    {
        hram[address - 0xFF80] = data;
    }
}