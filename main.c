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
    printf("%s\n", buffer[i]);
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
    size_t len = strlen(buffer[lineCount]);
    if (len > 0 && buffer[lineCount][len - 1] == '\n') {
        buffer[lineCount][len - 1] = '\0';
    }
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
      cursorPos.Y = lineCount;
      SetConsoleCursorPosition(hConsole, cursorPos); 
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
          if (cursorPos.X > strlen(buffer[cursorPos.Y])) {
            cursorPos.X = strlen(buffer[cursorPos.Y]);
          }
          break;
        case 77:
          if (cursorPos.X >= strlen(buffer[cursorPos.Y])) {
            break;
          }
          cursorPos.X++;
          break;
        case 80:
          if (cursorPos.Y == lineCount-1) {
            break;
          }
          cursorPos.Y++;
          if (cursorPos.X > strlen(buffer[cursorPos.Y])) {
            cursorPos.X = strlen(buffer[cursorPos.Y]);
          }
          break;
      }
    } else {
      if (c >= 32 && c <=126) {
        memmove(&buffer[cursorPos.Y][cursorPos.X+1], &buffer[cursorPos.Y][cursorPos.X], strlen(&buffer[cursorPos.Y][cursorPos.X])+1);
        buffer[cursorPos.Y][cursorPos.X] = c;
        cursorPos.X++;

      } else if (c == 8) {
        if (cursorPos.X == 0 && cursorPos.Y > 0) {
          int prevLineLen = strlen(buffer[cursorPos.Y - 1]);
          int currentLineLen = strlen(buffer[cursorPos.Y]);

          if (prevLineLen + currentLineLen < BUFFER_SIZE) {
            strcat(buffer[cursorPos.Y - 1], buffer[cursorPos.Y]);
            for (int i = cursorPos.Y; i < lineCount - 1; i++) {
              strcpy(buffer[i], buffer[i + 1]);
            }
            lineCount--;
            cursorPos.Y--;
            cursorPos.X = prevLineLen;
          }

        } else if (cursorPos.X > 0) {
          memmove(&buffer[cursorPos.Y][cursorPos.X - 1], 
                  &buffer[cursorPos.Y][cursorPos.X], strlen(buffer[cursorPos.Y]));
          cursorPos.X--;
        } 
      } else if (c == 13) {
        for (int i = lineCount; i > cursorPos.Y; i--) {
          strcpy(buffer[i], buffer[i - 1]);
        }
        strcpy(buffer[cursorPos.Y+1], &buffer[cursorPos.Y][cursorPos.X]);

        buffer[cursorPos.Y][cursorPos.X] = '\0';

        cursorPos.X = 0;
        cursorPos.Y++;
        lineCount++;
      }

      bufferChanged = 1;
    }
  }
  fclose(fpointer);

  return 0;
}
