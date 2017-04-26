#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <curses.h>
#include <math.h>
#include <sys/ioctl.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <libgen.h>

#define DEBUG 1

#define RD_WIDTH	70
#define RD_HEIGHT	28

	/*turnPage reads enough characters to fit within the confines of the reader window, and puts them inside currentPage*/
int turnPage(FILE *book);
	/*clearBackground writes an empty space over all characters*/
void clearBackground();
	/*drawBackground writes a tilde `~` periodically over the background*/
void drawBackground();
	/*drawBorder creates a border around the reader*/
void drawBorder();
	/*drawReader creates an empty box of size WIDTH x HEIGHT to display text in*/
void drawReader();
	/*drawText writes the text of the file into the reader box. it has optional arguments to highlight
	 *certain text at a certain offset. Passing NULL or -1 into it will cause it to not highlight any text*/
void drawText(char *highlighted, int offset);
	/*drawAll is a catch-all to call clearBackground, drawBackground, drawBorder, drawReader, and drawText. It takes
	 *the same parameters as drawText and passes them through to drawText*/
void drawAll(char *highlighted, int offset);
	/*find searches all pages of the book for the phrase requested and returns the offset of that phrase within that page
	 *the parameter "original" is to save the location of the reader's page in case the phrase is not found*/
int find(char *phrase, FILE *book, int original);
	/*saveBookmark will save a bookmark in ~/.books/bookmarks*/
void saveBookmark(char *bookName);
	/*readBookmark will open the book to the page defined in ~/.books/bookmarks*/
int readBookmark(char *bookName, int markNum);

	/*scr_width and scr_height are the dimensions of the terminal*/
int scr_width = -1;
int scr_height = -1;
	/*currentPageNum is the current page number. a single page is defined by how many characters can
	 *fit in a given window, so a page is different from screen size to screen size*/
int currentPageNum = 0;
	/*WIDTH and HEIGHT are the dimensions of the reader*/
int WIDTH = RD_WIDTH;
int HEIGHT = RD_HEIGHT;
	/*currentPage is all the characters which can were read from the file and fit inside the reader*/
char *currentPage;
int fileOffset = -1;

int bookmarksOpen = 0;
int bookmarkSelect = 0;

	/*getOption is used to get the value associated with some command line argument*/
char *getOption(int argc, char *argv[], char *desired_arg);
	/*returns the number of digits in some integer. Helper*/
unsigned int GetNumberOfDigits(unsigned int i);
char *getFileContents(FILE *file);

void showBookMarks(FILE *bookmarkFile, int index);


