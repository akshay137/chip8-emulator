#ifndef CHIP_8_CPU_DEFINED
#define CHIP_8_CPU_DEFINED

#ifdef __cplusplus
namespace chip8 {
extern "C" {
#endif

#include <stdint.h>

#define CHIP8_STACK_SIZE 24

struct chip8_system {
	uint64_t clock;	// counter for cycles
	uint8_t memory[4096];
	uint8_t display[64 * 32];
	uint8_t key[16];
	uint8_t V[16];
	uint16_t stack[CHIP8_STACK_SIZE];
	uint16_t I;
	uint16_t PC;
	uint16_t opcode;
	uint8_t sp;	// not part of actual system
	uint8_t delay_timer;
	uint8_t sound_timer;
};

extern int chip8_dump(struct chip8_system* sys);
extern int chip8_reset(struct chip8_system* sys);
extern int chip8_load_rom_file(struct chip8_system* sys, const char* file);

extern int chip8_tick(struct chip8_system* sys);

#ifdef __cplusplus
}}
#endif

#endif
