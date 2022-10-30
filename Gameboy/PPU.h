#pragma once

#include "SDL.h"
#include <queue>
#include <list>
#include <iterator>

#define HBLANK_CYCLES 204
#define OAM_CYCLES 80
#define XFER_CYCLES 289
#define TOTAL_SCANLINE_CYCLES 456
#define VBLANK_CYCLES 4560
#define SCANLINES_PER_FRAME 154
#define WIDTH 160
#define HEIGHT 144

static unsigned long DEFAULT_COLORS[4] = { 0xFFA96868, 0xFFEDB4A1, 0xFF764462, 0xFF2C2137 };

enum LCD_MODE { HBLANK, VBLANK, OAM, XFER };

enum FETCH_STATE { FS_TILE, FS_DATA0, FS_DATA1, FS_SLEEP, FS_PUSH };

struct OAM // 4 Bytes
{
	uint8_t Y;    // Y Position
	uint8_t X;    // X Position
	uint8_t Tile; // Tile Index
	//Flags Bit Field 
	unsigned CGB_PN : 3;    // Palette Number
	unsigned CGB_VBANK : 1; // Tile RAM-BANK
	unsigned PN : 1;        // Palette Number
	unsigned X_FLIP : 1;    // X Flip
	unsigned Y_FLIP : 1;    // Y Flip
	unsigned BG : 1;        // Background Priority  
};

struct JoyPad
{
	bool start;
	bool select;
	bool a;
	bool b;
	bool up;
	bool down;
	bool left;
	bool right;
};

class Bus;

class PPU
{
public:
	PPU();
	~PPU() = default;
	void ConnectBus(Bus* n) { bus = n; }
	uint8_t read(uint16_t address);
	void write(uint16_t address, uint8_t data);

	void Tick();
	void Update();
	void Events();
	void UpdatePalette(uint8_t data, uint8_t pal);
	void IncrementLY();
	void LoadSprites();

	//PPU FIFO MODES
	void ModeHBLANK();
	void ModeVBLANK();
	void ModeOAM();
	void ModeXFER();

	//Pipeline Functions
	void PipelineLoadTiles();
	void PipelineLoadSprites();

	bool PipelineAdd();
	void PipelineFetch();
	void PipelineProcess();
	void PipelinePushPixel();
	void PipelineLoadData(uint8_t offset);


	bool JoyPadButtonSelected();
	bool JoyPadDirSelected();
	void JoyPadSetSelected(uint8_t value);
	void Keys(bool down, uint32_t key_code);
	uint8_t JoyPadOut();

public:
	uint32_t frame_counter;
private:
	Bus* bus = nullptr;
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* texture;

	uint32_t buffer[160 * 144];
	uint32_t ticks;

	uint32_t bg_colors[4];
	uint32_t s1_colors[4];
	uint32_t s2_colors[4];

	std::queue<uint32_t> fifo;
	std::list<struct OAM> sprite_list;

	uint8_t fetched_entry_count;
	struct OAM fetched_entries[3];

	FETCH_STATE state;
	uint8_t line_x;
	uint8_t pushed_x;
	uint8_t fetch_x;
	uint8_t map_y;
	uint8_t map_x;
	uint8_t tile_y;
	uint8_t fifo_x;
	uint8_t bg_data[3]    = { 0x0 };
	uint8_t entry_data[6] = { 0x0 };


	bool button_selected;
	bool dir_selected;
	JoyPad controller;

private:
	bool GetBit(unsigned char i, int pos)
	{
		return (i & (1 << pos));
	}

	void SetBit(unsigned char& i, int pos, bool value)
	{
		if (value) //Sets
			i = (i | (1 << pos));
		else       //Resets
			i = (i & (~(1 << pos)));
	}
};
