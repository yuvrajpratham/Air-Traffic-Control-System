#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#define MAX_PASSENGERS 10
#define MAX_CARGO_ITEMS 100
#define MAX_AIRPORTS 10
#define CREW_WEIGHT_PASSENGER 75
#define NUM_CREW_MEMBERS_PASSENGER 7
#define CREW_WEIGHT_CARGO 75
#define NUM_CREW_MEMBERS_CARGO 2
#define TERMINATION_PLANE_ID -1

struct Plane {
    int plane_id;
    int type;
    int occupied_seats;
    int airport_departure;
    int airport_arrival;
    int total_weight;
};

struct Message {
    long mtype;
    struct {
        int plane_id;
        int airport_departure;
        int airport_arrival;
    } content;
};

int msgid; // Message queue identifier

void simulate_flight(struct Plane *plane);
void handle_passenger(int pipefd[2]);
void process_messages();
int calculate_total_weight(int weights[], int count, int type);
void delay(int seconds);
void cleanup_and_terminate();

int main() {
    key_t key = ftok(".", 'a');
    msgid = msgget(key, 0666 | IPC_CREAT);

    if (msgid == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == 0) { // Air traffic controller process
        process_messages();
        exit(EXIT_SUCCESS);
    }

    while (1) {
    
        struct Plane plane;
        printf("Enter Plane ID : ");
        scanf("%d", &plane.plane_id);

        if (plane.plane_id == -1) {
            cleanup_and_terminate();
            break;
        }

        printf("Enter Type of Plane (0 for cargo, 1 for passenger): ");
        scanf("%d", &plane.type);
        printf("Enter Airport Number for Departure: ");
        scanf("%d", &plane.airport_departure);
        printf("Enter Airport Number for Arrival: ");
        scanf("%d", &plane.airport_arrival);

        if (plane.type == 1) { // Passenger plane
            printf("Enter Number of Occupied Seats: ");
            scanf("%d", &plane.occupied_seats);
            int weights[2 * plane.occupied_seats];
            int count = 0;
            for (int i = 0; i < plane.occupied_seats; i++) {
                int pipefd[2];
                if (pipe(pipefd) == -1) {
                    perror("pipe");
                    exit(EXIT_FAILURE);
                }
                pid_t cpid = fork();
                if (cpid == 0) {
                    handle_passenger(pipefd);
                    exit(EXIT_SUCCESS);
                } else {
                    close(pipefd[1]);
                    read(pipefd[0], &weights[count], sizeof(int));
                    count++;
                    read(pipefd[0], &weights[count], sizeof(int));
                    count++;
                    close(pipefd[0]);
                    wait(NULL);
                }
            }
            plane.total_weight = calculate_total_weight(weights, count, plane.type);
        } else { // Cargo plane
            printf("Enter Number of Cargo Items: ");
            scanf("%d", &plane.occupied_seats);
            int weight, total_cargo_weight = 0;
            for (int i = 0; i < plane.occupied_seats; i++) {
                printf("Enter Weight of Cargo Item %d: ", i + 1);
                scanf("%d", &weight);
                total_cargo_weight += weight;
            }
            plane.total_weight = total_cargo_weight + NUM_CREW_MEMBERS_CARGO * CREW_WEIGHT_CARGO;
        }

        simulate_flight(&plane);
    }

    msgctl(msgid, IPC_RMID, NULL); // Remove message queue
    return 0;
}


void simulate_flight(struct Plane *plane) {
    struct Message msg;
    msg.mtype = 1; // Using type 1 for all messages
    msg.content.plane_id = plane->plane_id;
    msg.content.airport_departure = plane->airport_departure;
    msg.content.airport_arrival = plane->airport_arrival;

    msgsnd(msgid, &msg, sizeof(msg.content), 0);

    printf("Boarding/Loading...\n");
    delay(3);
    
    printf("Flight in progress...\n");
    delay(30);

    printf("Deboarding/Unloading...\n");
    delay(3);

    printf("Plane %d has successfully traveled from Airport %d to Airport %d!\n",
           plane->plane_id, plane->airport_departure, plane->airport_arrival);
}

void handle_passenger(int pipefd[2]) {
    close(pipefd[0]); // Close read end in child
    int luggage_weight, body_weight;
    printf("Enter Weight of Your Luggage: ");
    scanf("%d", &luggage_weight);
    printf("Enter Your Body Weight: ");
    scanf("%d", &body_weight);
    write(pipefd[1], &luggage_weight, sizeof(luggage_weight));
    write(pipefd[1], &body_weight, sizeof(body_weight));
    close(pipefd[1]); // Close write end
}

void process_messages() {
    FILE *file = fopen("AirTrafficController.txt", "a");  // Open the file for appending
    if (!file) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    struct Message msg;
    while (1) {
        if (msgrcv(msgid, &msg, sizeof(msg.content), 1, 0) == -1) {
            if (errno == EIDRM) {
                fprintf(stderr, "Message queue was deleted.\n");
                break;
            }
            continue;
        }

        if (msg.content.plane_id == TERMINATION_PLANE_ID) {
            fprintf(stderr, "Termination signal received. Stopping message processing.\n");
            break;
        }

        fprintf(file, "Plane %d has departed from Airport %d and will land at Airport %d.\n",
                msg.content.plane_id, msg.content.airport_departure, msg.content.airport_arrival);
        fflush(file); 
    }

    fclose(file);  // Close the file
}

void send_termination_message() {
    struct Message msg;
    msg.mtype = 1;  // Assuming 1 is the message type for normal operations
    msg.content.plane_id = TERMINATION_PLANE_ID;
    msg.content.airport_departure = 0;
    msg.content.airport_arrival = 0;

    if (msgsnd(msgid, &msg, sizeof(msg.content), 0) == -1) {
        perror("Failed to send termination message");
    }
}

int calculate_total_weight(int weights[], int count, int type) {
    int total_weight = 0;
    for (int i = 0; i < count; i++) {
        total_weight += weights[i];
    }
    if (type == 1) { // Passenger plane
        total_weight += NUM_CREW_MEMBERS_PASSENGER * CREW_WEIGHT_PASSENGER;
    }
    return total_weight;
}

void delay(int seconds) {
    sleep(seconds);
}

void cleanup_and_terminate() {
    fprintf(stderr, "Termination signal received. Cleaning up and terminating.\n");
    msgctl(msgid, IPC_RMID, NULL); // Remove message queue
    exit(EXIT_SUCCESS);
}
