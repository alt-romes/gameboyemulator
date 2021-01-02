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
#ifdef USE_GRAPHICS
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#endif

static const int TOTAL_SCANLINE_CYCLES = 456;

static int scanline_cycles_left = TOTAL_SCANLINE_CYCLES;


/*---- LCD Control Status -----------------------------------------*/


static const int MODE2_SCANLINE_CYCLES = 376; // 456-80 (M2 lasts the first 80 cycles)
static const int MODE3_SCANLINE_CYCLES = 204; // 376-172 (M3 lasts 172 cycles after M2)
                                              // M0 lasts from cycles 204 to 0

static unsigned char lcdc_is_enabled() {

    return (*lcdc & 0x80); // true if bit 7 of lcd control is set
}

static void change_lcd_mode(unsigned char mode) {

    // first clear the mode to 0 (in case mode == 0x0) (set first two bits to 0)
    *lcdc_stat &= 0xFC; // 1111 1100

    *lcdc_stat |= mode;
}

static int lcdmode_interrupt_is_enabled(unsigned char mode) {

    /* LCD Interrupt can be triggered by mode changes,
     * As long as its respective mode interrupt is enabled
     *      LCDC Status Register:
     *          Bit 3: Mode 0 Interrupt Enabled
     *          Bit 4: Mode 1 Interrupt Enabled
     *          Bit 5: Mode 2 Interrupt Enabled
     */

    if (mode == 0 && (*lcdc_stat & 0x8))
        return 1;
    else if (mode == 1 && (*lcdc_stat & 0x10))
        return 1;
    else if (mode == 2 && (*lcdc_stat & 0x20))
        return 1;

    return 0;
}

static void set_lcd_stat() {

    /* LCD goes through 4 different modes, defined with bits 1 and 0
     *      (0) 00: H-Blank
     *      (1) 01: V-Blank
     *      (2) 10: Searching Sprites Atts
     *      (3) 11: Transfering Data to LCD Driver
     */
    
    if (!lcdc_is_enabled()) {
        
        scanline_cycles_left = TOTAL_SCANLINE_CYCLES; // scanline is anchored 

        *lcd_ly = 0; // current scanline is set to 0

        // set the LCD mode to V-Blank (1) during 'LCD Disabled'
        change_lcd_mode(0x1); 

    } else {
    
        /*
         * Check in which of the following modes we are:
         *      V-Blank mode
         *      H-Blank
         *      SSpritesAttr
         *      TransfDataToDriver
         *
         * If the LCD Stat mode needs to change, interrupt LCD
         *
         * If LCD Y Compare == LCD Y (current scanline):
         *      Set coincidence flag, else, clear it
         *      Call interrupt
         *
         * Update LCDC(ontrol) STAT
         */
    
        unsigned char current_mode = *lcdc_stat & 0x3;
        unsigned char mode = 0;

        if (*lcd_ly >= 144)
            mode = 1;
        else if (scanline_cycles_left >= MODE2_SCANLINE_CYCLES)
            mode = 2;
        else if (scanline_cycles_left >= MODE3_SCANLINE_CYCLES)
            mode = 3;

        if ((mode != current_mode) && lcdmode_interrupt_is_enabled(mode))
            request_interrupt(LCDSTAT_INTERRUPT);

        if (*lcd_ly == *lcd_lyc) {
            
            *lcdc_stat |= 0x4; // Set Flag: LCDC Stat bit 2 is a flag for LY == LYC

            // If LCD interrupts are enabled for LY==LYC
            // (This 'interrupt enabled' is a flag in bit 6 of LCDC Stat)
            if (*lcdc_stat & 0x40)
                request_interrupt(LCDSTAT_INTERRUPT);
            
        } else
            *lcdc_stat &= ~0x4; // Clear LCDC Stat Bit 2 Flag

        change_lcd_mode(mode);

    }


}



/*---- Rendering --------------------------------------------------*/


const int SCREEN_WIDTH = 160;
const int SCREEN_HEIGHT = 144;

static unsigned char scanlinesbuffer[SCREEN_WIDTH*SCREEN_HEIGHT];

#ifdef USE_GRAPHICS
GLFWwindow* window;

static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
}
#endif

