#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <termios.h>
#include <time.h>
#include <signal.h>

// Game dimensions
#define FULL_WIDTH 133
#define FULL_HEIGHT 24
#define VIEW_WIDTH 48        // 3/5 of 80 = 48
#define PADDLE_HEIGHT 5
#define PADDLE_COL_LEFT 2    // Left paddle x position
#define PADDLE_COL_RIGHT 77  // Right paddle x position (FULL_WIDTH - 3)

// View offsets
#define PLAYER1_VIEW_START 0
#define PLAYER1_VIEW_END 47
#define PLAYER2_VIEW_START 32
#define PLAYER2_VIEW_END 79

// Ball settings
#define BALL_CHAR '@'
#define BALL_SPEED 0.5f

// Shared memory
#define SHARED_FILE "/dev/mem"
#define MMAP_OFFSET 0x200000000UL

// Game state structure
typedef struct {
    // Ball state
    float ball_x, ball_y;
    float ball_vx, ball_vy;

    // Paddle positions (y coordinate of top of paddle)
    int left_paddle_y;
    int right_paddle_y;

    // Game state
    int game_running;
    int left_score;
    int right_score;

    // Synchronization
    int player1_ready;
    int player2_ready;
} GameState;

// Terminal state for restoration
static struct termios orig_termios;
static int terminal_modified = 0;

// Cleanup function
static void disable_raw_mode(void) {
    if (terminal_modified) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
        terminal_modified = 0;
    }
    // Show cursor
    printf("\033[?25h");
    fflush(stdout);
}

// Enable raw mode for keyboard input
static void enable_raw_mode(void) {
    tcgetattr(STDIN_FILENO, &orig_termios);
    terminal_modified = 1;
    atexit(disable_raw_mode);

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    // Hide cursor
    printf("\033[?25l");
    fflush(stdout);
}

// Set stdin to non-blocking
static void set_nonblocking(void) {
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}

// ANSI escape codes for drawing
static void clear_screen(void) {
    printf("\033[2J");
    fflush(stdout);
}

static void move_cursor(int row, int col) {
    printf("\033[%d;%dH", row + 1, col + 1);
}

// Initialize shared memory
static GameState* init_shared_memory(int is_player1) {
    int fd;
    GameState *state;

    fd = open(SHARED_FILE, O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open /dev/mem (run as root?)");
        exit(1);
    }

    state = mmap(NULL, sizeof(GameState), PROT_READ | PROT_WRITE, MAP_SHARED, fd, MMAP_OFFSET);
    if (state == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    close(fd);

    if (is_player1) {
        // Initialize game state
        memset(state, 0, sizeof(GameState));
        state->ball_x = FULL_WIDTH / 2.0f;
        state->ball_y = FULL_HEIGHT / 2.0f;

        // Random initial direction
        srand(time(NULL));
        state->ball_vx = (rand() % 2 == 0) ? BALL_SPEED : -BALL_SPEED;
        state->ball_vy = ((rand() % 100) / 50.0f - 1.0f) * BALL_SPEED;

        state->left_paddle_y = (FULL_HEIGHT - PADDLE_HEIGHT) / 2;
        state->right_paddle_y = (FULL_HEIGHT - PADDLE_HEIGHT) / 2;
        state->game_running = 1;
        state->left_score = 0;
        state->right_score = 0;
        state->player1_ready = 1;
        state->player2_ready = 0;
    }

    return state;
}

// Read keyboard input (returns arrow key or 0)
// Returns: 'U' for up, 'D' for down, 'q' for quit, 0 for no input
static char read_key(void) {
    char c;
    if (read(STDIN_FILENO, &c, 1) != 1) {
        return 0;
    }

    if (c == 'q' || c == 'Q') {
        return 'q';
    }

    if (c == '\033') {
        char seq[2];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return 0;
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return 0;

        if (seq[0] == '[') {
            if (seq[1] == 'A') return 'U'; // Up arrow
            if (seq[1] == 'B') return 'D'; // Down arrow
        }
    }

    return 0;
}

#endif // COMMON_H
