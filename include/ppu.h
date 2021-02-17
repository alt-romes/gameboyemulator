#ifndef _PPU

#define _PPU

/*
 * Gameboy Emulator: Pixel Processing Unit
 *
 * Resources:
 *
 * > General
 * https://www.youtube.com/watch?v=HyzD8pNlpwI
 *
 * > LCD
 * http://www.codeslinger.co.uk/pages/projects/gameboy/lcd.html
 *
 * > OpenGL Textures
 * https://learnopengl.com/Getting-started/Textures
 */


#define SCREEN_MULTIPLIER 2

#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 144


extern unsigned char joypad_state;

void init_gui();
void render_frame();

void ppu(int cycles);

#endif
