/* This file contains functions that are not part of the visible interface.
 * So they are essentially helper functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "simfs.h"

/* Internal helper functions first.
 */

FILE *
openfs(char *filename, char *mode)
{
    FILE *fp;
    if((fp = fopen(filename, mode)) == NULL) {
        perror("openfs");
        exit(1);
    }
    return fp;
}

void
closefs(FILE *fp)
{
    if(fclose(fp) != 0) {
        perror("closefs");
        exit(1);
    }
}



void createfile(char *filesystemname, char *filename){


    if(strlen(filename) > 11){
        fprintf(stderr, "Error: Filename too long\n");
        exit(1);
    }

    fentry files[MAXFILES];
    fnode fnodes[MAXBLOCKS];
    FILE *fp = openfs(filesystemname, "r");

    if ((fread(files, sizeof(fentry), MAXFILES, fp)) == 0) {
        fprintf(stderr, "Error: could not read file entries\n");
        closefs(fp);
        exit(1);
    }

    if ((fread(fnodes, sizeof(fnode), MAXBLOCKS, fp)) == 0) {
        fprintf(stderr, "Error: could not read fnodes\n");
        closefs(fp);
        exit(1);
    }

    closefs(fp);
    int checkmaxfiles;
    checkmaxfiles = -1;
    for(int i=0; i<MAXFILES;i++){
        if(strcmp(files[i].name, filename) == 0){
            fprintf(stderr, "Error: file already exists\n");
            exit(1);
        }
        if(files[i].name[0] == '\0' && checkmaxfiles == -1){
            checkmaxfiles = 0;
            strcpy(files[i].name, filename);
            break;
        }
    }

    if(checkmaxfiles == -1){
        fprintf(stderr, "Error: No more files can be added\n");
        exit(1);
    }

    fp = openfs(filesystemname, "w");

    if(fwrite(files, sizeof(fentry), MAXFILES, fp) < MAXFILES) {
        fprintf(stderr, "Error: Failed to update files array\n");
        closefs(fp);
        exit(1);
    }
    if(fwrite(fnodes, sizeof(fnode), MAXBLOCKS, fp) < MAXBLOCKS) {
        fprintf(stderr, "Error: Failed to update fnodes array\n");
        closefs(fp);
        exit(1);
    }


    closefs(fp);
}

int numofblocks(int size){
    if((float)size / (float)BLOCKSIZE > size / BLOCKSIZE){
        return (size / BLOCKSIZE) + 1;
    }
    return (size / BLOCKSIZE);
}

void deletefile(char *filesystemname, char *filename){

    fentry files[MAXFILES];
    fnode fnodes[MAXBLOCKS];


    FILE *fp = openfs(filesystemname, "r");


    if ((fread(files, sizeof(fentry), MAXFILES, fp)) == 0) {
        fprintf(stderr, "Error: could not read file entries\n");
        closefs(fp);
        exit(1);
    }
    if ((fread(fnodes, sizeof(fnode), MAXBLOCKS, fp)) == 0) {
        fprintf(stderr, "Error: could not read fnodes\n");
        closefs(fp);
        exit(1);
    }
    closefs(fp);

    int nameexists;
    nameexists = -1;
    int numblocks;
    int index;
    for(int i=0;i<MAXFILES;i++){
        if(strcmp(files[i].name, filename)==0){
            files[i].name[0] = '\0';
            nameexists = 0;
            if(files[i].size != 0){
                index = i;
                numblocks = numofblocks(files[i].size);
                files[i].size = 0;
                files[i].firstblock = -1;
                break;
            }
        }
    }

    if(nameexists == -1){
        fprintf(stderr, "Error: File not found\n");
        exit(1);
    }
    int indexlist[numblocks];
    int curblock = files[index].firstblock;
    for(int i=0;i<numblocks;i++){
        indexlist[i] = curblock;
        fnodes[curblock].blockindex *= -1;
        fnodes[curblock].nextblock = -1;
        curblock = fnodes[curblock].nextblock;
    }

    fp = openfs(filesystemname, "r+");

    for(int i=0;i<numblocks;i++){
        char tempblock[BLOCKSIZE + 1] = {0};
        if(fseek(fp, indexlist[i]*BLOCKSIZE, SEEK_SET) != 0){
            fprintf(stderr, "Error: Failed to find the index in the unix file\n");
            closefs(fp);
            exit(1);
        }
        fwrite(tempblock, BLOCKSIZE, 1, fp);
        if (ferror(fp)) {
            fprintf(stderr, "Error: could not write to data block\n");
            closefs(fp);
            exit(1);
    }
    }
    closefs(fp);
    fp = openfs(filesystemname, "w");
    if(fwrite(files, sizeof(fentry), MAXFILES, fp) < MAXFILES) {
        fprintf(stderr, "Error: Failed to update files array\n");
        closefs(fp);
        exit(1);
    }

    if(fwrite(fnodes, sizeof(fnode), MAXBLOCKS, fp) < MAXBLOCKS) {
        fprintf(stderr, "Error: Failed to update fnodes array\n");
        closefs(fp);
        exit(1);
    }
    closefs(fp);
}


