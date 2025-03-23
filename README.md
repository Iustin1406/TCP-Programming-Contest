# Client-Server Programming Contest

This project implements a client-server system for a programming competition. Participants connect to the server, receive a problem, submit their solution, and receive real-time feedback. The server automatically compiles and tests solutions, updating the leaderboard.

## Architecture

* **Client:**
    * Connects to the server using TCP/IP.
    * Receives the remaining time and problem statement.
    * Can send commands (e.g., `check <username>`, `quit`).
    * Receives feedback and the final leaderboard.
* **Server:**
    * Accepts client connections and manages the competition time.
    * Sends the problem statement and receives user solutions.
    * Compiles and executes the received code, verifying results with official tests.
    * Updates the leaderboard based on the obtained score.

## Workflow

1.  **Server Startup:**
    * The server loads configuration from `config.txt` (port, maximum number of users, number of problems, maximum time).
    * Waits for client connections.
2.  **Client Connection:**
    * The client sends a connection request (`<server_address> <port> <username>`).
    * The server verifies the remaining time and sends the problem.
3.  **Solution Submission:**
    * The client can check their solution using `check <username>`.
    * The server compiles and runs the solution with validation tests.
    * Responds with the obtained score.
4.  **Leaderboard and Disconnection:**
    * If the user exits (`quit`), the server sends the updated leaderboard.
    * At the end of the competition, the final leaderboard is generated.

## Important Files

* `server.c` – Server source code.
* `client.c` – Client source code.
* `config.txt` – Server settings (port, number of problems, time).
* `leaderboard.txt` – Contestant leaderboard.
* `problems/` – Contains problem statements and associated tests.
* `solutions/` – User-submitted solutions.

## Features

* ✔️ TCP client-server connection
* ✔️ Automatic solution submission and processing
* ✔️ Automatic compilation and testing
* ✔️ Dynamic leaderboard
* ✔️ Time limit for solution submission

## How to Compile and Run

1.  **Compile the Server:**
    * `gcc server.c -o server`
2.  **Compile the Client:**
    * `gcc client.c -o client`
3.  **Run the Server:**
    * `./server`
4.  **Run the Client:**
    * `./client <server_address> <port> <username>`

## Configuration

* The `config.txt` file contains the server configuration:
    * Port number
    * Maximum number of users
    * Number of problems
    * Max time of the contest.

## Problems and Solutions

* The `problems/` directory contains problem statements and test cases.
* The `solutions/` directory stores user-submitted solutions.

## Leaderboard

* The `leaderboard.txt` file tracks the contestants' scores.
