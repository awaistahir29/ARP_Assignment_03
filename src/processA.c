#include "./../include/processA_utilities.h"

#include <stdio.h>
#include <bmpfile.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define NAME "out/NEW"
/*
Simply moving the circle in ncurces window which moves the 20x bigger circle in the 
shared memory.
Keep updating the circle in bitmap when movements occur in ncurces window of the processA
*/
int width = 1600;
int height = 600;
int depth = 4;
int init_width;
int init_height;
int new_width;
int new_height;
int shift = 20;
int radius;

// Data structure for storing the bitmap file
bmpfile_t *bmp;
// Data type for defining pixel colors (BGRA)
rgb_pixel_t pixel = {0, 0, 255, 0};
rgb_pixel_t pixel1 = {255, 255, 255, 255};

// Function to Remove the previous circle
void rmv_prevCircle(){
    for(int x = -radius; x <= radius; x++) {
                        for(int y = -radius; y <= radius; y++) {
                            // If distance is smaller, point is within the circle
                            if(sqrt(((x-10)*x) + (y*y) < radius)) {
                                bmp_set_pixel(bmp, new_width + x, new_height + y, pixel1);
                            }
                        }
                    }
}

// Function to Draw New Circle
void draw_newCircle(){
    for(int x = -radius; x <= radius; x++) {
                        for(int y = -radius; y <= radius; y++) {
                            // If distance is smaller, point is within the circle
                            if(sqrt(((x-10)*x) + (y*y) < radius)) {
                                bmp_set_pixel(bmp, new_width + x, new_height + y, pixel);
                            }
                        }
                    }
}

#define radius 30

// Typedef for circle center
typedef struct {
    int x,y;
}coordinate;

 /* Instantiate bitmap, passing three parameters:
    *   - width of the image (in pixels)
    *   - Height of the image (in pixels)
    *   - Depth of the image (1 for greyscale images, 4 for colored images)
*/
// Data type for defining pixel colors (BGRA)
rgb_pixel_t pixel_w = {255, 255, 255, 0};

// draw a circle in the center (cx,cy) for a bitmap
void circle_draw(int cx, int cy, bmpfile_t *bmp){
  for(int x = -radius; x <= radius; x++) {
    for(int y = -radius; y <= radius; y++) {
      // If distance is smaller, point is within the circle
      if(sqrt(x*x + y*y) < radius) {
          /*
          * Color the pixel at the specified (x,y) position
          * with the given pixel values
          */
          bmp_set_pixel(bmp, cx + x, cy + y, pixel);
      }
    }
  }
}

// draw a circle for the shared memory
void circle_drawAOS(bmpfile_t *bmp, rgb_pixel_t *ptr){

    rgb_pixel_t* p;
    
    for(int i = 0; i < height; i++){
      for(int j = 0; j < width; j++){

        p = bmp_get_pixel(bmp,j,i);

        
        ptr[i+j*height].alpha = p->alpha;
        ptr[i+j*height].blue = p->blue;
        ptr[i+j*height].green = p->green;
        ptr[i+j*height].red = p->red;

      }
    }

}


// delete the old circle by colouring everything white for the bitmap
void delete(int cx, int cy, bmpfile_t *bmp){
  for(int x = -radius; x <= radius; x++) {
    for(int y = -radius; y <= radius; y++) {
      // If distance is smaller, point is within the circle
      if(sqrt(x*x + y*y) < radius) {
          /*
          * Color the pixel at the specified (x,y) position
          * with the given pixel values
          */
          bmp_set_pixel(bmp, cx + x, cy + y, pixel_w);
      }
    }
  }
}

// delete the old circle by colouring everything white in the shared memory
void deleteAOS(rgb_pixel_t *ptr){
    
    for(int i = 0; i < height; i++){
      for(int j = 0; j < width; j++){
        ptr[i+j*height].alpha = pixel_w.alpha;
        ptr[i+j*height].blue = pixel_w.blue;
        ptr[i+j*height].green = pixel_w.green;
        ptr[i+j*height].red = pixel_w.red;
      }
    }
}

// function to find the center of the shared memory
coordinate find_center(bmpfile_t *bmp, rgb_pixel_t *ptr){
  
        int first = 0, last = 0;

        char msg[100];

        coordinate center;

        sprintf(msg," AHAHAHAH loooosers");

         for(int i = 0; i < height; i++){
          for(int j = 0; j < width; j++){
             if(ptr[i+j*height].blue == pixel.blue && first == 0){
              first = j;
             }
             if(ptr[i+j*height].blue != pixel.blue && first != 0){
              last = j-1;
              break;
             }
      }

      if(last - first == 2*radius){
                center.x = (last-first)/2; // cx
                center.y = i; // cy

                sprintf(msg," AHAHAHAH first %d last %d", first, last);
                sprintf(msg," AHAHAHAH cx %d cy %d", center.x,center.y);
                //file_logG("./logs/prova.txt", msg);

                break;
            }
      first = 0;
      last = 0;
    }
    circle_draw(center.x,center.y,bmp);

    return center;

}

// Main Functions
int main(int argc, char *argv[])
{
    // instantiation of the shared memory
    const char * shm_name = "/AOS";
    const int SIZE = width*height*sizeof(rgb_pixel_t);
    int shm_fd;
    rgb_pixel_t* ptr;
    shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == 1) {
        printf("Shared memory segment failed\n");
        exit(1);
    }

    ftruncate(shm_fd,SIZE);

    ptr = (rgb_pixel_t *)mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        printf("Map failed\n");
        return 1;
    }

    // Utility variable to avoid trigger resize event on launch
    int first_resize = TRUE;

    // Initialize UI
    init_console_ui();

    
   // variable to name the copies of the bitmap saved from time to time
    int val = 0;

    // variable to store the old center of the circle (delete function)
    int cx = width/2, cy = height/2;

    // variable to name the bitmap file to be saved
    char msg[100];

    // Initialize UI
    init_console_ui();

    // Data structure for storing the bitmap file
    bmpfile_t *bmp;
    
    bmp = bmp_create(width, height, depth);

    circle_draw(cx,cy,bmp);
    bmp_save(bmp, NAME);
    circle_drawAOS(bmp, ptr); 
    
    // Infinite loop
    while (TRUE)
    {
        // Get input in non-blocking mode
        int cmd = getch();

        // If user resizes screen, re-draw UI...
        if(cmd == KEY_RESIZE) {
            if(first_resize) {
                first_resize = FALSE;
            }
            else {
                reset_console_ui();
            }
        }

        // Else, if user presses print button...
        else if(cmd == KEY_MOUSE) {
            if(getmouse(&event) == OK) {
                if(check_button_pressed(print_btn, &event)) {
                    mvprintw(LINES - 1, 1, "Print button pressed");
                    // Save image as .bmp file
                    bmp_save(bmp, NAME);
                    refresh();
                    sleep(1);
                    for(int j = 0; j < COLS - BTN_SIZE_X - 2; j++) {
                        mvaddch(LINES - 1, j, ' ');
                    }
                }
            }
        }
        
        // If input is an arrow key, move circle accordingly...
        else if(cmd == KEY_LEFT || cmd == KEY_RIGHT || cmd == KEY_UP || cmd == KEY_DOWN) {
            move_circle(cmd);
            draw_circle();
            delete(cx,cy,bmp);
            deleteAOS(ptr);
            cx = circle.x*20;
            cy = circle.y*20;
            circle_draw(cx,cy,bmp);
            circle_drawAOS(bmp,ptr);
        }
        
    }

    endwin();
    return 0;
}
