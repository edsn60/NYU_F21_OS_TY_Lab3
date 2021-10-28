#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


char* concatenate_file(char **argv, int optind_){
    unsigned long size = 0;
    char *concatenated_file = (char*) malloc(sizeof(char));
    concatenated_file[0] = '\0';
    char *filename;
    FILE *fp;
    char *buffer = (char*) malloc(sizeof(char) * 1000);
    unsigned long read_size;
    for (char **file_ptr = &argv[optind_]; *file_ptr; file_ptr++){

        filename = *file_ptr;
        fp = fopen(filename, "rb");
        if (!fp){
            fprintf(stderr, "Error: failed to open file '%s', ignored\n", filename);
            continue;
        }
        while((read_size = fread(buffer, sizeof(char), 1000, fp)) != -1){

            size += read_size;
            concatenated_file = realloc(concatenated_file, sizeof(char) * (size + 1));
            strcat(concatenated_file, buffer);
            if (feof(fp)){
                break;
            }
        }
        concatenated_file[size] = '\0';
        fclose(fp);
    }
    return concatenated_file;
}


void encoding(char *content){
    if (!content || strlen(content) == 0){
        return;
    }
    char current_char = *content;
    int count = 0;
    for (char *c = content; *c; c++){
        if (*c == current_char){
            count++;
        }
        else{
            printf("%c", toascii(current_char));
            if (count > 1){
                // TODO:: byte format
                printf("%c", count);
            }
            current_char = *c;
            count = 1;
        }
    }
}


int main(int argc, char **argv) {

    int option = getopt(argc, argv, ":j:");

    if (option == 63){     // 63 == '?'
        fprintf(stderr, "Error: Unknown option '-%c' \n", optopt);
        exit(-1);
    }

    else if(option == 58 || option == -1){     // 58 == ':', means that no "-j" option, single thread
        printf("SingleThread\n");
    }
    else{
        printf("MultiThread\n");
        char *arg = optarg;
        long threads_count = strtol(arg, NULL, 10);
    }
    char *arg = optarg;
    long threads_count = strtol(arg, NULL, 10);
    printf("threads_count: %ld\n", threads_count);
    char *concatenated_file = concatenate_file(argv, optind);
    encoding(concatenated_file);
}
