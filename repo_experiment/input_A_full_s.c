// ==================== DATEI: src/utils.h ====================
// vim: noexpandtab:ts=4:sts=4:sw=4

#ifndef UTILS_H
#define UTILS_H

#include "gettext.h"		/* gettext */
#include <stdbool.h>		/* bool data type */

#define _(x) gettext(x)

typedef unsigned char PAPER_UNIT_CODE;

typedef struct {
	bool valid;
	PAPER_UNIT_CODE unit;
	int width;
	int height;
} PAPER_SIZE;

#define PUC_INVALID ((PAPER_UNIT_CODE) -1)
#define PUC_PT ((PAPER_UNIT_CODE) 0)
#define PUC_IN ((PAPER_UNIT_CODE) 1)
#define PUC_CM ((PAPER_UNIT_CODE) 2)

#define PS_INVALID ((PAPER_SIZE) { false, PUC_INVALID, -1, -1 })
#define PS_DEFAULT ((PAPER_SIZE) { true, PUC_CM, 2100, 2970 })

#define ABS(n) (((n) > 0) ? (n) : -(n))
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

PAPER_SIZE papersize_normalize(PAPER_SIZE size);
PAPER_SIZE papersize_get(char *name);

#endif // UTILS_H

// ==================== DATEI: src/utils.c ====================
// vim: noexpandtab:ts=4:sts=4:sw=4

#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
	bool valid;
	char *name;
	PAPER_UNIT_CODE code;
	double value;
} PAPER_UNIT_TYPE;

typedef struct {
	char *name;
	PAPER_SIZE size;
} PAPER_SIZE_TYPE;

#define UT_INVALID ((PAPER_UNIT_TYPE) { false, "", PUC_INVALID, (double) -1 })

static PAPER_UNIT_TYPE unit_types[] = {
	{ true, "cm", PUC_CM, (double) 28.3465 },
	{ true, "in", PUC_IN, (double) 72.0 },
	{ true, "pt", PUC_PT, (double) 1.0 }
};

// Size in units x100, to allow hundredths of the unit.
static PAPER_SIZE_TYPE size_types[] = {
	{ "a4", PS_DEFAULT },
	{ "a5", { true, PUC_CM, 1480, 2100 } },
	{ "letter", { true, PUC_IN, 850, 1100 } },
	{ "legal", { true, PUC_IN, 850, 1400 } },
	{ "oficio", { true, PUC_IN, 850, 1340 } },
	{ "invoice", { true, PUC_IN, 550, 850 } }
};

static double paperunit_getrate_bycode(PAPER_UNIT_CODE code) {
	double rate = -1;

	size_t n = ARRAY_SIZE(unit_types);
	for (size_t i = 0; i < n; i++) {
		PAPER_UNIT_TYPE ut = unit_types[i];

		if (ut.code == code) {
			rate = ut.value;
			break;
		}
	}

	return rate;
}

// Normalize paper size to points (pt).
PAPER_SIZE papersize_normalize(PAPER_SIZE size) {
	if (!size.valid)
		return size;
	
	PAPER_SIZE ps_converted;

	double rate = paperunit_getrate_bycode(size.unit);

	ps_converted.valid = size.valid;
	ps_converted.unit = PUC_PT;
	ps_converted.width = (size.width * rate) / 100;
	ps_converted.height = (size.height * rate) / 100;

	return ps_converted;
}

static PAPER_UNIT_TYPE paperunit_get_byname(char *name) {
	PAPER_UNIT_TYPE ut_got = UT_INVALID;

	size_t n = ARRAY_SIZE(unit_types);
	for (size_t i = 0; i < n; i++) {
		PAPER_UNIT_TYPE ut = unit_types[i];

		if (strcmp(name, ut.name) == 0) {
			ut_got = ut;
			break;
		}
	}

	return ut_got;
}

// Try to parse the paper size as custom.
static PAPER_SIZE papersize_trycustom(char *spec) {
	PAPER_SIZE ps_got = {};

	double width = 0;
	double height = 0;
	char unit[3] = "";

	int count = sscanf(spec, "%lfx%lf%2s", &width, &height, unit);

	if (count >= 2) {
		char *unit_name = (count == 3) ? unit : "cm";
		PAPER_UNIT_TYPE ut = paperunit_get_byname(unit_name);

		if (ut.valid) {
			ps_got.valid = true;
			ps_got.unit = ut.code;
			ps_got.width = (int) (ABS(width) * (double) 100.0);
			ps_got.height = (int) (ABS(height) * (double) 100.0);
		} else {
			ps_got = PS_INVALID;
		}
	} else {
		ps_got = PS_INVALID;
	}

	return ps_got;
}

// Look for named paper size.
static PAPER_SIZE papersize_trynamed(char *name) {
	PAPER_SIZE ps_got = PS_INVALID;

	size_t n = ARRAY_SIZE(size_types);
	for (size_t i = 0; i < n; i++) {
		PAPER_SIZE_TYPE pt = size_types[i];

		if (strcmp(name, pt.name) == 0) {
			ps_got = pt.size;
			break;
		}
	}

	return ps_got;
}

