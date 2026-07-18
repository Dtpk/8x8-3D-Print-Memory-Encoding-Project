#include <gb/gb.h>
#include <gbdk/console.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "huffman_gb_data.h"

enum State { STATE_BOOT, STATE_MENU, STATE_ENCODE, STATE_ENCODE_RESULT, STATE_DECODE };
uint8_t current_state = STATE_BOOT;

uint8_t key_curr = 0, key_prev = 0;
char input_text[16] = {0};
uint8_t input_len = 0;
uint8_t kb_x = 0, kb_y = 0;
uint8_t is_lowercase = 0;

uint8_t grid[8][8] = {{0}};
uint8_t cur_row = 0, cur_col = 0;

void play_beep(void) {
    NR10_REG = 0x16;
    NR11_REG = 0x40;
    NR12_REG = 0x73;
    NR13_REG = 0x00;
    NR14_REG = 0xC3;
}

void update_keys(void) {
    key_prev = key_curr;
    key_curr = joypad();
}

uint8_t key_hit(uint8_t key) { return (key_curr & key) && !(key_prev & key); }

int get_total_bits(void) {
    int total = 0;
    uint8_t i, j;
    for(i = 0; i < input_len; i++) {
        char c = input_text[i];
        for(j = 0; j < 67; j++) {
            if(huffman_codes[j].c == c) {
                const char* code = huffman_codes[j].code;
                while(*code++) total++;
                break;
            }
        }
    }
    return total;
}

void run_boot(void) {
    static uint8_t boot_frame = 0;
    if (boot_frame == 0) {
        cls();
        gotoxy(3, 8);
        printf("GEMINI SYSTEMS");
    }
    boot_frame++;
    if (boot_frame == 30) play_beep();
    if (boot_frame > 90) {
        cls();
        current_state = STATE_MENU;
        kb_x = 0;
    }
}

void run_menu(void) {
    gotoxy(2, 2);
    printf("HUFFMAN GB V1.9");
    gotoxy(3, 8);
    printf(kb_x == 0 ? "> 1. ENCODE MODE" : "  1. ENCODE MODE");
    gotoxy(3, 10);
    printf(kb_x == 1 ? "> 2. DECODE MODE" : "  2. DECODE MODE");
    
    if(key_hit(J_UP)) { kb_x = 0; play_beep(); }
    if(key_hit(J_DOWN)) { kb_x = 1; play_beep(); }
    if(key_hit(J_A)) {
        play_beep();
        cls();
        current_state = (kb_x == 0) ? STATE_ENCODE : STATE_DECODE;
        kb_x = 0; kb_y = 0;
        if (current_state == STATE_DECODE) {
            uint8_t r, c;
            for(r=0; r<8; r++) for(c=0; c<8; c++) grid[r][c] = 0;
            cur_row = 0; cur_col = 0;
        } else {
            input_len = 0;
            input_text[0] = 0;
        }
    }
}

void run_encode(void) {
    const char* kb_upper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789!-.?^M";
    const char* kb_lower = "abcdefghijklmnopqrstuvwxyz 0123456789!-.?^M";
    const char* kb = is_lowercase ? kb_lower : kb_upper;
    int total_bits = get_total_bits();
    uint8_t i;

    gotoxy(1, 0);
    printf("ENCODE: %s_   ", input_text);
    
    gotoxy(1, 2);
    if (total_bits > 64) {
        printf("![OVERFLOW!] ");
    } else {
        printf(is_lowercase ? "[LOWER] " : "[UPPER] ");
        printf("BITS:%d/64  ", total_bits);
    }

    for(i = 0; i < 43; i++) {
        uint8_t tx = i % 6;
        uint8_t ty = i / 6;
        gotoxy((uint8_t)(1 + tx * 3), (uint8_t)(4 + ty));
        if(kb_x == tx && kb_y == ty) {
            if (i == 41) printf("[^]");
            else if (i == 42) printf("[M]");
            else printf("[%c]", kb[i]);
        } else {
            if (i == 41) printf(" ^ ");
            else if (i == 42) printf(" M ");
            else printf(" %c ", kb[i]);
        }
    }

    if(key_hit(J_RIGHT)) { kb_x = (kb_x + 1) % 6; play_beep(); }
    if(key_hit(J_LEFT)) { kb_x = (kb_x + 5) % 6; play_beep(); }
    if(key_hit(J_DOWN)) { kb_y = (kb_y + 1) % 8; play_beep(); }
    if(key_hit(J_UP)) { kb_y = (kb_y + 7) % 8; play_beep(); }
    if(key_hit(J_SELECT)) { is_lowercase = !is_lowercase; play_beep(); cls(); }

    if(key_hit(J_A)) {
        uint8_t idx = kb_y * 6 + kb_x;
        if(idx < 41 && input_len < 15) {
            input_text[input_len++] = kb[idx];
            input_text[input_len] = 0;
            play_beep();
            cls();
        } else if (idx == 41) { // Shift
            is_lowercase = !is_lowercase; play_beep(); cls();
        } else if (idx == 42) { // Menu
            play_beep(); cls(); current_state = STATE_MENU;
        }
    }
    if(key_hit(J_B) && input_len > 0) { play_beep(); input_text[--input_len] = 0; cls(); }
    if(key_hit(J_START) && input_len > 0) { play_beep(); cls(); current_state = STATE_ENCODE_RESULT; }
}

