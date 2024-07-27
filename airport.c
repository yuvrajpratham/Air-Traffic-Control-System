#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>

#define MAX_RUNWAYS 10
#define MAX_MESSAGE_LEN 100

struct Runway {
    int id;
    int load_capacity;
    pthread_mutex_t lock;
};

struct Plane {
    int plane_id;
    int total_weight;
};

struct Message {
    long mtype;
    char mtext[100];
};

struct Runway runways[MAX_RUNWAYS];
int num_runways;
int airport_num;
int msgid;

void send_to_atc(int msgid, const char *mtext);
void cleanup_resources();
void handle_departure(struct Plane plane);
void handle_arrival(struct Plane plane);
void receive_from_atc(int msgid, char *mtext);
void handle_termination_intimation(int msgid);

int main() {
    printf("Enter Airport Number: ");
    scanf("%d", &airport_num);

    printf("Enter number of Runways: ");
    scanf("%d", &num_runways);

    printf("Enter loadCapacity of Runways (give as a space separated list in a single line): ");
    for (int i = 0; i < num_runways; i++) {
        scanf("%d", &runways[i].load_capacity);
        runways[i].id = i + 1;
        pthread_mutex_init(&runways[i].lock, NULL);
    }

    pthread_t tid;

    key_t key = ftok(".", 'a');
    msgid = msgget(key, 0666 | IPC_CREAT);
    if (msgid == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    char mtext[100];
    while (1) {
        handle_termination_intimation(msgid);
        struct Plane plane;
        receive_from_atc(msgid, mtext);
        if (strcmp(mtext, "Departure") == 0) {
            printf("Enter plane ID for departure: ");
            scanf("%d", &plane.plane_id);
            printf("Enter total weight for departure: ");
            scanf("%d", &plane.total_weight);
            pthread_create(&tid, NULL, (void *(*)(void *))handle_departure, (void *)&plane);
            pthread_join(tid, NULL);
        } else if (strcmp(mtext, "Arrival") == 0) {
            printf("Enter plane ID for arrival: ");
            scanf("%d", &plane.plane_id);
            printf("Enter total weight for arrival: ");
            scanf("%d", &plane.total_weight);
            pthread_create(&tid, NULL, (void *(*)(void *))handle_arrival, (void *)&plane);
            pthread_join(tid, NULL);
        }
    }

    return 0;
}

void send_to_atc(int msgid, const char *mtext) {
    struct Message message;
    message.mtype = 1;
    strncpy(message.mtext, mtext, sizeof(message.mtext) - 1);
    if (msgsnd(msgid, &message, sizeof(message.mtext), 0) == -1) {
        perror("msgsnd");
        exit(EXIT_FAILURE);
    }
}

void receive_from_atc(int msgid, char *mtext) {
    struct Message message;
    if (msgrcv(msgid, &message, sizeof(message.mtext), 2, 0) == -1) {
        perror("msgrcv");
        exit(EXIT_FAILURE);
    }
    strncpy(mtext, message.mtext, sizeof(message.mtext));
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

void handle_termination_intimation(int msgid) {
    char mtext[MAX_MESSAGE_LEN];
    receive_from_atc(msgid, mtext);
    if (strcmp(mtext, "Terminate") == 0 || strcmp(mtext, "TerminateNow") == 0) {
        cleanup_resources();
        send_message(msgid, 2, "Airport cleanup");
        char confirmation[MAX_MESSAGE_LEN];
        receive_message(msgid, 2, confirmation);
        printf("Received cleanup confirmation from air traffic controller.\n");
        exit(EXIT_SUCCESS);
    }
}


void cleanup_resources() {
    for (int i = 0; i < num_runways; i++) {
        pthread_mutex_destroy(&runways[i].lock);
    }
}

void handle_departure(struct Plane plane) {
    int selected_runway_id = -1;
    for (int i = 0; i < num_runways; i++) {
        pthread_mutex_lock(&runways[i].lock);
        if (runways[i].load_capacity >= plane.total_weight) {
            selected_runway_id = runways[i].id;
            break;
        }
        pthread_mutex_unlock(&runways[i].lock);
    }

    if (selected_runway_id == -1) {
        selected_runway_id = num_runways + 1;
        pthread_mutex_lock(&runways[selected_runway_id - 1].lock);
    }

    printf("Boarding/Loading process for Plane %d...\n", plane.plane_id);
    sleep(3);

    char message[100];
    snprintf(message, sizeof(message), "Plane %d has completed boarding/loading and taken off from Runway No. %d of Airport No. %d.", plane.plane_id, selected_runway_id, airport_num);
    send_to_atc(msgid, message);

    sleep(2);

    pthread_mutex_unlock(&runways[selected_runway_id - 1].lock);
}

void handle_arrival(struct Plane plane) {
    int selected_runway_id = -1;
    for (int i = 0; i < num_runways; i++) {
        pthread_mutex_lock(&runways[i].lock);
        if (runways[i].load_capacity >= plane.total_weight) {
            selected_runway_id = runways[i].id;
            break;
        }
        pthread_mutex_unlock(&runways[i].lock);
    }

    if (selected_runway_id == -1) {
        selected_runway_id = num_runways + 1;
        pthread_mutex_lock(&runways[selected_runway_id - 1].lock);
    }

    printf("Plane %d has landed on Runway No. %d of Airport No. %d.\n", plane.plane_id, selected_runway_id, airport_num);
    sleep(2);

    printf("Deboarding/Unloading process for Plane %d...\n", plane.plane_id);
    sleep(3);

    char message[100];
    snprintf(message, sizeof(message), "Plane %d has landed on Runway No. %d of Airport No. %d and has completed deboarding/unloading.", plane.plane_id, selected_runway_id, airport_num);
    send_to_atc(msgid, message);

    pthread_mutex_unlock(&runways[selected_runway_id - 1].lock);
}