int main(int argc, char **argv){
		/*Initialization; check for file*/
	if(argc < 2){
		fprintf(stderr, "Error:\n\tUsage: books [FILE] (OPTIONAL: -w [WIDTH] -h [HEIGHT] -p [PAGENUM] -b, --help for more explanation)\n");
		exit(1);
	}

		/*get options and deal with them*/
	char *configPath = malloc(strlen(getenv("HOME")) + strlen("/.books/config/config.cfg"));
	strcat(configPath, getenv("HOME"));
	strcat(configPath, "/.books/config/config.cfg");
	FILE *cfg = fopen(configPath, "r");
	char *value;
		/*-w defines the width of the reader*/
	if((value = getOption(argc, argv, "-w")) != NULL){
		WIDTH = atoi(value);
	}else if(cfg != NULL){
		fscanf(cfg, "width=%d", &WIDTH);
	}
		/*-h defines the height of the reader*/
	if((value = getOption(argc, argv, "-h")) != NULL){
		HEIGHT = atoi(value);
	}else if(cfg != NULL){
		fscanf(cfg, "height=%d", &HEIGHT);
	}
		/*if -b is present, then the reader will look for a bookmark*/
	if((value = getOption(argc, argv, "-b")) != NULL){
		fileOffset = readBookmark(basename(argv[1]), -1);
	}

		/*the max size of currentpage is the area of the reader*/
	currentPage = malloc(WIDTH * HEIGHT);

		/*-p is used to jump to a specific page*/
	if((value = getOption(argc, argv, "-p")) != NULL){
		currentPageNum = atoi(value);
	}

	FILE *book = fopen(argv[1], "r");
	if(book == NULL){
		fprintf(stderr, "Error: Couldn't open book\n");
		exit(1);
	}

		/*get terminal size*/
	struct winsize size;
	if(ioctl(0, TIOCGWINSZ, (char *)&size) < 0){
		fprintf(stderr, "Error getting winsize!\n");
		exit(1);
	}
	scr_height = size.ws_row;
	scr_width = size.ws_col;

		/*deal with UTF-8*/
	setlocale(LC_CTYPE, "");
	initscr();
	if(has_colors()){
		start_color();
		init_pair(1, COLOR_BLACK, COLOR_BLUE);
	}
		/*turn to page if -p is set*/
	if(fileOffset > -1){
		int toPage = currentPageNum;
		int toOffset = fileOffset;
		currentPageNum = 0;
		fileOffset = -1;
		while(1){
			turnPage(book);
			toPage = currentPageNum;
			if(fileOffset >= toOffset){
				break;
			}
		}
		rewind(book);
		currentPageNum = 0;
		for(int i = 0; i < toPage; i++){
			turnPage(book);
		}
		
	}else{
			/*if -p is not set, then just go to page 1*/
		turnPage(book);
	}
		/*no highlighting to be done, so pass NULL and -1*/
	drawAll(NULL, -1);
	refresh();

	while(1){
			/*a single character command like n or b will execute without a carriage return*/
		char command = getch();
			/*n for "next page"*/
		if(command == 'n'){
			if(!bookmarksOpen){
				turnPage(book);
				drawAll(NULL, -1);
			}else{
				bookmarkSelect++;
				drawAll(NULL, -1);
				
				char *bookmarkPath = calloc(strlen(getenv("HOME")) + strlen("/.books/bookmarks/") + strlen(basename(argv[1])) + strlen(".bookmark"), 1);
				strcat(bookmarkPath, getenv("HOME"));
				strcat(bookmarkPath, "/.books/bookmarks/");
				strcat(bookmarkPath, basename(argv[1]));
				strcat(bookmarkPath, ".bookmark");
				FILE *bookmarkFile = fopen(bookmarkPath, "r");
				if(bookmarkFile != NULL){
					showBookMarks(bookmarkFile, bookmarkSelect);
					fclose(bookmarkFile);
					mvprintw(scr_height - 1, scr_width -1, " ");
				}
			}
			refresh();
		}else if(command == 'b'){
				/*b for "back"*/
			if(!bookmarksOpen){
				rewind(book);
				int tmpPageNum = currentPageNum - 1;
				currentPageNum = 0;
				for(int i = 0; i < tmpPageNum; i++){
					if(turnPage(book) == 0)
						break;
				}

				drawAll(NULL, -1);
			}else{

				bookmarkSelect--;
				drawAll(NULL, -1);
				
				char *bookmarkPath = calloc(strlen(getenv("HOME")) + strlen("/.books/bookmarks/") + strlen(basename(argv[1])) + strlen(".bookmark"), 1);
				strcat(bookmarkPath, getenv("HOME"));
				strcat(bookmarkPath, "/.books/bookmarks/");
				strcat(bookmarkPath, basename(argv[1]));
				strcat(bookmarkPath, ".bookmark");
				FILE *bookmarkFile = fopen(bookmarkPath, "r");
				if(bookmarkFile != NULL){
					showBookMarks(bookmarkFile, bookmarkSelect);
					fclose(bookmarkFile);
					mvprintw(scr_height - 1, scr_width -1, " ");
				}
			}
			refresh();
		}else if(command == 'g'){
			if(bookmarksOpen){
				fileOffset = readBookmark(basename(argv[1]), bookmarkSelect);
				
				int toPage = 0;
				rewind(book);
				int toOffset = fileOffset;
				currentPageNum = 0;
				fileOffset = -1;
				while(1){
					turnPage(book);
					toPage = currentPageNum;
					if(fileOffset >= toOffset){
						break;
					}
				}
				rewind(book);
				currentPageNum = 0;
				for(int i = 0; i < toPage; i++){
					turnPage(book);
				}
				drawAll(NULL, -1);
				refresh();	
				bookmarksOpen = 0;
			}
		}else if(command == ':'){
				/*a semicolon or forward slash is used to execute more
				 *complicated commands, like searching or navigation
				 *and requires a full string plus a carriage return*/
			char *options = malloc(80);
			mvprintw(scr_height-1, 2, ":");
			getstr(options);
				/*g for "goto page#"*/
			if(options[0] == 'g' && strlen(options) > 1){
				options++;
				rewind(book);
				int tmpPageNum = atoi(options);
				if(tmpPageNum != 0){
					currentPageNum = 0;
					for(int i = 1; i <= tmpPageNum; i++){
						if(turnPage(book) == 0)
							break;
					}
				}
				drawAll(NULL, -1);
				refresh();
				options--;
				bookmarksOpen = 0;
				free(options);
			}else if(options[0] == 'q'){
					/*q for quit*/
				free(options);
				break;
			}else if(options[0] == 's'){
					/*s for saving a bookmark*/
				saveBookmark(basename(argv[1]));
				bookmarksOpen = 0;
				drawAll(NULL, -1);
				refresh();
				free(options);
			}else if(options[0] == 'b'){

				if(!bookmarksOpen){
	drawAll(NULL, -1);
					bookmarkSelect = 0;
					char *bookmarkPath = calloc(strlen(getenv("HOME")) + strlen("/.books/bookmarks/") + strlen(basename(argv[1])) + strlen(".bookmark"), 1);
					strcat(bookmarkPath, getenv("HOME"));
					strcat(bookmarkPath, "/.books/bookmarks/");
					strcat(bookmarkPath, basename(argv[1]));
					strcat(bookmarkPath, ".bookmark");
					FILE *bookmarkFile = fopen(bookmarkPath, "r");
					if(bookmarkFile != NULL){
						showBookMarks(bookmarkFile, bookmarkSelect);
						fclose(bookmarkFile);
						bookmarksOpen = 1;
						mvprintw(scr_height - 1, scr_width -1, " ");
					}else{
						mvprintw(2, scr_width - 32, "*============================*");
						mvprintw(3, scr_width - 32, "| Error: no bookmarks found  |");
						mvprintw(4, scr_width - 32, "*============================*");
						mvprintw(scr_height-1, scr_width-1, " ");
					}
				}else{
					drawAll(NULL, -1);
					bookmarksOpen = 0;
				}
				refresh();
			}else{
					/*else, just go back to what we were doing*/
				bookmarksOpen = 0;
				drawAll(NULL, -1);
				refresh();
				free(options);
			}
		}else if(command == '/'){
			bookmarksOpen = 0;
				/*forward slash is used for searching*/
			char *options = malloc(80);
			mvprintw(scr_height-1, 2, "/");
			getstr(options);
			if(strlen(options) == 0){
				drawAll(NULL, -1);
				refresh();
				free(options);
			}else{
				int off = find(options, book, currentPageNum);
				drawAll(options, off);
				refresh();
				free(options);
			}
		}
	}
	
	endwin();
}

