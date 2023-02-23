// export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./lib
// gcc -o main main.c -L./lib -I./include -lraylib -lm -lpthread -ldl -lrt -lX11 
// ./main

/* TODO:
 * En passant of pawns
 * Checkmate
 * Check mechanic:
 *	1. If a king is in check, movement should be defined based on the check position
 *	2. The king can't move to a position that would get itself into check
 *	3. Pieces can't be moved to a position that would make their king get into check
 */

#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include "include/raylib.h"

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 480

#define BOARD_SIZE 8
#define SQUARE_SIZE 60

// Last positions that were moved
// Used to draw a different color on the board
int last_moves[2][2] = {{-1, -1}, {-1, -1}};

// Load audio files
Sound capture_sound;
Sound move_sound;

struct GameState {
	int turn; 					// 0 = white, 1 = black
	bool clicked_piece; 		// rather a piece has been selected or not
	int clicked_piece_pos[2]; 	// position of the selected piece
	int num_moves;
	int** possible_moves;		// possible moves to draw on screen
	bool is_in_check;
	int attacker_position[2]; 	// Piece attacking the king making the check
};

// This function takes an array to store the coordinates of the clicked square
bool handle_click(int* coordinate)
{
	// Check if the left mouse button has been pressed
	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
		// Get the mouse position
		Vector2 mouse_pos = GetMousePosition();

		// Convert the position of the mouse to the corresponding position on the chess board
		coordinate[0] = (int) floor(mouse_pos.x / SQUARE_SIZE);
		coordinate[1] = (int) floor(mouse_pos.y / SQUARE_SIZE);

		// Return true to indicate that a click has been handled and coordinates have been updated
		return true;
	}

	// If the mouse button has not been clicked, return false to indicate that no action has been taken
	return false;
}

int** add_move(int** possible_moves, int* num_moves, int x, int y)
{
	// Increment the count of the number of possible moves
	++(*num_moves);

	// Resize the `possible_moves` array to make space for the new move
	int** temp = (int**) realloc(possible_moves, (*num_moves) * sizeof(int*));

	// If the reallocation was successful, update the `possible_moves` pointer
	if (temp) {
		possible_moves = temp;

		// Allocate space for the new move and add it to the end of the list
		possible_moves[(*num_moves) - 1] = (int*) malloc(2 * sizeof(int));
		possible_moves[(*num_moves) - 1][0] = x;
		possible_moves[(*num_moves) - 1][1] = y;
	}

	// Return a pointer to the updated `possible_moves` array
	return possible_moves;
}

// TODO: en passant
int** find_pawn_moves(int board[][8], int** possible_moves, int* num_moves, int piece, int player, int x, int y)
{
	// Black pawns move in the +y direction, white pawns move in the -y direction
	int direction = player == 0 ? 1 : -1;

	// Normal move
	if (board[y+direction][x] == -1) {
		possible_moves = add_move(possible_moves, num_moves, x, y+direction);
	}

	// Start position
	if ((y == 1 && player == 0) || (y == 6 && player == 1)) {
		if (board[y+2*direction][x] == -1 && board[y+direction][x] == -1) {
			possible_moves = add_move(possible_moves, num_moves, x, y+2*direction);
		}
	}

	// Attack move
	if (x > 0 && board[y+direction][x-1] >= (player == 0 ? 6 : 0) && board[y+direction][x-1] < (player == 0 ? 12 : 6)) {
		possible_moves = add_move(possible_moves, num_moves, x-1, y+direction);
	}

	if (x < 7 && board[y+direction][x+1] >= (player == 0 ? 6 : 0) && board[y+direction][x+1] < (player == 0 ? 12 : 6)) {
		possible_moves = add_move(possible_moves, num_moves, x+1, y+direction);
	}

	return possible_moves;
}

int** find_knight_moves(int board[][8], int** possible_moves, int* num_moves, int piece, int player, int x, int y)
{
	// Possible L-shaped moves
	// xd and dy represent the x and y offsets of each 
	// of the eight possible L-shaped moves
	int dx[8] = { 1, 2, 2, 1, -1, -2, -2, -1 };
	int dy[8] = { 2, 1, -1, -2, -2, -1, 1, 2 };

	// Check each possible L-shaped move
	for (int i = 0; i < 8; i++) {
		int cx = x + dx[i];
		int cy = y + dy[i];

		if (cx >= 0 && cx < 8 && cy >= 0 && cy < 8) {
			int target = board[cy][cx];

			// If the target square is empty or has an opponent's piece, add it to the list of possible moves
			if (target == -1 || (player == 0 && target > 5 && target <= 11) || (player == 1 && target >= 0 && target < 6)) {
				possible_moves = add_move(possible_moves, num_moves, cx, cy);
			}
		}
	}

	return possible_moves;
}

