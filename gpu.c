void printvram() {

    unsigned short vram_start = 0x00a7;
    unsigned short vram_end = 0x0100;

    printf("printing memory %d\n", vram_start);

    for (int i=vram_start; i<vram_end; i++) {
        printf("$%02x ", memory[i]);
    }

    printf("\n");

}

void print_tiles() {


    unsigned short vram_start = 0x9910;
    unsigned short vram_end = 0x992f;

    printvram();

    char tiles[1000][8][8];

    int tilesn = 0;

    // for each tile (read 16 bytes)
    for (int i=vram_start; i<vram_end-16; i+=16) {

        int row = 0;

        // for each row in the tile (read 2 bytes)
        for (int j=i; j<i+8-2; j+=2) {

            unsigned char low_colorb = memory[j];
            unsigned char high_colorb = memory[j+1];

            int column = 0;

            // for each bit in the two bytes for the row
            for(int z=0; z<7; z++) {
                    
                unsigned char mask = 1 << (7-z);

                unsigned char mlc = low_colorb & mask;
                unsigned char mhc = high_colorb & mask;

                char color = ' ';

                if (mlc > 0 && mhc > 0)
                    color = '@';
                else if (mhc > 0 && mlc == 0)
                    color = 'x';
                else if (mhc == 0 && mlc > 0)
                    color = '.';

                tiles[tilesn][row][column++] = color;

            }

            row++;

        }

        tilesn++;
            
    }


    for (int row=0; row<32; row++) {

        for (int byter=0; byter<8; byter++) {

            for (int i=0; i<32; i++) {

                for (int bytec = 0; bytec<8; bytec++) {

                    printf("%c ", tiles[row+i][byter][bytec]);

                }


            }

            printf("\n");

        }


    }






    
    /* for (int a=vram_start; a<vram_end-16; a+=16) { */

    /*     for (int i=a; i<a + 8; i++) { */
    /*         // 8 rows, each row is defined by two bits */
    /*         unsigned char lowercolorbyte = memory[i]; */
    /*         unsigned char uppercolorbyte = memory[i+1]; */

    /*         tile[x++] = ((uppercolorbyte & 128) >> 6) | ((lowercolorbyte & 128) >> 7); */
    /*         tile[x++] = ((uppercolorbyte & 64) >> 6) | ((lowercolorbyte & 64) >> 7); */
    /*         tile[x++] = ((uppercolorbyte & 32) >> 6) | ((lowercolorbyte & 32) >> 7); */
    /*         tile[x++] = ((uppercolorbyte & 16) >> 6) | ((lowercolorbyte & 16) >> 7); */
    /*         tile[x++] = ((uppercolorbyte & 8) >> 6) | ((lowercolorbyte & 8) >> 7); */
    /*         tile[x++] = ((uppercolorbyte & 4) >> 6) | ((lowercolorbyte & 4) >> 7); */
    /*         tile[x++] = ((uppercolorbyte & 2) >> 6) | ((lowercolorbyte & 2) >> 7); */
    /*         tile[x++] = ((uppercolorbyte & 1) >> 6) | ((lowercolorbyte & 1) >> 7); */

    /*         printf("%d %d %d %d %d %d %d %d\n", tile[i-a], tile[i-a+1], tile[i-a+2], tile[i-a+3], tile[i-a+4], tile[i-a+5], tile[i-a+6], tile[i-a+7]); */

    /*     } */

    /*     printf("\n"); */

    /* } */

}
