/*
This is a console based graphics engine that I built in order to
render some simple games and simulations to the screen with.  It's not very 
powerful and becomes a major performance bottleneck but it's a fun way to start
off on graphics programming.
Much of the actual windows API programming was 'borrowed' from OneLoneCoder on Youtube (link below).

-Zach

https://github.com/OneLoneCoder
*/

#pragma once

#ifndef UNICODE
#error Unicode is disabled
#endif

#include <iostream>
#include <utility>
#include <chrono>
#include <vector>
#include <list>
#include <thread>
#include <atomic>
#include <string>
#include <Windows.h>

// color library
enum COLOUR
{
   FG_BLACK = 0x0000,
   FG_DARK_BLUE = 0x0001,
   FG_DARK_GREEN = 0x0002,
   FG_DARK_CYAN = 0x0003,
   FG_DARK_RED = 0x0004,
   FG_DARK_MAGENTA = 0x0005,
   FG_DARK_YELLOW = 0x0006,
   FG_GREY = 0x0007,
   FG_DARK_GREY = 0x0008,
   FG_BLUE = 0x0009,
   FG_GREEN = 0x000A,
   FG_CYAN = 0x000B,
   FG_RED = 0x000C,
   FG_MAGENTA = 0x000D,
   FG_YELLOW = 0x000E,
   FG_WHITE = 0x000F,
   BG_BLACK = 0x0000,
   BG_DARK_BLUE = 0x0010,
   BG_DARK_GREEN = 0x0020,
   BG_DARK_CYAN = 0x0030,
   BG_DARK_RED = 0x0040,
   BG_DARK_MAGENTA = 0x0050,
   BG_DARK_YELLOW = 0x0060,
   BG_GREY = 0x0070,
   BG_DARK_GREY = 0x0080,
   BG_BLUE = 0x0090,
   BG_GREEN = 0x00A0,
   BG_CYAN = 0x00B0,
   BG_RED = 0x00C0,
   BG_MAGENTA = 0x00D0,
   BG_YELLOW = 0x00E0,
   BG_WHITE = 0x00F0,
};

// commonly used character library
enum SYMBOLS
{
   SQUARE = 0x2588,
   PHI = 1012,
};

// for key and mouse input
struct keyState
{
   bool pressed;
   bool released;
   bool held;
};

class ConsoleGraphicsEngine
{
protected:

   const float PI = 3.14159;
   CHAR_INFO* screen;
   int screenSize;
   HANDLE hConsoleOut, hConsole_Size, hConsoleIn, hConsoleInFocus;
   DWORD dwBytesWritten;
   SMALL_RECT winRect;
   COORD winCoords;
   CONSOLE_FONT_INFOEX cfi; // font handler

   keyState keys[256], mouse[5];
   short keyNewState[256] = { 0 };
   short keyOldState[256] = { 0 };
   bool mouseOldState[5] = { 0 };
   bool mouseNewState[5] = { 0 };

private:
   int mousePosX = 0, mousePosY = 0;

public:
   short screenWidth, screenHeight;
   wchar_t blankChar;   // char that engine assumes as blank, default constructs to ' '
   short blankColor;    // blank canvas color, default constructs to black


   ConsoleGraphicsEngine()
   {
      blankChar = ' ';
      blankColor = FG_BLACK;

      hConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
      hConsoleIn = GetStdHandle(STD_INPUT_HANDLE);
   }

   ConsoleGraphicsEngine(wchar_t blankCharIn, short blankColorIn)
   {
      blankChar = blankCharIn;
      blankColor = blankColorIn;

      hConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
      hConsoleIn = GetStdHandle(STD_INPUT_HANDLE);
   }