// Parse paper size (try named first, then custom).
PAPER_SIZE papersize_get(char *spec) {
	PAPER_SIZE ps = papersize_trynamed(spec);
	if (ps.valid)
		return ps;
	
	ps = papersize_trycustom(spec);
	if (ps.valid)
		return ps;
	
	return PS_INVALID;
}

// ==================== DATEI: src/sudoku.h ====================
// vim: noexpandtab:ts=4:sts=4:sw=4

#ifndef SUDOKU_H
#define SUDOKU_H

#include <stdbool.h>	/* bool */

#define STREAM_LENGTH 82

typedef enum { D_EASY = 30, D_NORMAL = 40, D_HARD = 50} DIFFICULTY;

const char*	difficulty_to_str(DIFFICULTY level);
char*	generate_puzzle(int holes);
int		solve(char puzzle[STREAM_LENGTH]);
bool	is_valid_puzzle(char puzzle[STREAM_LENGTH]);

#endif // SUDOKU_H

// ==================== DATEI: src/sudoku.c ====================
/*
vim: noexpandtab:ts=4:sts=4:sw=4

nudoku

Copyright (C) 2014 - 2024 Michael "jubalh" Vetter - jubalh _a-t_ iodoru.org

LICENCE:
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

/* INCLUDES */
#include "utils.h"		/* utility definitions */
#include <stdlib.h>		/* rand, malloc */
#include <time.h>		/* time */
#include <string.h>		/* strdup */
#include "sudoku.h"		/* enum */
#include "unistd.h"		/* sleep */

/* FUNCTIONS */

/* SOLVER */
bool is_valid_puzzle(char puzzle[STREAM_LENGTH])
{
	int rowEntry, colEntry, squareEntry;

	int rowEntryCounter[9] = {0};
	int colEntryCounter[9] = {0};
	int squareEntryCounter[9] = {0};

	for(int row=0; row<9; row++)
	{
		// count occurrences of digits in each row, column and square
		for (int col=0; col<9; col++)
		{
			// get the entries for one row, col and the whole square
			rowEntry = puzzle[row * 9 + col];
			colEntry = puzzle[col * 9 + row];
			squareEntry = puzzle[(((row % 3) + (row / 3) * 9) * 3 ) + (col % 3 + (col / 3) * 9)];
			// if its not an empty field
			// count occurence
			// -48 (ASCII to number) - 1 (array index starts at 0)
			if (rowEntry != '.')
			{
				rowEntryCounter[rowEntry - 49]++;
			}
			if (colEntry!= '.')
			{
				colEntryCounter[colEntry - 49]++;
			}
			if (squareEntry != '.')
			{
				squareEntryCounter[squareEntry - 49]++;
			}
		}
		// check if any digit occurs more than once per row, column or square
		for (int i = 0; i < 9; i++)
		{
			if (rowEntryCounter[i] > 1 || colEntryCounter[i] > 1 || squareEntryCounter[i] > 1)
				return false;
			rowEntryCounter[i] = 0;
			colEntryCounter[i] = 0;
			squareEntryCounter[i] = 0;
		}
	}
	return true;
}

static int get_candidates(char puzzle[STREAM_LENGTH], int pos) {
    int row = pos / 9, col = pos % 9;
    int box_row = (row / 3) * 3, box_col = (col / 3) * 3;
    int mask = 0;

    for (int i = 0; i < 9; i++) {
        if (puzzle[row * 9 + i] != '.') mask |= 1 << (puzzle[row * 9 + i] - '1');
        if (puzzle[i * 9 + col] != '.') mask |= 1 << (puzzle[i * 9 + col] - '1');
        if (puzzle[(box_row + i / 3) * 9 + box_col + i % 3] != '.')
            mask |= 1 << (puzzle[(box_row + i / 3) * 9 + box_col + i % 3] - '1');
    }
    return 0x1FF & ~mask;
}

static int find_best_cell(char puzzle[STREAM_LENGTH], int *cand_out) {
    int best_pos = -1, best_count = 10;

    for (int pos = 0; pos < 81; pos++) {
        if (puzzle[pos] != '.') continue;

        int candidates = get_candidates(puzzle, pos);
        int n = __builtin_popcount(candidates); // single CPU instruction

        if (n == 0) { *cand_out = 0; return -2; } // dead end
        if (n < best_count) {
            best_count = n;
            best_pos = pos;
            *cand_out = candidates;
            if (n == 1) break;
        }
    }
    return best_pos; // -1 if solved
}

// counts solutions (always backtracks)
static int count_solutions(char puzzle[STREAM_LENGTH], int count) {
    int candidates;
    int pos = find_best_cell(puzzle, &candidates);

    if (pos == -2) return count;     // dead end
    if (pos == -1) return count + 1; // solved

    for (int num = 0; num < 9 && count < 2; num++) {
        if (candidates & (1 << num)) {
            puzzle[pos] = '1' + num;
            count = count_solutions(puzzle, count);
            puzzle[pos] = '.'; // always reset
        }
    }
    return count;
}

// Fills puzzle with first solution (keeps values)
static bool fill_puzzle(char puzzle[STREAM_LENGTH]) {
    int candidates;
    int pos = find_best_cell(puzzle, &candidates);

    if (pos == -2) return false; // dead end
    if (pos == -1) return true;  // solved

    for (int num = 0; num < 9; num++) {
        if (candidates & (1 << num)) {
            puzzle[pos] = '1' + num;
            if (fill_puzzle(puzzle)) return true; // keep solution
            puzzle[pos] = '.';
        }
    }
    return false;
}

