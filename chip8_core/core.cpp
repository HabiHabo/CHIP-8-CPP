#include <cstdint>
#include <cstddef>  // for size_t
#include <algorithm>
#include <iterator>
#include <random>

uint64_t add(uint64_t left, uint64_t right){
    return left + right;
}

constexpr size_t FONTSET_SIZE = 80;
const uint8_t FONTSET [FONTSET_SIZE] = {
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
    0xF0, 0x80, 0xF0, 0x80, 0x80 // F
};



constexpr size_t SCREEN_WIDTH = 64;
constexpr size_t SCREEN_HEIGHT = 32;
constexpr size_t RAM_SIZE = 4096;
constexpr size_t NUM_REGS = 16;
constexpr size_t STACK_SIZE = 16;
constexpr size_t NUM_KEYS = 16;

constexpr uint16_t START_ADDR = 0x200;

struct Emu {

    public:
        uint16_t pc;
        uint8_t ram [RAM_SIZE];
        bool screen [SCREEN_WIDTH * SCREEN_HEIGHT];
        uint8_t v_reg [NUM_REGS];
        uint16_t i_reg;
        uint16_t sp;
        uint16_t stack [STACK_SIZE];
        bool keys [NUM_KEYS];
        uint8_t dt;
        uint8_t st;

        // The constructor uses for example ram and not ram[0] because it needs a pointer
        // to the first and last element, not the actual value

        Emu() : pc(START_ADDR), i_reg(0), sp(0), dt(0), st(0) {
            std::fill(ram, ram + RAM_SIZE, 0); 
            std::fill(screen, screen + (SCREEN_WIDTH * SCREEN_HEIGHT), false);
            std::fill(v_reg, v_reg + NUM_REGS, 0);
            std::fill(stack, stack + STACK_SIZE, 0);
            std::fill(keys, keys + NUM_KEYS, false);

            std::copy(FONTSET, FONTSET + FONTSET_SIZE, ram);
        }
        
        // new() function from rust implementation is not needed
        // simply write (for example):
        // Emu emulator;

        void reset() {
            *this = Emu(); // just re-run the constructor
        }

        void push(uint16_t val){
            stack[sp] = val;
            sp += 1;
        }

        uint16_t pop(){
            sp -= 1;
            return stack[sp];
        }

        void tick(){
            //Fetch
            uint16_t op = fetch();
            //Decode and Execute
            execute(op);
        }

        const bool* get_display() const {
            return screen;
        }

        void keypress(size_t key, bool pressed){
            keys[key] = pressed;
        }

        void load(const uint8_t* data, size_t length) {
            size_t start = START_ADDR;
            for (size_t i = 0; i < length; ++i) {
                ram[start + i] = data[i];
            }
        }

