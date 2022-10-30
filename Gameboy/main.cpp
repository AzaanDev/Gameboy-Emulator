#include "Gameboy.h"
#include <iostream>

int main(int argc, char* argv[])
{
	if (argc <= 1) {
		std::cout << "./Gameboy.exe PATH_TO_ROM_FILE" << std::endl;
		exit(1);
	}

	std::cout << "Loading the rom: " << argv[1] << std::endl;

	Gameboy boy(argv[1]);

	boy.Run();
	return 0;
}