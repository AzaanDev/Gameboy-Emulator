#include "Gameboy.h"
#include <thread>
#include <SDL.h>

Gameboy::Gameboy(const char* fileName)
{
	cartridge = std::make_shared<Cartridge>(fileName);
	bus.ConnectCart(cartridge);
}

void Gameboy::Run()
{
	std::thread t(&Gameboy::Clock, this);
	uint32_t prev_frame = 0;
	while (running)
	{
		bus.ppu.Events();
		if (prev_frame != bus.ppu.frame_counter) {
			bus.ppu.Update();
		}

		prev_frame = bus.ppu.frame_counter;
	}
	t.join();
}

void Gameboy::Clock()
{
	while (true)
	{
		bus.cpu.Step();
		bus.cpu.Clock();
	}
}
