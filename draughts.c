/* 
    Draughts Game for La-Fortuna Board provided by the University of Southampton

    Project for COMP2215 2018-2019 designed by Marcin Wiech

    Project structure inspired by github user MKLOL based on GNU General Public License v2.0
*/

#include <stdlib.h>
#include <stdio.h>
#include "lcd.h"
#include "ruota.h"
#include "rios.h"
#include <avr/interrupt.h>

#define LCD_WIDTH    320
#define LCD_HEIGHT    240
#define BOARD_SIZE  240
#define BOARD_SQUARE_SIZE 30

#define BLACK1 0x18C4
#define BLACK2 0x1001
#define SELECT_GREEN 0x76AD
#define SUPER_RED 0xD041
#define SUPER_BLUE 0x325D
#define BORDER_COLOUR 0x4228

#define true 1
#define false 0

typedef int8_t bool;

typedef struct {
    int8_t x, y;
} position;

typedef struct {
    position pos;
    bool player;
    bool moved;
} token;

token tokens[24] = {
        {{1, 0}, 0, 0},
        {{3, 0}, 0, 0},
        {{5, 0}, 0, 0},
        {{7, 0}, 0, 0},
        {{0, 1}, 0, 0},
        {{2, 1}, 0, 0},
        {{4, 1}, 0, 0},
        {{6, 1}, 0, 0},
        {{1, 2}, 0, 0},
        {{3, 2}, 0, 0},
        {{5, 2}, 0, 0},
        {{7, 2}, 0, 0},
        {{0, 7}, 1, 0},
        {{2, 7}, 1, 0},
        {{4, 7}, 1, 0},
        {{6, 7}, 1, 0},
        {{1, 6}, 1, 0},
        {{3, 6}, 1, 0},
        {{5, 6}, 1, 0},
        {{7, 6}, 1, 0},
        {{0, 5}, 1, 0},
        {{2, 5}, 1, 0},
        {{4, 5}, 1, 0},
        {{6, 5}, 1, 0}
};


void os_init(void);

void draw_start_board(void);

void draw_square(int8_t, int8_t);

void draw_token(int8_t, int8_t, int8_t);

void draw_token_at_position(token *);

void perform_action(void);

token *get_token_at_position(int8_t, int8_t);

bool is_valid_move(void);

bool can_move(void);

void move_cursor(int8_t, int8_t);

int check_switches(int);

// false is blue player and true is red player
bool turn = false;

token *selected_token = NULL;
token *pawn_replaced = NULL;
position previous_selected_position = {-1, -1};
position current_cursor_position = {0, 0};

void main(void) {

    os_init();

    os_add_task(check_switches, 100, 1);

    draw_start_board();

    sei();
    for (;;) {
    }

}

void os_init(void) {
    /* 8MHz clock, no prescaling (DS, p. 48) */
    CLKPR = (1 << CLKPCE);
    CLKPR = 0;

    init_lcd();
    os_init_scheduler();
    os_init_ruota();
}

/*
*	Drawing functions
*/
void draw_start_board() {
    clear_screen();

    int i;
    int j;

    for (i = 0; i < 8; i++) {
        for (j = 0; j < 8; j++) {
            draw_square(i, j);
        }
    }
}

void draw_square(int8_t x, int8_t y) {
    uint16_t colour;

    if (current_cursor_position.x == x && current_cursor_position.y == y) {
        colour = SELECT_GREEN;
    } else {
        colour = BORDER_COLOUR;
    }

    rectangle rect = {
            x * BOARD_SQUARE_SIZE+40,
            x * BOARD_SQUARE_SIZE + BOARD_SQUARE_SIZE+40,
            y * BOARD_SQUARE_SIZE,
            y * BOARD_SQUARE_SIZE + BOARD_SQUARE_SIZE,
    };

    fill_rectangle(rect, colour);

    if(!(current_cursor_position.x == x && current_cursor_position.y == y)){
        rectangle rectf_fill = {
                x * BOARD_SQUARE_SIZE+1+40,
                x * BOARD_SQUARE_SIZE + BOARD_SQUARE_SIZE-1+40,
                y * BOARD_SQUARE_SIZE+1,
                y * BOARD_SQUARE_SIZE + BOARD_SQUARE_SIZE-1,
        };

        if ((x + y) % 2 == 0) {
            colour = BLACK1;
        } else {
            colour = BLACK2;
        }

        fill_rectangle(rectf_fill, colour);

    }

    token *token_at = get_token_at_position(x, y);

    if (token_at != NULL) {
        draw_token_at_position(token_at);
    }
}

void draw_token(int8_t x, int8_t y, bool player_val) {

    uint16_t colour, inner_colour;

    if (player_val == 0) {
        colour = SUPER_BLUE;
        inner_colour = 0x10AF;
    } else {
        colour = SUPER_RED;
        inner_colour = 0x78C2;
    }
    rectangle rectf_fill = {
        x * BOARD_SQUARE_SIZE+7+40,
        x * BOARD_SQUARE_SIZE + BOARD_SQUARE_SIZE-7+40,
        y * BOARD_SQUARE_SIZE+7,
        y * BOARD_SQUARE_SIZE + BOARD_SQUARE_SIZE-7,
    };
    fill_rectangle(rectf_fill, colour);

    rectangle rectf_fill1 = {
        x * BOARD_SQUARE_SIZE+8+40,
        x * BOARD_SQUARE_SIZE + BOARD_SQUARE_SIZE-8+40,
        y * BOARD_SQUARE_SIZE+8,
        y * BOARD_SQUARE_SIZE + BOARD_SQUARE_SIZE-8,
    };
    fill_rectangle(rectf_fill1, inner_colour);

    rectangle rectf_fill2 = {
        x * BOARD_SQUARE_SIZE+10+40,
        x * BOARD_SQUARE_SIZE + BOARD_SQUARE_SIZE-10+40,
        y * BOARD_SQUARE_SIZE+10,
        y * BOARD_SQUARE_SIZE + BOARD_SQUARE_SIZE-10,
    };
    fill_rectangle(rectf_fill2, colour);
}

