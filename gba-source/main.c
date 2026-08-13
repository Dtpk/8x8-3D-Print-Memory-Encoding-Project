// ============================================================================
// SECTION 1: HARDWARE DEFINITIONS, GLOBALS & UART (BIT-BANG 2-WAY ACK)
// ============================================================================

#include <stdint.h>
#include <stdbool.h>

// GBA Hardware Definitions
#define REG_DISPCNT      *(volatile uint16_t*)0x04000000
#define REG_VCOUNT       *(volatile uint16_t*)0x04000006
#define REG_KEYINPUT     *(volatile uint16_t*)0x04000130
#define REG_BG0CNT       *(volatile uint16_t*)0x04000008
#define REG_BG1CNT       *(volatile uint16_t*)0x0400000A
#define REG_BG1HOFS      *(volatile uint16_t*)0x04000014
#define REG_BG1VOFS      *(volatile uint16_t*)0x04000016

// Sound Registers
#define REG_SOUND1CNT_L *(volatile uint16_t*)0x04000060
#define REG_SOUND1CNT_H *(volatile uint16_t*)0x04000062
#define REG_SOUND1CNT_X *(volatile uint16_t*)0x04000064
#define REG_SOUNDCNT_L  *(volatile uint16_t*)0x04000080
#define REG_SOUNDCNT_H  *(volatile uint16_t*)0x04000082
#define REG_SOUNDCNT_X  *(volatile uint16_t*)0x04000084

// SIO Registers
#define REG_SIOCNT       *(volatile uint16_t*)0x04000128
#define REG_SIODATA8    *(volatile uint16_t*)0x0400012A
#define REG_RCNT        *(volatile uint16_t*)0x04000134
#define REG_IME         *(volatile uint16_t*)0x04000208

// GPIO Bit Definitions for REG_RCNT
// Pin 3 (SI) -> TX Output to Pico GP5
// Pin 5 (SO) -> RX Input from Pico GP4
#define RCNT_GPIO         0x8000
#define RCNT_SI_DIR_OUT   0x0040
#define RCNT_SO_DIR_IN    0x0000

#define RCNT_SI_HIGH      (RCNT_GPIO | RCNT_SI_DIR_OUT | 0x0004)
#define RCNT_SI_LOW       (RCNT_GPIO | RCNT_SI_DIR_OUT)
#define RCNT_SO_READ      ((*(volatile uint16_t*)0x04000134) & 0x0002)

#define VIDEO_MODE_0    0x0000
#define BG0_ENABLE      0x0100
#define BG1_ENABLE      0x0200

#define MEM_VRAM        0x06000000
#define MEM_PALETTE     ((volatile uint16_t*)0x05000000)

// Input bits
#define KEY_A       0x0001
#define KEY_B       0x0002
#define KEY_SELECT  0x0004
#define KEY_START   0x0008
#define KEY_RIGHT   0x0010
#define KEY_LEFT    0x0020
#define KEY_UP      0x0040
#define KEY_DOWN    0x0080
#define KEY_R       0x0100
#define KEY_L       0x0200

// Ender 3 Bed Centering Offsets (235mm bed, 32mm matrix -> Center at 101.5mm)
#define BED_OFFSET_X 10150
#define BED_OFFSET_Y 10150
#define PLATE_SIZE   3200
#define BIT_SPACING  355
#define LAYER_HEIGHT 20    // 0.20mm standard height step

typedef struct { uint32_t data[8]; } Tile;
typedef uint16_t ScreenEntry;

#define TILE_MEM        ((Tile*)0x06000000)
#define MAP_MEM_BG0     ((ScreenEntry*)0x0600C000)
#define MAP_MEM_BG1     ((ScreenEntry*)0x0600E000)

#include "huffman_data.h"
#include "font_data.h"

enum State { STATE_BOOT, STATE_MENU, STATE_ENCODE, STATE_ENCODE_RESULT, STATE_DECODE, STATE_PRINT, STATE_PRINTER_CTRL, STATE_CABLE_TEST };
enum State current_state = STATE_BOOT;

uint16_t key_curr = 0, key_prev = 0;
char input_text[16] = {0};
int input_len = 0;
int kb_x = 0, kb_y = 0;
int is_lowercase = 0;
int printer_temp_set = 0;
int print_active = 0;

uint8_t grid[8][8] = {{0}};
int cur_row = 0, cur_col = 0;
uint16_t frame_count = 0;

// Global/Static Print State
int print_step = 0;
int print_substep = 0;
int print_row = 0, print_col = 0;
uint32_t print_wait_timer = 0;
char print_bits[128] = {0};
int print_bptr = 0;

void itoa_simple(int n, char* s);