void readfile(char *filesystemname, char *filename, int start, int length){
    fentry files[MAXFILES];
    fnode fnodes[MAXBLOCKS];


    FILE *fp = openfs(filesystemname, "r");


    if ((fread(files, sizeof(fentry), MAXFILES, fp)) == 0) {
        fprintf(stderr, "Error: could not read file entries\n");
        closefs(fp);
        exit(1);
    }
    if ((fread(fnodes, sizeof(fnode), MAXBLOCKS, fp)) == 0) {
        fprintf(stderr, "Error: could not read fnodes\n");
        closefs(fp);
        exit(1);
    }
    closefs(fp);

    int nameexists;
    nameexists = -1;
    int numblocks;
    int index;
    for(int i=0;i<MAXFILES;i++){
        if(strcmp(files[i].name, filename)==0){
            files[i].name[0] = '\0';
            nameexists = 0;
            if(files[i].size != 0){
                index = i;
                numblocks = numofblocks(files[i].size);
                break;
            }
        }
    }
    if(nameexists == -1){
        fprintf(stderr, "Error: File not found\n");
        exit(1);
    }
    int indexlist[numblocks];
    int curblock = files[index].firstblock;
    for(int i=0;i<numblocks;i++){
        indexlist[i] = curblock;
        curblock = fnodes[curblock].nextblock;
    }

    fp = openfs(filesystemname, "r+");
    if(BLOCKSIZE - start >= length){
        if(fseek(fp, start + (files[index].firstblock * BLOCKSIZE), SEEK_SET) != 0){
            fprintf(stderr, "Error: Failed to find the index in the unix file\n");
            closefs(fp);
            exit(1);
        }
        char read[length + 1];
        fread(read, 1, length, fp);
        fwrite(read, 1, length, stdout);
    }
    else{
        int size;
        size = 0;

        int curblock;
        curblock = files[indexlist[0]].firstblock;


        fseek(fp, (BLOCKSIZE * curblock) + start, SEEK_SET);
        int space;
        space = BLOCKSIZE - start;
        int read;
        read = 0;
        for(int i=0;i < numblocks;i++){
            char sectionblock[space + 1];
            sectionblock[0] = '\0';
            fgets(sectionblock, space + 1, fp);
            fwrite(sectionblock, 1, space, stdout);
            size += space;
            if(i + 1 != numblocks){
                curblock = fnodes[curblock].nextblock;
                fseek(fp, BLOCKSIZE * curblock, SEEK_SET);
            }
            /*
            if(i + 1 == numblocks){
                int bytes_to_write = BLOCKSIZE - (length - read);

                char zerobuf[BLOCKSIZE] = {0};
                if (bytes_to_write != 0  && fwrite(zerobuf, bytes_to_write, 1, fp) < 1) {
                    fprintf(stderr, "Error: write failed on init\n");
                    closefs(fp);
                    exit(1);
                }
            }*/
            read += space;
            if(length - read > BLOCKSIZE){
                space = BLOCKSIZE;
            }
            else{
                space = length - read;
            }
        }
    }


}

