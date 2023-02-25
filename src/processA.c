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
#include <sys/socket.h>
#include <sys/mman.h>
#include <fcntl.h> 
#include <sys/shm.h> 
#include <string.h> 
#include <netinet/in.h>
#include <netdb.h> 
#include <strings.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

#define sem_1 "/sem_AV_1" //Sem_procuder
#define sem_2 "/sem_AV_2" //Sem_consumer
#define shm_name "/AOS"

//pointer to log file
FILE *logfile;

/*Parameters*/
const int SM_WIDTH = 1600;
const int SM_HEIGHT = 600;
const int DEPTH = 4;
const int RADIUS = 30;
int n_curses_width = 90;
int n_curses_height = 30;
const int COLOR_SEG = SM_WIDTH*SM_HEIGHT;
const int SM_FACTOR = 20;
const int BMP_CIRC_RADIUS = 60;
const int SHM_SIZE = SM_WIDTH*SM_HEIGHT*4;

// Data structure for storing the bitmap file
bmpfile_t *bmp;
// Data type for defining pixel colors (BGRA)
rgb_pixel_t pixel = {0, 0, 255, 0};
rgb_pixel_t empty_pixel = {255, 255, 255, 0}; //White pixel

/*Shared memory*/
int shm_fd; 
rgb_pixel_t * ptr;

/*Print counter*/
int print_counter = 0;

/*Semaphores*/ 
sem_t * sem_id1;
sem_t * sem_id2;

/*Socket descriptor for client or server mode*/
int sockfd;
int newsockfd;
int mode;
char address[256] = "";

struct sigaction ignore_action;

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

/* Save the current bitmap object to 
out directory starting from 0 */
bool take_snapshot(){
    char path[20];

    snprintf(path, 20, "out/%d.bmp", print_counter);
    print_counter += 1;

    bmp_save(bmp, path);   
}

/*Draw a colored circle getting the bitmap object and
the center of the circle*/
void draw__colored_circle_bmp(bmpfile_t * bmp, int xc, int yc){   
  for(int x = -RADIUS; x <= RADIUS; x++) {
        for(int y = -RADIUS; y <= RADIUS; y++) {
        // If distance is smaller, point is within the circle
            if(sqrt(x*x + y*y) < RADIUS) {
                /*
                * Color the pixel at the specified (x,y) position
                * with the given pixel values
                */
                bmp_set_pixel(bmp, xc + x, yc + y, pixel);
            }
        }
    }
}

/*Draw a white circle getting the bitmap object and
the center of the circle*/
void draw__empty_circle_bmp(bmpfile_t * bmp, int xc, int yc){
    for(int x = -RADIUS; x <= RADIUS; x++) {
        for(int y = -RADIUS; y <= RADIUS; y++) {
        // If distance is smaller, point is within the circle
            if(sqrt(x*x + y*y) < RADIUS) {
                /*
                * Color the pixel at the specified (x,y) position
                * with the given pixel values
                */
                bmp_set_pixel(bmp, xc + x, yc + y, empty_pixel);
            }
        }
    }
}

void load_bmp_to_shm(bmpfile_t * bmp, rgb_pixel_t * ptr){
    int pos = 0;   

    /*Sem wait*/
    sem_wait(sem_id1);

    /*Loading pixel*/
    for(int i = 0; i < SM_WIDTH; i++){
        for(int j = 0; j < SM_HEIGHT; j++){
            pos = (i*SM_WIDTH)+j+1; 
            ptr++;             
            /*BGRA*/
            rgb_pixel_t * tmp_p = bmp_get_pixel(bmp,i,j);
            int b = tmp_p->blue;
            int g = tmp_p->green;
            int r = tmp_p->red;
            int a = tmp_p->alpha;
            rgb_pixel_t alfio = {b,g,r,a};          
            *ptr = alfio;                
        }
    }

    /*sem signal*/
    sem_post(sem_id2);    
}

/*Reset function to clear all bitmap obj
after a resize of the window*/
void reset_bmp(bmpfile_t * bmp) {
    for(int i = 0; i < SM_WIDTH; i++){
        for(int j = 0; j < SM_HEIGHT; j++){
            bmp_set_pixel(bmp, i, j, empty_pixel);
        }
    }
}