   void ConstructConsole(short screenWidthIn, short screenHeightIn, int fontw, int fonth)
   {
      screenWidth = screenWidthIn;
      screenHeight = screenHeightIn;
      screenSize = screenWidth * screenHeight;

      winRect = { 0, 0, 1, 1 };
      SetConsoleWindowInfo(hConsoleOut, TRUE, &winRect);

      // setting up screen buffer
      winCoords = { screenWidth, screenHeight };
      SetConsoleScreenBufferSize(hConsoleOut, winCoords);
      SetConsoleActiveScreenBuffer(hConsoleOut);

      // screen font resize
      CONSOLE_FONT_INFOEX cfi;
      cfi.cbSize = sizeof(cfi);
      cfi.nFont = 0;
      cfi.dwFontSize.X = fontw;
      cfi.dwFontSize.Y = fonth;
      cfi.FontFamily = FF_DONTCARE;
      cfi.FontWeight = FW_NORMAL;
      wcscpy_s(cfi.FaceName, L"Console");
      SetCurrentConsoleFontEx(hConsoleOut, false, &cfi);

      // resize the window
      winRect = { 0, 0, screenWidth - 1, screenHeight - 1 };
      SetConsoleWindowInfo(hConsoleOut, TRUE, &winRect);

      // mouse input enable
      SetConsoleMode(hConsoleIn, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);

      // allocating screen array memory
      screen = new CHAR_INFO[screenSize];
      memset(screen, 0, sizeof(CHAR_INFO) * screenSize);

      ClearScreen();
   }

   // ---------------------------------------------------------------------------
   //
   // Getting keyboard input / mouse input
   //
   // ---------------------------------------------------------------------------

   // processes key/mouse states
   void ProcessKeys()
   {
      // keyboard strokes
      for (int i = 0; i < 256; i++)
      {
         keyNewState[i] = GetAsyncKeyState(i);

         keys[i].pressed = false;
         keys[i].released = false;

         // if key state has changed, keystate is updated
         if (keyNewState[i] != keyOldState[i])
         {
            if (keyNewState[i] & 0x8000)
            {
               keys[i].pressed = !keys[i].held;
               keys[i].held = true;
            }
            else
            {
               keys[i].released = true;
               keys[i].held = false;
            }
         }

         keyOldState[i] = keyNewState[i];
      }

      // setting up mouse input
      INPUT_RECORD input[32];
      DWORD events = 0;
      GetNumberOfConsoleInputEvents(hConsoleIn, &events);
      if (events > 0) ReadConsoleInput(hConsoleIn, input, events, &events);

      for (DWORD i = 0; i < events; i++)
      {
         switch (input[i].EventType)
         {
         case MOUSE_EVENT:
         {
            switch (input[i].Event.MouseEvent.dwEventFlags)
            {
            case MOUSE_MOVED:
            {
               mousePosX = input[i].Event.MouseEvent.dwMousePosition.X;
               mousePosY = input[i].Event.MouseEvent.dwMousePosition.Y;
            }
            break;
            case 0:
            {
               for (int m = 0; m < 5; m++)
                  mouseNewState[m] = (input[i].Event.MouseEvent.dwButtonState & (1 << m)) > 0;
            }
            break;
            default:
               break;
            }
         }
         break;
         default:
            break;
         }
      }
      for (int j = 0; j < 5; j++)
      {
         mouse[j].pressed = false;
         mouse[j].released = false;
         if (mouseNewState[j] != mouseOldState[j])
         {
            if (mouseNewState[j])
            {
               mouse[j].pressed = true;
               mouse[j].held = true;
            }
            else
            {
               mouse[j].released = true;
               mouse[j].held = false;
            }
         }
         mouseOldState[j] = mouseNewState[j];
      }
   }

   // gets keystate for selected key
   keyState GetKeyState(int key) { return keys[key]; }

   // mouse coord getters and mouse button state retriever
   int MouseXPos() { return mousePosX; }
   int MouseYPos() { return mousePosY; }
   keyState GetMouseState(int key) { return mouse[key]; }

   // ---------------------------------------------------------------------------
   //
   // Basic pixel read and write methods
   //
   // ---------------------------------------------------------------------------