int solve(char puzzle[STREAM_LENGTH]) {
    if (!is_valid_puzzle(puzzle))
        return 0;

    // count solutions on a copy
    char copy[STREAM_LENGTH];
    strncpy(copy, puzzle, STREAM_LENGTH);
    int count = count_solutions(copy, 0);

    // fill original if solvable
    if (count >= 1)
        fill_puzzle(puzzle);

    return count;
}
/* GENERATOR */
/* Generator code is influenced by: http://rubyquiz.strd6.com/quizzes/182-sudoku-generator */
static int rand_int(int n)
{
	int rnd;
	int limit = RAND_MAX - RAND_MAX % n;

	do {
		rnd = rand();
	} while (rnd >= limit);
	return (rnd % n);
}

static void shuffle(char *array, int n)
{
	int i, j;
	char tmp;

	for (i = n - 1; i > 0; i--)
	{
		j = rand_int(i + 1);
		tmp = array[j];
		array[j] = array[i];
		array[i] = tmp;
	}
}

static char* create_random_numbers()
{
	char numbers[10] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', '\0'};
	shuffle(numbers, 9);
	return strdup(numbers);
}

static char* generate_seed()
{
	char *stream = (char*)malloc(81);
	int index = 0;
	int iSquare = 0;

	char* upperleft = create_random_numbers();
	char* center = create_random_numbers();
	char* lowerright = create_random_numbers();

	//first three rows
	for (int i = 0; i < 3; i++)
	{
		for(int j = 0; j < 3; j++)
			stream[index++] = upperleft[iSquare++];
		for(int j = 0; j < 6 ; j++)
			stream[index++] = '.';
	}
	iSquare = 0;
	//second three rows
	for (int i = 0; i < 3; i++)
	{
		for(int j = 0; j < 3 ; j++)
			stream[index++] = '.';
		for(int j = 0; j < 3; j++)
			stream[index++] = center[iSquare++];
		for(int j = 0; j < 3 ; j++)
			stream[index++] = '.';
	}
	iSquare = 0;
	//third three rows
	for (int i = 0; i < 3; i++)
	{
		for(int j = 0; j < 6 ; j++)
			stream[index++] = '.';
		for(int j = 0; j < 3; j++)
			stream[index++] = lowerright[iSquare++];
	}

	stream[81] = '\0';

	free(upperleft);
	free(center);
	free(lowerright);

	return stream;
}

static void punch_holes(char *stream, int count)
{
	int i = 0;
	while (i < count)
	{
		int random = rand() % 80 + 1;
		char temp = stream[random];

		if (stream[random] != '.')
		{
			stream[random] = '.';

			char puzzle_copy[STREAM_LENGTH];
			strncpy(puzzle_copy, stream, STREAM_LENGTH);

			// check if puzzle has only 1 solution
			if (solve(puzzle_copy) == 1)
			{
				i++;
			}
			else
			{
				// restore removed value
				stream[random] = temp;
			}
		}
	}
}

const char* difficulty_to_str(DIFFICULTY level)
{
	switch(level)
	{
		case D_HARD:
			return _("hard");
		case D_NORMAL:
			return _("normal");
		case D_EASY:
		default:
			return _("easy");
	}
}

char* generate_puzzle(int holes)
{
	char* stream;

	stream = generate_seed();
	solve(stream);
	punch_holes(stream, holes);
	return stream;
}

// ==================== DATEI: src/main.c ====================
/*
vim: noexpandtab:ts=4:sts=4:sw=4

nudoku

Copyright (C) 2014 - 2024 Michael "jubalh" Vetter - jubalh _a-t_ iodoru.org

LICENCE:
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

/* INCLUDES */
#include "utils.h"				/* utility definitions */
#include <stdlib.h>				/* rand, srand */
#include <unistd.h>				/* getopt */
#include <sys/types.h>			/* stat type */
#include <sys/stat.h>			/* stat function */
#include <ncurses.h>			/* ncurses */
#include <time.h>				/* time */
#include <string.h>				/* strcmp, strlen */
#include <locale.h>				/* setlocale */
#include "sudoku.h"				/* sudoku functions */
#ifdef ENABLE_CAIRO
#include "outp.h"				/* output functions */
#endif

/* DEFINES */
//#define VERSION				"0.1" //gets set via autotools
#define GRID_LINES				19
#define GRID_COLS				37
#define GRID_Y					3
#define GRID_X					3
#define INFO_LINES				25
#define INFO_COLS				30
#define INFO_Y					3
#define INFO_X					GRID_X + GRID_COLS + 5
#define GRID_NUMBER_START_Y		1
#define GRID_NUMBER_START_X		2
#define GRID_LINE_DELTA			4
#define GRID_COL_DELTA			2
#define STATUS_LINES			1
#define STATUS_COLS				GRID_COLS + INFO_COLS
#define STATUS_Y				1
#define STATUS_X				GRID_X
#define MAX_HINT_RANDOM_TRY		20
#define SUDOKU_LENGTH			STREAM_LENGTH - 1
#define COLOR_HIGHLIGHT			4
#define COLOR_HIGHLIGHT_CURSOR	5
#define COLOR_USER_HIGHLIGHT	6
#define UNDO_STACK_SIZE			SUDOKU_LENGTH * 10 // arbitrary length. overflows shouldn't cause an error, just a limit in history length.
#define STATE_FILE_NAME         "state.save"