int** find_bishop_moves(int board[][8], int** possible_moves, int* num_moves, int piece, int player, int x, int y)
{
	for (int dirx = -1; dirx <= 1; dirx += 2) {
		for (int diry = -1; diry <= 1; diry += 2) {
			if (dirx == 0 && diry == 0) continue;

			int cx = x + dirx;
			int cy = y + diry;
			while (cx >= 0 && cx < 8 && cy >= 0 && cy < 8) {
				int target = board[cy][cx];

				if (target == -1) {
					possible_moves = add_move(possible_moves, num_moves, cx, cy);
				} else if ((player == 0 && target > 5 && target <= 11) || (player == 1 && target >= 0 && target < 6)) {
					possible_moves = add_move(possible_moves, num_moves, cx, cy);
					break;
				} else {
					break;
				}

				cx += dirx;
				cy += diry;
			}
		}
	}
	return possible_moves;
}

int** find_rook_moves(int board[][8], int** possible_moves, int* num_moves, int piece, int player, int x, int y)
{
	for (int dirx = -1; dirx <= 1; ++dirx) {
		for (int diry = -1; diry <= 1; ++diry) {
			if (dirx == 0 && diry == 0) continue;
			if (dirx % 2 != 0 && diry % 2 != 0) continue;

			int cx = x + dirx;
			int cy = y + diry;
			while (cx >= 0 && cx < 8 && cy >= 0 && cy < 8) {
				int target = board[cy][cx];

				if (target == -1) {
					possible_moves = add_move(possible_moves, num_moves, cx, cy);
				} else if ((player == 0 && target > 5 && target <= 11) || (player == 1 && target >= 0 && target < 6)) {
					possible_moves = add_move(possible_moves, num_moves, cx, cy);
					break;
				} else {
					break;
				}

				cx += dirx;
				cy += diry;
			}
		}
	}

	return possible_moves;
}

int** find_queen_moves(int board[][8], int** possible_moves, int* num_moves, int piece, int player, int x, int y)
{
	for (int dirx = -1; dirx <= 1; ++dirx) {
		for (int diry = -1; diry <= 1; ++diry) {
			if (dirx == 0 && diry == 0) continue;

			int cx = x + dirx;
			int cy = y + diry;
			while (cx >= 0 && cx < 8 && cy >= 0 && cy < 8) {
				int target = board[cy][cx];

				if (target == -1) {
					possible_moves = add_move(possible_moves, num_moves, cx, cy);
				} else if ((player == 0 && target > 5 && target <= 11) || (player == 1 && target >= 0 && target < 6)) {
					possible_moves = add_move(possible_moves, num_moves, cx, cy);
					break;
				} else {
					break;
				}

				cx += dirx;
				cy += diry;
			}
		}
	}

	return possible_moves;
}

// TODO: incomplete
int** find_king_moves(int board[][8], int** possible_moves, int* num_moves, int piece, int player, int x, int y)
{
	for (int dx = -1; dx <= 1; ++dx) {
		for (int dy = -1; dy <= 1; ++dy) {
			if (dx == 0 && dy == 0) continue;

			int cx = x + dx;
			int cy = y + dy;
			if (cx >= 0 && cx < 8 && cy >= 0 && cy < 8) {
				int target = board[cy][cx];
				if (target == -1 || ((player == 0 && target > 5 && target <= 11) || (player == 1 && target >= 0 && target < 6))) {
					possible_moves = add_move(possible_moves, num_moves, cx, cy);
				}
			}
		}
	}

	return possible_moves;
}

int** find_possible_moves(int board[][8], int** possible_moves, int* piece_coordinate, int* num_moves, struct GameState* game)
{
	int piece = board[piece_coordinate[1]][piece_coordinate[0]];
	int player = (piece < 6) ? 0 : 1;  // 0 for black, 1 for white
	if (player == 1) piece -= 6;

	int x = piece_coordinate[0];
	int y = piece_coordinate[1];

	switch (piece) {
		case 0:  // Pawn
			possible_moves = find_pawn_moves(board, possible_moves, num_moves, piece, player, x, y);
			break;
		case 1:  // Knight
			possible_moves = find_knight_moves(board, possible_moves, num_moves, piece, player, x, y);
			break;
		case 2:  // Bishop
			possible_moves = find_bishop_moves(board, possible_moves, num_moves, piece, player, x, y);
			break;
		case 3:  // Rook
			possible_moves = find_rook_moves(board, possible_moves, num_moves, piece, player, x, y);
			break;
		case 4:  // Queen
			possible_moves = find_queen_moves(board, possible_moves, num_moves, piece, player, x, y);
			break;
		case 5:  // King
			possible_moves = find_king_moves(board, possible_moves, num_moves, piece, player, x, y);
			break;
		default:
			break;
	}

	return possible_moves;
}