static void init_gui() {
#ifdef USE_GRAPHICS
    /* Initialize the library */
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Gameboy", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(-1);
    }

    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    /* Glew initialization */
    if (glewInit() != GLEW_OK) exit(1);

    /* Create shaders */
    const char* vert_shader = "\
        #version 330 core\n\
        layout (location = 0) in vec3 iPos;\
        layout (location = 1) in vec2 iTexCoord;\
        out vec2 TexCoord;\
        void main()\
        {\
            TexCoord = vec2(iTexCoord.x, iTexCoord.y);\
            gl_Position = vec4(iPos, 1.0);\
        }\
    ";

    unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vert_shader, NULL);
    glCompileShader(vertex);

    // print compile errors
    int success;
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    char infoLog[512];
    if(!success)
    {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        printf("Compile Error: %s\n", infoLog);
    };

    const char* frag_shader = "\
        #version 330 core\n\
        out vec4 frag_color;\
        in vec2 TexCoord;\
        uniform sampler2D tex;\
        void main()\
        {\
            frag_color = vec4(texture(tex, TexCoord).r);\
        }\
    ";

    unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &frag_shader, NULL);
    glCompileShader(fragment);

    // print compile errors
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        printf("Compile Error: %s\n", infoLog);
    };

    unsigned int prog_id = glCreateProgram();

    glAttachShader(prog_id, vertex);
    glAttachShader(prog_id, fragment);
    glLinkProgram(prog_id);

    glGetProgramiv(prog_id, GL_LINK_STATUS, &success);
    if(!success)
    {
        glGetProgramInfoLog(prog_id, 512, NULL, infoLog);
        printf("Compile Error: %s\n", infoLog);
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    glUseProgram(prog_id);

    float vertices[] = {
        // positions          // texture coords
         1.0f,  1.0f, 0.0f,   1.0f, 0.0f,   // top right
         1.0f, -1.0f, 0.0f,   1.0f, 1.0f,   // bottom right
        -1.0f, -1.0f, 0.0f,   0.0f, 1.0f,   // bottom left
        -1.0f,  1.0f, 0.0f,   0.0f, 0.0f    // top left 
    };
    unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    unsigned int vertex_attr_buf, vertex_data_buf, elem_data_buf;
    glGenVertexArrays(1, &vertex_attr_buf);
    glGenBuffers(1, &vertex_data_buf);
    glGenBuffers(1, &elem_data_buf);


    glBindVertexArray(vertex_attr_buf); /* we only need to set it once */

    glBindBuffer(GL_ARRAY_BUFFER, vertex_data_buf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elem_data_buf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    int textureLoc = glGetUniformLocation(prog_id, "tex");

    glUniform1i(textureLoc, 0); // Set the active texture location (default is 0) (when bindTexture, it'll bind to active texture, and we can have multiple of these)

    /* Bind texture once, since we only use one and we won't be changing it */
    unsigned int tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glClearColor(0, 0, 0, 1);
#endif
}


static void render_frame() {
#ifdef USE_GRAPHICS
    if(!glfwWindowShouldClose(window)) {

        /* processInput(window); ?? */ 

        /* Render here */

        /* glClear(GL_COLOR_BUFFER_BIT); */ // don't need it for now

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, scanlinesbuffer);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        /* Swap front and back buffers
         * (back buffer is being written to, front buffer is being rendered)
         */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();

    }
    else {
        glfwDestroyWindow(window);
        glfwTerminate();
        exit(0);
    }
#endif
}



static void render_sprites() {

    // TODO
}

