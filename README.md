# Air-Traffic-Control-System
Table of Contents
Problem Statement
Assignment Constraints
Submission Guidelines
Plagiarism Policy
Demo Guidelines
Problem Statement
In this assignment, we will create an Air Traffic Control System using various OS concepts. The system consists of the following entities:

Plane (Each plane is a single-threaded process):
Passenger plane: includes passengers, 5 cabin crew members, and 2 pilots
Cargo plane: includes only 2 pilots
Airport (Each airport is a multi-threaded process)
Air Traffic Controller (ATC) (This is a single-threaded process)
Passenger (Each passenger process is a child of the corresponding plane process and is single-threaded)
Cleanup (This is a single-threaded process)
The overall block diagram of the Air Traffic Control System is depicted in the problem statement document. Note that the diagram only depicts passenger plane processes.

Detailed Requirements
Plane Process (plane.c)

Execution creates a separate plane process.
Plane ID, type, number of passengers, luggage weight, and body weight are input by the user.
For passenger planes, child processes are created for each passenger.
Total weight of the plane is calculated.
Communication with ATC for departure and arrival processes.
Air Traffic Controller (airtrafficcontroller.c)

Manages the planes and directs them to airports.
Handles a single message queue for communication.
Logs plane departures and arrivals in AirTrafficController.txt.
Cleanup Process (cleanup.c)

Runs concurrently, asking for system termination.
Communicates termination to the ATC when required.
Airport Process (airport.c)

Execution creates a separate airport process.
Manages multiple runways with load capacities.
Handles plane arrivals and departures using best-fit selection for runways.
Synchronization and IPC
Pipes for communication between plane and passenger processes.
Single message queue for communication between ATC and other processes.
Mutexes/Semaphores for synchronization in airport runway management.