// Free the moves
void free_moves(int** moves, int num_moves)
{
	for (int i = 0; i < num_moves; ++i) {
		free(moves[i]);
	}

	free(moves);
}

bool is_move_in_array_of_moves(int** moves, int num_moves, int x, int y)
{
	for (int i = 0; i < num_moves; ++i) {
		if ((moves[i][0] == x) && (moves[i][1] == y)) return true;
	}

	return false;
}

// Returns in game->attacker_position the position of the attacker
bool is_position_being_attacked(int board[][8], int x, int y, struct GameState* game) 
{
	int victim = (board[y][x] < 6) ? 0 : 1; // 0 = black, 1 = white

	for (int i = 0; i < 8; ++i) {
		for (int j = 0; j < 8; ++j) {
			if (board[j][i] != -1) {
				int piece = board[j][i];
				if (victim == 0 && piece >= 6 && piece <= 11) { // Black being attacked by white
					int num_moves = 0;
					int** possible_moves = NULL;
					int position[2] = {i, j};
					possible_moves = find_possible_moves(board, possible_moves, position, &num_moves, game);

					if (is_move_in_array_of_moves(possible_moves, num_moves, x, y)) {
						game->attacker_position[0] = i;
						game->attacker_position[1] = j;
						return true;
					}

					free_moves(possible_moves, num_moves);
				} else if (victim == 1 && piece >= 0 && piece <= 5) { // White being attacked by black
					int num_moves = 0;
					int** possible_moves = NULL;
					int position[2] = {i, j};
					possible_moves = find_possible_moves(board, possible_moves, position, &num_moves, game);

					if (is_move_in_array_of_moves(possible_moves, num_moves, x, y)) {
						game->attacker_position[0] = i;
						game->attacker_position[1] = j;
						return true;
					}

					free_moves(possible_moves, num_moves);
				}
			}
		}
	}

	return false;
}

bool is_king_in_check(int board[][8], struct GameState* game)
{
	for (int i = 0; i < 8; ++i) {
		for (int j = 0; j < 8; ++j) {
			int piece = board[j][i];
			if (piece == 5) { // Black King
				if (is_position_being_attacked(board, i, j, game)) {
					return true;
				}
			} else if (piece == 11) { // White King
				if (is_position_being_attacked(board, i, j, game)) return true;
			}
		}
	}

	return false;
}

/*
 * Check if a move is valid for a given piece on the board.
 * 
 * 'board' The chess board.
 * 'piece_coordinate' The coordinate of the piece to move.
 * 'move_coordinate' The coordinate to move the piece to.
 * 
 * returns true if the move is valid, false otherwise.
 */
bool is_move_valid(int board[][8], int* piece_coordinate, int* move_coordinate, struct GameState* game)
{
	// Get the piece and move coordinates
	int piece_x = piece_coordinate[0];
	int piece_y = piece_coordinate[1];
	int move_x = move_coordinate[0];
	int move_y = move_coordinate[1];

	// Make sure the piece is on the board
	if (piece_x < 0 || piece_x > 7 || piece_y < 0 || piece_y > 7) return false;

	// Make sure the piece and the move coordinates are not the same
	if (piece_x == move_x && piece_y == move_y) return false;

	// Find all possible moves for the piece
	int num_moves = game->num_moves;
	int** possible_moves = game->possible_moves;

	// Check if the move is one of the possible moves
	if (possible_moves != NULL) {
		for (int i = 0; i < num_moves; ++i) {
			if (possible_moves[i] != NULL) {
				if ((possible_moves[i][0] == move_x) && (possible_moves[i][1] == move_y)) {
					// The move is valid
					// free the memory used by 'possible_moves' and return true
					free_moves(possible_moves, num_moves);
					return true;
				}
			}
		}
	}

	// The move is not one of the possible moves
	// free the memory used by 'possible_moves' and return false
	free_moves(possible_moves, num_moves);
	return false;
}

/*
 * Makes a move on the board.
 *
 * 'board' The chess board.
 * 'piece_coordinate' The coordinate of the piece to move.
 * 'move_coordinate' The coordinate to move the piece to.
 *
 */