#ifdef DEBUG
#define EXAMPLE_STREAM "4.....8.5.3..........7......2.....6.....8.4......1.......6.3.7.5..2.....1.4......"
#endif // DEBUG

typedef struct move
{
    int x;
    int y;
    char prev_val;
} move_t;

/* GLOBALS */
static bool  g_useColor = true;
static bool  g_playing = false;
static bool  g_useHighlights = false;
static bool  g_output_stream = false;		/* is the -o flag set */
static char* g_provided_stream = NULL;		/* in case of -s flag the user provides the sudoku stream */
static bool  g_resume_game = false;         /* in case of -r flag and saved game state */
static int   g_resume_level;                /* store difficulty of the resume game */
static int   g_hint_counter;
static char  plain_board[STREAM_LENGTH];
static char  user_board[STREAM_LENGTH];
static char* g_outputFilename = NULL;		/* in case -p/-i flag we get a filename passed for outputting */
static int   g_sudokuCount = 1;				/* in case of -n we can the numbers of sudoku that should end up in the PDf (-p) */
static PAPER_SIZE g_pdfSize = PS_DEFAULT;	/* in case of -S we get the PDF paper size from its name (a4/letter) */
static bool  g_outIsPDF;
static DIFFICULTY g_level = D_EASY;
static WINDOW *grid, *infobox, *status;
static move_t	g_undo_stack[UNDO_STACK_SIZE];	/* Stack of previous moves */
static int   g_undo_stack_index = 0;

/* FUNCTIONS */
static void print_version(void)
{
	printf("nudoku version " VERSION "\n\n\
Copyright (C) Michael Vetter 2014 - 2024\n\
License GPLv3+: GNU GPL version 3 or later.\n\
This is free software, you are free to modify and redistribute it.\n");
#ifdef DEBUG
	printf("Debug enabled\n");
#endif // DEBUG
}

static void print_usage(void)
{
	printf(_("nudoku [option]\n\n"));
	printf(_("Options:\n"));
	printf(_("-h help:\t\tPrint this help\n"));
	printf(_("-v version:\t\tPrint version\n"));
	printf(_("-c nocolor:\t\tDo not use colors\n"));
	printf(_("-o output:\t\tOutput stream (inverse of -s)\n"));
	printf(_("-d difficulty:\t\tChoose between: easy, normal, hard\n"));
	printf(_("-s stream:\t\tUser provided sudoku stream\n"));
	printf(_("-r resume:\t\tResume the last saved game\n"));
	printf(_("-p filename:\t\tOutput PDF\n"));
	printf(_("-n filename:\t\tNumber of sudokus to put in PDF\n"));
	printf(_("-i filename:\t\tOutput PNG image\n"));
	printf(_("-S papername:\t\tPDF paper size (e.g., 'a4', 'letter', 'legal'; default 'a4') or 'WIDTHxHEIGHT[unit]'\n"));
}

static bool is_valid_stream(char *s)
{
	char *p = s;
	short n = 0;
	while ((*p) != '\0')
	{
		if (n++ > SUDOKU_LENGTH)
			break;

		if(!((*p >= 49 && *p <= 57) || *p == '.' ))
		{
			printf(_("Character %c at position %d is not allowed.\n"), *p, n);
			return false;
		}
		p++;
	}

	if (n != SUDOKU_LENGTH )
	{
		printf(_("Stream has to be %d characters long.\n"), SUDOKU_LENGTH);
		return false;
	}

	if (!is_valid_puzzle(s))
	{
		printf(_("Stream does not represent a valid sudoku puzzle.\n"));
		return false;
	}

	return true;
}

char* get_saved_file_path(void)
{
	char* home_path = getenv("XDG_STATE_HOME");
	char* fallback_path = NULL;

	if (home_path == NULL)
	{
		// fallback to $HOME/.local/state
		home_path = getenv("HOME");

		if (home_path == NULL)
			return NULL;

		const char* local_path = "/.local/state";
		size_t len_home_path = strlen(home_path);
		size_t len_local_path = strlen(local_path);

		fallback_path = malloc(len_home_path + len_local_path + 1);

		strcpy(fallback_path, home_path);
		strcat(fallback_path, local_path);

		home_path = fallback_path;
	}

	size_t len_home_path = strlen(home_path);

	const char* dir_name = "/nudoku/";
	size_t len_dir_name = strlen(dir_name);

	char* dir_path = malloc(len_home_path + len_dir_name + 1);
	strcpy(dir_path, home_path);
	strcat(dir_path, dir_name);

	struct stat st = {0};
	if (stat(dir_path, &st) == -1)
		mkdir(dir_path, 0700);

	const char* file_name = STATE_FILE_NAME;
	size_t len_file = strlen(file_name);
	size_t len_dir_path = strlen(dir_path);

	char* file_path = malloc(len_dir_path + len_file + 1);
	if (file_path == NULL)
	{
		free(dir_path);
		free(fallback_path);
		return NULL;
	}

	strcpy(file_path, dir_path);
	strcat(file_path, file_name);

	free(dir_path);
	free(fallback_path);

	return file_path;
}

