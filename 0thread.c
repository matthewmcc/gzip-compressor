//
//  Created by Matthew McCullough on 9/09/14.
//  Copyright (c) 2014 mmm. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <string.h>
#include <pthread.h>

#define BUF_SIZE 1024

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

int main(int argc, char * argv[])
{
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
    
    int len = 1;
    char buf[BUF_SIZE];
    
    for(;;) {
        len = (int)gzread(infile, buf, BUF_SIZE);
        
        // Test for file transfer complete
        if(len == 0) break;
        
        modify(buf);
        
        if(gzwrite(outfile, buf, len) != len) {
            printf("File write failure\n");
            exit(-1);
        }
    }
    
    end = clock();
    // Compute the duration
    duration = ((double)( end - start )) / CLOCKS_PER_SEC;
    printf("Execution time %f\n", duration);
    
    gzclose(infile);
    gzclose(outfile);
    
    exit(0);
}