int find(char *phrase, FILE *book, int originalPage){
		/*-1 by default so that if nothing is found, no highlighting is done*/
	int j = -1;
	while(1){
			/*j is assigned to the return value of turnPage,
			 *which returns 0 when it reaches EOF, meaning the phrase
			 *was not found from this location onwards*/
		if(j == 0){
				/*check from beginning of the book?*/
			mvprintw((scr_height - 8)/2, (scr_width - 20)/2, 	"*==================*");
			mvprintw((scr_height - 8)/2 + 1, (scr_width - 20)/2,	"|  Could not find  |");
			mvprintw((scr_height - 8)/2 + 2, (scr_width - 20)/2,	"|  phrase entered. |");
			mvprintw((scr_height - 8)/2 + 3, (scr_width - 20)/2,	"|   restart from   |");
			mvprintw((scr_height - 8)/2 + 4, (scr_width - 20)/2,	"|    beginning?    |");
			mvprintw((scr_height - 8)/2 + 5, (scr_width - 20)/2,	"|                  |");
			mvprintw((scr_height - 8)/2 + 6, (scr_width - 20)/2,	"|       (Y/N)      |");
			mvprintw((scr_height - 8)/2 + 7, (scr_width - 20)/2,	"*==================*");
			char choice = getch();
			if(tolower(choice) == 'y'){
				rewind(book);
				currentPageNum = 0;
					/*recursively search until user says no, which returns -1, meaning no highlighting needs to be done*/
				return find(phrase, book, originalPage);
			}else{
					/*if no, then just return -1*/
				rewind(book);
				currentPageNum = 0;
				for(int i = 0; i < originalPage; i++){
					if(turnPage(book) == 0)
						break;
				}
				return -1;
			}
			return -1;
		}
		j = turnPage(book);

		char *location = strstr(currentPage, phrase);

			/*non-NULL means the phrase was found on this page.
			 *Return the starting address of the substring minus the
			 *starting address of the search phrase, to get the offset
			 *of the substring in the search phrase*/
		if(location != NULL){
			return (int)(location - currentPage);
		}
	}
}

