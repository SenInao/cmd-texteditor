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

int printBuffer(char** buffer, int lineCount, HANDLE hConsole, int startLine, ScreenSize size) {
  COORD startPos = {0, 0};

  system("cls");
  SetConsoleCursorPosition(hConsole, startPos);

  for (int i = startLine; i < lineCount; i++) {
    if (i - startLine == size.rows - 1) {
      break;
    }
    printf("%s\n", buffer[i]);
  }

  return 0;
}

void printInfo(HANDLE hConsole, CONSOLE_SCREEN_BUFFER_INFO csbi, int saved, char filename[], char isSaved[]) {
  ScreenSize size = getScreenSize(csbi);
  COORD bottom = {0, size.rows-1};
  SetConsoleCursorPosition(hConsole, bottom);
  memset(isSaved, 0, sizeof(&isSaved));
  if (saved) {
    strcat(isSaved, "SAVED");
  } else {
    strcat(isSaved, "EDITED, ctrl+s to save");
  }
  printf("%s | %s", filename, isSaved);
}

void readFileToBuffer(char filename[], char ***buffer, int *lineCount, int *capacity) {
  FILE *fpointer = fopen(filename, "r");
  if (fpointer == NULL) {
    fprintf(stderr, "Error opening file\n");
  }

  while (fgets((*buffer)[*lineCount], BUFFER_SIZE, fpointer) != NULL) {
    size_t len = strlen((*buffer)[*lineCount]);
    if (len > 0 && (*buffer)[*lineCount][len - 1] == '\n') {
        (*buffer)[*lineCount][len - 1] = '\0';
    }
    (*lineCount)++;
    if (*lineCount>=*capacity) {
      *capacity+=1;
      *buffer = realloc((*buffer), (*capacity) * sizeof(char*));
      for (int i = *lineCount; i < *capacity; i++) {
        (*buffer)[i] = (char*)malloc(BUFFER_SIZE);
      }
    }
  }

  fclose(fpointer);
}

void saveBuffer(char filename[], char **buffer, int lineCount) {
  FILE *fpointer = fopen(filename, "w");
  for (int i = 0; i < lineCount; i++) {
    fprintf(fpointer, "%s\n", buffer[i]);
  }
  fclose(fpointer);
}

int main() {
  HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  COORD cursorPos = {0, 0};
  char filename[256];
  char **buffer = NULL;
  int startLine = 0;
  int lineCount = 0;
  int capacity = INITIAL_CAPACITY;
  char isSaved[30];

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

  readFileToBuffer(filename, &buffer, &lineCount, &capacity);

  int c;
  boolean bufferChanged = 1;
  boolean saved = 1;
  ScreenSize size = getScreenSize(csbi);
  
  while (1) {
    size = getScreenSize(csbi);

    if (bufferChanged) {
      printBuffer(buffer, lineCount, hConsole, startLine, size);
      printInfo(hConsole, csbi, saved, filename, isSaved);
    }
    SetConsoleCursorPosition(hConsole, cursorPos); 

    bufferChanged = 0;
    c = _getch();

    int realY = cursorPos.Y + startLine;

    if (c == 27) {
      cursorPos.Y = 0;
      system("cls");
      SetConsoleCursorPosition(hConsole, cursorPos); 
      break;
    }

    if (c == 0 || c == 224) { // Extended key prefix
      c = _getch();
      switch (c) {
        case 75: //LEFT arrow
          if (cursorPos.X == 0) {
            break;
          }
          cursorPos.X--;
          break;
        case 72: //UP arrow
          if (cursorPos.Y == 0) {
            if (startLine > 0) {
              startLine--;
              bufferChanged = 1;
            }
            break;
          }
          cursorPos.Y--;
          if (cursorPos.X > strlen(buffer[realY-1])) {
            cursorPos.X = strlen(buffer[realY-1]);
          }
          break;
        case 77: //RIGHT arrow
          if (cursorPos.X >= strlen(buffer[realY])) {
            break;
          }
          cursorPos.X++;
          break;
        case 80: //DOWN arrow
          if (realY == lineCount-1) {
            break;
          } 
          if (cursorPos.Y >= size.rows-2 && cursorPos.Y < lineCount-1) {
            startLine++;
            bufferChanged = 1;
            break;
          }
          cursorPos.Y++;
          if (cursorPos.X > strlen(buffer[realY+1])) {
            cursorPos.X = strlen(buffer[realY+1]);
          }
          break;
      }

    } else {
      saved = 0;
      if (c >= 32 && c <=126) { //Any writable character
        memmove(&buffer[realY][cursorPos.X+1], &buffer[realY][cursorPos.X], strlen(&buffer[realY][cursorPos.X])+1);
        buffer[realY][cursorPos.X] = c;
        cursorPos.X++;

      } else if (c == 8) { //BACKSPACE
        if (cursorPos.X == 0 && cursorPos.Y + startLine > 0) { //Go to previous line if backspace on first character
          int prevLineLen = strlen(buffer[realY - 1]);
          int currentLineLen = strlen(buffer[realY]);

          if (prevLineLen + currentLineLen < BUFFER_SIZE) {
            strcat(buffer[realY - 1], buffer[realY]);
            for (int i = realY; i < lineCount - 1; i++) {
              strcpy(buffer[i], buffer[i + 1]);
            }
            lineCount--;
            cursorPos.X = prevLineLen;
            
            if (cursorPos.Y == 0) {
              startLine--;
            } else {
              cursorPos.Y--;
            }
          }

        } else if (cursorPos.X > 0) {
          memmove(&buffer[realY][cursorPos.X - 1], 
                  &buffer[realY][cursorPos.X], strlen(buffer[realY]));
          cursorPos.X--;
        } 

      } else if (c == 13) { //ENTER

        if (lineCount == capacity) { //Realloc if needed
          capacity+=1;
          buffer = realloc(buffer, capacity * sizeof(char*));
          for (int i = lineCount; i < capacity; i++) {
            buffer[i] = (char*)malloc(BUFFER_SIZE);
          }
        }

        for (int i = lineCount; i > realY; i--) {
          strcpy(buffer[i], buffer[i - 1]);
        }
        strcpy(buffer[realY+1], &buffer[realY][cursorPos.X]);

        buffer[realY][cursorPos.X] = '\0';

        cursorPos.X = 0;
        lineCount++;
        if (realY - startLine < size.rows - 2) {
          cursorPos.Y++;
        } else {
          startLine++;
        }

      } else if (c == 19) { //CTRL+S
        saveBuffer(filename, buffer, lineCount);
        saved = 1;
      }
      bufferChanged = 1;
    }
  }

  return 0;
}
