#include "cpu.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

static inline uint16_t get_opcode(struct chip8_system* sys)
{
	return ((uint16_t)(sys->memory[sys->PC]) << 8)
		| ((uint16_t)(sys->memory[sys->PC + 1]));
}

const uint16_t sprites[] = {0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75};
const uint8_t chip8_fontset[80] =
{
	0xf0, 0x90, 0x90, 0x90, 0xf0,	//0
	0x20, 0x60, 0x20, 0x20, 0x70,	// 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0,	// 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0,	// 3
	0x90, 0x90, 0xF0, 0x10, 0x10,	// 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0,	// 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0,	// 6
	0xF0, 0x10, 0x20, 0x40, 0x40,	// 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0,	// 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0,	// 9
	0xF0, 0x90, 0xF0, 0x90, 0x90,	// A
	0xE0, 0x90, 0xE0, 0x90, 0xE0,	// B
	0xF0, 0x80, 0x80, 0x80, 0xF0,	// C
	0xE0, 0x90, 0x90, 0x90, 0xE0,	// D
	0xF0, 0x80, 0xF0, 0x80, 0xF0,	// E
	0xF0, 0x80, 0xF0, 0x80, 0x80 	// F
};

void draw(struct chip8_system* sys, uint8_t x, uint8_t y, uint8_t n)
{
	sys->V[15] = 0;
	for (int j = 0; j < n; j++)
	{
		uint8_t pix = sys->memory[sys->I + j];
		for (int i = 0; i < 8; i++)
		{
			if ((pix & (0x80 >> i)) != 0)
			{
				int index = (x + i + ((y + j) * 64));
				if (sys->display[index] == 1)
					sys->V[15] = 1;
				sys->display[index] ^= 1;
			}
		}
	}
}

int chip8_dump(struct chip8_system* sys)
{
	printf("opcode: %4x\n", sys->opcode);
	printf("PC: %4x\n", sys->PC);
	printf("I: %4x\n", sys->I);
	printf("sp: %d\n", sys->sp);
	printf("stack: %4x\n", sys->stack[sys->sp == 0 ? sys->sp : (sys->sp - 1)]);
	printf("clock: %ld\n", sys->clock);
	printf("\n");
	return 0;
}

int chip8_reset(struct chip8_system* sys)
{
	sys->PC = 0;
	sys->I = 0;
	sys->sp = 0;
	memset(sys->memory, 0, 4096);
	memcpy(sys->memory, chip8_fontset, 80);
	memset(sys->V, 0, 16);
	memset(sys->stack, 0, CHIP8_STACK_SIZE);
	memset(sys->display, 0, 64 * 32);
	return 0;
}

int chip8_load_rom_file(struct chip8_system* sys, const char* file)
{
	FILE* f = fopen(file, "rb");
	if (f == NULL)
	{
		perror(file);
		return -1;
	}
	
	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0, SEEK_SET);
	
	if (size > (4096 - 512))
	{
		printf("Not enough memory\n");
		return -1;
	}
	
	printf("rom size: %ldb\n", size);
	size_t br = fread(sys->memory + 0x200, 1, size, f);
	if (br != size)
	{
		perror(file);
		return -1;
	}
	
	fclose(f);
	sys->PC = 0x200;
	return 0;
}

void print_opcode(struct chip8_system* sys, const char* msg)
{
	printf("%s:[%4x]:%4x\n", msg, sys->PC,
		(((uint16_t)(sys->memory[sys->PC]) << 8) | sys->memory[sys->PC + 1]));
}