void clearBackground(){
	for(int i = 0; i < scr_height; i++){
		for(int j = 0; j < scr_width; j++){
			mvprintw(i, j, " ");
		}
	}
}

void saveBookmark(char *bookname){
		/*save page # to ~/.books/bookmarks*/
	char *filePathSave = calloc(strlen(getenv("HOME")) + strlen("/.books/bookmarks/") + strlen(bookname) + strlen(".bookmark"), 1);
	strcat(filePathSave, getenv("HOME"));
	strcat(filePathSave, "/.books/bookmarks/");
	strcat(filePathSave, bookname);
	strcat(filePathSave, ".bookmark");

		/*Preserve history*/
	FILE *bookmark;
	bookmark = fopen(filePathSave, "r");
	char *oldBookmarks = "";

	if(bookmark !=NULL){
		oldBookmarks = getFileContents(bookmark);
		fclose(bookmark);
	}
	
		/*Delete contents*/
	bookmark = fopen(filePathSave, "w");
	
	if(bookmark == NULL){
		mvprintw(scr_height -1, 2, "ERROR: Couldn't save bookmark");
		exit(1);
		return;
	}
	
	fprintf(bookmark, "time=%d\nlocation=%d\n%s", (int)time(NULL), fileOffset, oldBookmarks);
	fclose(bookmark);
	free(filePathSave);
}

int readBookmark(char *bookname, int markNum){
		/*get Page # from ~/.books/bookmarks*/
	char *filePathSave = calloc(strlen(getenv("HOME")) + strlen("/.books/bookmarks/") + strlen(bookname) + strlen(".bookmark"), 1);
	strcat(filePathSave, getenv("HOME"));
	strcat(filePathSave, "/.books/bookmarks/");
	strcat(filePathSave, bookname);
	strcat(filePathSave, ".bookmark");
	FILE *bookmark = fopen(filePathSave, "r");
	if(bookmark == NULL){
		mvprintw(scr_height -1, 2, "ERROR: Couldn't open bookmark");
		return 1;
	}
	int location = -1;
	int time = -1;
	if(markNum == -1){
		fscanf(bookmark, "time=%d\n", &time);
		fscanf(bookmark, "location=%d\n", &location);
	}else{
		for(int i = 0; i <= markNum; i++){
			fscanf(bookmark, "time=%d\n", &time);
			fscanf(bookmark, "location=%d\n", &location);
		}
	}
	free(filePathSave);
	return location;
}

void drawBackground(){
	for(int i = 0; i < scr_height; i++){
		for(int j = 0; j < scr_width; j++){
			if((j + ((i % 2) * 4)) % 8 == 0){
				mvprintw(i, j, "~");
			}
		}
	}
}

void drawBorder(){
	mvprintw((scr_height - HEIGHT)/2 - 2, (scr_width - WIDTH)/2 - 2, "#");
	mvprintw((scr_height - HEIGHT)/2 - 2, (scr_width - WIDTH)/2 + WIDTH + 1, "#");
	mvprintw((scr_height - HEIGHT)/2 + HEIGHT + 1, (scr_width - WIDTH)/2 - 2, "#");
	mvprintw((scr_height - HEIGHT)/2 + HEIGHT + 1, (scr_width - WIDTH)/2 + WIDTH + 1, "#");
	for(int i = 0; i < HEIGHT + 2; i++){
		mvprintw((scr_height - HEIGHT)/2 + i-1, (scr_width - WIDTH)/2 - 2, "|");
		mvprintw((scr_height - HEIGHT)/2 + i-1, (scr_width - WIDTH)/2 + WIDTH + 1, "|");
	}
	for(int i = 0; i < WIDTH+2; i++){
		mvprintw((scr_height - HEIGHT)/2 -2, (scr_width - WIDTH)/2 + i - 1, "=");
		mvprintw((scr_height - HEIGHT)/2 + HEIGHT + 1, (scr_width - WIDTH)/2 + i - 1, "=");

	}
}

void drawReader(){
	for(int i = 0; i < HEIGHT+2; i++){
		for(int j = 0; j < WIDTH+2; j++){
			mvprintw((scr_height - HEIGHT)/2 + i-1, (scr_width - WIDTH)/2 + j-1, " ");
		}
	}
}

