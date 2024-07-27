#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>

#define MSG_SIZE 100

// Message structure for communication between cleanup process and air traffic controller
struct Message {
    long mtype;
    char mtext[MSG_SIZE];
};

// Function prototypes
void send_message(int msgid, int mtype, const char *mtext);
void receive_message(int msgid, int mtype, char *mtext);

int main() {
    int msgid;
    key_t key = ftok(".", 'a');
    msgid = msgget(key, 0666 | IPC_CREAT);

    // Get the message queue ID
    if ((msgid = msgget(key, 0666)) == -1) {
        perror("msgget");
        exit(1);
    }

    // Send termination request to air traffic controller
    char response;
    do {
        printf("Do you want the Air Traffic Control System to terminate? (Y for Yes and N for No): ");
        scanf(" %c", &response);
    } while (response != 'Y' && response != 'N');

    if (response == 'Y') {
        printf("Sending termination request to air traffic controller...\n");
        send_message(msgid, 2, "Terminate");
        printf("Sent termination request to air traffic controller.\n");
        
        // Receive termination confirmation from air traffic controller
        char confirmation[MSG_SIZE];
        receive_message(msgid, 2, confirmation);
        printf("Received confirmation message from air traffic controller: %s\n", confirmation);
    }

    // Delete the message queue
    if (msgctl(msgid, IPC_RMID, NULL) == -1) {
        perror("msgctl");
        exit(1);
    }
    printf("Message queue deleted.\n");

    return 0;
}

// Function to send message to message queue
void send_message(int msgid, int mtype, const char *mtext) {
    struct Message message;
    message.mtype = mtype;
    strncpy(message.mtext, mtext, sizeof(message.mtext) - 1);
    if (msgsnd(msgid, &message, sizeof(message.mtext), 0) == -1) {
        perror("msgsnd");
        exit(1);
    }
}

// Function to receive message from message queue
void receive_message(int msgid, int mtype, char *mtext) {
    struct Message message;
    if (msgrcv(msgid, &message, sizeof(message.mtext), mtype, 0) == -1) {
        perror("msgrcv");
        exit(1);
    }
    strncpy(mtext, message.mtext, sizeof(message.mtext));
}
