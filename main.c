// export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./lib
// gcc -o main main.c -L./lib -I./include -lraylib -lm -lpthread -ldl -lrt -lX11 
// ./main

#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include "include/raylib.h"

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 480

#define BOARD_SIZE 8
#define SQUARE_SIZE 60

// Last positions that were moved
// Used to draw a different color on the board
int last_moves[2][2] = {{-1, -1}, {-1, -1}};

struct GameState {
	int turn; // 0 = white, 1 = black
	bool clicked_piece;
	int clicked_piece_pos[2];
};

bool handle_click(int board[][8], int* coordinate)
{
	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
		// Get the mouse position
		Vector2 mouse_pos = GetMousePosition();

		// Convert the mouse position to a position on the board
		coordinate[0] = (int) floor(mouse_pos.x / SQUARE_SIZE);
		coordinate[1] = (int) floor(mouse_pos.y / SQUARE_SIZE);

		return true;
	}

	return false;
}

/*
	0 = pawn
	1 = knight
	2 = bishop
	3 = rook
	4 = queen
	5 = king

	6 = pawn
	7 = knight
	8 = bishop
	9 = rook
	10 = queen
	11 = king
*/
int** find_possible_moves(int board[][8], int* piece_coordinate)
{
	int piece = board[piece_coordinate[1]][piece_coordinate[0]];
	int player = (piece < 6) ? 0 : 1;  // 0 for white, 1 for black
	int x = piece_coordinate[0];
	int y = piece_coordinate[1];
	int** possible_moves = NULL;

	switch (piece) {
		case 0:  // Pawn
			// Check if pawn is in initial position
			// If it is:
				// valid moves is either 1 ahead or 2 ahead
			// If it's not in initial position:
				// valid moves is either 1 ahead or two diagonals ahead if there's a piece there
			break;
		case 1:  // Knight

			break;
		case 2:  // Bishop

			break;
		case 3:  // Rook

			break;
		case 4:  // Queen

			break;
		case 5:  // King

			break;
		default:
			break;
	}

	return possible_moves;
}

bool is_move_valid(int board[][8], int* piece_coordinate, int* move_coordinate, struct GameState* game)
{
	int piece_x = piece_coordinate[0];
	int piece_y = piece_coordinate[1];
	int move_x = move_coordinate[0];
	int move_y = move_coordinate[1];
	int attacker = board[piece_y][piece_x];

	// Make sure the piece is on the board
	if (piece_x < 0 || piece_x > 7 || piece_y < 0 || piece_y > 7) return false;

	// Make sure the piece and the move coordinates are not the same
	if (piece_x == move_x && piece_y == move_y) return false;

	printf("Attacker: %d; Move:(%d, %d)->(%d, %d); Target: %d\n", attacker, piece_x, piece_y, move_x, move_y, board[move_y][move_x]);

	// TODO: check if requested move is in possible moves
	int** possible_moves = find_possible_moves(board, piece_coordinate);

	return true;
}

void move(int board[][8], int* piece_coordinate, int* move_coordinate)
{
	int piece = board[piece_coordinate[1]][piece_coordinate[0]];
	board[piece_coordinate[1]][piece_coordinate[0]] = -1;
	board[move_coordinate[1]][move_coordinate[0]] = piece;

	last_moves[0][0] = piece_coordinate[0];
	last_moves[0][1] = piece_coordinate[1];
	last_moves[1][0] = move_coordinate[0];
	last_moves[1][1] = move_coordinate[1];
}

bool is_click_valid(int board[][8], int* piece_coordinate, int* move_coordinate, struct GameState* game)
{
	int x = move_coordinate[0];
	int y = move_coordinate[1];
	int piece = board[y][x];

	if (game->clicked_piece == false) { // Selecting piece to move
		if (piece == -1) return false;

		if (game->turn == 0) { // White Turn
			// Tried to move black
			if (piece < 6 || piece > 11) return false;
		} else { // Black Turn
			// Tried to move white
			if (piece < 0 || piece > 5) return false;
		}

		piece_coordinate[0] = x;
		piece_coordinate[1] = y;
		game->clicked_piece_pos[0] = x;
		game->clicked_piece_pos[1] = y;
		game->clicked_piece = true;
	} else { // Selecting place to move piece
		if (game->turn == 0) { // White Turn
			// Tried to move white on white
			if (piece > 5) {
				game->clicked_piece = false;
				return false;
			}

			if (is_move_valid(board, piece_coordinate, move_coordinate, game)) {
				move(board, piece_coordinate, move_coordinate);
				game->turn = 1;
			} else {
				printf("Invalid move (%d, %d)\n", move_coordinate[0], move_coordinate[1]);
				return false;
			}
		} else { // Black Turn
			// Tried to move black on black
			if (piece >= 0 && piece < 6) {
				game->clicked_piece = false;
				return false;
			}

			if (is_move_valid(board, piece_coordinate, move_coordinate, game)) {
				move(board, piece_coordinate, move_coordinate);
				game->turn = 0;
			} else {
				printf("Invalid move (%d, %d)\n", move_coordinate[0], move_coordinate[1]);
				return false;
			}
		}

		game->clicked_piece = false;
	}

	return true;
}

void draw_board(Texture2D pieces[12], int board[8][8], struct GameState game)
{
	// Colors of each square
	Color light_color = (Color) {240,217,183, 255};
	Color dark_color = (Color) {180,135,103, 255};
	Color last_move_color = (Color) {42, 75, 130, 255};
	Color clicked_piece_color = (Color) {96, 136, 204, 255};

	for (int i = 0; i < BOARD_SIZE; ++i) {
		for (int j = 0; j < BOARD_SIZE; ++j) {
			// Calculate the position of the square
			int x = i * SQUARE_SIZE;
			int y = j * SQUARE_SIZE;

			// Set the color of the square
			if (((i == last_moves[0][0] && j == last_moves[0][1]) || (i == last_moves[1][0] && j == last_moves[1][1])) && last_moves[0][0] != -1) {
				DrawRectangle(x, y, SQUARE_SIZE, SQUARE_SIZE, last_move_color);
			} else if (game.clicked_piece && (i == game.clicked_piece_pos[0] && j == game.clicked_piece_pos[1])) {
				DrawRectangle(x, y, SQUARE_SIZE, SQUARE_SIZE, clicked_piece_color);
			} 
			else {
				if ((i + j) % 2 == 0) {
					DrawRectangle(x, y, SQUARE_SIZE, SQUARE_SIZE, dark_color);
				} else {
					DrawRectangle(x, y, SQUARE_SIZE, SQUARE_SIZE, light_color);
				}
			}

			// Draw the piece
			int piece = board[j][i];
			if (piece != -1) DrawTexture(pieces[piece], x, y, WHITE);
		}
	}
}

// TODO: check both kings to see if they have possible moves in case they are being attacked
bool is_checkmate(int board[][8])
{
	return false;
}

int main(int argc, char* argv[])
{
	// Initialize game constants
	struct GameState game = {0, false, {-1, -1}};

	// Create Window
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Chess");

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

	while (!WindowShouldClose()) {
		if (is_checkmate(board)) break;

		BeginDrawing();
		ClearBackground(RAYWHITE);

		// Draw the game
		draw_board(pieces, board, game);

		int move_coordinate[2];
		int piece_coordinate[2];
		if (handle_click(board, move_coordinate)) {
			if (!is_click_valid(board, piece_coordinate, move_coordinate, &game)) {
				printf("Invalid move (%d, %d)\n", move_coordinate[0], move_coordinate[1]);
			}
		}

		EndDrawing();
	}

	CloseWindow();

	return 0;
}