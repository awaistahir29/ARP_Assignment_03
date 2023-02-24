#include "./../include/processB_utilities.h"


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
#define radius 30
/*
Simply accessing the shared memory (BitMap) and performing some feature extraction to display trajectory.
Step 0:
    Reduce teh pixels again with the factor of 20x so that we get normal circled value that we gave
    in the process A.

Step 1:
    We will get the pixels from the shared memory which is the bitmap by scanning the pixels
    using the function (bmp_get_pixel()) more details in header file of the bitmap
    /usr/local/include -> file containing the functions
    Which return the pixels that we had set in the process A 

Step 2:
    Scan whole pixels and do some feature extraction like Comparing each line passing from the circle
    to compute the length of the line and if length is equal to the diameter of the circle then the
    centered point of the circle is the mid point of the circle

Step 3:
Taking that mid point we draw the trajectory:
    BY storing all the mid points to an array and display these centre points(Pixels) along with the realtime
    coming centered pixels in to the screen
*/

// Typedef for circle center
typedef struct {
    int x,y;
}coordinate;

 /* Instantiate bitmap, passing three parameters:
    *   - width of the image (in pixels)
    *   - Height of the image (in pixels)
    *   - Depth of the image (1 for greyscale images, 4 for colored images)
*/
int width = 1600;
int height = 600;
int depth = 4;

// Data type for defining pixel colors (BGRA)
rgb_pixel_t pixel = {0, 0, 255, 0};
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

         for(int i = 0; i < height; i++){
          for(int j = 0; j < width; j++){
             if(ptr[i+j*height].red == pixel.red && ptr[i+j*height].blue != pixel.blue && ptr[i+j*height].green != pixel.green && first == 0){
              first = j;
             }
             if(ptr[i+j*height].red != pixel.red && first != 0){
              last = j-1;
              break;
             }
            }

            if(last - first == 2*radius){
                        center.x = (last-first)/2; // cx
                        center.y = i; // cy
                        break;
                    }
            first = 0;
            last = 0;
    }


    circle_draw(center.x,center.y,bmp);

    return center;
}
int main(int argc, char const *argv[])
{
    // instantiation of the shared memory
    const char * shm_name = "/AOS";
    const int SIZE = width*height*sizeof(rgb_pixel_t);
    int shm_fd;
    rgb_pixel_t * ptr;
    shm_fd = shm_open(shm_name, O_RDONLY, 0666);
    if (shm_fd == 1) {
        printf("Shared memory segment failed\n");
        exit(1);
    }

    ptr = (rgb_pixel_t *)mmap(0, SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        printf("Map failed\n");
        return 1;
    }

    // array for the center
    coordinate *center = NULL;

    center = (coordinate*) malloc(10 * sizeof (coordinate));

    center[0].x = width/2;
    center[0].y = height/2;

    

    // normalized center for the ncurse window
    int cx, cy;

    
    // Utility variable to avoid trigger resize event on launch
    int first_resize = TRUE;

    // Initialize UI
    init_console_ui();

    // Data structure for storing the bitmap file
    bmpfile_t *bmp;
    
    bmp = bmp_create(width, height, depth);

    circle_draw(800, 300, bmp);

    bmp_save(bmp, "out/SNOP.bmp");

    // Infinite loop
    while (TRUE) {

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

        else {
            //delete(center[0].x, center[0].y, bmp);
            center[0] = find_center(bmp, ptr);
            bmp_save(bmp, "out/SNOP.bmp");
            cx = center[0].x/20;
            cy = center[0].y/20;
            mvaddch(cy, cx, '0');
            //mvaddch(center[0].y, center[0].x, '0');
            refresh();
            sleep(10);
        }
    }

    endwin();
    return 0;
}