int pack_huffman_bits() {
    int bptr = 0;
    for(int i = 0; i < input_len; i++) {
        char c = input_text[i];
        for(int j = 0; j < 67; j++) {
            if(huffman_codes[j].c == c) {
                const char* code = huffman_codes[j].code;
                while(*code && bptr < 64) print_bits[bptr++] = *code++;
                break;
            }
        }
    }
    return bptr;
}

void vblank_wait() {
    while(*(volatile uint16_t*)0x04000006 >= 160);
    while(*(volatile uint16_t*)0x04000006 < 160);
}

void update_keys() {
    key_prev = key_curr;
    key_curr = ~REG_KEYINPUT & 0x03FF;
}

int key_hit(uint16_t key) { return (key_curr & key) && !(key_prev & key); }

void play_beep() {
    REG_SOUND1CNT_L = 0x0000;
    REG_SOUND1CNT_H = 0xF240;
    REG_SOUND1CNT_X = 0x8700;
}

// Function Prototypes
void uart_init();
void uart_quiet();
void uart_raw_putc(char c);
void wait_ack();
void uart_putc(char c);
void uart_puts(const char* s);
void gcode_cmd(const char* cmd);

static void delay_9600_bit() {
    for (volatile int i = 0; i < 350; i++) {
        asm volatile("nop");
    }
}

// UART Functions (Bit-Bang Pin 3 TX / Pin 5 RX)
void uart_init() {
    REG_SIOCNT = 0x0000;
    REG_RCNT = RCNT_SI_HIGH;
}

void uart_quiet() {
    REG_RCNT = 0x0000;
}

// Low-level send byte without ACK wait
void uart_raw_putc(char c) {
    uint16_t prev_ime = REG_IME;
    REG_IME = 0;

    REG_RCNT = RCNT_SI_LOW; // Start Bit
    delay_9600_bit();

    for(int i=0; i<8; i++) {
        if(c & 1) REG_RCNT = RCNT_SI_HIGH;
        else      REG_RCNT = RCNT_SI_LOW;
        c >>= 1;
        delay_9600_bit();
    }

    REG_RCNT = RCNT_SI_HIGH; // Stop Bit
    delay_9600_bit();

    REG_IME = prev_ime;
}

// Blocking wait for ACK ('.') from Pico (Handles indefinite heat/home holds)
void wait_ack() {
    uint16_t prev_ime = REG_IME;
    REG_IME = 0;

    // Wait for Start Bit on Pin 5 (SO drops LOW) when Pico sends '.' ACK
    while (RCNT_SO_READ != 0);

    // Half-bit delay to sample in middle of bit cell
    for (volatile int i = 0; i < 175; i++) asm volatile("nop");

    // Read 8 bits
    for (int i = 0; i < 8; i++) {
        delay_9600_bit();
    }

    // Stop bit delay
    delay_9600_bit();

    REG_IME = prev_ime;
}

// Send byte and wait for Pico ACK on newline
void uart_putc(char c) {
    uart_raw_putc(c);

    // Once line termination is sent, wait for Pico ACK before returning
    if (c == '\n') {
        wait_ack();
    }
}

void uart_puts(const char* s) {
    while(*s) uart_putc(*s++);
}

// Send raw G-code string with newline and wait for Pico ACK
void gcode_cmd(const char* cmd) {
    uart_puts(cmd);
    uart_puts("\r\n");
}

// Zero-dependency integer formatter (Outputs fixed point 100x)
void uart_put_fixed2(int32_t val_100) {
    if (val_100 < 0) {
        uart_putc('-');
        val_100 = -val_100;
    }

    int32_t whole = 0;
    while (val_100 >= 100) {
        val_100 -= 100;
        whole++;
    }
    int32_t frac = val_100;

    static const int32_t powers[] = {100000, 10000, 1000, 100, 10, 1};
    bool started = false;

    for (int i = 0; i < 6; i++) {
        int32_t p = powers[i];
        int count = 0;
        while (whole >= p) {
            whole -= p;
            count++;
        }
        if (count > 0 || started || i == 5) {
            uart_putc('0' + count);
            started = true;
        }
    }

    uart_putc('.');
    int tens = 0;
    while (frac >= 10) {
        frac -= 10;
        tens++;
    }
    uart_putc('0' + tens);
    uart_putc('0' + frac);
}

void gcode_move(int32_t x, int32_t y, int32_t z, int32_t e, int f) {
    if (e != 0) uart_puts("G1 X");
    else uart_puts("G0 X");

    uart_put_fixed2(x);

    uart_puts(" Y");
    uart_put_fixed2(y);

    uart_puts(" Z");
    uart_put_fixed2(z);

    if (e != 0) {
        uart_puts(" E");
        uart_put_fixed2(e);
    }

    uart_puts(" F");
    static const int powers_f[] = {10000, 1000, 100, 10, 1};
    bool started_f = false;
    for (int i = 0; i < 5; i++) {
        int p = powers_f[i];
        int count = 0;
        while (f >= p) {
            f -= p;
            count++;
        }
        if (count > 0 || started_f || i == 4) {
            uart_putc('0' + count);
            started_f = true;
        }
    }

    uart_puts("\r\n");
}

