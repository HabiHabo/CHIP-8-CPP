#include "core.h"

#include <algorithm>
#include <iterator>
#include <random>

// Define FONTSET here (extern const in header)
const uint8_t FONTSET[FONTSET_SIZE] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

// Simple add function
uint64_t add(uint64_t left, uint64_t right){
    return left + right;
}

// Constructor
Emu::Emu() : pc(START_ADDR), i_reg(0), sp(0), dt(0), st(0) {
    std::fill(ram, ram + RAM_SIZE, 0);
    std::fill(screen, screen + (SCREEN_WIDTH * SCREEN_HEIGHT), false);
    std::fill(v_reg, v_reg + NUM_REGS, 0);
    std::fill(stack, stack + STACK_SIZE, 0);
    std::fill(keys, keys + NUM_KEYS, false);

    std::copy(FONTSET, FONTSET + FONTSET_SIZE, ram);
}

void Emu::reset() {
    *this = Emu();
}

void Emu::push(uint16_t val) {
    stack[sp] = val;
    sp += 1;
}

uint16_t Emu::pop() {
    sp -= 1;
    return stack[sp];
}

void Emu::tick() {
    uint16_t op = fetch();
    execute(op);
}

const bool* Emu::get_display() const {
    return screen;
}

void Emu::keypress(size_t key, bool pressed) {
    keys[key] = pressed;
}

void Emu::load(const uint8_t* data, size_t length) {
    size_t start = START_ADDR;
    for (size_t i = 0; i < length; ++i) {
        ram[start + i] = data[i];
    }
}