// Main Functions
int main(int argc, char *argv[])
{

    //open Log file
    logfile = fopen("ProcessA.txt", "a");
    if (logfile == NULL)
    {
        printf("an error occured while creating ProcessA's log File\n");
        return 0;
    }
        fprintf(logfile, "***log file created***\n");
        fflush(logfile);

        //Writing in log file
        fprintf(logfile, "p - ProcessA Log File\n");

    printf("Select Execution Mode: \n\n\n PRESS 1: Normal Mode \n\n PRESS 2: Server Mode \n\n PRESS 3: Client Mode\n\n");
    scanf("%d", &mode);  

    switch (mode) {
        case 1:{
            /*Normal mode*/
            
        } break;
        case 2:{
            /*Server Mode*/

            struct sockaddr_in serv_addr;
            struct sockaddr_in cli_addr;
            int portno, clilen;

            sockfd = check(socket(AF_INET, SOCK_STREAM, 0));

            bzero((char *) &serv_addr, sizeof(serv_addr));

            printf("\n Enter the port number: ");
            scanf("%d", &portno);                       

            serv_addr.sin_family = AF_INET;
            serv_addr.sin_addr.s_addr = INADDR_ANY;
            serv_addr.sin_port = htons(portno);

            if (check(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) < 0) {
                perror("ERROR in binding");
            }else{
                printf("\nReady...");
            }   

            listen(sockfd,5);            
            clilen = sizeof(cli_addr);

            newsockfd = check(accept(sockfd, (struct sockaddr *) &cli_addr, &clilen));
            if (newsockfd < 0) {
                perror("ERROR to accept");
            }

        } break;
        case 3:{
            /*client case*/
            int r = -1;

            int portno, clilen;
            struct sockaddr_in serv_addr;
            struct hostent *server;

            char buffer[256];int n;
            do{
                printf("\n Enter the address: ");
                scanf("%s", address);
                printf("\n Enter the port: ");
                scanf("%d", &portno); 

                sockfd = check(socket(AF_INET, SOCK_STREAM, 0));
                if (sockfd < 0) {
                    perror("ERROR opening socket");
                }

                server = gethostbyname(address);
                if (server == NULL) {
                    fprintf(stderr,"ERROR, no such host\n");
                    exit(0);
                }  

                bzero((char *) &serv_addr, sizeof(serv_addr));
                serv_addr.sin_family = AF_INET;

                bcopy((char *)server->h_addr, 
                (char *)&serv_addr.sin_addr.s_addr,
                server->h_length);

                serv_addr.sin_port = htons(portno);

                r = check(connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)));
                if(r != 0){
                    printf("Error during connection...\n");
                }                   
            
            }while(r != 0);

            /* Ignore the SIGINT signal throw when client try to 
            write to a closed socket */
            memset(&ignore_action, 0, sizeof(ignore_action));
            ignore_action.sa_handler = SIG_IGN;
            sigaction(SIGUSR1, &ignore_action, NULL);
        }
    }

    // Normal execution, use keyboard input
    // Utility variable to avoid trigger resize event on launch
    int first_resize = TRUE;

    //Open semaphores
    sem_id1 = sem_open(sem_1, 0);
    sem_id2 = sem_open(sem_2, 0);

    // Initialize UI
    init_console_ui();

    // Instantiate bitmap
    bmp = bmp_create(SM_WIDTH, SM_HEIGHT, DEPTH);

    // create the shared memory object
    shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);

    /* configure the size of the shared memory object */
    ftruncate(shm_fd, SHM_SIZE);

    /* memory map the shared memory object */
    ptr = mmap(0, SHM_SIZE, PROT_WRITE, MAP_SHARED, shm_fd, 0);

    // Infinite loop
    while (TRUE) {
        // Get input in non-blocking mode
        int cmd = getch();
        float scale_x = (float)SM_WIDTH/(float)(COLS-BTN_SIZE_X);
        float scale_y = (float)SM_HEIGHT/LINES;

        // If user resizes screen, re-draw UI...
        if(cmd == KEY_RESIZE) {
            if(first_resize) {
                first_resize = FALSE;
            }
            else {
                reset_console_ui();
                reset_bmp(bmp);
                float scale_x = (float)SM_WIDTH/(float)(COLS-BTN_SIZE_X);
                float scale_y = (float)SM_HEIGHT/(float)LINES;
                draw__colored_circle_bmp(bmp, floor(circle.x*scale_x), floor(circle.y*scale_y));
                load_bmp_to_shm(bmp, ptr);
            }
        }

        // Else, if user presses print button...
        else if(cmd == KEY_MOUSE) {
            if(getmouse(&event) == OK) {
                if(check_button_pressed(print_btn, &event)) {
                    mvprintw(LINES - 1, 1, "Print button pressed");

                    /*Call print to file function*/
                    take_snapshot();

                    refresh();
                    sleep(1);
                    for(int j = 0; j < COLS - BTN_SIZE_X - 2; j++) {
                        mvaddch(LINES - 1, j, ' ');
                    }
                }
            }
        }

        if(mode == 2){
            /*Server mode*/
            char input_string[5];

            int n = read(newsockfd,input_string,5);

            if(n < 3){                
                /*Error / Socket closed from the other side*/
                close(newsockfd); //Close the socket from the server side                              
            }else{                
                /*Select case*/
                int com = atoi(input_string);

                if(com == KEY_LEFT || com == KEY_RIGHT || com == KEY_UP || com == KEY_DOWN){
                    draw__empty_circle_bmp(bmp, floor(circle.x*scale_x), floor(circle.y*scale_y));
                    move_circle(com);
                    draw__colored_circle_bmp(bmp, floor(circle.x*scale_x), floor(circle.y*scale_y));
                    draw_circle();
                    /*Sync with Shared memory image*/
                    load_bmp_to_shm(bmp, ptr);
                }               
               
            }
            
        }

        // If input is an arrow key, move circle accordingly...
        else if(cmd == KEY_LEFT || cmd == KEY_RIGHT || cmd == KEY_UP || cmd == KEY_DOWN) {            

            switch (mode) {
                case 1: {
                    // Mode 1
                    draw__empty_circle_bmp(bmp, floor(circle.x * scale_x), floor(circle.y * scale_y));
                    move_circle(cmd);
                    draw__colored_circle_bmp(bmp, floor(circle.x * scale_x), floor(circle.y * scale_y));
                    draw_circle();
                    load_bmp_to_shm(bmp, ptr);
                    break;
                }
                case 3: {
                    // Client mode 
                    char str_cmd[5];
                    snprintf(str_cmd, 5, "%d", cmd);

                    int n = check(write(sockfd, str_cmd, 5));
                    if (n == -1) {
                        perror("Socket write error");
                        close(sockfd);
                    } else if (n != 5) {
                        fprintf(stderr, "Unexpected number of bytes written: %d\n", n);
                    } else {
                        draw__empty_circle_bmp(bmp, floor(circle.x * scale_x), floor(circle.y * scale_y));
                        move_circle(cmd);
                        draw__colored_circle_bmp(bmp, floor(circle.x * scale_x), floor(circle.y * scale_y));
                        draw_circle();
                        load_bmp_to_shm(bmp, ptr);
                    }
                    break;
                }
            }
        }
    }
    
    endwin();
    return 0;
}