void itoa_simple(int n, char* s) {
    int i = 0;
    if (n == 0) { s[i++] = '0'; s[i] = 0; return; }
    if (n < 0) { s[i++] = '-'; n = -n; }
    char buf[16];
    int j = 0;
    while(n > 0) { buf[j++] = (n % 10) + '0'; n /= 10; }
    while(j > 0) s[i++] = buf[--j];
    s[i] = 0;
}

void draw_char(int x, int y, char c) {
    if(x >= 0 && x < 30 && y >= 0 && y < 20) {
        MAP_MEM_BG0[y * 32 + x] = (ScreenEntry)c;
    }
}

void draw_string(int x, int y, const char* s) {
    while(*s) draw_char(x++, y, *s++);
}

void clear_screen() {
    for(int i = 0; i < 32 * 32; i++) MAP_MEM_BG0[i] = 0;
}

// ============================================================================
// SECTION 2: UI MENUS & INPUT HANDLING
// ============================================================================

void run_boot() {
    static int boot_frame = 0;
    clear_screen();
    draw_string(8, 9, "OMEGA lul SYSTEMS");
    boot_frame++;
    if (boot_frame == 30) play_beep();
    if (boot_frame > 120) {
        clear_screen();
        current_state = STATE_MENU;
        kb_x = 0;
    }
}

void run_menu() {
    draw_string(2, 2, "HUFFMAN PRINTER FINAL");
    draw_string(4, 5, kb_x == 0 ? "> 1. ENCODE MODE" : "  1. ENCODE MODE");
    draw_string(4, 7, kb_x == 1 ? "> 2. DECODE MODE" : "  2. DECODE MODE");
    draw_string(4, 9, kb_x == 2 ? "> 3. 3D PRINT" : "  3. 3D PRINT");
    draw_string(4, 11, kb_x == 3 ? "> 4. PRINTER CTRL" : "  4. PRINTER CTRL");
    draw_string(4, 13, kb_x == 4 ? "> 5. CABLE TEST" : "  5. CABLE TEST");

    if(key_hit(KEY_UP)) { kb_x = (kb_x == 0) ? 4 : kb_x - 1; play_beep(); }
    if(key_hit(KEY_DOWN)) { kb_x = (kb_x == 4) ? 0 : kb_x + 1; play_beep(); }
    if(key_hit(KEY_A)) {
        play_beep();
        clear_screen();
        if (kb_x == 0) current_state = STATE_ENCODE;
        else if (kb_x == 1) current_state = STATE_DECODE;
        else if (kb_x == 2) { current_state = STATE_PRINT; print_step = 0; print_active = 1; uart_init(); }
        else if (kb_x == 3) { current_state = STATE_PRINTER_CTRL; uart_init(); }
        else { current_state = STATE_CABLE_TEST; }
        kb_x = 0; kb_y = 0;
    }
}

void run_printer_ctrl() {
    draw_string(2, 1, "PRINTER CONTROL");
    draw_string(4, 3, kb_x == 0 ? "> AUTO HOME (G28)"    : "  AUTO HOME (G28)");
    draw_string(4, 5, kb_x == 1 ? "> PREHEAT (200/55)"   : "  PREHEAT (200/55)");
    draw_string(4, 7, kb_x == 2 ? "> COOL DOWN (OFF)"   : "  COOL DOWN (OFF)");
    draw_string(4, 9, kb_x == 3 ? "> EXTRUDE (10mm)"    : "  EXTRUDE (10mm)");
    draw_string(4, 11, kb_x == 4 ? "> RAISE Z (+50mm)"   : "  RAISE Z (+50mm)");
    draw_string(4, 13, kb_x == 5 ? "> OFF MOTORS (M84)"  : "  OFF MOTORS (M84)");

    draw_string(2, 18, "SELECT:MENU");

    if(key_hit(KEY_UP)) { kb_x = (kb_x == 0) ? 5 : kb_x - 1; play_beep(); }
    if(key_hit(KEY_DOWN)) { kb_x = (kb_x == 5) ? 0 : kb_x + 1; play_beep(); }

    if(key_hit(KEY_A)) {
        play_beep();
        if (kb_x == 0) {
            gcode_cmd("G28");
            draw_string(2, 17, "COMMAND SENT: HOME ");
        } else if (kb_x == 1) {
            gcode_cmd("M140 S55");
            gcode_cmd("M104 S200");
            printer_temp_set = 1;
            draw_string(2, 17, "COMMAND SENT: HEAT ");
        } else if (kb_x == 2) {
            gcode_cmd("M104 S0");
            gcode_cmd("M140 S0");
            printer_temp_set = 0;
            draw_string(2, 17, "COMMAND SENT: COOL ");
        } else if (kb_x == 3) {
            gcode_cmd("G91");
            gcode_cmd("G1 E10 F100");
            gcode_cmd("G90");
            draw_string(2, 17, "COMMAND SENT: EXTR ");
        } else if (kb_x == 4) {
            gcode_cmd("G91");
            gcode_cmd("G1 Z50 F1500");
            gcode_cmd("G90");
            draw_string(2, 17, "COMMAND SENT: Z+50 ");
        } else if (kb_x == 5) {
            gcode_cmd("M84");
            draw_string(2, 17, "COMMAND SENT: M84  ");
        }
    }

    if(key_hit(KEY_SELECT)) { play_beep(); clear_screen(); current_state = STATE_MENU; uart_quiet(); }
}

