#include <stdint.h>

// GBA Hardware Definitions
#define REG_DISPCNT     *(volatile uint16_t*)0x04000000
#define REG_VCOUNT      *(volatile uint16_t*)0x04000006
#define REG_KEYINPUT    *(volatile uint16_t*)0x04000130
#define REG_BG0CNT      *(volatile uint16_t*)0x04000008
#define REG_BG1CNT      *(volatile uint16_t*)0x0400000A
#define REG_BG1HOFS     *(volatile uint16_t*)0x04000014
#define REG_BG1VOFS     *(volatile uint16_t*)0x04000016

// Sound Registers
#define REG_SOUNDCNT_X  *(volatile uint16_t*)0x04000084
#define REG_SOUNDCNT_L  *(volatile uint16_t*)0x04000082
#define REG_SOUNDCNT_H  *(volatile uint16_t*)0x04000088
#define REG_SOUND1CNT_L *(volatile uint16_t*)0x04000060
#define REG_SOUND1CNT_H *(volatile uint16_t*)0x04000062
#define REG_SOUND1CNT_X *(volatile uint16_t*)0x04000064

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

typedef struct { uint32_t data[8]; } Tile;
typedef uint16_t ScreenEntry;

#define TILE_MEM        ((Tile*)0x06000000)
#define MAP_MEM_BG0     ((ScreenEntry*)0x0600C000)
#define MAP_MEM_BG1     ((ScreenEntry*)0x0600E000)

#include "huffman_data.h"
#include "font_data.h"

enum State { STATE_BOOT, STATE_MENU, STATE_ENCODE, STATE_ENCODE_RESULT, STATE_DECODE };
enum State current_state = STATE_BOOT;

uint16_t key_curr = 0, key_prev = 0;
char input_text[16] = {0};
int input_len = 0;
int kb_x = 0, kb_y = 0;
int is_lowercase = 0;

uint8_t grid[8][8] = {{0}};
int cur_row = 0, cur_col = 0;
uint16_t frame_count = 0;

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

void run_boot() {
    static int boot_frame = 0;
    clear_screen();
    draw_string(8, 9, "GEMINI SYSTEMS");
    boot_frame++;
    if (boot_frame == 30) play_beep();
    if (boot_frame > 120) {
        clear_screen();
        current_state = STATE_MENU;
        kb_x = 0;
    }
}

void run_menu() {
    draw_string(2, 2, "HUFFMAN V1.8 GOLD+");
    draw_string(4, 8, kb_x == 0 ? "> 1. ENCODE MODE" : "  1. ENCODE MODE");
    draw_string(4, 10, kb_x == 1 ? "> 2. DECODE MODE" : "  2. DECODE MODE");
    if(key_hit(KEY_UP)) { kb_x = 0; play_beep(); }
    if(key_hit(KEY_DOWN)) { kb_x = 1; play_beep(); }
    if(key_hit(KEY_A)) {
        play_beep();
        clear_screen();
        current_state = (kb_x == 0) ? STATE_ENCODE : STATE_DECODE;
        kb_x = 0; kb_y = 0;
    }
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

    if(key_hit(KEY_RIGHT)) { kb_x = (kb_x + 1) % 10; play_beep(); }
    if(key_hit(KEY_LEFT)) { kb_x = (kb_x + 9) % 10; play_beep(); }
    if(key_hit(KEY_DOWN)) { kb_y = (kb_y + 1) % 5; play_beep(); }
    if(key_hit(KEY_UP)) { kb_y = (kb_y + 4) % 5; play_beep(); }
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
    if(key_hit(KEY_RIGHT)) { cur_col = (cur_col + 1) % 8; play_beep(); }
    if(key_hit(KEY_LEFT)) { cur_col = (cur_col + 7) % 8; play_beep(); }
    if(key_hit(KEY_DOWN)) { cur_row = (cur_row + 1) % 8; play_beep(); }
    if(key_hit(KEY_UP)) { cur_row = (cur_row + 7) % 8; play_beep(); }
    if(key_hit(KEY_A)) { play_beep(); grid[cur_row][cur_col] = 1; cur_col = (cur_col + 1) % 8; if(cur_col == 0) cur_row = (cur_row + 1) % 8; }
    if(key_hit(KEY_B)) { play_beep(); grid[cur_row][cur_col] = 0; cur_col = (cur_col + 1) % 8; if(cur_col == 0) cur_row = (cur_row + 1) % 8; }
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

int main() {
    REG_DISPCNT = VIDEO_MODE_0 | BG0_ENABLE | BG1_ENABLE;
    REG_BG0CNT = 0x1800; 
    REG_BG1CNT = 0x1C01; 

    // Improved Sound init
    REG_SOUNDCNT_X = 0x0080; 
    REG_SOUNDCNT_H = 0x0002; 
    REG_SOUNDCNT_L = 0x1177; 

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
        }
    }
    return 0;
}
