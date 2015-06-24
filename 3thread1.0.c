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

buffer rbuf;
buffer wbuf;

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
    while(buf[j] != '\0') {
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
        pthread_mutex_lock(&rbuf.lock[b]);
        
        // Buffer full. Wait for cond empty buf
        if(rbuf.len[b] != 0) pthread_cond_wait(&rbuf.cv_eb, &rbuf.lock[b]);
        
        rbuf.len[b] = (int)gzread(infile, rbuf.buf[b], BUF_SIZE - 1);
        if(rbuf.len[b] < 0) ERROR("File from read failure");
        
        // Conditions for end of read
        if(rbuf.len[b] == 0) {
            rbuf.len[b] = BUF_SIZE + 1;
            pthread_mutex_unlock(&rbuf.lock[b]);
            pthread_cond_signal(&rbuf.cv_fb);
            gzclose(infile);
            pthread_exit(NULL);
        }
        
        // Unlocks mutex and signals modifer;
        pthread_mutex_unlock(&rbuf.lock[b]);
        pthread_cond_signal(&rbuf.cv_fb);
        
        // Increments or loops index
        b + 1 == ARRAY_SIZE ? b = 0 : b++;
    }
}

// Modify's string stored in rbuf and writes it to wbuf
void * modifier() {
    int b = 0;
    char tmp[BUF_SIZE];
    int t;
    
    for(;;) {
        pthread_mutex_lock(&rbuf.lock[b]);
        // Condition if rbuf is empty
        if(rbuf.len[b] == 0)
            pthread_cond_wait(&rbuf.cv_fb, &rbuf.lock[b]);
        
        // Condition if read is complete
        if(rbuf.len[b] > BUF_SIZE) break;
        
        memcpy(tmp, rbuf.buf[b], rbuf.len[b]);
        t = rbuf.len[b];
        rbuf.len[b] = 0;
        
        // Unlock read mutex and signals
        pthread_mutex_unlock(&rbuf.lock[b]);
        pthread_cond_signal(&rbuf.cv_eb);
        
        //Modifies buf
        modify(tmp);
        
        pthread_mutex_lock(&wbuf.lock[b]);
        // Condition if wbuf is full
        if(wbuf.len[b] != 0) pthread_cond_wait(&wbuf.cv_eb, &wbuf.lock[b]);
        
        memcpy(wbuf.buf[b], tmp, t);
        wbuf.len[b] = t;
        
        // Unlock write mutex and signals
        pthread_mutex_unlock(&wbuf.lock[b]);
        pthread_cond_signal(&wbuf.cv_fb);
        
        // Increment or loop index
        b + 1 == ARRAY_SIZE ? b = 0 : b++;
    }
    
    pthread_mutex_lock(&wbuf.lock[b]);
    // Condition if wbuf is full
    if(wbuf.len[b] != 0) pthread_cond_wait(&wbuf.cv_eb, &wbuf.lock[b]);
    
    wbuf.len[b] = rbuf.len[b];
    pthread_mutex_unlock(&rbuf.lock[b]);
    pthread_mutex_unlock(&wbuf.lock[b]);
    pthread_cond_signal(&wbuf.cv_fb);
    pthread_exit(NULL);
}

// Compresses and writes wbuf to outfile
void * writer() {
    int b = 0;
    
    for(;;) {
        pthread_mutex_lock(&wbuf.lock[b]);
        // Condition if wbuf is empty
        if(wbuf.len[b] == 0) pthread_cond_wait(&wbuf.cv_fb, &wbuf.lock[b]);
        
        // Condition if write is complete
        if(wbuf.len[b] > BUF_SIZE) {
            pthread_mutex_unlock(&wbuf.lock[b]);
            gzclose(outfile);
            pthread_exit(NULL);
        }
        
        // Compress and write buffer to file
        if(gzwrite(outfile, wbuf.buf[b], wbuf.len[b]) != wbuf.len[b])
            ERROR("Write to file fail");
        
        // Reset buffer lenght
        wbuf.len[b] = 0;
        
        pthread_mutex_unlock(&wbuf.lock[b]);
        pthread_cond_signal(&wbuf.cv_eb);
        
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
    printf("Writing to file %s\n", argv[2]);
    outfile = gzopen(argv[2], "wb");
    if(!outfile) ERROR2("File open fail:", argv[2]);
    
    // Initiates mutexs and condition variables
    int i;
    for(i =0; i < ARRAY_SIZE; i++) {
        pthread_mutex_init(&rbuf.lock[i], NULL);
        pthread_mutex_init(&wbuf.lock[i], NULL);
    }
    pthread_cond_init(&rbuf.cv_eb, NULL);
    pthread_cond_init(&rbuf.cv_fb, NULL);
    pthread_cond_init(&wbuf.cv_eb, NULL);
    pthread_cond_init(&wbuf.cv_fb, NULL);
    
    
    /* creates thread pointers and ids */
    pthread_t read_t, mod_t, write_t;
    pthread_create(&read_t, NULL, reader, NULL);
    pthread_create(&mod_t, NULL, modifier, NULL);
    pthread_create(&write_t, NULL, writer, NULL);
    
    /* waiting on threads to finishs */
    pthread_join(read_t, NULL);
    pthread_join(mod_t, NULL);
    pthread_join(write_t, NULL);
    
    // Destroys mutex before terminating
    for(i = 0; i < ARRAY_SIZE; i++) {
        if(pthread_mutex_destroy(&rbuf.lock[i]) < 0) ERROR("Read Mutex was not destroyed");
        if(pthread_mutex_destroy(&wbuf.lock[i]) < 0) ERROR("Write Mutex was not destroyed");
    }
    
    if(pthread_cond_destroy(&rbuf.cv_eb) < 0) ERROR("Read Mutex condition variable was not destroyed");
    if(pthread_cond_destroy(&rbuf.cv_fb) < 0) ERROR("Read Mutex condition variable was not destroyed");
    if(pthread_cond_destroy(&wbuf.cv_eb) < 0) ERROR("Write Mutex condition variable was not destroyed");
    if(pthread_cond_destroy(&wbuf.cv_fb) < 0) ERROR("Write Mutex condition variable was not destroyed");
    
    end = clock();
    // Compute the duration
    duration = ((double)( end - start )) / CLOCKS_PER_SEC;
    printf("Execution time %f\n", duration);
    
    pthread_exit(NULL);  // Waiting on threads then closing
}