bool get_board_save(char user_board[], char plain_board[])
{
	int c;
	int i = 0;
	char board[2 * STREAM_LENGTH];
	char tmp_board[STREAM_LENGTH];

	char* file_path = get_saved_file_path();
	FILE* fp = fopen(file_path, "r");
	unlink(file_path);
	free(file_path);

	if ( fp == NULL )
		return false;

	while ( (c = fgetc(fp)) != EOF )
	{
		board[i] = c;
		if (i >= sizeof(board) - 1) {
			board[sizeof(board) - 1] = '\0';
			break;
		}
		i++;
	}

	fclose(fp);

	strncpy(user_board, board, STREAM_LENGTH);
	strcpy(plain_board, board + STREAM_LENGTH);
	strcpy(tmp_board, plain_board);

	if (!is_valid_stream(tmp_board))
		return false;

	// set difficulty level from save
	int count = 0;
	const char *tmp = plain_board;
	while((tmp = strchr(tmp, '.')) != NULL)
	{
		count++;
		tmp++;
	}

	g_resume_level = count;

	return true;
}

bool save_stream(char user_board[], char plain_board[], int n)
{
	char* file_path = get_saved_file_path();
	FILE* fp = fopen(file_path, "w");
	free(file_path);

	if ( fp == NULL )
		return false;

	for ( int i=0; i<n; ++i )
	{
		fprintf(fp, "%c", user_board[i]);
	}
	for ( int i=0; i<n; ++i )
	{
		fprintf(fp, "%c", plain_board[i]);
	}
	fclose(fp);

	return true;
}

void generate_stream_output(int difficulty) {
	char* stream = generate_puzzle(difficulty);

	printf("%s\n", stream);

	free(stream);
}

static void parse_arguments(int argc, char *argv[])
{
	int opt;
	while ((opt = getopt(argc, argv, "hvcors:d:p:i:n:S:")) != -1)
	{
		switch (opt)
		{
			case 'h':
				print_usage();
				exit(EXIT_SUCCESS);
			case 'v':
				print_version();
				exit(EXIT_SUCCESS);
			case 'c':
				g_useColor = false;
				break;
			case 'o':
				g_output_stream = true;
				break;
			case 's':
				if (!is_valid_stream(optarg))
					exit(EXIT_FAILURE);
				g_provided_stream = strdup(optarg);
				break;
			case 'r':
				if (get_board_save(user_board, plain_board))
				{
					g_resume_game = true;
					g_level = g_resume_level; // set global variables in one place
				}
				else
				{
					printf(_("Game save is missing or corrupted, try starting new game.\n"));
					exit(EXIT_FAILURE);
				}
				break;
			case 'd':
				if (strcmp(optarg, "easy") == 0)
					g_level = D_EASY;
				else if (strcmp(optarg, "normal") == 0)
					g_level = D_NORMAL;
				else if (strcmp(optarg, "hard") == 0)
					g_level = D_HARD;
				else
				{
					print_usage();
					exit(EXIT_FAILURE);
				}
				break;
			// output pdf
			case 'p':
				g_outputFilename = strdup(optarg);
				g_outIsPDF = true;
				break;
			// output png image
			case 'i':
				g_outputFilename = strdup(optarg);
				g_outIsPDF = false;
				break;
			// numbers of sudoku for output pdf
			case 'n':
				g_sudokuCount = atoi(optarg);
				break;
			// PDF paper size
			case 'S':
				g_pdfSize = papersize_get(optarg);
				if (!g_pdfSize.valid) {
					print_usage();
					exit(EXIT_FAILURE);
				}
				break;
			default:
				print_usage();
				exit(EXIT_FAILURE);
		}
	}
}

static void cleanup(void)
{
	endwin();
}

static void init_curses(void)
{
	initscr();
	use_default_colors();
	clear();
	atexit(cleanup);
	cbreak();
	noecho();

	if(g_useColor)
	{
		if(has_colors())
		{
			start_color();
			init_pair(1, COLOR_GREEN, -1);
			init_pair(2, COLOR_BLUE, -1);
			// user input color
			init_pair(3, COLOR_CYAN, -1);
			// Highlight color
			init_pair(COLOR_HIGHLIGHT, COLOR_BLACK, COLOR_WHITE);
			// Cursor highlight color
			init_pair(COLOR_HIGHLIGHT_CURSOR, COLOR_BLACK, COLOR_MAGENTA);
			// User input highlight color
			init_pair(COLOR_USER_HIGHLIGHT, COLOR_BLACK, COLOR_CYAN);
		}
		else
		{
			printw( _("Your terminal doesn't support colors.\nTry the nocolor (-c) option.\n"));
			getch();
			exit(EXIT_FAILURE);
		}
	}
}