        void execute (uint16_t op){
            uint16_t digit1 = (op & 0xF000) >> 12;
            uint16_t digit2 = (op & 0x0F00) >> 8;
            uint16_t digit3 = (op & 0x00F0) >> 4;
            uint16_t digit4 = op & 0x000F;

            if (op == 0x0000){
                return;
            }
            else if (digit1 == 0 && digit2 == 0 && digit3 == 0xE && digit4 == 0){
                std::fill(screen, screen + (SCREEN_WIDTH * SCREEN_HEIGHT), false);
            }
            else if (digit1 == 0x1){
                pc = op & 0x0FFF;
            }
            else if (digit1 == 0x2){
                push(pc);
                pc = op & 0x0FFF;
            }
            else if (digit1 == 0x3){
                size_t x = digit2;
                uint8_t nn = op & 0x00FF;
                if (v_reg[x] == nn){
                    pc += 2;
                }
            }
            else if (digit1 == 0x4){
                size_t x = digit2;
                uint8_t nn = op & 0x00FF;
                if (v_reg[x] != nn){
                    pc += 2;
                }
            }
            else if (digit1 == 0x5 && digit4 == 0) {
                size_t x = digit2;
                size_t y = digit3;
                if (v_reg[x] == v_reg[y]) {
                    pc += 2;
                }
            }

            else if (digit1 == 0x6) {
                size_t x = digit2;
                uint8_t nn = op & 0x00FF;
                v_reg[x] = nn;
            }

            else if (digit1 == 0x7) {
                size_t x = digit2;
                uint8_t nn = op & 0x00FF;
                // wrapping add for uint8_t, normal + in c++ will wrap naturally due to unsigned overflow
                v_reg[x] = v_reg[x] + nn;
            }

            else if (digit1 == 0x8) {
                size_t x = digit2;
                size_t y = digit3;

                if (digit4 == 0x0) {
                    v_reg[x] = v_reg[y];
                }
                else if (digit4 == 0x1) {
                    v_reg[x] |= v_reg[y];
                }
                else if (digit4 == 0x2) {
                    v_reg[x] &= v_reg[y];
                }
                else if (digit4 == 0x3) {
                    v_reg[x] ^= v_reg[y];
                }
                else if  (digit4 == 0x4) {
                    // c++ doesn't have an overflow_add like rust so

                    uint16_t sum = static_cast<uint16_t>(v_reg[x]) + static_cast<uint16_t>(v_reg[y]); // widen operands and add
                    v_reg[x] = static_cast<uint8_t>(sum & 0xFF); // "reduce" 8bit and put into v_reg
                    v_reg[0xF] = (sum > 0xFF) ? 1 : 0; // check if overflow had happened using sum
                }
                else if  (digit4 == 0x5) {
                    // overflow_sub functions similarly here

                    uint16_t sum = static_cast<uint16_t>(v_reg[x]) - static_cast<uint16_t>(v_reg[y]);
                    v_reg[x] = static_cast<uint8_t>(sum & 0xFF);
                    v_reg[0xF] = (v_reg[x] >= v_reg[y]) ? 1 : 0; //here we instead check if we subtracted our x by a bigger y, which would mean underflow
                }
                else if (digit4 == 0x6) {
                    uint8_t z = v_reg[x] & 0x1;
                    v_reg[x] >>= 1;
                    v_reg[0xF] = z;
                }
                else if (digit4 == 0x7) {
                    uint16_t diff = static_cast<uint16_t>(v_reg[y]) - static_cast<uint16_t>(v_reg[x]);
                    v_reg[0xF] = (v_reg[y] >= v_reg[x]) ? 1 : 0;
                    v_reg[x] = static_cast<uint8_t>(diff & 0xFF);
                }
                else if (digit4 == 0xE) {
                    uint8_t msb = (v_reg[x] >> 7) & 0x1;
                    v_reg[x] <<= 1;
                    v_reg[0xF] = msb;
                }
            }

            else if (digit1 == 0x9 && digit4 == 0) {
                size_t x = digit2;
                size_t y = digit3;
                if (v_reg[x] != v_reg[y]) {
                    pc += 2;
                }
            }
            else if (digit1 == 0xA) {
                uint16_t nnn = op & 0x0FFF;
                i_reg = nnn;
            }
            else if (digit1 == 0xB) {
                uint16_t nnn = op & 0x0FFF;
                pc = static_cast<uint16_t>(v_reg[0]) + nnn;
            }
            else if (digit1 == 0xC) {
                size_t x = digit2;
                uint8_t nn = op & 0x00FF;
                
                // not even gonna act like I understand this random number stuff
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<uint16_t> dist(0, 255);

                uint8_t rng = static_cast<uint8_t>(dist(gen));

                v_reg[x] = rng & nn;
            }
            else if (digit1 == 0xD) {
                uint16_t x_cord = v_reg[digit2];
                uint16_t y_cord = v_reg[digit3];
                uint16_t height = digit4;

                bool flipped = false;

                for (uint16_t row = 0; row < height; ++row) {
                    uint16_t address = i_reg + row;
                    uint8_t pixels = ram[address];
                    for (uint8_t col = 0; col < 8; ++col) {
                        if ((pixels & (0b10000000 >> col)) != 0) {
                            size_t x = (x_cord + col) % SCREEN_WIDTH;
                            size_t y = (y_cord + row) % SCREEN_HEIGHT;
                            size_t pixel_id = x + SCREEN_WIDTH * y;
                            flipped |= screen[pixel_id];
                            screen[pixel_id] ^= true;
                        }
                    }
                }
                v_reg[0xF] = flipped ? 1 : 0;
            }
            else if (digit1 == 0xE && digit3 == 0x9 && digit4 == 0xE) {
                size_t x = digit2;
                uint8_t vx = v_reg[x];
                if (keys[vx]) {
                    pc += 2;
                }
            }
            else if (digit1 == 0xE && digit3 == 0xA && digit4 == 0x1) {
                size_t x = digit2;
                uint8_t vx = v_reg[x];
                if (!keys[vx]) {
                    pc += 2;
                }
            }
            else if (digit1 == 0xF){
                if (digit3 == 0x0){
                    if (digit4 == 0x7){
                        size_t x = digit2;
                        v_reg[x] = dt;
                    }
                    if (digit4 == 0xA){
                        size_t x = digit2;
                        bool pressed = false;
                        for (size_t i = 0; i < NUM_KEYS; ++i) {
                            if (keys[i]) {
                                v_reg[x] = static_cast<uint8_t>(i);
                                pressed = true;
                                break;
                            }
                        }
                        if (!pressed) {
                            pc -= 2;
                        }
                    }
                }
                else if (digit3 == 0x1){
                    if (digit4 == 0x5){
                        size_t x = digit2;
                        dt = v_reg[x];
                    }
                    else if (digit4 == 0x8){
                        size_t x = digit2;
                        st = v_reg[x];
                    }
                    else if (digit4 == 0xE){
                        size_t x = digit2;
                        i_reg = static_cast<uint16_t>(i_reg + v_reg[x]);
                    }
                }
            }
            else if (digit1 == 0xF && digit3 == 0x2 && digit4 == 0x9) {
                size_t x = digit2;
                uint16_t c = v_reg[x];
                i_reg = static_cast<uint16_t>(c * 5);
            }
            else if (digit1 == 0xF && digit3 == 0x3 && digit4 == 0x3) {
                size_t x = digit2;
                uint8_t vx = v_reg[x];

                uint8_t hundred = vx / 100;
                uint8_t ten = (vx / 10) % 10;
                uint8_t one = vx % 10;

                ram[i_reg] = hundred;
                ram[i_reg + 1] = ten;
                ram[i_reg + 2] = one;
            }
            else if (digit1 == 0xF && digit3 == 0x5 && digit4 == 0x5) {
                size_t x = digit2;
                for (size_t v_iterate = 0; v_iterate <= x; ++v_iterate) {
                    ram[i_reg + v_iterate] = v_reg[v_iterate];
                }
            }
            else if (digit1 == 0xF && digit3 == 0x6 && digit4 == 0x5) {
                size_t x = digit2;
                for (size_t v_iterate = 0; v_iterate <= x; ++v_iterate) {
                    v_reg[v_iterate] = ram[i_reg + v_iterate];
                }
            }
            else {
                throw std::runtime_error("Unimplemented opcode");
            }


        }




        uint16_t fetch (){
            uint16_t higher_byte = ram[pc];
            uint16_t lower_byte = ram[pc + 1];
            uint16_t op = (higher_byte << 8) | lower_byte;
            pc += 2;
            return op;
        }

        void tick_timers(){
            if (dt > 0) {
                dt -= 1;
            }
            if (st > 0) {
                st -= 1;
            }
        }
};


