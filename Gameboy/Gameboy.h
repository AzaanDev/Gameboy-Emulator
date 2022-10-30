#pragma once

#include "cartridge.h"
#include "bus.h"

#define MAX_CYCLES 69905

class Gameboy
{
public:
	Gameboy(const char* fileName);
	~Gameboy() = default;
	void Run();
	void Clock();

private:
	Bus bus;
	bool running = true;
	std::shared_ptr<Cartridge> cartridge;
};