void draw_token_at_position(token *token) {
        draw_token(token->pos.x, token->pos.y, token->player);
}

/*
*	Functions for the game
*/

/*
*   When clicking the center button
*/
void perform_action() {
    token *token_at;

    if (selected_token == NULL) {
        token_at = get_token_at_position(current_cursor_position.x, current_cursor_position.y);
        if (token_at != NULL && token_at->player == turn) {
            previous_selected_position.x = current_cursor_position.x;
            previous_selected_position.y = current_cursor_position.y;

            token_at->pos.x = -1;
            token_at->pos.y = -1;
            selected_token = token_at;

        }
    } else {
        int8_t x_taken = -2, y_taken = -2;

        if (is_valid_move() && can_move()) {
            token_at = get_token_at_position(current_cursor_position.x, current_cursor_position.y);

            if (token_at != NULL && token_at->player != selected_token->player) {
                x_taken = token_at->pos.x;
                y_taken = token_at->pos.y;

                if((previous_selected_position.x < current_cursor_position.x)){
                    selected_token->pos.x = x_taken+1;
                }else{
                    selected_token->pos.x = current_cursor_position.x-1;
                }

                if(previous_selected_position.y < current_cursor_position.y){
                    selected_token->pos.y = current_cursor_position.y+1;
                }
                else{
                    selected_token->pos.y = y_taken-1;
                }
                token_at->pos.x = -2;
                token_at->pos.y = -2;
                turn = 1 - turn;
                
            }else{
                selected_token->pos.x = current_cursor_position.x;
                selected_token->pos.y = current_cursor_position.y;
            }
            turn = 1 - turn;

        } else {
            selected_token->pos.x = previous_selected_position.x;
            selected_token->pos.y = previous_selected_position.y;
        }

        draw_square(selected_token->pos.x, selected_token->pos.y);

        selected_token = NULL;
        previous_selected_position.x = -1;
        previous_selected_position.y = -1;
    }

    draw_square(current_cursor_position.x, current_cursor_position.y);
    
}

token *get_token_at_position(int8_t x, int8_t y) {
    
    int8_t i;

    for (i = 0; i < 24; i++) {
        if (tokens[i].pos.x == x && tokens[i].pos.y == y) {
            return & tokens[i];
        }
    }
    return NULL;
}

bool is_valid_move() {
    if (selected_token->pos.x == -1 && selected_token->pos.y == -1) {
     
        if (abs(previous_selected_position.x - current_cursor_position.x) <= 1) {
            if (selected_token->player == 0
                && (abs(previous_selected_position.y - current_cursor_position.y) 
                && abs(previous_selected_position.x - current_cursor_position.x) == 1)) {
                return true;
            } else if (selected_token->player == 1
                    && (abs(previous_selected_position.x - current_cursor_position.x)
                    && abs(previous_selected_position.x - current_cursor_position.x) == 1)) {
                return true;
            }
        }
    }
    return false;
}

bool can_move() {
    token *token_at;
    int8_t new_x, new_y;
    int8_t x = current_cursor_position.x, y = current_cursor_position.y;

    token_at = get_token_at_position(x, y);
    if(token_at == NULL && x >= 0 && x < 8 && y >=0 && y < 8){
        return true;
    }
    if (token_at->player == selected_token->player) {
        return false;
    }
    if (token_at->player != selected_token->player) {

        if((previous_selected_position.x < x)){
            new_x = x+1;
        }else{
            new_x = x-1;
        }

        if(previous_selected_position.y < y){
            new_y = y+1;
        }
        else{
            new_y = y-1;
        }

        if(new_y < 0 || new_y > 7 || new_x < 0 || new_x > 7 || get_token_at_position(new_x, new_y) != NULL){
            return false;
        }      
    }
    return true;
}


/*
*  Functions for handling the movement of a token
*/
void move_cursor(int8_t x, int8_t y) {
    bool changed = false;

    position previous_pos = {
            current_cursor_position.x,
            current_cursor_position.y
    };

    if (previous_pos.x + x >= 0 && previous_pos.x + x < 8) {
        current_cursor_position.x = previous_pos.x + x;
        changed = true;
    }

    if (previous_pos.y + y >= 0 && previous_pos.y + y < 8) {
        current_cursor_position.y = previous_pos.y + y;
        changed = true;
    }

    if (changed) {
        draw_square(previous_pos.x, previous_pos.y);
        draw_square(current_cursor_position.x, current_cursor_position.y);
    }
}


int check_switches(int state) {

    if (get_switch_press(_BV(SWN))) {
        move_cursor(0, -1);
    }

    if (get_switch_press(_BV(SWE))) {
        move_cursor(1, 0);
    }

    if (get_switch_press(_BV(SWS))) {
        move_cursor(0, 1);
    }

    if (get_switch_press(_BV(SWW))) {
        move_cursor(-1, 0); 
    }

    if (get_switch_press(_BV(SWC))) {
        perform_action();
    }
    return state;
}