void run_cable_test() {
    static int gpio_high = 0;
    static int uart_on = 0;

    draw_string(2, 2, "CABLE TEST MODE");
    draw_string(4, 6, kb_x == 0 ? (gpio_high ? "> PIN 3 (SI): HIGH" : "> PIN 3 (SI): LOW ") : (gpio_high ? "  PIN 3 (SI): HIGH" : "  PIN 3 (SI): LOW "));
    draw_string(4, 8, kb_x == 1 ? (uart_on ? "> UART BLAST (A): ON" : "> UART BLAST (A): OFF") : (uart_on ? "  UART BLAST (A): ON" : "  UART BLAST (A): OFF"));

    draw_string(2, 14, "Test: Pin 3 (SI) & GND");
    draw_string(2, 15, "Black: P3, White: P6(GND)");
    draw_string(2, 18, "SELECT:MENU");

    if (uart_on && key_hit(KEY_A)) {
        uart_init();
        for (int i = 0; i < 10; i++) {
            uart_raw_putc('U');
        }
        draw_string(2, 11, "BURST SENT!    ");
    } else if (!uart_on) {
        REG_SIOCNT = 0x0000;
        if (gpio_high) {
            REG_RCNT = RCNT_SI_HIGH;
        } else {
            REG_RCNT = RCNT_SI_LOW;
        }
    }

    if(key_hit(KEY_UP) || key_hit(KEY_DOWN)) { kb_x = !kb_x; play_beep(); }
    if(key_hit(KEY_A) && kb_x == 0) {
        play_beep();
        gpio_high = !gpio_high;
    } else if (key_hit(KEY_A) && kb_x == 1) {
        play_beep();
        uart_on = !uart_on;
    }

    if(key_hit(KEY_SELECT)) { play_beep(); clear_screen(); current_state = STATE_MENU; uart_quiet(); }
}

int get_total_bits() {
    int total = 0;
    for(int i = 0; i < input_len; i++) {
        char c = input_text[i];
        for(int j = 0; j < 67; j++) {
            if(huffman_codes[j].c == c) {
                const char* code = huffman_codes[j].code;
                while(*code++) total++;
                break;
            }
        }
    }
    return total;
}

void run_encode() {
    const char* kb_upper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789!-.?";
    const char* kb_lower = "abcdefghijklmnopqrstuvwxyz 0123456789!-.?";
    const char* kb = is_lowercase ? kb_lower : kb_upper;

    int total_bits = get_total_bits();
    draw_string(1, 1, "ENCODE:");
    draw_string(9, 1, input_text);
    draw_char(9 + input_len, 1, '_');

    if (total_bits > 64) {
        draw_string(1, 3, "![OVERFLOW!] ");
    } else {
        draw_string(1, 3, is_lowercase ? "[LOWER]  " : "[UPPER]  ");
        draw_string(11, 3, "BITS:");
        char bcount[4];
        bcount[0] = '0' + (total_bits / 10);
        bcount[1] = '0' + (total_bits % 10);
        bcount[2] = 0;
        draw_string(17, 3, bcount);
        draw_string(19, 3, "/64");
    }

    for(int i = 0; i < 41; i++) {
        int tx = i % 10;
        int ty = i / 10;
        draw_char(2 + tx * 2, 5 + ty * 2, kb[i]);
        if(kb_x == tx && kb_y == ty) {
            draw_char(1 + tx * 2, 5 + ty * 2, '[');
            draw_char(3 + tx * 2, 5 + ty * 2, ']');
        } else {
            draw_char(1 + tx * 2, 5 + ty * 2, ' ');
            draw_char(3 + tx * 2, 5 + ty * 2, ' ');
        }
    }

    draw_string(1, 16, "L/R: SHIFT  START: GRID");
    draw_string(1, 17, "B: DELETE   SELECT: MENU");

    if(key_hit(KEY_RIGHT)) { kb_x = (kb_x == 9) ? 0 : kb_x + 1; play_beep(); }
    if(key_hit(KEY_LEFT)) { kb_x = (kb_x == 0) ? 9 : kb_x - 1; play_beep(); }
    if(key_hit(KEY_DOWN)) { kb_y = (kb_y == 4) ? 0 : kb_y + 1; play_beep(); }
    if(key_hit(KEY_UP)) { kb_y = (kb_y == 0) ? 4 : kb_y - 1; play_beep(); }
    if(key_hit(KEY_L) || key_hit(KEY_R)) { is_lowercase = !is_lowercase; play_beep(); }

    if(key_hit(KEY_A)) {
        int idx = kb_y * 10 + kb_x;
        if(idx < 41 && input_len < 15) {
            input_text[input_len++] = kb[idx];
            input_text[input_len] = 0;
            play_beep();
        }
    }
    if(key_hit(KEY_B) && input_len > 0) { play_beep(); input_text[--input_len] = 0; clear_screen(); }
    if(key_hit(KEY_START) && input_len > 0) { play_beep(); clear_screen(); current_state = STATE_ENCODE_RESULT; }
    if(key_hit(KEY_SELECT)) { play_beep(); clear_screen(); current_state = STATE_MENU; }
}

