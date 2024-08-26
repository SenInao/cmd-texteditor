#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <stdlib.h>
#include <conio.h>

#define BUFFER_SIZE 1024
#define INITIAL_CAPACITY 10

typedef struct {
  int columns;
  int rows;
} ScreenSize;

ScreenSize getScreenSize(CONSOLE_SCREEN_BUFFER_INFO csbi) {
    ScreenSize size;
    size.columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    size.rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    return size;
}

int printBuffer(char** buffer, int lineCount, HANDLE hConsole) {
  COORD startPos = {0, 0};

  system("cls");
  SetConsoleCursorPosition(hConsole, startPos);

  for (int i = 0; i < lineCount; i++) {
    printf("%s", buffer[i]);
  }

  return 0;
}

int main() {
  HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  COORD cursorPos = {0, 0};
  char filename[256];
  char **buffer = NULL;
  int lineCount = 0;
  int capacity = INITIAL_CAPACITY;

  buffer = malloc(capacity * sizeof(char*));
  for (int i = 0; i < capacity; i++) {
    buffer[i] = (char*)malloc(BUFFER_SIZE);
  }

  if (hConsole == INVALID_HANDLE_VALUE) {
    fprintf(stderr, "Error getting console handle\n");
    return 1;
  }
  if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) {
    fprintf(stderr, "Error getting console screen buffer info\n");
    return 1;
  }

  system("cls");
  SetConsoleCursorPosition(hConsole, cursorPos); 

  printf("Enter filename to edit: ");
  scanf("%s", filename);

  FILE *fpointer = fopen(filename, "r");
  if (fpointer == NULL) {
    fprintf(stderr, "Error opening file\n");
  }

  while (fgets(buffer[lineCount], BUFFER_SIZE, fpointer) != NULL) {
    lineCount++;
    if (lineCount>=capacity) {
      capacity+=1;
      buffer = realloc(buffer, capacity * sizeof(char*));
      for (int i = lineCount; i < capacity; i++) {
        buffer[i] = (char*)malloc(BUFFER_SIZE);
      }
    }
  }


  int c;
  boolean bufferChanged = 1;
  while (1) {

    if (bufferChanged) {
      printBuffer(buffer, lineCount, hConsole);
    }
    SetConsoleCursorPosition(hConsole, cursorPos); 

    bufferChanged = 0;
    c = _getch();

    if (c == 27) {
      break;
    }

    if (c == 0 || c == 224) { // Extended key prefix
      c = _getch();
      switch (c) {
        case 75:
          if (cursorPos.X == 0) {
            break;
          }
          cursorPos.X--;
          break;
        case 72:
          if (cursorPos.Y == 0) {
            break;
          }
          cursorPos.Y--;
          if (cursorPos.X > strlen(buffer[cursorPos.Y])-1) {
            cursorPos.X = strlen(buffer[cursorPos.Y])-1;
          }
          break;
        case 77:
          if (cursorPos.X >= strlen(buffer[cursorPos.Y])) {
            break;
          }
          cursorPos.X++;
          break;
        case 80:
          cursorPos.Y++;
          if (cursorPos.X > strlen(buffer[cursorPos.Y])-1) {
            cursorPos.X = strlen(buffer[cursorPos.Y])-1;
          }
          break;
      }
    } else {
      if (c >= 32 && c <=126) {
        memmove(&buffer[cursorPos.Y][cursorPos.X+1], &buffer[cursorPos.Y][cursorPos.X], strlen(&buffer[cursorPos.Y][cursorPos.X])+1);
        buffer[cursorPos.Y][cursorPos.X] = c;
        cursorPos.X++;

      } else if (c == 8) { // Backspace key
        if (cursorPos.X == 0 && cursorPos.Y > 0) {
          int prevLineLen = strlen(buffer[cursorPos.Y - 1]);
          int currentLineLen = strlen(buffer[cursorPos.Y]);

          // Check if there's enough space in the previous line
          if (prevLineLen + currentLineLen < BUFFER_SIZE) {
            // Append current line to the end of previous line
            memcpy(buffer[cursorPos.Y - 1] + prevLineLen, buffer[cursorPos.Y], currentLineLen);
            buffer[cursorPos.Y - 1][prevLineLen + currentLineLen] = '\0';  // Null-terminate

            // Shift all lines after current line up by one
            for (int i = cursorPos.Y; i < capacity- 1; i++) {
              memcpy(buffer[i], buffer[i + 1], capacity);
            }
            memset(buffer[capacity- 1], 0, capacity);  // Clear last line

            // Move cursor to end of previous line
            cursorPos.Y--;
            cursorPos.X = prevLineLen;
          }
        } else if (cursorPos.X > 0) {
          // Remove character before cursor
          memmove(&buffer[cursorPos.Y][cursorPos.X - 1], 
                  &buffer[cursorPos.Y][cursorPos.X], 
                  capacity- cursorPos.X);
          cursorPos.X--;
        }
      }
      bufferChanged = 1;
    }
  }

  fclose(fpointer);

  return 0;
}
