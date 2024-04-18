#include "message.h"

void parse_message(char *str, struct Message *msg) {
    // prase the message from str (in the form of type:size:source:data) to the struct Message
    char temp_str[MAX_MESSAGE];
    strcpy(temp_str, str);
    int headerlen = 0;

    //$ type
    char *type_char = strtok(temp_str, ":");  // split string into tokens
    headerlen += strlen(type_char) + 1;  // +1 is for ":"
    msg->type = atoi(type_char);

    //$ size
    char *size_char = strtok(NULL, ":");  // NULL: uses the position right after the end of the last token
    headerlen += strlen(size_char) + 1;
    msg->size = atoi(size_char);

    //$ source
    int source_end = headerlen; // headerlen is now at the first char of source
    for (; str[source_end] != ':'; source_end++)
        ;
    char source[source_end - headerlen + 1];
    memcpy(source, &str[headerlen], source_end - headerlen);
    source[source_end - headerlen] = '\0';
    memcpy(msg->source, source, source_end - headerlen + 1);

    //$ data
    memset(msg->data, 0, MAX_DATA);
    source_end++;
    char data[msg->size + 1];
    memcpy(data, &str[source_end], msg->size);
    data[msg->size] = '\0';
    memcpy(msg->data, data, msg->size + 1);
}

void print_msg(char *msg, bool sent) {
    if (sent)
        printf("Sent: ");
    else
        printf("Received: ");

    char temp_str[MAX_MESSAGE];
    strcpy(temp_str, msg);

    char *tpye_str = strtok(temp_str, ":");  // cut the first MACRO and print its name
    int type       = atoi(tpye_str);
    print_MACRO(type);
    printf("%s\n", msg + strlen(tpye_str));
}

void print_MACRO(int num) {
    // print MACRO in print_msg()
    switch (num) {
    case LOGIN:
        printf("LOGIN");
        break;
    case LO_ACK:
        printf("LO_ACK");
        break;
    case LO_NAK:
        printf("LO_NAK");
        break;
    case LOGOUT:
        printf("LOGOUT");
        break;
    case JOIN:
        printf("JOIN");
        break;
    case JN_ACK:
        printf("JN_ACK");
        break;
    case JN_NAK:
        printf("JN_NAK");
        break;
    case LEAVE_SESS:
        printf("LEAVE_SESS");
        break;
    case LS_ACK:
        printf("LS_ACK");
        break;
    case LS_NAK:
        printf("LS_NAK");
        break;
    case NEW_SESS:
        printf("NEW_SESS");
        break;
    case NS_ACK:
        printf("NS_ACK");
        break;
    case NS_NAK:
        printf("NS_NAK");
        break;
    case MESSAGE:
        printf("MESSAGE");
        break;
    case MESSAGE_ACK:
        printf("MESSAGE_ACK");
        break;
    case MESSAGE_NAK:
        printf("MESSAGE_NAK");
        break;
    case PRIVATE:
        printf("PRIVATE");
        break;
    case PRIVATE_ACK:
        printf("PRIVATE_ACK");
        break;
    case PRIVATE_NAK:
        printf("PRIVATE_NAK");
        break;
    case QUERY:
        printf("QUERY");
        break;
    case QU_ACK:
        printf("QU_ACK");
        break;
    case QUIT:
        printf("QUIT");
        break;
    default:
        printf("UNKNOWN");
        break;
    }
}


int get_MACRO(char *command) {
    // return MACRO to each command
    if (strcmp(command, "/login") == 0) {
        return LOGIN;
    } else if (strcmp(command, "/logout") == 0) {
        return LOGOUT;
    } else if (strcmp(command, "/joinsession") == 0) {
        return JOIN;
    } else if (strcmp(command, "/leavesession") == 0) {
        return LEAVE_SESS;
    } else if (strcmp(command, "/createsession") == 0) {
        return NEW_SESS;
    } else if (strcmp(command, "/list") == 0) {
        return QUERY;
    } else if (strcmp(command, "/quit") == 0) {
        return QUIT;
    } else if (command[0] == ':') { // private message is in the form of :<user_to_send> <private_message>
        return PRIVATE;
    } else {
        return MESSAGE;
    }
}