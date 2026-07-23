// SLICE um 'generate_puzzle' (depth=1)
// 8/49 Funktionen, ~4494/33100 Zeichen (13% des Repos)

// --- generate_pdf  (src/outp.c) ---
}

int generate_pdf(int difficulty, int sudokus_count, PAPER_SIZE paperSize, const char* filename)
{
	cairo_surface_t *surface;
	cairo_t *cr;

	PAPER_SIZE ps_points = papersize_normalize(paperSize);
	surface = cairo_pdf_surface_create(filename, ps_points.width, ps_points.height);
	cr = cairo_create(surface);

	prepare_new_page(cr, difficulty, true);

	DUPLE_INT di_initial_pad = { 100, 270 };
	DUPLE_INT di_intra_pad = { 100, 150 };

	DUPLE_INT d_fit = get_no_sudokus_fit(cr, ps_points, di_initial_pad, di_intra_pad);

	if (d_fit.x <= 0 || d_fit.y <= 0) {
		printf(_("ERROR: Page dimensions are too small for sudokus to fit!\n"));
		printf(_("Please check the dimensions provided in '-S' and try again.\n"));

		return finish_pdf_generation(surface, cr, false);
	}

	int no_sudokus_per_page = d_fit.x * d_fit.y;

	for (int i=0; i < sudokus_count; i++)
	{
		int cur_sudoku_col = (i % no_sudokus_per_page) % d_fit.x;
		int cur_sudoku_row = (i % no_sudokus_per_page) / d_fit.x;

		DUPLE_INT start = {
			di_initial_pad.x + (cur_sudoku_col * SUDOKU_SPACE(int, di_intra_pad.x)),
			di_initial_pad.y + (cur_sudoku_row * SUDOKU_SPACE(int, di_intra_pad.y))
		};

		cairo_translate(cr, start.x, start.y);

		char* stream;
		stream = generate_puzzle(difficulty);

#ifdef DEBUG
		printf("%d: %s\n", i, stream);
#endif  

		draw_grid(stream, cr);

		cairo_translate(cr, -start.x, -start.y);

		if ((i + 1) < sudokus_count && (i + 1) % no_sudokus_per_page == 0) {
			 
			cairo_show_page(cr);
			prepare_new_page(cr, difficulty, false);
		}

		free(stream);
	}

	return finish_pdf_generation(surface, cr, true);
}

// --- generate_png  (src/outp.c) ---
}

void generate_png(int difficulty, char* filename)
{
	cairo_surface_t *surface;
	cairo_t *cr;

	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 453, 453);
	cr = cairo_create(surface);

	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);

	cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, 10.0);

	char* stream;
	stream = generate_puzzle(difficulty);

	cairo_translate(cr, 2, 2);
	draw_grid(stream, cr);
	cairo_surface_write_to_png(surface, filename);

	cairo_destroy(cr);
	cairo_surface_destroy(surface);

	free(stream);
}

// --- generate_puzzle  (src/sudoku.c) ---
}

char* generate_puzzle(int holes)
{
	char* stream;

	stream = generate_seed();
	solve(stream);
	punch_holes(stream, holes);
	return stream;
}

// --- generate_seed  (src/sudoku.c) ---
}

static char* generate_seed()
{
	char *stream = (char*)malloc(81);
	int index = 0;
	int iSquare = 0;

	char* upperleft = create_random_numbers();
	char* center = create_random_numbers();
	char* lowerright = create_random_numbers();

	 
	for (int i = 0; i < 3; i++)
	{
		for(int j = 0; j < 3; j++)
			stream[index++] = upperleft[iSquare++];
		for(int j = 0; j < 6 ; j++)
			stream[index++] = '.';
	}
	iSquare = 0;
	 
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

// --- generate_stream_output  (src/main.c) ---
}

void generate_stream_output(int difficulty) {
	char* stream = generate_puzzle(difficulty);

	printf("%s\n", stream);

	free(stream);
}

// --- new_puzzle  (src/main.c) ---
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

	 
	strcpy(plain_board, stream);
	strcpy(user_board, stream);

	fill_grid(plain_board, plain_board, GRID_NUMBER_START_X, GRID_NUMBER_START_Y);

	g_playing = true;
}

// --- punch_holes  (src/sudoku.c) ---
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

			 
			if (solve(puzzle_copy) == 1)
			{
				i++;
			}
			else
			{
				 
				stream[random] = temp;
			}
		}
	}
}

// --- solve  (src/sudoku.c) ---
}

int solve(char puzzle[STREAM_LENGTH]) {
    if (!is_valid_puzzle(puzzle))
        return 0;

     
    char copy[STREAM_LENGTH];
    strncpy(copy, puzzle, STREAM_LENGTH);
    int count = count_solutions(copy, 0);

     
    if (count >= 1)
        fill_puzzle(puzzle);

    return count;
}