   // converts a given x,y point to the 1D array and plots a character
   void PaintPixel(int x, int y, short pixelChar = 0x2588, short color = 0x000F)
   {
      // uncomment for default x and y filtering
      //if (x >= screenWidth || y >= screenHeight || x < 0 || y < 0) return;

      // uncomment for 'taurus' rendering (buggy**)
      //if (x < 0) x += screenWidth - 1;
      //else if (x > screenWidth) x += 0;
      //if (y < 0) y += screenHeight - 1;
      //else if (y > screenHeight) y += 0;

      if (x >= screenWidth || y >= screenHeight || x < 0 || y < 0) return;

      screen[screenWidth * y + x].Char.UnicodeChar = pixelChar;
      screen[screenWidth * y + x].Attributes = color;
   }
   // pixel deletion
   void ClearScreenPixel(int x, int y)
   {
      screen[screenWidth * y + x].Char.UnicodeChar = blankChar;
      screen[screenWidth * y + x].Attributes = blankColor;
   }

   // returns the type of pixel char located at the coords
   char GetPixel(int x, int y)
   {
      return screen[screenWidth * y + x].Char.UnicodeChar;
   }

   // renders current screen image
   void OutputConsole()
   {
      WriteConsoleOutput(hConsoleOut, screen, { (short)screenWidth, (short)screenHeight }, { 0,0 }, &winRect);
   }

   // clears entire screen - sets to blank character
   void ClearScreen()
   {
      for (int i = 0; i < screenSize; i++)
      {
         screen[i].Char.UnicodeChar = blankChar;
         screen[i].Attributes = blankColor;
      }
      screen[screenSize - 1].Char.UnicodeChar = '\0';
   }

   // ---------------------------------------------------------------------------
   //
   // Rendering Methods
   //
   // ---------------------------------------------------------------------------

   // basic fill command: fills rectangle between two points.  (x1 < x2) , (y1 < y2)
   void Fill(int x1, int y1, int x2, int y2, short inputChar = 0x2588, short color = 0x000F)
   {
      int ystart = y1;
      while (x1 <= x2)
      {
         y1 = ystart;
         while (y1 <= y2)
         {
            PaintPixel(x1, y1, inputChar, color);
            y1++;
         }
         x1++;
      }
   }
   // encircles screen area with a border
   void DrawBorder(short borderChar = 0x2588, short color = 0x000F)
   {
      for (int i = 0; i <= screenWidth; i++)
      {
         if (i <= screenHeight)
         {
            screen[i].Char.UnicodeChar = borderChar;
            screen[i * screenWidth].Char.UnicodeChar = borderChar;
            screen[i * screenWidth + screenWidth - 1].Char.UnicodeChar = borderChar;
            screen[i].Attributes = color;
            screen[i * screenWidth].Attributes = color;
            screen[i * screenWidth + screenWidth - 1].Attributes = color;
         }
         else
         {
            screen[i].Char.UnicodeChar = borderChar;
            screen[i].Attributes = color;
         }
      }
      for (int x = screenSize - screenWidth; x < screenSize; x++)
      {
         screen[x].Char.UnicodeChar = borderChar;
         screen[x].Attributes = color;
      }
   }

   // plots a line between two points, brensenham line algorithm
   void DrawLine(int x1, int y1, int x2, int y2, short inputChar = 0x2588, short color = 0x000F)
   {
      int dy = y2 - y1, dx = x2 - x1;

      if (abs(dy) < abs(dx))
      {
         if (x1 > x2)
            DrawLineLow(x2, y2, x1, y1, inputChar, color);
         else
            DrawLineLow(x1, y1, x2, y2, inputChar, color);
      }
      else if (y1 > y2)
         DrawLineHigh(x2, y2, x1, y1, inputChar, color);
      else
         DrawLineHigh(x1, y1, x2, y2, inputChar, color);
   }
   // some private sub-functions for the line algorithm
private:
   void DrawLineLow(int x1, int y1, int x2, int y2, short inputChar = 0x2588, short color = 0x000F)
   {
      int dy = y2 - y1, dx = x2 - x1;
      int D;
      int yi = 1;
      int y = y1;;

      if (dy < 0)
      {
         yi = -1;
         dy *= -1;
      }
      D = 2 * dy - dx;

      for (int x = x1; x <= x2; x++)
      {
         PaintPixel(x, y, inputChar, color);
         if (D > 0)
         {
            y += yi;
            D += 2 * (dy - dx);
         }
         else
            D += 2 * dy;
      }
   }
   void DrawLineHigh(int x1, int y1, int x2, int y2, short inputChar = 0x2588, short color = 0x000F)
   {
      int dy = y2 - y1, dx = x2 - x1;
      int D;
      int xi = 1;
      int x = x1;;

      if (dx < 0)
      {
         xi = -1;
         dx *= -1;
      }
      D = 2 * dx - dy;

      for (int y = y1; y <= y2; y++)
      {
         PaintPixel(x, y, inputChar, color);
         if (D > 0)
         {
            x += xi;
            D += 2 * (dx - dy);
         }
         else
            D += 2 * dx;
      }
   }

public:
   // constructs a triangle outline
   void DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, short inputChar = 0x2588, short color = 0x000F)
   {
      DrawLine(x1, y1, x2, y2, inputChar, color);
      DrawLine(x2, y2, x3, y3, inputChar, color);
      DrawLine(x3, y3, x1, y1, inputChar, color);
   }

   // draws outline of a circle
   void DrawCircle(int xc, int yc, int r, short inputChar = 0x2588, short color = 0x000F)
   {
      int x = 0, y = r;
      int d = 3 - 2 * r;
      Circle(xc, yc, x, y, inputChar, color);
      while (y >= x)
      {
         if (d > 0) d += 4 * (x++ - y--) + 10;
         else d += 4 * x++ + 6;
         Circle(xc, yc, x, y, inputChar, color);
      }
   }

