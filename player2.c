#include "common.h"

static GameState *game;

void signal_handler(int sig) {
    (void)sig;
    game->game_running = 0;
}

void draw_game(void) {
    // Clear screen and move to top
    printf("\033[H");

    // Draw score
    printf("Player 1: %d | Player 2: %d", game->left_score, game->right_score);
    printf("\033[K\n");

    // Draw game area (player 2 sees columns 32-79)
    // We draw from local x=0 to VIEW_WIDTH, but map to global 32-79
    for (int y = 0; y < FULL_HEIGHT; y++) {
        for (int local_x = 0; local_x < VIEW_WIDTH; local_x++) {
            int global_x = PLAYER2_VIEW_START + local_x;
            char c = ' ';

            // Draw borders
            if (y == 0 || y == FULL_HEIGHT - 1) {
                c = '-';
            } else if (local_x == 0) {
                c = '|';
            } else if (local_x == VIEW_WIDTH - 1) {
                c = '|';
            }

            // Draw right paddle
            if (global_x == PADDLE_COL_RIGHT &&
                y >= game->right_paddle_y &&
                y < game->right_paddle_y + PADDLE_HEIGHT) {
                c = '#';
            }

            // Draw ball
            int ball_x = (int)game->ball_x;
            int ball_y = (int)game->ball_y;
            if (global_x == ball_x && y == ball_y &&
                ball_x >= PLAYER2_VIEW_START && ball_x <= PLAYER2_VIEW_END) {
                c = BALL_CHAR;
            }

            putchar(c);
        }
        printf("\033[K\n");
    }

    printf("Controls: Up/Down arrows | 'q' to quit\033[K");
    fflush(stdout);
}

int main(void) {
    // Initialize shared memory (connect to existing)
    game = init_shared_memory(0);

    // Mark player 2 as ready
    game->player2_ready = 1;

    // Set up signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Set up terminal
    enable_raw_mode();
    set_nonblocking();
    clear_screen();

    // Main game loop
    while (game->game_running) {
        // Read keyboard input
        char key = read_key();
        if (key == 'q') {
            game->game_running = 0;
            break;
        }
        if (key == 'U') {
            if (game->right_paddle_y > 1) {
                game->right_paddle_y--;
            }
        }
        if (key == 'D') {
            if (game->right_paddle_y < FULL_HEIGHT - PADDLE_HEIGHT - 1) {
                game->right_paddle_y++;
            }
        }

        // Draw the game
        draw_game();

        // Sleep for ~60fps
        usleep(16000);
    }

    // Cleanup
    clear_screen();
    move_cursor(0, 0);
    printf("Game Over!\n");
    printf("Final Score - Player 1: %d | Player 2: %d\n",
           game->left_score, game->right_score);

    munmap(game, sizeof(GameState));

    return 0;
}
