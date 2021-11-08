//
// Created by Ysw on 2021/10/31.
//
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>


unsigned char *encoding(char *content, size_t size){
    unsigned char *result = (unsigned char*) malloc(sizeof(unsigned char) * (size * 2 + 1));
    if (!result){
        fprintf(stderr, "Error: malloc failed in encoder.c:12\n");
        exit(-1);
    }
    unsigned char *idx = result;
    if (!content || strlen(content) == 0){
        result[0] = '\0';
        return result;
    }
    char current_char = *content;
    int count = 0;
    for (int i = 0; i < size; i++){
        if (content[i] == current_char){
            count++;
        }
        else{
            *idx = toascii(current_char);
            idx++;
            *idx = (unsigned char)count;
            idx++;
            current_char = content[i];
            count = 1;
        }
    }
    *idx = toascii(current_char);
    idx++;
    *idx = (char)count;
    idx++;
    *idx = '\0';
    return result;
}