void run_encode_result(void) {
    uint8_t r, c, i, j;
    char bits[65] = {0};
    uint8_t bptr = 0;
    
    gotoxy(1, 1);
    printf("BITS (8X8):");
    
    for(i = 0; i < input_len; i++) {
        char ch = input_text[i];
        for(j = 0; j < 67; j++) {
            if(huffman_codes[j].c == ch) {
                const char* code = huffman_codes[j].code;
                while(*code && bptr < 64) bits[bptr++] = *code++;
                break;
            }
        }
    }
    
    for(r = 0; r < 8; r++) {
        for(c = 0; c < 8; c++) {
            uint8_t idx = r * 8 + c;
            char m = (idx < bptr) ? (bits[idx] == '1' ? 'O' : '.') : '.';
            gotoxy((uint8_t)(4 + c * 2), (uint8_t)(3 + r));
            printf("%c", m);
        }
    }
    
    gotoxy(1, 14);
    printf("SELECT: MENU");
    if(key_hit(J_SELECT)) {
        play_beep();
        cls();
        current_state = STATE_MENU;
    }
}

void run_decode(void) {
    uint8_t r, c;
    gotoxy(1, 1);
    printf("DECO: A=1 B=0 ST=CLR");
    for(r = 0; r < 8; r++) {
        for(c = 0; c < 8; c++) {
            gotoxy((uint8_t)(4 + c * 2), (uint8_t)(3 + r));
            if(r == cur_row && c == cur_col) printf(">");
            else printf(" ");
            printf("%c", grid[r][c] ? 'O' : '.');
        }
    }
    
    if(key_hit(J_RIGHT)) { cur_col = (cur_col + 1) % 8; play_beep(); }
    if(key_hit(J_LEFT)) { cur_col = (cur_col + 7) % 8; play_beep(); }
    if(key_hit(J_DOWN)) { cur_row = (cur_row + 1) % 8; play_beep(); }
    if(key_hit(J_UP)) { cur_row = (cur_row + 7) % 8; play_beep(); }
    
    if(key_hit(J_A)) { 
        play_beep(); 
        grid[cur_row][cur_col] = 1; 
        cur_col = (cur_col + 1) % 8; 
        if(cur_col == 0) cur_row = (cur_row + 1) % 8; 
    }
    if(key_hit(J_B)) { 
        play_beep(); 
        grid[cur_row][cur_col] = 0; 
        cur_col = (cur_col + 1) % 8; 
        if(cur_col == 0) cur_row = (cur_row + 1) % 8; 
    }
    if(key_hit(J_START)) { 
        play_beep(); 
        for(c=0; c<8; c++) grid[cur_row][c] = 0; 
    }

    // Decoding logic
    {
        char decoded[17];
        uint8_t dptr = 0;
        int node = 0;
        memset(decoded, 0, 17);
        for(r = 0; r < 8; r++) {
            for(c = 0; c < 8; c++) {
                node = grid[r][c] ? huffman_tree[node].right : huffman_tree[node].left;
                if(huffman_tree[node].c != 0) {
                    if(dptr < 16) decoded[dptr++] = huffman_tree[node].c;
                    node = 0;
                }
            }
        }
        gotoxy(1, 13);
        printf("MSG:                "); // Clear line
        gotoxy(1, 13);
        printf("MSG: %s", decoded);
    }
    
    gotoxy(1, 15);
    printf("SELECT: MENU");
    
    if(key_hit(J_SELECT)) { play_beep(); cls(); current_state = STATE_MENU; }
}

int main(void) {
    NR52_REG = 0x80; // Turn on sound
    NR50_REG = 0x77; // Max volume
    NR51_REG = 0xFF; // All channels

    DISPLAY_ON;
    while(1) {
        update_keys();
        switch(current_state) {
            case STATE_BOOT: run_boot(); break;
            case STATE_MENU: run_menu(); break;
            case STATE_ENCODE: run_encode(); break;
            case STATE_ENCODE_RESULT: run_encode_result(); break;
            case STATE_DECODE: run_decode(); break;
        }
        wait_vbl_done();
    }
    return 0;
}