void run_encode_result() {
    draw_string(1, 1, "BITS (8X8):");
    char bits[128] = {0};
    int bptr = 0;
    for(int i = 0; i < input_len; i++) {
        char c = input_text[i];
        for(int j = 0; j < 67; j++) {
            if(huffman_codes[j].c == c) {
                const char* code = huffman_codes[j].code;
                while(*code && bptr < 64) bits[bptr++] = *code++;
                break;
            }
        }
    }
    for(int r = 0; r < 8; r++) {
        for(int c = 0; c < 8; c++) {
            int idx = r * 8 + c;
            char m = (idx < bptr) ? (bits[idx] == '1' ? 'O' : '.') : '.';
            draw_char(8 + c * 2, 4 + r * 2, m);
        }
    }
    draw_string(1, 19, "SELECT:MENU");
    if(key_hit(KEY_SELECT)) { play_beep(); clear_screen(); current_state = STATE_MENU; }
}

void run_decode() {
    draw_string(1, 1, "DECODE: A=1 B=0 START=CLR");
    for(int r = 0; r < 8; r++) {
        for(int c = 0; c < 8; c++) {
            draw_char(8 + c * 2, 3 + r * 2, grid[r][c] ? 'O' : '.');
            if(r == cur_row && c == cur_col) draw_char(7 + c * 2, 3 + r * 2, '>');
            else draw_char(7 + c * 2, 3 + r * 2, ' ');
        }
    }
    if(key_hit(KEY_RIGHT)) { cur_col = (cur_col == 7) ? 0 : cur_col + 1; play_beep(); }
    if(key_hit(KEY_LEFT)) { cur_col = (cur_col == 0) ? 7 : cur_col - 1; play_beep(); }
    if(key_hit(KEY_DOWN)) { cur_row = (cur_row == 7) ? 0 : cur_row + 1; play_beep(); }
    if(key_hit(KEY_UP)) { cur_row = (cur_row == 0) ? 7 : cur_row - 1; play_beep(); }
    if(key_hit(KEY_A)) { play_beep(); grid[cur_row][cur_col] = 1; cur_col = (cur_col == 7) ? 0 : cur_col + 1; if(cur_col == 0) cur_row = (cur_row == 7) ? 0 : cur_row + 1; }
    if(key_hit(KEY_B)) { play_beep(); grid[cur_row][cur_col] = 0; cur_col = (cur_col == 7) ? 0 : cur_col + 1; if(cur_col == 0) cur_row = (cur_row == 7) ? 0 : cur_row + 1; }
    if(key_hit(KEY_START)) { play_beep(); for(int i=0; i<8; i++) grid[cur_row][i] = 0; }

    char decoded[32] = {0};
    int dptr = 0, node = 0;
    for(int r = 0; r < 8; r++) {
        for(int c = 0; c < 8; c++) {
            node = grid[r][c] ? huffman_tree[node].right : huffman_tree[node].left;
            if(huffman_tree[node].c != 0) {
                if(dptr < 31) decoded[dptr++] = huffman_tree[node].c;
                node = 0;
            }
        }
    }
    draw_string(1, 18, "MSG:");
    draw_string(6, 18, "                ");
    draw_string(6, 18, decoded);
    draw_string(1, 19, "SELECT:MENU");
    if(key_hit(KEY_SELECT)) { play_beep(); clear_screen(); current_state = STATE_MENU; }
}

