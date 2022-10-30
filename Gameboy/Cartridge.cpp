#include "Cartridge.h"
#include <iostream>

Cartridge::Cartridge(const char* fileName)
{
	std::ifstream file(fileName, std::ios::binary);
	if (!file.is_open())
	{
		std::cout << "Error: File could not be opened" << std::endl;
		exit(EXIT_FAILURE);
	}
	file.seekg(0, std::ios::end);
	int size = file.tellg();
	file.seekg(0, std::ios::beg);
	m_Rom.resize(size);
	file.read((char*)m_Rom.data(), size);


	m_Title = "";
	for (int i = 0; ; i++)
	{
		if (m_Rom[i + 308] == 00)
			break;
		m_Title += m_Rom[308 + i];
	}
	m_CartridgeType = m_Rom[0x147];
	m_RomSize = m_Rom[0x148];
	m_RamSize = m_Rom[0x149];
}

uint8_t Cartridge::read(uint16_t address)
{
	return (m_Rom[address]);
}

void Cartridge::write(uint16_t address, uint8_t data)
{
	return;
}

void Cartridge::CartridgeHeader()
{
	std::map<uint8_t, std::string> CartInfo = {
	{0x00, "ROM ONLY"},
	{0x01, "MBC1"},
	{0x02, "MBC1 + RAM"},
	{0x03, "MBC1 + RAM + BATTERY"},
	{0x05, "MBC2"},
	{0x06, "MBC2 + BATTERY"},
	{0x08, "ROM + RAM"},
	{0x09, "ROM + RAM + BATTERY"},
	{0x0B, "MMM01"},
	{0x0C, "MMM01 + RAM"},
	{0x0D, "MMM01 + RAM + BATTERY"},
	{0x11, "MBC3"},
	{0x13, "MBC3 + RAM + BATTERY"},
	{0x19, "MBC5"},
	{0x20, "MBC6"},
	};

	std::map<uint8_t, std::string> CartSize = {
		{0x00, "32 KByte"},
		{0x01, "64 KByte"},
		{0x02, "128 KByte"},
		{0x03, "256 KByte"},
		{0x04, "512 KByte"},
		{0x05, "1 MByte"},
		{0x06, "2 MByte"},
		{0x07, "4 MByte"},
		{0x08, "8 MByte"},
		{0x52, "1.1 MByte"},
		{0x53, "1.2 MByte"},
		{0x54, "1.5 MByte"},
	};

	std::map<uint8_t, int> CartRam = {
		{0x00, 0},
		{0x01, 0},
		{0x02, 8},
		{0x03, 32},
		{0x04, 128},
		{0x05,  64},
	};

	std::cout << "Title    : " << m_Title << std::endl;
	std::cout << "Type     : " << CartInfo.find(m_CartridgeType)->second << std::endl;
	std::cout << "ROM SIZE : " << CartSize.find(m_RomSize)->second << std::endl;
	std::cout << "RAM SIZE : " << std::to_string(CartRam.find(m_RamSize)->second) << " KB" << std::endl;
};