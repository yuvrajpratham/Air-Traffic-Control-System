#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#define MAX_AIRPORTS 10
#define MAX_MESSAGE_LEN 100

struct Message {
    long mtype;
    char mtext[MAX_MESSAGE_LEN];
};

// Function prototypes
void handle_air_traffic_controller(int num_airports);
void send_message(int msgid, int mtype, const char *mtext);
void receive_message(int msgid, int mtype, char *mtext);
bool is_empty(const char *filename);
void append_to_file(const char *filename, const char *message);
bool all_planes_departed(const char *filename, int num_planes);
void cleanup_and_terminate(int msgid);

int main() {
    int num_airports;

    printf("Enter the number of airports to be handled/managed: ");
    scanf("%d", &num_airports);
    if (num_airports < 2 || num_airports > MAX_AIRPORTS) {
        printf("Invalid number of airports. Please enter a number between 2 and %d.\n", MAX_AIRPORTS);
        exit(EXIT_FAILURE);
    }

    handle_air_traffic_controller(num_airports);

    return 0;
}

// Function to handle air traffic controller process
void handle_air_traffic_controller(int num_airports) {
    int msgid;
    key_t key = ftok(".", 'a');
    msgid = msgget(key, 0666 | IPC_CREAT);

    if ((msgid = msgget(key, 0666)) == -1) {
        printf("Error occurred while creating message queue. Key: %d, errno: %d\n", key, errno);
        perror("msgget");
        exit(1);
    }

    FILE *fp = fopen("AirTrafficController.txt", "a");
    if (fp == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    struct Message message;
    int num_planes = 0;
    int planes_departed = 0;

    while (planes_departed < num_planes || !is_empty("AirTrafficController.txt")) {
        receive_message(msgid, 1, message.mtext);

        if (strstr(message.mtext, "Plane ID:") != NULL) {
            append_to_file("AirTrafficController.txt", message.mtext);
            num_planes++;
        } else if (strstr(message.mtext, "Plane deboarding completed") != NULL) {
            planes_departed++;
        } else if (strstr(message.mtext, "Airport termination confirmation") != NULL) {
            num_airports--;
        } else if (strstr(message.mtext, "Terminate") != NULL) {
            printf("Received termination request from cleanup process. Sending confirmation...\n");
            send_message(msgid, 2, "Termination confirmation");
            cleanup_and_terminate(msgid);
        }

        if (all_planes_departed("AirTrafficController.txt", num_planes) && num_airports == 0) {
            cleanup_and_terminate(msgid);
        }
    }

    fclose(fp);
}

void send_message(int msgid, int mtype, const char *mtext) {
    struct Message message;
    message.mtype = mtype;
    strncpy(message.mtext, mtext, sizeof(message.mtext) - 1);
    if (msgsnd(msgid, &message, sizeof(message.mtext), 0) == -1) {
        perror("msgsnd");
        exit(EXIT_FAILURE);
    }
}

void receive_message(int msgid, int mtype, char *mtext) {
    struct Message message;
    if (msgrcv(msgid, &message, sizeof(message.mtext), mtype, 0) == -1) {
        if (errno != ENOMSG) { // Ignore if no message available
            perror("msgrcv");
            exit(EXIT_FAILURE);
        }
    }
    strncpy(mtext, message.mtext, sizeof(message.mtext));
}

// Function to check if a file is empty
bool is_empty(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fclose(fp);
    return size == 0;
}

// Function to append a message to a file
void append_to_file(const char *filename, const char *message) {
    FILE *fp = fopen(filename, "a");
    if (fp == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    fprintf(fp, "%s\n", message);
    fclose(fp);
}

bool all_planes_departed(const char *filename, int num_planes) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    char line[MAX_MESSAGE_LEN];
    int planes = 0;
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strstr(line, "Plane") != NULL) {
            planes++;
        }
    }
    fclose(fp);
    return planes >= num_planes;
}

void cleanup_and_terminate(int msgid) {

    send_message(msgid, 2, "Air traffic controller cleanup");
    char confirmation[MAX_MESSAGE_LEN];
    receive_message(msgid, 2, confirmation);
    printf("Received cleanup confirmation from airport process.\n");
    exit(EXIT_SUCCESS);
}
