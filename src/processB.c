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
#include <sys/socket.h>
#include <sys/mman.h>
#include <fcntl.h> 
#include <sys/shm.h> 
#include <string.h> 
#include <netinet/in.h>
#include <netdb.h> 
#include <strings.h>
#include <signal.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>

#define radius 30
#define sem_1 "/sem_AV_1" 
#define sem_2 "/sem_AV_2" 
#define shm_name "/AOS"

//pointer to log file
FILE *logfile;

/*Parameters*/
const int SM_WIDTH = 1600;
const int SM_HEIGHT = 600;
const int DEPTH = 4;
const int RADIUS = 30;
int n_curses_width = 80;
int n_curses_height = 30;
const int COLOR_SEG = SM_WIDTH*SM_HEIGHT;
const int SM_FACTOR = 20;
const int BMP_CIRC_RADIUS = 60;
const int SHM_SIZE = SM_WIDTH*SM_HEIGHT*4;

/*Shared memory*/
int shm_fd; 
rgb_pixel_t * ptr;

/*Semaphore*/
sem_t * sem_id1;
sem_t * sem_id2;

/*Variable to store the execution mode (normal, server or client)*/
int mode;

int check(int retval)
{
    if (retval == -1)
    {
        fprintf(logfile, "\nERROR (" __FILE__ ":%d) -- %s\n", __LINE__, strerror(errno));
        fflush(logfile);
        fclose(logfile);
        printf("\tAn error has been reported on log file.\n");
        fflush(stdout);
        exit(-1);
    }
    return retval;
}

int main(int argc, char const *argv[])
{
    //open Log file
    logfile = fopen("ProcessB.txt", "a");
    if (logfile == NULL)
    {
        printf("an error occured while creating ProcessB's log File\n");
        return 0;
    }
        fprintf(logfile, "***log file created***\n");
        fflush(logfile);

        //Writing in log file
        fprintf(logfile, "p - ProcessB Log File\n");

    if(mode == 2 || mode == 3) {
        char address[256];
        int port;
        printf("\n Enter the Address: ");
        scanf("%s", address);
        printf("\n Enter the Port: ");
        scanf("%d", &port);
        // create a socket and connect to the companion application
        int sockfd = check(socket(AF_INET, SOCK_STREAM, 0));
        struct sockaddr_in serv_addr;
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);
        inet_pton(AF_INET, address, &serv_addr.sin_addr);
        int status = check(connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)));
        if (status == -1) {
        // handle error
    }
    }

    // Utility variable to avoid trigger resize event on launch
    int first_resize = TRUE;

    // Initialize UI
    init_console_ui();

    //Open semaphore
    sem_id1 = sem_open(sem_1, 0);
    sem_id2 = sem_open(sem_2, 0);

    // create the shared memory object
    shm_fd = check(shm_open(shm_name, O_CREAT | O_RDWR, 0666));

    /* configure the size of the shared memory object */
    ftruncate(shm_fd, SHM_SIZE);

    /* memory map the shared memory object */
    ptr = mmap(0, SHM_SIZE, PROT_WRITE, MAP_SHARED, shm_fd, 0);

    // Infinite loop
    while (TRUE) {       

        /*Wait to enter in the critic section*/
        sem_wait(sem_id2);

        /*Create a temporal pointer*/
        rgb_pixel_t * tmp_ptr = ptr+1;

        int count = 1;
        int sum_x = 0;
        int sum_y = 0;

        for(int i = 0; i < SM_WIDTH; i++){            
            for(int j = 0; j < SM_HEIGHT; j++){   
                
                rgb_pixel_t * p = tmp_ptr++;
                int r = p->red;
                int g = p->green;
                int b = p->blue;
                int a = p->alpha;

                /*Searching colored pixel*/   
                if((r < 255) || (g < 255) || (b < 255)){                   
                    sum_x += i;
                    sum_y += j;
                    count++;
                }
            }            
        }        

        int x = sum_x/count;
        int y = sum_y/count;

        float scale_x = (float)SM_WIDTH/(float)COLS;
        float scale_y = (float)SM_HEIGHT/(float)LINES;      
        
        mvaddch(y/scale_y, x/scale_x, '0');
        refresh();        

        /*Signal exiting from the critic section*/
        sem_post(sem_id1);  

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
        
    }

    endwin();
    return 0;
}