void writefile(char *filesystemname, char *filename, int start, int length){
    fentry files[MAXFILES];
    fnode fnodes[MAXBLOCKS];



    FILE *fp = openfs(filesystemname, "r");

    if(fseek(fp, 0, SEEK_END) != 0){
        fprintf(stderr, "Error: Failed to read unix file\n");
        closefs(fp);
        exit(1);
    }

    if(ftell(fp) < start){
        fprintf(stderr, "Error: Starting index out of range\n");
        closefs(fp);
        exit(1);
    }
    closefs(fp);
    fp = openfs(filesystemname, "r");


    if ((fread(files, sizeof(fentry), MAXFILES, fp)) == 0) {
        fprintf(stderr, "Error: could not read file entries\n");
        closefs(fp);
        exit(1);
    }
    if ((fread(fnodes, sizeof(fnode), MAXBLOCKS, fp)) == 0) {
        fprintf(stderr, "Error: could not read fnodes\n");
        closefs(fp);
        exit(1);
    }
    closefs(fp);


    int nameexists;
    nameexists = -1;
    int numblocks;
    numblocks = 0;
    //int *indices;
    int fentryindex;
    for(int i=0;i<MAXFILES;i++){
        if(strcmp(files[i].name, filename)==0){


            fentryindex = i;
            nameexists = 0;

            //figure out the number of blocks the file takes up
            if((((float)start + (float)length) / (float)BLOCKSIZE > (float)(files[i].size) / (float)BLOCKSIZE)){

                int indexlist[numofblocks((start + length))];
                //indices = indexlist;
                numblocks = numofblocks((start + length));


                int counter;
                counter = 0;
                int nblock;
                if(files[i].firstblock != -1){
                    nblock = files[i].firstblock;
                    for(int j=0; j < (numofblocks(files[i].size));j++){
                        indexlist[j] = fnodes[nblock].blockindex;
                        nblock = fnodes[nblock].nextblock;
                    }
                }
                int used;
                used = -1;
                for(int j=0; j < (numblocks - numofblocks(files[i].size));j++){
                    for(int k=0; k < MAXBLOCKS; k++){
                        if(fnodes[k].blockindex < 0 && fnodes[k].nextblock == -1){
                            for(int l=0; l < j;l++){
                                if(indexlist[l] == fnodes[k].blockindex * -1){
                                    used = 0;
                                }
                            }
                            if(used != 0){
                                indexlist[j + numofblocks(files[i].size)] = fnodes[k].blockindex * -1;
                                counter += 1;
                                break;
                            }
                            used = -1;
                        }
                    }
                }
                if(counter != (numblocks - numofblocks(files[i].size))){
                    fprintf(stderr, "Error: not enough memory\n");
                    exit(1);
                }

                if(files[i].firstblock == -1){
                    files[i].firstblock = indexlist[0];
                    fnodes[indexlist[0]].blockindex *= -1;
                    if(numblocks > 1){
                        fnodes[indexlist[0]].nextblock = indexlist[1];
                    }
                }
                if(numblocks > 1){
                    nblock = indexlist[1];
                }
                else{
                    nblock = indexlist[numblocks - numofblocks(files[i].size) - 1];
                }

                for(int j=0; j < (numblocks - numofblocks(files[i].size) - 1); j++){
                    if(fnodes[nblock].blockindex < 0){
                        fnodes[nblock].blockindex *= -1;
                    }
                    if(j + 2 != numblocks - numofblocks(files[i].size)){
                        fnodes[nblock].nextblock = indexlist[j + 2];
                    }
                    nblock = fnodes[nblock].nextblock;
                }

            }
            else{
                //int nblock;
                //int indexlist[numofblocks(files[i].size)];
                numblocks = numofblocks(files[i].size);
                //indices = indexlist;
            }
            break;
        }
    }

    if(nameexists == -1){
        fprintf(stderr, "Error: File not found\n");
        exit(1);
    }

    /*char input[length + 1];
    for(int i=0; i< length;i++){
        input[i] = 0b0;
    }*/

    fp = openfs(filesystemname, "r+");

    if(BLOCKSIZE - start >= length){
        if(fseek(fp, start + (files[fentryindex].firstblock * BLOCKSIZE), SEEK_SET) != 0){
            fprintf(stderr, "Error: Failed to find the index in the unix file\n");
            closefs(fp);
            exit(1);
        }

        char input[length + 1];
        input[0] = '\0';
        if(fgets(input, length + 1, stdin) == NULL){
            fprintf(stderr, "Error: input failed\n");
            exit(1);
        }



        fwrite(input, 1, length, fp);
        //fputs(input, fp);
        int bytes_to_write = (BLOCKSIZE - (length % BLOCKSIZE)) % BLOCKSIZE;
        char zerobuf[BLOCKSIZE] = {0};
        if (bytes_to_write != 0  && fwrite(zerobuf, bytes_to_write, 1, fp) < 1) {
            fprintf(stderr, "Error: write failed on init\n");
            closefs(fp);
            exit(1);
        }
        files[fentryindex].size = BLOCKSIZE - bytes_to_write;

    }
    else{
        int size;
        size = 0;

        int curblock;
        curblock = files[fentryindex].firstblock;
        for(int i=0;i< start;i++){
            char single[2];

            if(fseek(fp, curblock * BLOCKSIZE + i, SEEK_SET) != 0){
                fprintf(stderr, "Error: Failed to find the index in the unix file\n");
                closefs(fp);
                exit(1);
            }
            if(fread(single, 1, 1, fp) != 1){
                fprintf(stderr, "Error: Failed to read from unix file\n");
                closefs(fp);
                exit(1);
            }
            if(single[0] != 0b0){
                size += 1;
            }
        }
        closefs(fp);
        fp = openfs(filesystemname, "r+");
        fseek(fp, (BLOCKSIZE * curblock) + start, SEEK_SET);
        int space;
        space = BLOCKSIZE - start;
        int read;
        read = 0;
        for(int i=0;i < numblocks;i++){
            char sectionblock[space + 1];
            sectionblock[0] = '\0';
            fgets(sectionblock, space + 1, stdin);
            fwrite(sectionblock, 1, space, fp);
            size += space;
            if(i + 1 != numblocks){
                curblock = fnodes[curblock].nextblock;
                fseek(fp, BLOCKSIZE * curblock, SEEK_SET);
            }
            if(i + 1 == numblocks){
                //fseek(fp, (BLOCKSIZE * startingindex) + space + 1, SEEK_SET);
                int bytes_to_write = BLOCKSIZE - (length - read);

                char zerobuf[BLOCKSIZE] = {0};
                if (bytes_to_write != 0  && fwrite(zerobuf, bytes_to_write, 1, fp) < 1) {
                    fprintf(stderr, "Error: write failed on init\n");
                    closefs(fp);
                    exit(1);
                }
            }
            read += space;
            if(length - read > BLOCKSIZE){
                space = BLOCKSIZE;
            }
            else{
                space = length - read;
            }
        }
        files[fentryindex].size = size;
    }
    fseek(fp, 0, SEEK_SET);
    if(fwrite(files, sizeof(fentry), MAXFILES, fp) < MAXFILES) {
        fprintf(stderr, "Error: Failed to update files array\n");
        closefs(fp);
        exit(1);
    }

    if(fwrite(fnodes, sizeof(fnode), MAXBLOCKS, fp) < MAXBLOCKS) {
        fprintf(stderr, "Error: Failed to update fnodes array\n");
        closefs(fp);
        exit(1);
    }

    closefs(fp);
}




/* File system operations: creating, deleting, reading from, and writing to files.
 */

// Signatures omitted; design as you wish.