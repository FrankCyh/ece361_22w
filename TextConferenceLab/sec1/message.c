#include "message.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void parse_message(char *str, struct Message* msg) {

    char temp_str[MAX_GLOBAL];  // arbitrary, roughly 20
    strcpy(temp_str, str);
    int headerlen = 0;

    memset(msg->data, 0, MAX_DATA);

    char *type_char = strtok(temp_str, ":"); // split string into tokens

    headerlen += strlen(type_char) + 1; // +1 is for ":"
    msg->type = atoi(type_char);

    char *size_char = strtok(NULL, ":"); // NULL: uses the position right after the end of the last token
    headerlen += strlen(size_char) + 1;
    msg->size = atoi(size_char);

    int source_end = headerlen;
    for (; str[source_end] != ':'; source_end++)
        continue;

    char source[source_end - headerlen + 1];
    memcpy(source, &str[headerlen], source_end - headerlen);
    source[source_end - headerlen] = '\0';
    memcpy(msg->source, source, source_end - headerlen + 1);

    source_end++;
    
    char data[msg->size + 1];
    memcpy(data, &str[source_end], msg->size);
    data[msg->size] = '\0';
    memcpy(msg->data, data, msg->size + 1);

    printf("%d:%d:%s:%s\n", msg->type, msg->size, msg->source, msg->data);

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    /*int type = 0;
    int size = 0;

    int pt = 0;
    while (str[pt] != ':') {
        type = type * 10 + str[pt] - '0';
        pt++;
    }
    msg->type = type;

    pt++;
    while (str[pt] != ':') {
        size = size * 10 + str[pt] - '0';
        pt++;
    }
    msg->size = size;

    pt++;
    int st_pt = pt;
    while (str[pt] != ':')
        pt++;

    char source[sizeof(char) * (pt - st_pt + 1)];
    memcpy (source, &str[st_pt], pt - st_pt);
    source[pt - st_pt] = '\0';
    memcpy(msg->source, source, pt - st_pt + 1);

    pt++;
    st_pt = pt;
    
    char data[sizeof(char) * (size + 1)];
    memcpy (data, &str[st_pt], size);
    data[size] = '\0';
    memcpy(msg->data, data, size + 1);
    
    printf("%d:%d:%s:%s\n", msg->type, msg->size, msg->source, msg->data);*/
}