static void render_tiles() {

#ifdef DEBUGPPU
    printf("RENDERING TILES\n");
#endif


    /* From the pandocs:

       LCD Control:

        Bit 7 - LCD Display Enable             (0=Off, 1=On)
        Bit 6 - Window Tile Map Display Select (0=9800-9BFF, 1=9C00-9FFF)
        Bit 5 - Window Display Enable (0=Off, 1=On)
        Bit 4 - BG & Window Tile Data Select (0=8800-97FF, 1=8000-8FFF)
        Bit 3 - BG Tile Map Display Select (0=9800-9BFF, 1=9C00-9FFF)
        Bit 2 - OBJ (Sprite) Size (0=8x8, 1=8x16)
        Bit 1 - OBJ (Sprite) Display Enable (0=Off, 1=On)
        Bit 0 - BG Display (for CGB see below) (0=Off, 1=On)

        Bootstrap LCDC: 145 (10010001)

     */
    
    unsigned char using_window = 0;
    unsigned short tile_data_base_address;                          // location where tiles are described 
    tile_data_base_address = ((*lcdc >> 4) & 1) ? 0x8000 : 0x8800;  // Test LCDC Bit 4 (if 0, the ids are signed bytes and we'll have to check it ahead)

    unsigned short tilemap_base_address;            // location where background (either window or actual BG)
                                                    // tiles to be used are specified (by id)


    if (((*lcdc >> 5 ) & 1)                         // Test LCDC Bit 5 (enables or disables Window)
            && (*lcd_windowy <= *lcd_ly))           // and if current scanline is within window Y position
        using_window = 1;

    if (!using_window)
        tilemap_base_address = ((*lcdc >> 3) & 1) ? 0x9C00 : 0x9800; // Test LCDC Bit 3 (which BG tile map address)

    else
        tilemap_base_address = ((*lcdc >> 6) & 1) ? 0x9C00 : 0x9800; // Test LCDC Bit 6 (which Window tile map address)

    // Find tile row, then pixel row in tile,
    // and add the whole line of pixels to framebuffer
    
    unsigned char yPos = *lcd_scy + *lcd_ly;    // Y pos in 256x256 background
    unsigned char tile_row = (char) (yPos / 8); // 8 pixels per tile

    unsigned char pixels_drawn = 0;
    while (pixels_drawn < 160) {

        unsigned char xPos = pixels_drawn + *lcd_scx;        // X pos in 256x256 background
        unsigned char tile_col = (char) (xPos / 8);   // 8 pixels per tile

        short tile_id;
       
        // Get the tile id from tilemap + offset,
        // Then get the data for that tile id

        unsigned short tile_address = tilemap_base_address+(tile_row*32)+tile_col;   // tile ids are organized from tilemap_base as 32 rows of 32 bytes
        unsigned short tile_data_location = tile_data_base_address;

        if ((*lcdc >> 4) & 1) {                         // if tile identity is unsigned
            tile_id = (unsigned char) memory[tile_address];
            tile_data_location += (tile_id*16);         // each tile is 16 bytes long, start at 0
        } else {
            tile_id = (char) memory[tile_address];
            tile_data_location += ((tile_id+128) * 16); // each tile is 16 bytes, start at offset 128 (signed)
        }


        // figure out pixel line in tile, load color data for that line
        // get colors from pallete, mix with data to create color 
        // set pixel with color in scanlinesbuffer (which will be used in create framebuffer & render)

        unsigned char line_byte_in_tile = (yPos % 8) * 2;   // each tile has 8 vertical lines, each line uses 2 bytes

        unsigned char hi_color_bit = memory[tile_data_location+line_byte_in_tile];
        unsigned char lo_color_bit = memory[tile_data_location+line_byte_in_tile+1];


        /*  Background Pallete Register
              Bit 7-6 - Shade for Color Number 3
              Bit 5-4 - Shade for Color Number 2
              Bit 3-2 - Shade for Color Number 1
              Bit 1-0 - Shade for Color Number 0

            Possible shades of grey
              0  White
              1  Light gray
              2  Dark gray
              3  Black
         */

        unsigned char pallete_colors[4];
        pallete_colors[3] = *lcd_bgp >> 6;
        pallete_colors[2] = (*lcd_bgp >> 4) & 0x3;
        pallete_colors[1] = (*lcd_bgp >> 2) & 0x3;
        pallete_colors[0] = *lcd_bgp & 0x3;

        // For each tile, add horizontal pixels until the end of the tile (or until the end of the screen),
        // Add those pixels to the scanlines buffer, and add up to the amount of pixels drawn
        unsigned char tile_pixels_drawn = 0;
        unsigned char hpixel_in_tile = (xPos % 8);
        for(; hpixel_in_tile < 8 && pixels_drawn+hpixel_in_tile<160+*lcd_scx; hpixel_in_tile++) {

            unsigned char pixel_color = 0;
            
            pixel_color |= (hi_color_bit >> (7 - hpixel_in_tile)) & 1; // 7-pixel because pixel 0 is in bit 7
            pixel_color <<= 1;
            pixel_color |= (lo_color_bit >> (7 - hpixel_in_tile)) & 1;

            scanlinesbuffer[(*lcd_ly)*160 + pixels_drawn + hpixel_in_tile] = (3-pallete_colors[pixel_color])*85; // Do 3-color bc 0 = white and 3 = black

            tile_pixels_drawn++;
        }

        pixels_drawn+=tile_pixels_drawn;
             
    }

#ifdef DEBUGPPU
    printf("Pixels Drawn: %d\n", pixels_drawn);
#endif

}



/*---- Main Logic and Execution -----------------------------------*/


static void draw_scanline() {

    if (*lcdc & 0x1)    // LCDC Bit 0 enables or disables Background (BG + Window) Display
        render_tiles();

    if (*lcdc & 0x2)    // LCDC Bit 1 enables or disables Sprites
        render_sprites();
}

void ppu(int cycles) {

    set_lcd_stat();

    if (!lcdc_is_enabled())
        return;

    scanline_cycles_left -= cycles;

    /* If the scanline is completely drawn according to the time passed in cycles */
    if (scanline_cycles_left <= 0) {
        
        (*lcd_ly)++; /* Scanline advances, so we update the LCDC Y-Coordinate
                      * register, because it needs to hold the current scanline
                      */

        scanline_cycles_left = TOTAL_SCANLINE_CYCLES;

        unsigned char current_scanline = *lcd_ly;
        
        /* VBLANK is confirmed when LY >= 144, the resolution is 160x144,
         * but since the first scanline is 0, the scanline number 144 is actually the 145th.
         *
         * It means the cpu can use the VRAM without worrying, because it's not being used.
         * (Scanlines >= 144 and <= 153 are +invisible scanlines')
         */
        if (current_scanline == 144)
            request_interrupt(VBLANK_INTERRUPT); 

        else if (current_scanline > 153) /* if scanline goes above 153, reset to 0 */
            current_scanline = 0;

        else if (current_scanline < 144)
            draw_scanline();

    }
    

}

