//
// Created by Ysw on 2021/10/31.
//
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>


char *encoding(char *content){
    char *result = (char*) malloc(sizeof(char) * 8193);
    if (!result){
        fprintf(stderr, "Error: malloc failed in encoder.c:11\n");
        exit(-1);
    }
    int idx = 0;
    if (!content || strlen(content) == 0){
        result[0] = '\0';
        return result;
    }
    char current_char = *content;
    int count = 0;
    for (char *c = content; *c; c++){
        if (*c == current_char){
            count++;
        }
        else{
            result[idx++] = toascii(current_char);
            result[idx++] = (char)count;
            current_char = *c;
            count = 1;
        }
    }
    result[idx++] = current_char;
    result[idx++] = (char)count;
    result[idx] = '\0';
    return result;
}