private:
   // circle subfunction drawing eigth points
   void Circle(int xc, int yc, int x, int y, short inputChar = 0x2588, short color = 0x000F)
   {
      PaintPixel(xc - x, yc - y, inputChar, color);
      PaintPixel(xc - y, yc - x, inputChar, color);
      PaintPixel(xc + y, yc - x, inputChar, color);
      PaintPixel(xc + x, yc - y, inputChar, color);
      PaintPixel(xc - x, yc + y, inputChar, color);
      PaintPixel(xc - y, yc + x, inputChar, color);
      PaintPixel(xc + y, yc + x, inputChar, color);
      PaintPixel(xc + x, yc + y, inputChar, color);
   }
public:
   // constructs a filled in triangle.  taken from source below
   // https://www.avrfreaks.net/sites/default/files/triangles.c
   void FillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, short inputChar = 0x2588, short color = 0x000F)
   {
      auto SWAP = [](int& x, int& y) { int t = x; x = y; y = t; };
      auto drawline = [&](int sx, int ex, int ny) { for (int i = sx; i <= ex; i++) PaintPixel(i, ny, inputChar, color); };

      int t1x, t2x, y, minx, maxx, t1xp, t2xp;
      bool changed1 = false;
      bool changed2 = false;
      int signx1, signx2, dx1, dy1, dx2, dy2;
      int e1, e2;
      // Sort vertices
      if (y1 > y2) { SWAP(y1, y2); SWAP(x1, x2); }
      if (y1 > y3) { SWAP(y1, y3); SWAP(x1, x3); }
      if (y2 > y3) { SWAP(y2, y3); SWAP(x2, x3); }

      t1x = t2x = x1; y = y1;   // Starting points
      dx1 = (int)(x2 - x1); if (dx1 < 0) { dx1 = -dx1; signx1 = -1; }
      else signx1 = 1;
      dy1 = (int)(y2 - y1);

      dx2 = (int)(x3 - x1); if (dx2 < 0) { dx2 = -dx2; signx2 = -1; }
      else signx2 = 1;
      dy2 = (int)(y3 - y1);

      if (dy1 > dx1) {   // swap values
         SWAP(dx1, dy1);
         changed1 = true;
      }
      if (dy2 > dx2) {   // swap values
         SWAP(dy2, dx2);
         changed2 = true;
      }

      e2 = (int)(dx2 >> 1);
      // Flat top, just process the second half
      if (y1 == y2) goto next;
      e1 = (int)(dx1 >> 1);

      for (int i = 0; i < dx1;) {
         t1xp = 0; t2xp = 0;
         if (t1x < t2x) { minx = t1x; maxx = t2x; }
         else { minx = t2x; maxx = t1x; }
         // process first line until y value is about to change
         while (i < dx1) {
            i++;
            e1 += dy1;
            while (e1 >= dx1) {
               e1 -= dx1;
               if (changed1) t1xp = signx1;//t1x += signx1;
               else          goto next1;
            }
            if (changed1) break;
            else t1x += signx1;
         }
         // Move line
      next1:
         // process second line until y value is about to change
         while (1) {
            e2 += dy2;
            while (e2 >= dx2) {
               e2 -= dx2;
               if (changed2) t2xp = signx2;//t2x += signx2;
               else          goto next2;
            }
            if (changed2)     break;
            else              t2x += signx2;
         }
      next2:
         if (minx > t1x) minx = t1x; if (minx > t2x) minx = t2x;
         if (maxx < t1x) maxx = t1x; if (maxx < t2x) maxx = t2x;
         drawline(minx, maxx, y);    // Draw line from min to max points found on the y
                               // Now increase y
         if (!changed1) t1x += signx1;
         t1x += t1xp;
         if (!changed2) t2x += signx2;
         t2x += t2xp;
         y += 1;
         if (y == y2) break;

      }
   next:
      // Second half
      dx1 = (int)(x3 - x2); if (dx1 < 0) { dx1 = -dx1; signx1 = -1; }
      else signx1 = 1;
      dy1 = (int)(y3 - y2);
      t1x = x2;

      if (dy1 > dx1) {   // swap values
         SWAP(dy1, dx1);
         changed1 = true;
      }
      else changed1 = false;

      e1 = (int)(dx1 >> 1);

      for (int i = 0; i <= dx1; i++) {
         t1xp = 0; t2xp = 0;
         if (t1x < t2x) { minx = t1x; maxx = t2x; }
         else { minx = t2x; maxx = t1x; }
         // process first line until y value is about to change
         while (i < dx1) {
            e1 += dy1;
            while (e1 >= dx1) {
               e1 -= dx1;
               if (changed1) { t1xp = signx1; break; }//t1x += signx1;
               else          goto next3;
            }
            if (changed1) break;
            else   	   	  t1x += signx1;
            if (i < dx1) i++;
         }
      next3:
         // process second line until y value is about to change
         while (t2x != x3) {
            e2 += dy2;
            while (e2 >= dx2) {
               e2 -= dx2;
               if (changed2) t2xp = signx2;
               else          goto next4;
            }
            if (changed2)     break;
            else              t2x += signx2;
         }
      next4:

         if (minx > t1x) minx = t1x; if (minx > t2x) minx = t2x;
         if (maxx < t1x) maxx = t1x; if (maxx < t2x) maxx = t2x;
         drawline(minx, maxx, y);
         if (!changed1) t1x += signx1;
         t1x += t1xp;
         if (!changed2) t2x += signx2;
         t2x += t2xp;
         y += 1;
         if (y > y3) return;
      }
   }