static void _draw_grid()
{
	int i, j;

	for(i = 0;i < 10;i++)
	{
		for(j = 0;j < 10;j++)
		{
			if (g_useColor)
			{
				if(i % 3 == 0)
					wattron(grid, A_BOLD|COLOR_PAIR(2));
				if(j % 3 == 0)
					wattron(grid, A_BOLD|COLOR_PAIR(2));
			}

			if(i == 0 && j == 0)
				waddch(grid, ACS_ULCORNER);
			else if(i == 0 && j == 9)
				waddch(grid, ACS_URCORNER);
			else if(i == 9 && j == 9)
				waddch(grid, ACS_LRCORNER);
			else if(i == 9 && j == 0)
				waddch(grid, ACS_LLCORNER);
			else if(i == 0)
				waddch(grid, ACS_TTEE);
			else if(i == 9)
				waddch(grid, ACS_BTEE);
			else if(j == 0)
				waddch(grid, ACS_LTEE);
			else if(j == 9)
				waddch(grid, ACS_RTEE);
			else
				waddch(grid, ACS_PLUS);

			if(g_useColor && (j % 3 == 0) && (i % 3 != 0))
			{
				wattron(grid, A_BOLD|COLOR_PAIR(1));
			}
			if(j < 9)
			{
				waddch(grid, ACS_HLINE);
				waddch(grid, ACS_HLINE);
				waddch(grid, ACS_HLINE);
			}
			if(g_useColor && (i % 3 == 0))
			{
				wattron(grid, A_BOLD|COLOR_PAIR(1));
			}
		}
		for(j = 0;j < 10 && i < 9;j++)
		{
			if(g_useColor && (j % 3 == 0))
				wattron(grid, A_BOLD|COLOR_PAIR(2));
			if(j > 0)
				wprintw(grid, "   ");
			waddch(grid, ACS_VLINE);
			if(g_useColor && (j % 3 == 0))
			{
				wattron(grid, A_BOLD|COLOR_PAIR(1));
			}
		}
	}
}

static void init_windows(void)
{
	keypad(stdscr, true);

	status = newwin(STATUS_LINES, STATUS_COLS, STATUS_Y, STATUS_X);

	grid = newwin(GRID_LINES, GRID_COLS, GRID_Y, GRID_X);
	_draw_grid();

	infobox = newwin(INFO_LINES, INFO_COLS, INFO_Y, INFO_X);
	if (g_useColor)
	{
		wbkgd(infobox, COLOR_PAIR(2));
		wattron(infobox, A_BOLD|COLOR_PAIR(2));
	}

	wprintw(infobox, "nudoku %s\n", VERSION);

	if (!g_provided_stream)
		wprintw(infobox, _("level: %s\n\n"), difficulty_to_str(g_level) );
	else
		wprintw(infobox, "\n\n");

	if (g_useColor)
	{
		wattroff(infobox, A_BOLD|COLOR_PAIR(2));
		wattron(infobox, COLOR_PAIR(1));
	}
	wprintw(infobox, _("Movement\n"));
	wprintw(infobox, _(" h - Move left\n"));
	wprintw(infobox, _(" j - Move down\n"));
	wprintw(infobox, _(" k - Move up\n"));
	wprintw(infobox, _(" l - Move right\n\n"));
	wprintw(infobox, _("Commands\n"));
	wprintw(infobox, _(" c - Check solution\n"));
	wprintw(infobox, _(" H - Give a hint\n"));
	if (g_useColor)
	{
		wprintw(infobox, _(" m - Toggle marks\n"));
	}
	wprintw(infobox, _(" N - New puzzle\n"));
	wprintw(infobox, _(" G - Save\n"));
	wprintw(infobox, _(" Q - Quit\n"));
	wprintw(infobox, _(" r - Redraw\n"));
	wprintw(infobox, _(" S - Solve puzzle\n"));
	wprintw(infobox, _(" x - Delete number\n"));
	wprintw(infobox, _(" u - Undo previous action\n"));
	if (g_useColor)
	{
		wattroff(infobox, COLOR_PAIR(1));
	}
}

static int undo_stack_push(move_t move)
{
	if (g_undo_stack_index >= UNDO_STACK_SIZE)
	{
		return -1;
	}
	g_undo_stack[g_undo_stack_index++] = move;

	return 0;
}

static int undo_stack_pop(move_t *move)
{
	if (g_undo_stack_index <= 0)
		return -1;

	*move = g_undo_stack[--g_undo_stack_index];

	return 0;
}

static int get_character_at_grid(char* board, int x, int y)
{
	int posx, posy;
	posy = (y-GRID_NUMBER_START_Y)/GRID_COL_DELTA;
	posx = (x-GRID_NUMBER_START_X)/GRID_LINE_DELTA;
	return board[posy*9+posx];
}

