#include "common.h"

static GameState *game;

void signal_handler(int sig) {
    (void)sig;
    game->game_running = 0;
}

void reset_ball(int direction) {
    game->ball_x = FULL_WIDTH / 2.0f;
    game->ball_y = FULL_HEIGHT / 2.0f;
    game->ball_vx = direction * BALL_SPEED;
    game->ball_vy = ((rand() % 100) / 50.0f - 1.0f) * BALL_SPEED * 0.5f;
}

void update_ball(void) {
    // Update position
    game->ball_x += game->ball_vx;
    game->ball_y += game->ball_vy;

    // Bounce off top/bottom walls
    if (game->ball_y < 1) {
        game->ball_y = 1;
        game->ball_vy = -game->ball_vy;
    }
    if (game->ball_y >= FULL_HEIGHT - 1) {
        game->ball_y = FULL_HEIGHT - 2;
        game->ball_vy = -game->ball_vy;
    }

    // Check left paddle collision
    if (game->ball_x <= PADDLE_COL_LEFT + 1 && game->ball_x >= PADDLE_COL_LEFT) {
        int ball_int_y = (int)game->ball_y;
        if (ball_int_y >= game->left_paddle_y &&
            ball_int_y < game->left_paddle_y + PADDLE_HEIGHT) {
            game->ball_x = PADDLE_COL_LEFT + 1;
            game->ball_vx = -game->ball_vx;
            // Add spin based on where ball hits paddle
            float hit_pos = (game->ball_y - game->left_paddle_y) / PADDLE_HEIGHT;
            game->ball_vy += (hit_pos - 0.5f) * 0.5f;
        }
    }

    // Check right paddle collision
    if (game->ball_x >= PADDLE_COL_RIGHT - 1 && game->ball_x <= PADDLE_COL_RIGHT) {
        int ball_int_y = (int)game->ball_y;
        if (ball_int_y >= game->right_paddle_y &&
            ball_int_y < game->right_paddle_y + PADDLE_HEIGHT) {
            game->ball_x = PADDLE_COL_RIGHT - 1;
            game->ball_vx = -game->ball_vx;
            // Add spin based on where ball hits paddle
            float hit_pos = (game->ball_y - game->right_paddle_y) / PADDLE_HEIGHT;
            game->ball_vy += (hit_pos - 0.5f) * 0.5f;
        }
    }

    // Limit vertical speed
    if (game->ball_vy > BALL_SPEED * 1.5f) game->ball_vy = BALL_SPEED * 1.5f;
    if (game->ball_vy < -BALL_SPEED * 1.5f) game->ball_vy = -BALL_SPEED * 1.5f;

    // Scoring
    if (game->ball_x < 0) {
        game->right_score++;
        reset_ball(-1); // Ball goes toward left player
    }
    if (game->ball_x >= FULL_WIDTH) {
        game->left_score++;
        reset_ball(1); // Ball goes toward right player
    }
}

void draw_game(void) {
    // Clear screen and move to top
    printf("\033[H");

    // Draw score
    printf("Player 1: %d | Player 2: %d", game->left_score, game->right_score);
    if (!game->player2_ready) {
        printf("   [Waiting for Player 2...]");
    }
    printf("\033[K\n");

    // Draw game area (player 1 sees columns 0-47)
    for (int y = 0; y < FULL_HEIGHT; y++) {
        for (int x = PLAYER1_VIEW_START; x <= PLAYER1_VIEW_END; x++) {
            char c = ' ';

            // Draw borders
            if (y == 0 || y == FULL_HEIGHT - 1) {
                c = '-';
            } else if (x == PLAYER1_VIEW_START) {
                c = '|';
            } else if (x == PLAYER1_VIEW_END) {
                c = '|';
            }

            // Draw left paddle
            if (x == PADDLE_COL_LEFT &&
                y >= game->left_paddle_y &&
                y < game->left_paddle_y + PADDLE_HEIGHT) {
                c = '#';
            }

            // Draw ball
            int ball_x = (int)game->ball_x;
            int ball_y = (int)game->ball_y;
            if (x == ball_x && y == ball_y &&
                ball_x >= PLAYER1_VIEW_START && ball_x <= PLAYER1_VIEW_END) {
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
    // Initialize shared memory
    game = init_shared_memory(1);

    // Set up signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Set up terminal
    enable_raw_mode();
    set_nonblocking();
    clear_screen();

    printf("Waiting for Player 2 to connect...\n");
    printf("(Player 2 should run ./player2 in another terminal)\n");
    fflush(stdout);

    // Main game loop
    while (game->game_running) {
        // Read keyboard input
        char key = read_key();
        if (key == 'q') {
            game->game_running = 0;
            break;
        }
        if (key == 'U') {
            if (game->left_paddle_y > 1) {
                game->left_paddle_y--;
            }
        }
        if (key == 'D') {
            if (game->left_paddle_y < FULL_HEIGHT - PADDLE_HEIGHT - 1) {
                game->left_paddle_y++;
            }
        }

        // Update ball physics (player 1 is authoritative)
        if (game->player2_ready) {
            update_ball();
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