void drawText(char *highlighted, int offset){
	int currentLine = 0;
	int currentChar = 0;
	for(int i = 0; i < WIDTH * HEIGHT; i++){
			/*if we've hit a newline, if we've hit the width of the reader, or if we've hit the end of the string,
			 *go to a new line*/
		if(currentPage[i] == '\n' || currentChar == WIDTH || currentPage[i] == '\0'){
			currentLine++;
			currentChar = 0;
		}
		
			/*write any character that's not a newline. newlines are just dealt with by incrementing currentLine*/
		if(currentPage[i] != '\n'){
				/*highlight when offset is not negative, AND when highlighted is provided. If find returns -1, nothing
				 *will be highlighted even if a search term was provided*/
			if(highlighted != NULL && offset > -1 && has_colors()){
				if(i >= offset && i < offset + strlen(highlighted))
					/*highlight color is blue*/
				attron(COLOR_PAIR(1));
			}
			mvprintw((scr_height - HEIGHT)/2 + currentLine, (scr_width - WIDTH)/2 + currentChar, "%c", currentPage[i]);
			currentChar++;
				/*reset color*/
			attroff(COLOR_PAIR(1));
		}
			/*once we've hit the end of the reader, return*/
		if((((scr_height - HEIGHT)/2) + currentLine) >= ((scr_height - HEIGHT)/2 + HEIGHT)){
			return;
		}
	}

}

void drawAll(char *highlighted, int offset){
	clearBackground();
	drawBackground();
	drawReader();
	drawBorder();
	drawText(highlighted, offset);
		/*print the page #*/
	mvprintw(2, 1, ":");
	for(int i = 0; i < strlen("Page #") + GetNumberOfDigits(currentPageNum) + 4; i++){
		mvprintw(1, i+1, "=");
		mvprintw(3, i+1, "=");
	}
	mvprintw(2, strlen("Page #") + GetNumberOfDigits(currentPageNum) + 4, ":");
	mvprintw(2,3, "Page #%d", currentPageNum);

	mvprintw(scr_height - 1, scr_width - 1, " ");
}

int turnPage(FILE *book){
		/*increment the apge*/
	fileOffset = ftell(book) + 1;
	currentPageNum++;
	int currentChar = 0;
	int currentLine = 0;
		/*empty out the page*/
	memset(currentPage, 0, WIDTH * HEIGHT);
	
	char c;
		/*read a MAX of the area of the reader*/
	for(int i = 0; i < WIDTH * HEIGHT; i++){
		c = fgetc(book);
			/*returning 0 will signal to find() that we need to start our search over again*/
		if(c == EOF){
			return 0;
		}
			/*increment line on newline and when we've hit the edge of the reader*/
		if(c == '\n' || (currentChar + 1) % WIDTH == 0){
			currentLine++;
			currentChar = 0;
		}
		currentChar++;
		currentPage[i] = c;
			/*once we've hit the end, return 1, indicating the book is not over*/
		if(currentLine == HEIGHT)
			return 1;
	}
}

char *getOption(int argc, char **argv, char *desired_arg){
	for(int i = 0; i < argc; i++){
		if(strcmp(desired_arg, argv[i]) == 0){
			if(argc == i + 1){
				return "1";
			}else{
				return argv[i+1];
			}
		}
	}
	return NULL;
}

unsigned int GetNumberOfDigits (unsigned int i){
    return i > 0 ? (int) log10 ((double) i) + 1 : 1;
}

char *getFileContents(FILE *file){
	rewind(file);
	char c;
	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	char *contents = calloc(size + 1, 1);
	int i = 0;
	rewind(file);
	while((c = fgetc(file)) != EOF){
		contents[i++] = c;
	}
	rewind(file);
	return contents;
}

void showBookMarks(FILE *bookmarkFile, int index){
	rewind(bookmarkFile);
	int time = 0;
	int location = 0;
	int i = 0;
	mvprintw(i++, scr_width - 30, "*============================*");
	mvprintw(i++, scr_width - 30, "|           Bookmarks         ");
	mvprintw(i++, scr_width - 30, "*============================*");
	while(fscanf(bookmarkFile, "time=%d\nlocation=%d\n", &time, &location) != EOF && i != 10){
		if(index == i - 3){
			attron(COLOR_PAIR(1));
		}
		mvprintw(i, scr_width - 30, "| ");
		mvprintw(i, scr_width - 28, "@ %d", location);
		for(int j = 0; j < 26 - GetNumberOfDigits(location); j++){
			mvprintw(i, scr_width - (26 - GetNumberOfDigits(location)) + j, " ");
		}
		i++;
		attroff(COLOR_PAIR(1));
	}
	mvprintw(i++, scr_width - 30, "*============================*");

	mvprintw(scr_height - 1, scr_width -1, " ");
	refresh();
}