// ============================================================================
// SECTION 3: 3D PRINTING ENGINE (AIR GAP + FULL TOP BEZEL & CORNER NOTCH)
// ============================================================================

// Helper to draw outer notched perimeter wall (3.0mm top-left corner chamfer)
void gcode_draw_notched_perimeter(int32_t z, int e_val, int feed_rate) {
    int32_t x0 = BED_OFFSET_X;
    int32_t y0 = BED_OFFSET_Y;
    int32_t x1 = BED_OFFSET_X + PLATE_SIZE;
    int32_t y1 = BED_OFFSET_Y + PLATE_SIZE;
    int32_t notch = 300; // 3.0mm corner chamfer

    // Move to start of top-left notch
    gcode_move(x0 + notch, y1, z, 0, 9000);
    gcode_cmd("G1 E2.50 F1500");

    // Trace 5-point notched outer square
    gcode_move(x1, y1, z, e_val, feed_rate);         // Top edge to Top-Right
    gcode_move(x1, y0, z, e_val, feed_rate);         // Right edge to Bottom-Right
    gcode_move(x0, y0, z, e_val, feed_rate);         // Bottom edge to Bottom-Left
    gcode_move(x0, y1 - notch, z, e_val, feed_rate); // Left edge to Notch start
    gcode_move(x0 + notch, y1, z, e_val, feed_rate); // Diagonal Chamfer Notch

    gcode_cmd("G1 E-2.50 F1500");
}

