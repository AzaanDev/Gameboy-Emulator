#include "PPU.h"
#include "Bus.h"
#include <fstream>

PPU::PPU()
{
	SDL_Init(SDL_INIT_VIDEO);
	window = SDL_CreateWindow("Gameboy", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 320, 288, SDL_WINDOW_RESIZABLE);
	renderer = SDL_CreateRenderer(window, -1, 0);
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 160, 144);

	for (int i = 0; i < (144 * 160); i++)
		buffer[i] = 0x0;

	for (int i = 0; i < 4; i++)
	{
		bg_colors[i] = DEFAULT_COLORS[i];
		s1_colors[i] = DEFAULT_COLORS[i];
		s2_colors[i] = DEFAULT_COLORS[i];
	}


	frame_counter = 0;
	ticks = 0;

	state = FS_TILE;
	line_x   = 0;
	pushed_x = 0;
	fetch_x  = 0;
	map_y    = 0;
	map_x    = 0;
	tile_y   = 0;
	fifo_x   = 0;
}

uint8_t PPU::read(uint16_t address)
{
	return bus->read(address);
}

void PPU::write(uint16_t address, uint8_t data)
{
	bus->write(address, data);
}


void PPU::Tick()
{
	ticks++;
	switch (bus->LCDStatus.ModeFlag)
	{
	case HBLANK:
		ModeHBLANK();
		break;
	case VBLANK:
		ModeVBLANK();
		break;
	case OAM:
		ModeOAM();
		break;
	case XFER:
		ModeXFER();
		break;
	}
}

void PPU::Update()
{
	SDL_UpdateTexture(texture, NULL, buffer, 160 * sizeof(uint32_t));
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

void PPU::Events()
{
	SDL_Event e;
	while (SDL_PollEvent(&e))
	{

		if (e.type == SDL_KEYDOWN)
		{
			Keys(true, e.key.keysym.sym);
		}

		if (e.type == SDL_KEYUP)
		{
			Keys(false, e.key.keysym.sym);
		}

		if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE) {
			exit(1);
		}
	}
}

void PPU::UpdatePalette(uint8_t data, uint8_t pal)
{
	uint32_t* colors = bg_colors;
	switch (pal)
	{
	case 1:
		colors = s1_colors;
		break;
	case 2:
		colors = s2_colors;
		break;
	}
	colors[0] = DEFAULT_COLORS[data & 0b11];
	colors[1] = DEFAULT_COLORS[(data >> 2) & 0b11];
	colors[2] = DEFAULT_COLORS[(data >> 4) & 0b11];
	colors[3] = DEFAULT_COLORS[(data >> 6) & 0b11];
}

void PPU::ModeOAM()
{
	if (ticks >= OAM_CYCLES)
	{
		bus->LCDStatus.ModeFlag = LCD_MODE::XFER;
		state = FS_TILE;
		line_x = 0;
		fetch_x = 0;
		pushed_x = 0;
		fifo_x = 0;
	}
}

void PPU::ModeXFER()
{
	PipelineProcess();
	if (ticks >= (OAM_CYCLES + XFER_CYCLES))
	{
		std::queue<uint32_t> temp;
		std::swap(temp, fifo);

		if (bus->LCDStatus.I_HBlank)
			bus->cpu.RequestInterrupt(I_LCD_STAT);

		bool ly_coincidence = bus->ly == bus->lyc;
		if (ly_coincidence && bus->LCDStatus.I_LYC)
			bus->cpu.RequestInterrupt(I_LCD_STAT);

		bus->LCDStatus.LYCFlag = ly_coincidence;
		bus->LCDStatus.ModeFlag = LCD_MODE::HBLANK;
	}
}

static uint32_t FrameRate = 1000 / 60;
static long PrevFrameRate = 0;
static long StartTimer = 0;
static long FrameCount = 0;


void PPU::ModeHBLANK()
{
	if (ticks >= TOTAL_SCANLINE_CYCLES)
	{
		bus->ly++;
		ticks = 0;
		if (bus->ly >= HEIGHT)
		{
			bus->LCDStatus.ModeFlag = LCD_MODE::VBLANK;
			bus->cpu.RequestInterrupt(I_VBLANK);
			if (bus->LCDStatus.I_VBlank)
				bus->cpu.RequestInterrupt(I_LCD_STAT);
			
			frame_counter++;

			//LIMIT FPS TO 60
			uint32_t end = SDL_GetTicks();
			uint32_t time = end - PrevFrameRate;


			if (time < FrameRate)
				SDL_Delay(FrameRate - time);
			if (end - StartTimer >= 1000)
			{
				uint32_t fps = FrameCount;
				StartTimer = end;
				FrameCount = 0;
			}
			FrameCount++;
			PrevFrameRate = SDL_GetTicks();
			
		}
		else
			bus->LCDStatus.ModeFlag = LCD_MODE::OAM;

	}
}


