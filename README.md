# 2-Player Pong Game

A terminal-based 2-player Pong game where each player runs a separate program, communicating via shared memory mapped to `/dev/mem`.

## Requirements

- RISC-V Linux system
- Root privileges (for `/dev/mem` access)
- RISC-V cross-compiler for building: `riscv64-linux-gnu-gcc`

## Building

```bash
make
```

## Running

Run each player in a separate terminal:

**Terminal 1 (Player 1 - Left side):**
```bash
sudo ./player1
```

**Terminal 2 (Player 2 - Right side):**
```bash
sudo ./player2
```

Player 1 must start first. Player 2 connects to the shared memory created by Player 1.

## Controls

| Key | Action |
|-----|--------|
| Up Arrow | Move paddle up |
| Down Arrow | Move paddle down |
| q | Quit game |

## Game Details

- Each player sees 3/5 of the playing field (48 of 80 columns)
- Player 1 sees the left portion, Player 2 sees the right portion
- Ball is displayed as `@`
- Paddles are displayed as `#`
- Ball bounces off walls and paddles
- Score resets ball to center

## Shared Memory

- File: `/dev/mem`
- Offset: `0x200000000`
- Size: ~56 bytes (GameState structure)

## Files

| File | Description |
|------|-------------|
| `common.h` | Shared definitions and game state structure |
| `player1.c` | Left player program (handles ball physics) |
| `player2.c` | Right player program |
| `Makefile` | Build configuration |