void move(int board[][8], int* piece_coordinate, int* move_coordinate)
{
	// Retrieve what the moved piece is
	int piece = board[piece_coordinate[1]][piece_coordinate[0]];

	// Changes piece's position to empty
	board[piece_coordinate[1]][piece_coordinate[0]] = -1;

	// Play sound depending if it's a move or a capture
	if (board[move_coordinate[1]][move_coordinate[0]] != -1) {
		PlaySound(capture_sound);
	} else {
		PlaySound(move_sound);
	}

	if (piece == 0 && move_coordinate[1] == 7) {
		// If black piece moves to end of board turns to queen
		board[move_coordinate[1]][move_coordinate[0]] = 4;
	} else if (piece == 6 && move_coordinate[1] == 0) {
		// If white piece moves to end of board turns to queen
		board[move_coordinate[1]][move_coordinate[0]] = 10;
	} else {
		// If it's a normal move/capture, just overwrites position to piece value
		board[move_coordinate[1]][move_coordinate[0]] = piece;
	}

	// Updates 'last_moves' positions to for the draw function
	last_moves[0][0] = piece_coordinate[0];
	last_moves[0][1] = piece_coordinate[1];
	last_moves[1][0] = move_coordinate[0];
	last_moves[1][1] = move_coordinate[1];
}

/*
 * Checks if the click on the screen is a valid click
 *
 * 'board' The chess board.
 * 'piece_coordinate' The coordinate of the piece to move.
 * 'move_coordinate' The coordinate to move the piece to.
 * 'game' The current state of the game.
 *
 */
bool is_click_valid(int board[][8], int* piece_coordinate, int* move_coordinate, struct GameState* game)
{
	// Get move position
	int x = move_coordinate[0];
	int y = move_coordinate[1];
	// Get the piece from the move position
	int piece = board[y][x];

	if (game->clicked_piece == false) { // Selecting piece to move
		// If clicked on empty space returns false
		if (piece == -1) return false;

		if (game->turn == 0) { // White Turn
			// Tried to move black
			if (piece < 6 || piece > 11) return false;
		} else { // Black Turn
			// Tried to move white
			if (piece < 0 || piece > 5) return false;
		}

		// Updates 'piece_coordinate' for the selected piece to move
		piece_coordinate[0] = x;
		piece_coordinate[1] = y;

		// Updates game->clicked_piece_pos and game->clicked_piece for the draw function
		game->clicked_piece_pos[0] = x;
		game->clicked_piece_pos[1] = y;
		game->clicked_piece = true;

		// Update game->possible_moves of the clicked piece
		int num_moves = 0;
		int** possible_moves = NULL;
		possible_moves = find_possible_moves(board, possible_moves, piece_coordinate, &num_moves, game);
		game->num_moves = num_moves;
		game->possible_moves = possible_moves;
	} else { // Selecting place to move piece
		// Deselects the selected piece
		game->clicked_piece = false;
		if (game->turn == 0) { // White Turn
			// Tried to move white on white
			if (piece > 5) return false;

			// If the move position is valid, calls move, otherwise returns false
			if (is_move_valid(board, piece_coordinate, move_coordinate, game)) {
				move(board, piece_coordinate, move_coordinate);
				game->turn = 1;
			} else {
				return false;
			}
		} else { // Black Turn
			// Tried to move black on black
			if (piece >= 0 && piece < 6) return false;

			// If the move position is valid, calls move, otherwise returns false
			if (is_move_valid(board, piece_coordinate, move_coordinate, game)) {
				move(board, piece_coordinate, move_coordinate);
				game->turn = 0;
			} else {
				return false;
			}
		}

		// After a move is done, checks if the king is in check and updates game->is_in_check
		if (is_king_in_check(board, game)) {
			game->is_in_check = true;
			printf("King is in check by (%d, %d)\n", game->attacker_position[0], game->attacker_position[1]);
		} else {
			game->is_in_check = false;
		}
	}

	// Return true after the move was finished
	return true;
}

