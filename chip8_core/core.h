#pragma once

#include <cstdint>
#include <cstddef>  // for size_t

// Function declarations
uint64_t add(uint64_t left, uint64_t right);

// Constants
constexpr size_t FONTSET_SIZE = 80;
extern const uint8_t FONTSET[FONTSET_SIZE];

constexpr size_t SCREEN_WIDTH = 64;
constexpr size_t SCREEN_HEIGHT = 32;
constexpr size_t RAM_SIZE = 4096;
constexpr size_t NUM_REGS = 16;
constexpr size_t STACK_SIZE = 16;
constexpr size_t NUM_KEYS = 16;

constexpr uint16_t START_ADDR = 0x200;

// Emulator struct declaration
struct Emu {

public:
    uint16_t pc;
    uint8_t ram[RAM_SIZE];
    bool screen[SCREEN_WIDTH * SCREEN_HEIGHT];
    uint8_t v_reg[NUM_REGS];
    uint16_t i_reg;
    uint16_t sp;
    uint16_t stack[STACK_SIZE];
    bool keys[NUM_KEYS];
    uint8_t dt;
    uint8_t st;

    Emu();

    void reset();

    void push(uint16_t val);

    uint16_t pop();

    void tick();

    const bool* get_display() const;

    void keypress(size_t key, bool pressed);

    void load(const uint8_t* data, size_t length);

    void execute(uint16_t op);

    uint16_t fetch();

    void tick_timers();
};