static void fill_grid(char *user_board, char *plain_board, int x_cursor, int y_cursor)
{
	int row, col, x, y;
	int n;
	int c;
	int selected;
	bool isUserInput;
	int m;

	selected = get_character_at_grid(user_board, x_cursor, y_cursor);
	wstandend(grid);
	y = GRID_NUMBER_START_Y;
	for(row=0; row < 9; row++)
	{
		x = GRID_NUMBER_START_X;
		for(col=0; col < 9; col++)
		{
			isUserInput = true;
			n = user_board[row*9+col];
			if(n == '.')
				c = ' ';
			else
			{
				c = n;
				m = plain_board[row*9+col];
				if(n == m)
					isUserInput = false;
			}
			if (g_useColor && g_useHighlights && selected == c)
			{
				if (x == x_cursor && y == y_cursor)
					wattron(grid, COLOR_PAIR(COLOR_HIGHLIGHT_CURSOR));
				else if (isUserInput)
					wattron(grid, COLOR_PAIR(COLOR_USER_HIGHLIGHT));
				else
					wattron(grid, COLOR_PAIR(COLOR_HIGHLIGHT));
			}
			if(isUserInput && selected != c)
				wattron(grid, COLOR_PAIR(3));
			mvwprintw(grid, y, x, "%c", c);
			if(isUserInput && selected != c)
				wattroff(grid, COLOR_PAIR(3));
			if (g_useColor && g_useHighlights && selected == c)
			{
				if (x == x_cursor && y == y_cursor)
					wattroff(grid, COLOR_PAIR(COLOR_HIGHLIGHT_CURSOR));
				else if (isUserInput)
					wattroff(grid, COLOR_PAIR(COLOR_USER_HIGHLIGHT));
				else
					wattroff(grid, COLOR_PAIR(COLOR_HIGHLIGHT));
			}
			x += GRID_LINE_DELTA;
		}
		y += GRID_COL_DELTA;
	}
}

static void new_puzzle(void)
{
	int holes = g_level;
	char* stream;

	if (g_provided_stream)
		stream = g_provided_stream;
	else
		stream = generate_puzzle(holes);

	if (!g_provided_stream)
		free(stream);

	//todo
	strcpy(plain_board, stream);
	strcpy(user_board, stream);

	fill_grid(plain_board, plain_board, GRID_NUMBER_START_X, GRID_NUMBER_START_Y);

	g_playing = true;
}

static bool hint(void)
{
	char tmp_board[STREAM_LENGTH];
	int i, j, solved, try = 0;

	strcpy(tmp_board, user_board);
	solved = solve(tmp_board);
	if (solved != 0)
	{
		do
		{
			i = rand() % 8 + 1;
			j = rand() % 8 + 1;
			try++;
			if ( user_board[i*9+j] == '.')
			{
				user_board[i*9+j] = tmp_board[i*9+j];
				return true;
			}
		} while (try < MAX_HINT_RANDOM_TRY);
	}
	return false;
}

