/*                              */
/*  Matthew McCullough 1058868  */
/*  Comp301 Assignment 3        */
/*                              */

#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <string.h>
#include <pthread.h>

#define BUF_SIZE 1024
#define ARRAY_SIZE 10

typedef struct {
    char buf[ARRAY_SIZE][BUF_SIZE];
    pthread_mutex_t lock[ARRAY_SIZE];
    int len[ARRAY_SIZE];
    pthread_cond_t cv_eb, cv_fb;  // Buffer empty, Buffer full
} buffer;

buffer buf;

gzFile * infile;
gzFile * outfile;

/* functions called after error */
void ERROR(const char * msg) {
    printf("%s\n", msg);
    exit(0);
}

void ERROR2(const char * msg, char * s) {
    printf("%s%s\n", msg, s);
    exit(0);
}

/* Modifies input string */
void modify(char * buf) {
    int j = 0;
    while(buf[j] != 0) {
        if (buf[j] == 'a' || buf[j] == 'A') {
            buf[j] = '4';
        } else if (buf[j] == 'e' || buf[j] == 'E') {
            buf[j] = '3';
        } else if (buf[j] == 'i' || buf[j] == 'I') {
            buf[j] = '1';
        } else if (buf[j] == 'o' || buf[j] == 'O') {
            buf[j] = '0';
        } else if (buf[j] == 's' || buf[j] == 'S') {
            buf[j] = '5';
        }
        j++;
    }
    return;
}

// Reads given compressed file and stores read in buffer
void * reader() {
    int b = 0; // Loop counter
    for(;;) {

        pthread_mutex_lock(&buf.lock[b]);
        
        // Buffer full. Wait for cond empty buf
        if(buf.len[b] != 0) pthread_cond_wait(&buf.cv_eb, &buf.lock[b]);
        
        buf.len[b] = (int)gzread(infile, buf.buf[b], BUF_SIZE - 1);

        // Test if read didn't work
        if(buf.len[b] < 0) ERROR("File from read failure");
        
        // printf("%s\n", buf.buf[b]);
        
        // Conditions for end of read
        if(buf.len[b] == 0) {
            pthread_mutex_unlock(&buf.lock[b]);
            pthread_cond_signal(&buf.cv_fb);
            buf.len[b] = BUF_SIZE + 1;
            gzclose(infile);
            pthread_exit(NULL);
        }
        
        // Modify string and update buffers
        modify(buf.buf[b]);
        
        // Unlock mutexs and signal threads of the buffer change
        pthread_mutex_unlock(&buf.lock[b]);
        pthread_cond_signal(&buf.cv_fb);
        
        // Increment or loop index
        b + 1 == ARRAY_SIZE ? b = 0 : b++;
    }
}

// Compresses and writes wbuf to outfile
void * writer() {
    int b = 0;
    
    for(;;) {
        pthread_mutex_lock(&buf.lock[b]);
        // Condition if wbuf is empty
        if(buf.len[b] == 0) pthread_cond_wait(&buf.cv_fb, &buf.lock[b]);
        
        // Condition if write is complete
        if(buf.len[b] > BUF_SIZE) {
            pthread_mutex_unlock(&buf.lock[b]);
            gzclose(outfile);
            pthread_exit(NULL);
        }
        
        // Compress and write buffer to file
        if (gzwrite(outfile, buf.buf[b], buf.len[b]) != buf.len[b])
            ERROR("Write to file fail");
        
        // Set buffer lenght back to zero
        buf.len[b] = 0;
        
        pthread_mutex_unlock(&buf.lock[b]);
        pthread_cond_signal(&buf.cv_eb);
        
        b + 1 == ARRAY_SIZE ? b = 0 : b++;
    }
}

int main(int argc,  char * argv[]) {
    // Execution time test
    clock_t start, end;
    double duration;
    start = clock();
    
    // Test correct number of input have been entered
    if(argc > 3 || argc < 3)
        ERROR("Usage: [file to modify] [file to store modified file in]");
    
    // Opens file and tests return type
    printf("Opening file %s\n", argv[1]);
    infile = gzopen(argv[1], "rb");
    if(!infile) ERROR2("File open fail:", argv[1]);
    outfile = gzopen(argv[2], "wb");
    if(!outfile) ERROR2("File open fail:", argv[2]);
    
    // Initiates mutexs and condition variables
    int i;
    for(i =0; i < ARRAY_SIZE; i++)
        pthread_mutex_init(&buf.lock[i], NULL);

    pthread_cond_init(&buf.cv_eb, NULL);
    pthread_cond_init(&buf.cv_fb, NULL);
    
    /* creates thread pointers and ids */
    pthread_t read_t, write_t;
    pthread_create(&read_t, NULL, reader, NULL);
    pthread_create(&write_t, NULL, writer, NULL);
    
    /* waiting on threads to finishs */
    pthread_join(read_t, NULL);
    pthread_join(write_t, NULL);
    
    // Destroys mutex before terminating
    for(i = 0; i < ARRAY_SIZE; i++)
        if(pthread_mutex_destroy(&buf.lock[i]) < 0) ERROR("Write Mutex was not destroyed");
    
    
    if(pthread_cond_destroy(&buf.cv_eb) < 0) ERROR("Write Mutex condition variable was not destroyed");
    if(pthread_cond_destroy(&buf.cv_fb) < 0) ERROR("Write Mutex condition variable was not destroyed");
    
    end = clock();
    // Compute the duration
    duration = ((double)( end - start )) / CLOCKS_PER_SEC;
    printf("Execution time %f\n", duration);
    
    pthread_exit(NULL);  // Waiting on threads then closing
}