void run_print(void) {
    if (print_step == 0) {
        if (!print_active) return;
        draw_string(2, 4, "PRINT INITIALIZING");
        print_bptr = pack_huffman_bits();
        print_row = 0; print_col = 0; print_substep = 0; print_wait_timer = 0;
        print_step = 1;

    } else if (print_step == 1) {
        draw_string(2, 4, "HOMING & HEATING  ");
        gcode_cmd("G28");
        gcode_cmd("M106 S0");
        gcode_cmd("M190 S55");  // Wait for bed 55C
        gcode_cmd("M109 S200"); // Wait for nozzle 200C
        gcode_cmd("G90");
        gcode_cmd("M83");
        print_step++;

    } else if (print_step == 2) {
        draw_string(2, 4, "PURGING & SKIRT   ");
        gcode_move(1000, 2000, 28, 0, 9000);
        gcode_move(1000, 20000, 28, 1500, 1500);
        gcode_move(1040, 20000, 28, 0, 9000);
        gcode_move(1040, 2000, 28, 1500, 1500);
        gcode_cmd("G1 E-2.50 F1500");

        // Skirt surrounding the notched plate
        int32_t sk_min_x = BED_OFFSET_X - 500;
        int32_t sk_min_y = BED_OFFSET_Y - 500;
        int32_t sk_max_x = BED_OFFSET_X + PLATE_SIZE + 500;
        int32_t sk_max_y = BED_OFFSET_Y + PLATE_SIZE + 500;

        gcode_move(sk_min_x, sk_min_y, 20, 0, 9000);
        gcode_cmd("G1 E2.50 F1500");
        gcode_move(sk_max_x, sk_min_y, 20, 200, 1500);
        gcode_move(sk_max_x, sk_max_y, 20, 200, 1500);
        gcode_move(sk_min_x, sk_max_y, 20, 200, 1500);
        gcode_move(sk_min_x, sk_min_y, 20, 200, 1500);
        gcode_cmd("G1 E-2.50 F1500");
        print_step++;

    } else if (print_step == 3 || print_step == 4) {
        // LAYERS 1 & 2: BOTTOM BASE (Z = 0.20mm & 0.40mm)
        int32_t z_base = (print_step == 3) ? 20 : 40;
        int feed_rate = 1500;
        draw_string(2, 4, print_step == 3 ? "LAYER 1: BOTTOM B1" : "LAYER 2: BOTTOM B2");

        gcode_draw_notched_perimeter(z_base, 110, feed_rate);

        int32_t in_min_x = BED_OFFSET_X + 50;
        int32_t in_max_x = BED_OFFSET_X + PLATE_SIZE - 50;
        int32_t in_min_y = BED_OFFSET_Y + 50;
        int32_t in_max_y = BED_OFFSET_Y + PLATE_SIZE - 50;

        gcode_move(in_min_x, in_min_y, z_base, 0, 9000);
        gcode_cmd("G1 E2.50 F1500");

        if (print_step == 3) {
            // Layer 1: Horizontal Serpentine
            bool dir = true;
            for (int32_t y = in_min_y; y <= in_max_y; y += 35) {
                if (dir) {
                    gcode_move(in_max_x, y, z_base, 110, feed_rate);
                    if (y + 35 <= in_max_y) gcode_move(in_max_x, y + 35, z_base, 15, feed_rate);
                } else {
                    gcode_move(in_min_x, y, z_base, 110, feed_rate);
                    if (y + 35 <= in_max_y) gcode_move(in_min_x, y + 35, z_base, 15, feed_rate);
                }
                dir = !dir;
            }
        } else {
            // Layer 2: Vertical Serpentine
            bool dir = true;
            for (int32_t x = in_min_x; x <= in_max_x; x += 35) {
                if (dir) {
                    gcode_move(x, in_max_y, z_base, 110, feed_rate);
                    if (x + 35 <= in_max_x) gcode_move(x + 35, in_max_y, z_base, 20, feed_rate);
                } else {
                    gcode_move(x, in_min_y, z_base, 110, feed_rate);
                    if (x + 35 <= in_max_x) gcode_move(x + 35, in_min_y, z_base, 20, feed_rate);
                }
                dir = !dir;
            }
        }
        gcode_cmd("G1 E-2.50 F1500");
        print_step++;

    } else if (print_step == 5) {
        // LAYER 3: DATA CORE BITS (Z = 0.60mm - SINGLE PASS)
        draw_string(2, 4, "LAYER 3: DATA CORE");
        gcode_cmd("M106 S255"); // Turn on cooling fan

        if (print_row == 0 && print_col == 0) {
            gcode_draw_notched_perimeter(60, 32, 2400);
        }

        int idx = print_row * 8 + print_col;
        char bit = (idx < print_bptr) ? print_bits[idx] : '0';
        int32_t px = BED_OFFSET_X + (int32_t)(print_col + 1) * BIT_SPACING;
        int32_t py = BED_OFFSET_Y + (int32_t)((7 - print_row) + 1) * BIT_SPACING;

        // Z-Hop Travel to Target Bit Node
        gcode_move(px - 100, py - 100, 110, 0, 9000);
        gcode_move(px - 100, py - 100, 60, 0, 9000);
        gcode_cmd("G1 E2.50 F1500");

        if (bit == '1') {
            gcode_move(px + 100, py - 100, 60, 22, 2000);
            gcode_move(px + 100, py + 100, 60, 22, 2000);
            gcode_move(px - 100, py + 100, 60, 22, 2000);
            gcode_move(px - 100, py - 100, 60, 22, 2000);

            gcode_move(px - 60, py - 50, 60, 0, 9000);
            gcode_move(px + 60, py - 50, 60, 16, 2000);

            gcode_move(px + 60, py,      60, 0, 9000);
            gcode_move(px - 60, py,      60, 16, 2000);

            gcode_move(px - 60, py + 50, 60, 0, 9000);
            gcode_move(px + 60, py + 50, 60, 16, 2000);
        } else {
            gcode_move(px + 100, py - 100, 60, 22, 2000);
            gcode_move(px + 100, py,       60, 12, 2000);
            gcode_move(px - 100, py,       60, 22, 2000);
            gcode_move(px - 100, py - 100, 60, 12, 2000);

            gcode_move(px - 60, py - 50, 60, 0, 9000);
            gcode_move(px + 60, py - 50, 60, 16, 2000);
        }

        gcode_cmd("G1 E-2.50 F1500");
        gcode_move(px + 60, py + 50, 110, 0, 9000);

        draw_char(8 + print_col * 2, 8 + print_row, bit == '1' ? 'O' : 'o');

        print_col++;
        if (print_col >= 8) {
            print_col = 0; print_row++;
            if (print_row >= 8) {
                print_step++; print_row = 0; print_col = 0; return;
            }
        }

    } else if (print_step == 6) {
        // LAYER 4: RECESSED POCKET FRAME (Z = 0.80mm - WALL ONLY, NO BITS)
        draw_string(2, 4, "LAYER 4: WALL FRAME");
        gcode_draw_notched_perimeter(80, 32, 2400);
        print_step++;

    } else if (print_step == 7 || print_step == 8) {
        // LAYERS 5 & 6: TOP ROOF / CAP (Z = 1.00mm & 1.20mm)
        int32_t z_top = (print_step == 7) ? 100 : 120;
        int feed_rate = 1800;
        draw_string(2, 4, print_step == 7 ? "LAYER 5: TOP CAP 1" : "LAYER 6: TOP CAP 2");

        gcode_draw_notched_perimeter(z_top, 110, feed_rate);

        int32_t in_min_x = BED_OFFSET_X + 50;
        int32_t in_max_x = BED_OFFSET_X + PLATE_SIZE - 50;
        int32_t in_min_y = BED_OFFSET_Y + 50;
        int32_t in_max_y = BED_OFFSET_Y + PLATE_SIZE - 50;

        gcode_move(in_min_x, in_min_y, z_top, 0, 9000);
        gcode_cmd("G1 E2.50 F1500");

        if (print_step == 7) {
            // Horizontal Cap
            bool dir = true;
            for (int32_t y = in_min_y; y <= in_max_y; y += 40) {
                if (dir) {
                    gcode_move(in_max_x, y, z_top, 95, feed_rate);
                    if (y + 40 <= in_max_y) gcode_move(in_max_x, y + 40, z_top, 12, feed_rate);
                } else {
                    gcode_move(in_min_x, y, z_top, 95, feed_rate);
                    if (y + 40 <= in_max_y) gcode_move(in_min_x, y + 40, z_top, 12, feed_rate);
                }
                dir = !dir;
            }
        } else {
            // Vertical Cap
            bool dir = true;
            for (int32_t x = in_min_x; x <= in_max_x; x += 40) {
                if (dir) {
                    gcode_move(x, in_max_y, z_top, 95, feed_rate);
                    if (x + 40 <= in_max_x) gcode_move(x + 40, in_max_y, z_top, 12, feed_rate);
                } else {
                    gcode_move(x, in_min_y, z_top, 95, feed_rate);
                    if (x + 40 <= in_max_x) gcode_move(x + 40, in_min_y, z_top, 12, feed_rate);
                }
                dir = !dir;
            }
        }
        gcode_cmd("G1 E-2.50 F1500");
        print_step++;

    } else if (print_step == 9 || print_step == 10) {
        // LAYERS 7 & 8: RAISED NOTCHED TOP BEZEL / LIP (Z = 1.40mm & 1.60mm)
        int32_t z_rim = (print_step == 9) ? 140 : 160;
        draw_string(2, 4, print_step == 9 ? "LAYER 7: TOP BEZEL 1" : "LAYER 8: TOP BEZEL 2");

        gcode_draw_notched_perimeter(z_rim, 110, 2000);
        print_step++;

    } else if (print_step >= 11) {
        draw_string(2, 4, "SANDWICH COMPLETE!");
        draw_string(2, 18, "SELECT: MENU      ");

        if (print_active) {
            gcode_cmd("G91");
            gcode_cmd("G1 Z20 F1500");
            gcode_cmd("G90");
            gcode_cmd("G1 Y220 F3000");
            gcode_cmd("M104 S0");
            gcode_cmd("M140 S0");
            gcode_cmd("M84");
            print_active = 0;
        }

        if (key_hit(KEY_SELECT) || key_hit(KEY_START) || key_hit(KEY_A)) {
            play_beep();
            clear_screen();
            current_state = STATE_MENU;
            uart_quiet();
        }
    }
}