void draw_board(Texture2D pieces[12], int board[8][8], struct GameState game)
{
	// Colors of each square
	Color light_color = (Color) {240,217,183, 255};
	Color dark_color = (Color) {180,135,103, 255};
	Color last_move_color = (Color) {42, 75, 130, 255};
	Color clicked_piece_color = (Color) {96, 136, 204, 255};

	// Iterates through the board
	for (int i = 0; i < BOARD_SIZE; ++i) {
		for (int j = 0; j < BOARD_SIZE; ++j) {
			// Calculate the position of the square
			int x = i * SQUARE_SIZE;
			int y = j * SQUARE_SIZE;

			// Sets the color of each square based on position
			if (((i == last_moves[0][0] && j == last_moves[0][1]) || (i == last_moves[1][0] && j == last_moves[1][1])) && last_moves[0][0] != -1) {
				// Paints the last move position
				DrawRectangle(x, y, SQUARE_SIZE, SQUARE_SIZE, last_move_color);
			} else if (game.clicked_piece && (i == game.clicked_piece_pos[0] && j == game.clicked_piece_pos[1])) {
				// Paints the clicked piece position
				DrawRectangle(x, y, SQUARE_SIZE, SQUARE_SIZE, clicked_piece_color);
			} 
			else {
				// Paints the board
				if ((i + j) % 2 == 0) {
					DrawRectangle(x, y, SQUARE_SIZE, SQUARE_SIZE, dark_color);
				} else {
					DrawRectangle(x, y, SQUARE_SIZE, SQUARE_SIZE, light_color);
				}
			}

			// Draw the pieces of each position
			int piece = board[j][i];
			if (piece != -1) DrawTexture(pieces[piece], x, y, WHITE);

			// Draws all the position from the selected piece
			if (game.clicked_piece == true) {
				if (game.possible_moves != NULL && game.num_moves > 0) {
					for (int k = 0; k < game.num_moves; ++k) {
						int move_pos_x = game.possible_moves[k][0];
						int move_pos_y = game.possible_moves[k][1];

						if (i == move_pos_x && j == move_pos_y) {
							if (board[j][i] != -1) {
								DrawRectangleLinesEx((Rectangle){x, y, SQUARE_SIZE, SQUARE_SIZE}, 3, RED);
							} else {
								Vector2 center = { (float)(x + SQUARE_SIZE/2), (float)(y + SQUARE_SIZE/2) };
								DrawRing(center, 8.0, 0.0, 0, 360, 0, RED);							
							}
						}
					}
				}
			}
		}
	}
}

int main(int argc, char* argv[])
{
	// Initialize game constants
	struct GameState game = {0, false, {-1, -1}};

	// Create Window and init sounds
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Chess");
	InitAudioDevice();

	// Load sounds
	capture_sound = LoadSound("sounds/capture.mp3");
	move_sound = LoadSound("sounds/move.mp3");

	// Load texture of each piece
	Texture2D pieces[12];
	pieces[0] = LoadTexture("images/pawn_b.png");
	pieces[1] = LoadTexture("images/knight_b.png");
	pieces[2] = LoadTexture("images/bishop_b.png");
	pieces[3] = LoadTexture("images/rook_b.png");
	pieces[4] = LoadTexture("images/queen_b.png");
	pieces[5] = LoadTexture("images/king_b.png");
	pieces[6] = LoadTexture("images/pawn_w.png");
	pieces[7] = LoadTexture("images/knight_w.png");
	pieces[8] = LoadTexture("images/bishop_w.png");
	pieces[9] = LoadTexture("images/rook_w.png");
	pieces[10] = LoadTexture("images/queen_w.png");
	pieces[11] = LoadTexture("images/king_w.png");

	// Start position of the pieces on the chess board
	int board[8][8] = {
		{3, 1, 2, 4, 5, 2, 1, 3},
		{0, 0, 0, 0, 0, 0, 0, 0},
		{-1, -1, -1, -1, -1, -1, -1, -1},
		{-1, -1, -1, -1, -1, -1, -1, -1},
		{-1, -1, -1, -1, -1, -1, -1, -1},
		{-1, -1, -1, -1, -1, -1, -1, -1},
		{6, 6, 6, 6, 6, 6, 6, 6},
		{9, 7, 8, 10, 11, 8, 7, 9}
	};

	// Game main loop
	while (!WindowShouldClose()) {
		// TODO: If it's a checkmate game must end

		// Calls raylib's draw function and paints background white
		BeginDrawing();
		ClearBackground(RAYWHITE);

		// Draw the board
		draw_board(pieces, board, game);

		// Coordinates for moves and selected pieces
		int move_coordinate[2];
		int piece_coordinate[2];

		// If the player clicks on the screen, puts the position at 'move_coordinate'
		if (handle_click(move_coordinate)) {
			// Checks if the click on the screen is a valid click
			// If it's valid, either select the piece or do the desired move
			if (!is_click_valid(board, piece_coordinate, move_coordinate, &game)) {
				printf("Invalid move (%d, %d)\n", move_coordinate[0], move_coordinate[1]);
			}
		}

		// Calls raylib's function to finish drawing
		EndDrawing();
	}

	// Clean up resources
	UnloadSound(capture_sound);
	UnloadSound(move_sound);
	CloseAudioDevice();
	CloseWindow();

	return 0;
}