public:

   // constructs n-sided polygon inscribed in circle. Optional offset angle in degrees, default 0 (+X axis) start
   void DrawPolygon(int x_pos, int y_pos, int nSides, int radius, float offsetAngle = 0, short inputChar = 0x2588, short color = 0x000F)
   {
      offsetAngle *= PI / 180;
      int x1, y1, x2, y2;
      float angle = offsetAngle;

      x2 = cos(angle) * radius + x_pos;
      y2 = sin(angle) * radius + y_pos;

      for (int i = 0; i <= nSides; i++)
      {
         angle += 2 * PI / nSides;
         x1 = x2;
         y1 = y2;

         x2 = cos(angle) * radius + x_pos;
         y2 = sin(angle) * radius + y_pos;

         DrawLine(x1, y1, x2, y2, inputChar, color);
      }
   }

   // prints string at a specified location
   void PrintString(int x_pos, int y_pos, std::string statement)
   {
      for (int i = 0; i < statement.length(); i++)
      {
         PaintPixel(x_pos, y_pos, statement[i]);
         x_pos++;
      }
   }

   // wchar string
   void PrintStringW(int x, int y, std::wstring c, short col = 0x000F)
   {
      for (size_t i = 0; i < c.size(); i++)
      {
         if (c[i] != L' ')
         {
            screen[y * screenWidth + x + i].Char.UnicodeChar = c[i];
            screen[y * screenWidth + x + i].Attributes = col;
         }
      }
   }
};