void Emu::execute(uint16_t op) {
    printf("PC: 0x%03X, Opcode: 0x%04X\n", pc, op);

    uint16_t digit1 = (op & 0xF000) >> 12;
    uint16_t digit2 = (op & 0x0F00) >> 8;
    uint16_t digit3 = (op & 0x00F0) >> 4;
    uint16_t digit4 = op & 0x000F;

    if (op == 0x0000) {
        // NOP
        pc += 2;
        return;
    }

    if (digit1 == 0x0) {
        if (digit2 == 0 && digit3 == 0xE && digit4 == 0) {
            // 00E0 CLS
            std::fill(screen, screen + (SCREEN_WIDTH * SCREEN_HEIGHT), false);
            pc += 2;
            return;
        }
        else if (digit2 == 0 && digit3 == 0xE && digit4 == 0xE) {
            // 00EE RET
            pc = pop();
            pc += 2;
            return;
        }
        else {
            throw std::runtime_error("Unknown 0x0 opcode");
        }
    }
    else if (digit1 == 0x1) {
        // 1NNN JP addr
        pc = op & 0x0FFF;
        return;
    }
    else if (digit1 == 0x2) {
        // 2NNN CALL addr
        push(pc);
        pc = op & 0x0FFF;
        return;
    }
    else if (digit1 == 0x3) {
        // 3XNN SE Vx, byte
        size_t x = digit2;
        uint8_t nn = op & 0x00FF;
        if (v_reg[x] == nn)
            pc += 4;
        else
            pc += 2;
        return;
    }
    else if (digit1 == 0x4) {
        // 4XNN SNE Vx, byte
        size_t x = digit2;
        uint8_t nn = op & 0x00FF;
        if (v_reg[x] != nn)
            pc += 4;
        else
            pc += 2;
        return;
    }
    else if (digit1 == 0x5 && digit4 == 0) {
        // 5XY0 SE Vx, Vy
        size_t x = digit2;
        size_t y = digit3;
        if (v_reg[x] == v_reg[y])
            pc += 4;
        else
            pc += 2;
        return;
    }
    else if (digit1 == 0x6) {
        // 6XNN LD Vx, byte
        size_t x = digit2;
        v_reg[x] = op & 0x00FF;
        pc += 2;
        return;
    }
    else if (digit1 == 0x7) {
        // 7XNN ADD Vx, byte
        size_t x = digit2;
        v_reg[x] = v_reg[x] + (op & 0x00FF);
        pc += 2;
        return;
    }
    else if (digit1 == 0x8) {
        size_t x = digit2;
        size_t y = digit3;

        if (digit4 == 0x0)
            v_reg[x] = v_reg[y];
        else if (digit4 == 0x1)
            v_reg[x] |= v_reg[y];
        else if (digit4 == 0x2)
            v_reg[x] &= v_reg[y];
        else if (digit4 == 0x3)
            v_reg[x] ^= v_reg[y];
        else if (digit4 == 0x4) {
            uint16_t sum = v_reg[x] + v_reg[y];
            v_reg[0xF] = (sum > 0xFF) ? 1 : 0;
            v_reg[x] = sum & 0xFF;
        }
        else if (digit4 == 0x5) {
            v_reg[0xF] = (v_reg[x] >= v_reg[y]) ? 1 : 0;
            v_reg[x] = v_reg[x] - v_reg[y];
        }
        else if (digit4 == 0x6) {
            v_reg[0xF] = v_reg[x] & 0x1;
            v_reg[x] >>= 1;
        }
        else if (digit4 == 0x7) {
            v_reg[0xF] = (v_reg[y] >= v_reg[x]) ? 1 : 0;
            v_reg[x] = v_reg[y] - v_reg[x];
        }
        else if (digit4 == 0xE) {
            v_reg[0xF] = (v_reg[x] >> 7) & 0x1;
            v_reg[x] <<= 1;
        }
        else {
            throw std::runtime_error("Unknown 8XYN opcode");
        }
        pc += 2;
        return;
    }
    else if (digit1 == 0x9 && digit4 == 0) {
        // 9XY0 SNE Vx, Vy
        size_t x = digit2;
        size_t y = digit3;
        if (v_reg[x] != v_reg[y])
            pc += 4;
        else
            pc += 2;
        return;
    }
    else if (digit1 == 0xA) {
        // ANNN LD I, addr
        i_reg = op & 0x0FFF;
        pc += 2;
        return;
    }
    else if (digit1 == 0xB) {
        // BNNN JP V0, addr
        pc = v_reg[0] + (op & 0x0FFF);
        return;
    }
    else if (digit1 == 0xC) {
        // CXNN RND Vx, byte
        size_t x = digit2;
        uint8_t nn = op & 0x00FF;
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<uint16_t> dist(0, 255);
        uint8_t rng = dist(gen) & 0xFF;
        v_reg[x] = rng & nn;
        pc += 2;
        return;
    }
    else if (digit1 == 0xD) {
        // DXYN DRW Vx, Vy, nibble
        uint16_t x_cord = v_reg[digit2];
        uint16_t y_cord = v_reg[digit3];
        uint16_t height = digit4;

        v_reg[0xF] = 0;

        for (uint16_t row = 0; row < height; ++row) {
            uint16_t address = i_reg + row;
            uint8_t pixels = ram[address];
            for (uint8_t col = 0; col < 8; ++col) {
                if ((pixels & (0b10000000 >> col)) != 0) {
                    size_t x = (x_cord + col) % SCREEN_WIDTH;
                    size_t y = (y_cord + row) % SCREEN_HEIGHT;
                    size_t pixel_id = x + SCREEN_WIDTH * y;

                    if (screen[pixel_id])
                        v_reg[0xF] = 1;

                    screen[pixel_id] ^= true;
                }
            }
        }
        pc += 2;
        return;
    }
    else if (digit1 == 0xE) {
        size_t x = digit2;
        if (digit3 == 0x9 && digit4 == 0xE) {
            // EX9E SKP Vx
            if (keys[v_reg[x]])
                pc += 4;
            else
                pc += 2;
            return;
        }
        else if (digit3 == 0xA && digit4 == 0x1) {
            // EXA1 SKNP Vx
            if (!keys[v_reg[x]])
                pc += 4;
            else
                pc += 2;
            return;
        }
        else {
            throw std::runtime_error("Unknown EX?? opcode");
        }
    }
    else if (digit1 == 0xF) {
        size_t x = digit2;
        uint8_t last_two = (digit3 << 4) | digit4;

        if (last_two == 0x07) {
            // FX07 LD Vx, DT
            v_reg[x] = dt;
            pc += 2;
            return;
        }
        else if (last_two == 0x0A) {
            // FX0A LD Vx, K
            bool key_pressed = false;
            for (size_t i = 0; i < NUM_KEYS; ++i) {
                if (keys[i]) {
                    v_reg[x] = i;
                    key_pressed = true;
                    break;
                }
            }
            if (!key_pressed)
                return;  // wait, don't advance pc
            pc += 2;
            return;
        }
        else if (last_two == 0x15) {
            // FX15 LD DT, Vx
            dt = v_reg[x];
            pc += 2;
            return;
        }
        else if (last_two == 0x18) {
            // FX18 LD ST, Vx
            st = v_reg[x];
            pc += 2;
            return;
        }
        else if (last_two == 0x1E) {
            // FX1E ADD I, Vx
            i_reg = i_reg + v_reg[x];
            pc += 2;
            return;
        }
        else if (last_two == 0x29) {
            // FX29 LD F, Vx
            i_reg = 5 * v_reg[x];
            pc += 2;
            return;
        }
        else if (last_two == 0x33) {
            // FX33 LD B, Vx
            uint8_t vx = v_reg[x];
            ram[i_reg] = vx / 100;
            ram[i_reg + 1] = (vx / 10) % 10;
            ram[i_reg + 2] = vx % 10;
            pc += 2;
            return;
        }
        else if (last_two == 0x55) {
            // FX55 LD [I], Vx
            for (size_t i = 0; i <= x; ++i) {
                ram[i_reg + i] = v_reg[i];
            }
            pc += 2;
            return;
        }
        else if (last_two == 0x65) {
            // FX65 LD Vx, [I]
            for (size_t i = 0; i <= x; ++i) {
                v_reg[i] = ram[i_reg + i];
            }
            pc += 2;
            return;
        }
        else {
            throw std::runtime_error("Unknown FX?? opcode");
        }
    }
    else {
        char buffer[50];
        snprintf(buffer, sizeof(buffer), "Unimplemented opcode: 0x%04X", op);
        throw std::runtime_error(buffer);
    }
}


uint16_t Emu::fetch() {
    uint16_t op = (ram[pc] << 8) | ram[pc + 1];
    return op;
}

void Emu::tick_timers() {
    if (dt > 0) {
        dt--;
    }
    if (st > 0) {
        st--;
    }
}