// ============================================================================
// SECTION 4: MAIN ENTRY POINT
// ============================================================================

int main() {
    REG_DISPCNT = VIDEO_MODE_0 | BG0_ENABLE | BG1_ENABLE;
    REG_BG0CNT = 0x1800;
    REG_BG1CNT = 0x1C01;

    REG_SOUNDCNT_X = 0x0080;
    REG_SOUNDCNT_L = 0x1177;
    REG_SOUNDCNT_H = 0x0002;

    uart_quiet();

    for(int y = 0; y < 8; y++) TILE_MEM[0].data[y] = 0;

    for(int i = 0; i < 128; i++) {
        for(int y = 0; y < 8; y++) {
            uint8_t row = font8x8_basic[i][y];
            uint32_t tr = 0;
            for(int x = 0; x < 8; x++) if(row & (1 << (7 - x))) tr |= (1 << (x * 4));
            TILE_MEM[i].data[y] = tr;
        }
    }

    for(int y = 0; y < 8; y++) {
        TILE_MEM[127].data[y] = (y == 0) ? 0x22222222 : 0x00000002;
    }

    for(int i = 0; i < 256; i++) MEM_PALETTE[i] = 0;
    MEM_PALETTE[1] = 0x7FFF; // White
    MEM_PALETTE[2] = 0x318C; // Brighter Grid

    for(int i = 0; i < 32 * 32; i++) MAP_MEM_BG1[i] = 127;

    while(1) {
        vblank_wait(); update_keys(); frame_count++;
        REG_BG1HOFS = frame_count / 2;
        REG_BG1VOFS = frame_count / 4;
        switch(current_state) {
            case STATE_BOOT: run_boot(); break;
            case STATE_MENU: run_menu(); break;
            case STATE_ENCODE: run_encode(); break;
            case STATE_ENCODE_RESULT: run_encode_result(); break;
            case STATE_DECODE: run_decode(); break;
            case STATE_PRINT: run_print(); break;
            case STATE_PRINTER_CTRL: run_printer_ctrl(); break;
            case STATE_CABLE_TEST: run_cable_test(); break;
        }
    }
    return 0;
}