int chip8_tick(struct chip8_system* sys)
{
	uint16_t PC_incr = 2;
	sys->opcode = get_opcode(sys);
	
	switch (sys->opcode >> 12)
	{
		case 0:	// call
			switch (sys->opcode)
			{
				case 0x00e0:	// clear screen
					memset(sys->display, 0, 64 * 32);
					break;
				
				case 0x00ee:	// return
					sys->PC = sys->stack[--sys->sp];
					break;
				
				default:	// call rca 1802 program
					// TODO
					break;
			}
			break;
		
		case 1:	// jump to address
			sys->PC = sys->opcode & 0x0fff;
			PC_incr = 0;
			break;
		
		case 2:	// call function / subroutine
			if (sys->sp > CHIP8_STACK_SIZE)
			{
				printf("stack overflow\n");
			}
			sys->stack[sys->sp++] = sys->PC;
			sys->PC = sys->opcode & 0x0fff;
			PC_incr = 0;
			break;
		
		case 3:	// if V[n] == const value
			if (sys->V[(sys->opcode >> 8) & 0xf] == (sys->opcode & 0xff))
				PC_incr = 4;
			break;
		
		case 4:	// if V[n] != const value
			if (sys->V[(sys->opcode >> 8) & 0xf] != (sys->opcode & 0xff))
				PC_incr = 4;
			break;
		
		case 5:	// if V[a] == V[b]
			if (sys->V[(sys->opcode >> 8) & 0xf] == sys->V[(sys->opcode >> 4) & 0xf])
				PC_incr = 4;
			break;
		
		case 6:	// V[n] = const value
			sys->V[(sys->opcode >> 8) & 0xf] = (uint8_t)(sys->opcode & 0xff);
			break;
		
		case 7:	// V[n] += const value
			sys->V[(sys->opcode >> 8) & 0xf] += (uint8_t)(sys->opcode & 0xff);
			break;
		
		case 8:	// assigments
			switch (sys->opcode & 0xf)
			{
				case 0:	// V[a] = V[b]
					sys->V[(sys->opcode >> 8) & 0xf] = sys->V[(sys->opcode >> 4) & 0xf];
					break;
				
				case 1:	// V[a] |= V[b]
					sys->V[(sys->opcode >> 8) & 0xf] |= sys->V[(sys->opcode >> 4) & 0xf];
					break;
				
				case 2:	// V[a] &= V[b]
					sys->V[(sys->opcode >> 8) & 0xf] &= sys->V[(sys->opcode >> 4) & 0xf];
					break;
				
				case 3:	// V[a] ^= V[b]
					sys->V[(sys->opcode >> 8) & 0xf] ^= sys->V[(sys->opcode >> 4) & 0xf];
					break;
				
				case 4:	// V[a] += V[b]
					;
					uint16_t t = (uint16_t)sys->V[(sys->opcode >> 8) & 0xf]
						+ sys->V[(sys->opcode >> 4) & 0xf];
					sys->V[(sys->opcode >> 8) & 0xf] = t & 0xff;
					sys->V[15] = (t >> 8) & 0x1;
					break;
				
				case 5:	// V[a] -= V[b]
					sys->V[15] = sys->V[(sys->opcode >> 8) & 0xf]
						> sys->V[(sys->opcode >> 4) & 0xf];
					sys->V[(sys->opcode >> 8) & 0xf] -= sys->V[(sys->opcode >> 4) & 0xf];
					break;
				
				case 6:	// V[n] >> 1
					sys->V[15] = sys->V[(sys->opcode >> 8) & 0xf] & 0x1;
					sys->V[(sys->opcode >> 8) & 0xf] =
						sys->V[(sys->opcode >> 8) & 0xf] >> 1;
					break;
				
				case 7:	// V[a] = V[b] - V[a];
					sys->V[15] = sys->V[(sys->opcode >> 8) & 0xf]
						> sys->V[(sys->opcode >> 4) & 0xf];
					sys->V[(sys->opcode >> 8) & 0xf] = sys->V[(sys->opcode >> 4) & 0xf]
						- sys->V[(sys->opcode >> 8) & 0xf];
					break;
				
				case 0xe:	// V[n] << 1
					sys->V[15] = sys->V[(sys->opcode >> 8) & 0xf] >> 7;
					sys->V[(sys->opcode >> 8) & 0xf] =
						sys->V[(sys->opcode >> 8) & 0xf] << 1;
					break;
				
				default:
					print_opcode(sys, "unrecognized opcode");
					return 0;
					break;
			}
			break;
		
		case 9:	// if (V[a] != V[b])
			if (sys->V[(sys->opcode >> 8) & 0xf] != sys->V[(sys->opcode >> 4) & 0xf])
				PC_incr = 4;
			break;
		
		case 0xa:	// I = address
			sys->I = sys->opcode & 0x0fff;
			break;
		
		case 0xb:	// PC = V[0] + address
			sys->PC = (uint16_t)sys->V[0] + (sys->opcode & 0x0fff);
			break;
		
		case 0xc:	// V[a] = rand() & const value
			sys->V[(sys->opcode >> 8) & 0xf] = (rand() & 0xff) + (sys->opcode & 0xff);
			break;
		
		case 0xd:	// draw sprite at (x, y); last nibble is height of sprite
			draw(sys, sys->V[(sys->opcode >> 8) & 0xf],
				sys->V[(sys->opcode >> 4) & 0xf], sys->opcode & 0xf);
			break;
		
		case 0xe:	// key operation
			switch (sys->opcode & 0xff)
			{
				case 0x9e:	// if key() == V[n]
					if (sys->key[sys->V[(sys->opcode >> 8) & 0xf]] != 0)
						PC_incr = 4;
					break;
				
				case 0xa1:	// if key() != V[n]
					if (sys->key[sys->V[(sys->opcode >> 8) & 0xf]] == 0)
						PC_incr = 4;
					break;
				
				default:
					print_opcode(sys, "unrecognized opcode");
					return 0;
					break;
			}
			break;
		
		case 0xf:	// timer and memory
			;
			uint8_t reg_i = (sys->opcode >> 8) & 0xf;
			switch (sys->opcode & 0xff)
			{
				case 0x07:	// V[n] = delay_timer
					sys->V[reg_i] = sys->delay_timer;
					break;
				
				case 0x0a:	// blocking key input
					;
					uint8_t key_event = 0;
					uint8_t key = 0;
					for (int i = 0; i < 16; i++)
					{
						if (sys->key[i] == 1)
						{
							key_event = 1;
							key = i;
						}
					}
					key_event == 0 ? (PC_incr = 0) : (sys->V[reg_i] = sys->key[key]);
					break;
				
				case 0x15:	// delay_timer = V[n]
					sys->delay_timer = sys->V[reg_i];
					break;
				
				case 0x18:	// sound_timer = V[n]
					sys->sound_timer = sys->V[reg_i];
					break;
				
				case 0x1e:	// I += V[n]
					sys->I += sys->V[reg_i];
					break;
				
				case 0x29:	// I = sprite_address[V[n]]
					sys->I = sprites[sys->V[reg_i]];
					break;
				
				case 0x33:	// bcd
					;
					uint8_t num = sys->V[reg_i];
					sys->memory[sys->I] = num / 100;
					sys->memory[sys->I + 1] = (num % 100) / 10;
					sys->memory[sys->I + 2] = num % 10;
					break;
				
				case 0x55:	// reg_dump(V[0]...V[n])
					for (int i = 0; i < reg_i; i++)
						sys->memory[sys->I + i] = sys->V[i];
					break;
				
				case 0x65:	// reg_load(V[0]...V[n])
					for (int i = 0; i < reg_i; i++)
						sys->V[i] = sys->memory[sys->I + i];
					break;
				
				default:
					print_opcode(sys, "unrecognized opcode");
					return 0;
					break;
			}
			break;
		
		default:
			print_opcode(sys, "unrecognized opcode");
			return 0;
			break;
	} // opcode switch ends here
	
	if (sys->sound_timer > 0)
	{
		if (sys->sound_timer == 1)
			printf("beep\n");
		sys->sound_timer--;
	}
	if (sys->delay_timer > 0)
	{
		sys->delay_timer--;
	}
	
	sys->clock += 1;
	sys->PC += PC_incr;
	return 1;
}