void PPU::ModeVBLANK()
{
	if (ticks >= VBLANK_CYCLES)
	{
		bus->ly++;
		ticks = 0;
		if (bus->ly == SCANLINES_PER_FRAME)
		{
			bus->LCDStatus.ModeFlag = LCD_MODE::OAM;
			bus->ly = 0;
		}
	}
}

bool PPU::PipelineAdd()
{
	if (fifo.size() > 8)
		return false;

	int x = fetch_x - (8 - (bus->scx % 8));
	for (int i = 0; i < 8; i++)
	{
		int bit = 7 - i;
		uint8_t hi = !!(bg_data[1] & (1 << bit));
		uint8_t lo = !!(bg_data[2] & (1 << bit)) << 1;
		uint32_t color = bg_colors[hi | lo];

		if (x >= 0)
		{
			fifo.push(color);
			fifo_x++;
		}
	}
	return true;
}

void PPU::PipelineFetch()
{
	switch (state)
	{
	case FS_TILE:
	{
		if (bus->LCDControl.BGEnable)
		{
			bg_data[0] = read(bus->LCDControl.BGTileMapArea ? 0x9C00 : 0x9800 +
				(map_x / 8) + ((map_y / 8) * 32));

			if (bus->LCDControl.BGWinTileDataArea ? 0x8000 : 0x8800 == 0x8800)
				bg_data[0] += 128;
		}

		state = FS_DATA0;
		fetch_x += 8;
	}   break;

	case FS_DATA0:
	{
		bg_data[1] = read(bus->LCDControl.BGWinTileDataArea ? 0x8000 : 0x8800 +
			(bg_data[0] * 16) + tile_y);
		state = FS_DATA1;
	}	break;

	case FS_DATA1:
	{
		bg_data[2] = read(bus->LCDControl.BGWinTileDataArea ? 0x8000 : 0x8800 +
			(bg_data[0] * 16) + tile_y + 1);
		state = FS_SLEEP;
	}	break;

	case FS_SLEEP:
	{
		state = FS_PUSH;
	}	break;
	case FS_PUSH:
	{
		if (PipelineAdd()) {
			state = FS_TILE;
		}
	}	break;
	}
}

void PPU::PipelineProcess()
{
	map_y = bus->ly + bus->scy;
	map_x = fetch_x + bus->scx;
	tile_y = ((bus->ly + bus->scy) % 8) * 2;
	if (!(ticks & 1)) {
		PipelineFetch();
	}
	PipelinePushPixel();
}

void PPU::PipelinePushPixel()
{
	if (fifo.size() > 8)
	{
		uint32_t pixel = fifo.front();
		fifo.pop();
		if (line_x >= (bus->scx % 8))
		{
			buffer[pushed_x + (bus->ly * WIDTH)] = pixel;
			pushed_x++;
		}
		line_x++;
	}
}

/*
void PPU::PipelineLoadData(uint8_t offset)
{
	int y = bus->ly;
	uint8_t height = bus->LCDControl.OBJSize ? 16 : 8;

	for (int i = 0; i < fetched_entry_count; i++) {
		uint8_t ty = ((y + 16) - fetched_entries[i].Y) * 2;

		if (fetched_entries[i].Y_FLIP) {
			ty = ((height * 2) - 2) - ty;
		}

		uint8_t tile_index = fetched_entries[i].Tile;

		if (height == 16) {
			tile_index &= ~(1); 
		}

		entry_data[(i * 2) + offset] = read(0x8000 + (tile_index * 16) + ty + offset);
	}
}
*/


void PPU::Keys(bool down, uint32_t key_code)
{
	switch (key_code)
	{
	case SDLK_z:
		controller.a = down; break;
	case SDLK_x:
		controller.b = down; break;
	case SDLK_SPACE:
		controller.start = down; break;
	case SDLK_TAB:
		controller.select = down; break;
	case SDLK_UP:
		controller.up = down; break;
	case SDLK_DOWN:
		controller.down = down; break;
	case SDLK_LEFT:
		controller.left = down; break;
	case SDLK_RIGHT:
		controller.right = down; break;
	}
}

bool PPU::JoyPadButtonSelected()
{
	return button_selected;
}

bool PPU::JoyPadDirSelected()
{
	return dir_selected;
}

void PPU::JoyPadSetSelected(uint8_t value)
{
	button_selected = value & 0x20;
	dir_selected = value & 0x10;
}

uint8_t PPU::JoyPadOut()
{
	uint8_t result = 0xCF;
	if (!JoyPadButtonSelected())
	{
		if (controller.start)
		{
			result &= ~(1 << 3);
		}
		if (controller.select)
		{
			result &= ~(1 << 2);
		}
		if (controller.b)
		{
			result &= ~(1 << 1);
		}
		if (controller.a)
		{
			result &= ~(1 << 0);
		}
	}

	if (!JoyPadDirSelected())
	{
		if (controller.left)
		{
			result &= ~(1 << 1);
		}
		if (controller.right)
		{
			result &= ~(1 << 0);
		}
		if (controller.up)
		{
			result &= ~(1 << 2);
		}
		if (controller.down)
		{
			result &= ~(1 << 3);
		}
	}

	return result;
}