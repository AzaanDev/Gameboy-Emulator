#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <fstream>
#include <map>
#include <iterator>

class Cartridge
{
public:
	Cartridge(const char* fileName);
	~Cartridge() = default;
	uint8_t read(uint16_t address);
	void write(uint16_t address, uint8_t data);
	void CartridgeHeader();

private:
	std::vector<uint8_t> m_Rom;
	std::string m_Title;
	uint8_t m_CartridgeType;
	uint8_t m_RomSize;
	uint8_t m_RamSize;
};