int main(int argc, char *argv[])
{
#if ENABLE_NLS
	/* Set up internationalization */
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
#endif
	bool run = true;
	int key, x, y, posx, posy;

	parse_arguments(argc, argv);
	srand(time(NULL));

	if (g_output_stream)
	{
		generate_stream_output(g_level);
		return EXIT_SUCCESS;
	}

	if (g_outputFilename)
	{
#ifdef ENABLE_CAIRO
		return generate_output(g_level, g_outputFilename, g_sudokuCount, g_outIsPDF, g_pdfSize);
#else
		printf(_("nudoku is compiled without cairo support.\n"));
		printf(_("To use the output feature, please compile with --enable-cairo.\n"));
		return 1;
#endif
	}

	init_curses();
	init_windows();

#ifdef DEBUG
	strcpy(plain_board, EXAMPLE_STREAM);
	strcpy(user_board, EXAMPLE_STREAM);
	fill_grid(plain_board, plain_board, GRID_NUMBER_START_X, GRID_NUMBER_START_Y);
	g_playing = true;
#else

	if ( g_resume_game )
	{
		fill_grid(user_board, plain_board, GRID_NUMBER_START_X, GRID_NUMBER_START_Y);
		g_playing = true;
	}
	else
		new_puzzle();

#endif // DEBUG

	refresh();
	wrefresh(grid);
	wrefresh(infobox);

	y = GRID_NUMBER_START_Y;
	x = GRID_NUMBER_START_X;
	wmove(grid, y, x);
	while(run)
	{
#ifdef DEBUG
		mvprintw(0, 0, "y: %.2d x: %.2d", y, x);
#endif // DEBUG
		refresh();
		wrefresh(grid);
		key = getch();
		// clear status window
		werase(status);
		switch(key)
		{
			case 'h':
			case KEY_LEFT:
				if (x-GRID_NUMBER_START_X <= 0) {
					x = GRID_NUMBER_START_X + GRID_LINE_DELTA*8;
				}
				else if(x>5)
				{
					x -= GRID_LINE_DELTA;

				}
				if(g_playing)
				{
					// if we have highlighting enabled, we need to redraw the whole grid
					// so we can have the new colors in the matching colors.
					// this should only be done when we are playing, because plain_board
					// is actually the one being solved and thus displayed.
					// this is true for all movement keys.
					fill_grid(user_board, plain_board, x, y);
				}
				break;
			case 'l':
			case KEY_RIGHT:
				if ((x-GRID_NUMBER_START_X) / GRID_LINE_DELTA >= 8) {
					x = GRID_NUMBER_START_X;
				}
				else if(x<34)
				{
					x += GRID_LINE_DELTA;
				}
				if(g_playing)
				{
					fill_grid(user_board, plain_board, x, y);
				}
				break;
			case 'k':
			case KEY_UP:
				if (y-GRID_NUMBER_START_Y <= 0) {
					y = GRID_NUMBER_START_Y + GRID_COL_DELTA*8;
				}
				else if(y>2)
				{
					y -= GRID_COL_DELTA;
				}
				if(g_playing)
				{
					fill_grid(user_board, plain_board, x, y);
				}
				break;
			case 'j':
			case KEY_DOWN:
				if ((y-GRID_NUMBER_START_Y) / GRID_COL_DELTA >= 8) {
					y = GRID_NUMBER_START_Y;
				}
				else if(y<17)
				{
					y += GRID_COL_DELTA;
				}
				if(g_playing)
				{
					fill_grid(user_board, plain_board, x, y);
				}
				break;
			case 'Q':
			case 27:
				run = false;
				break;
			case 'r':
			case KEY_RESIZE:
				redrawwin(grid);
				redrawwin(infobox);
				break;
			case 'S':
				if(g_playing)
				{
					g_useHighlights = false;
					werase(status);
					mvwprintw(status, 0, 0, _("Solving puzzle..."));
					refresh();
					wrefresh(status);
					solve(plain_board);
					fill_grid(plain_board, plain_board, x, y);
					werase(status);
					mvwprintw(status, 0, 0, _("Solved"));
					g_playing = false;
				}
				break;
			case 'N':
				g_useHighlights = false;
				g_hint_counter = 0;
				g_undo_stack_index = 0;

				werase(status);
				mvwprintw(status, 0, 0, _("Generating puzzle..."));
				refresh();
				wrefresh(status);
				new_puzzle();
				werase(status);
				g_playing = true;

				if (g_provided_stream)
				{
					free(g_provided_stream);
					g_provided_stream = NULL;
				}
				break;
			case 'c':
				if(g_playing)
				{
					int solvable;
					char tmp_board[STREAM_LENGTH];

					werase(status);

					strcpy(tmp_board, user_board);
					solvable= solve(tmp_board);

					if(solvable == 0)
					{
						mvwprintw(status, 0, 0, _("Not correct"));
					}
					else
					{
						if (strchr(user_board, '.') == NULL)
						{
							mvwprintw(status, 0, 0, _("Solved"));

							if (g_hint_counter > 0)
							{
								char t[256];
								sprintf(t, _(" with the help of %d hints"), g_hint_counter);
								mvwprintw(status, 0, 6, "%s", t);
							}

							g_playing = false;
						}
						else
						{
							mvwprintw(status, 0, 0, _("Correct so far"));
						}
					}
				}
				break;
			// delete
			case KEY_DC:
			case KEY_BACKSPACE:
			case 127:
			case 'x':
				if(g_playing)
				{
					posy = (y-GRID_NUMBER_START_Y)/GRID_COL_DELTA;
					posx = (x-GRID_NUMBER_START_X)/GRID_LINE_DELTA;
					// if on empty position
					if(plain_board[posy*9+posx] == '.')
					{
						// Push coordinates to undo stack
						undo_stack_push((move_t){x, y, user_board[posy*9+posx]});
						user_board[posy*9+posx] = '.';
						wprintw(grid, " ");
					}
					break;
				}
			case 'H':
				if (g_playing && hint())
				{
					g_hint_counter++;
					fill_grid(user_board, plain_board, x, y);
					werase(status);
					mvwprintw(status, 0, 0, _("Provided hint"));
				}
				break;
			case 'm':
				// Ignore 'm' if we have no colors
				if (g_playing && g_useColor)
				{
					g_useHighlights = !g_useHighlights;
					fill_grid(user_board, plain_board, x, y);
				}
				break;
			case 'G':
				if (save_stream(user_board, plain_board, STREAM_LENGTH))
					mvwprintw(status, 0, 0, _("Saved!"));
				else
				{
					mvwprintw(status, 0, 0, _("Can't save the game!"));
				}
				break;
			case 'u': // Undo
				{
					move_t old_move;
					if (undo_stack_pop(&old_move))
					{	// Stack empty
						break;
					}
					x = old_move.x;
					y = old_move.y;
					posy = (y-GRID_NUMBER_START_Y)/GRID_COL_DELTA;
					posx = (x-GRID_NUMBER_START_X)/GRID_LINE_DELTA;
					user_board[posy*9+posx] = old_move.prev_val;
					fill_grid(user_board, plain_board, x, y);
					break;
				}

			default:
				break;
		}
		/*if user inputs a number*/
		if(key >= 49 && key <= 57 && g_playing)
		{
			posy = (y-GRID_NUMBER_START_Y)/GRID_COL_DELTA;
			posx = (x-GRID_NUMBER_START_X)/GRID_LINE_DELTA;
			// if on empty position
			if(plain_board[posy*9+posx] == '.')
			{
				// Push coordinates to undo stack
				undo_stack_push((move_t){x, y, user_board[posy*9+posx]});
				// add inputted number to grid
				user_board[posy*9+posx] = key;
				// redraw grid to update highlight
				fill_grid(user_board, plain_board, x, y);
			}
		}
		wmove(grid, y,x);
		refresh();
		wrefresh(status);
		wrefresh(grid);
		wrefresh(infobox);
	}

	if (g_provided_stream)
		free(g_provided_stream);
	if (g_outputFilename)
		free(g_outputFilename);

	endwin();
	return EXIT_SUCCESS;
}

