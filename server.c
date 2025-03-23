#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>

extern int errno;

time_t start_time_global = 0;
int timer_started = 0;

char *readFile(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (!fp)
    {
        perror("eroare la deschiderea fisierului");
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);
    char *content = (char *)malloc(size + 1);
    if (!content)
    {
        perror("eroare la alocarea mem");
        fclose(fp);
        return NULL;
    }
    fread(content, 1, size, fp);
    content[size] = '\0';
    fclose(fp);
    return content;
}

void run_command(const char *command)
{
    int result = system(command);
    if (result != 0)
    {
        fprintf(stderr, "Eroare la rularea comenzii: %s\n", command);
    }
}
int compare_files(const char *file1, const char *file2)
{
    char command[100];
    snprintf(command, sizeof(command), "diff %s %s", file1, file2);
    return system(command); // 0 daca fiseirele sunt identice
}  
void update_leaderboard(char *name, int score)
{
    char line[50];
    char temp_file[] = "leaderboard_temp.txt";
    FILE *file = fopen("leaderboard.txt", "r");
    FILE *temp = fopen(temp_file, "w");
    int found = 0;

    if (!file)
    {
        file = fopen("leaderboard.txt", "w");
        fprintf(file, "%s-1-%d\n", name, score); // adaug concurentul
        fclose(file);
        return;
    }

    // verific daca a mai dat vreun submit inainte
    while (fgets(line, sizeof(line), file))
    {
        char current_name[20];
        int attempts, current_score;

        sscanf(line, "%[^-]-%d-%d", current_name, &attempts, &current_score);

        if (strcmp(current_name, name) == 0)
        {
            found = 1;
            if (score > current_score)
            {
                fprintf(temp, "%s-%d-%d\n", name, attempts + 1, score);
            }
            else
            {
                fprintf(temp, "%s-%d-%d\n", name, attempts + 1, current_score);
            }
        }
        else
        {
            fprintf(temp, "%s", line);
        }
    }

    if (!found)
    {
        fprintf(temp, "%s-1-%d\n", name, score);
    }

    fclose(file);
    fclose(temp);
    remove("leaderboard.txt");
    rename(temp_file, "leaderboard.txt");
}
void exec_command(int prob_num, const char *username)
{
    int score = 0;
    char input_test1[256], expected_output_test1[256];
    sprintf(input_test1, "solutions/tests/problem%d/test1.in", prob_num);
    sprintf(expected_output_test1, "solutions/tests/problem%d/exp_out1.txt", prob_num);

    char input_test2[256], expected_output_test2[256];
    sprintf(input_test2, "solutions/tests/problem%d/test2.in", prob_num);
    sprintf(expected_output_test2, "solutions/tests/problem%d/exp_out2.txt", prob_num);

    char compile_command[512];
    sprintf(compile_command, "gcc -o solution solutions/%s/main.c", username);
    run_command(compile_command);

    // rulam testele
    char run_test1[512], run_test2[512];
    sprintf(run_test1, "./solution < %s > solutions/tests/problem%d/test1.out", input_test1, prob_num);
    sprintf(run_test2, "./solution < %s > solutions/tests/problem%d/test2.out", input_test1, prob_num);
    run_command(run_test1);
    run_command(run_test2);
    char output_path1[512], output_path2[512];
    sprintf(output_path1, "solutions/tests/problem%d/test1.out", prob_num);
    sprintf(output_path2, "solutions/tests/problem%d/test2.out", prob_num);
    // comparam outputurile
    if (compare_files(output_path1, expected_output_test1) == 0)
    {
        printf("Testul 1 a trecut cu succes!\n");
        score += 50;
    }
    else
    {
        printf("Testul 1 a esuat!\n");
    }

    if (compare_files(output_path2, expected_output_test2) == 0)
    {
        score += 50;
        printf("Testul 2 a trecut cu succes!\n");
    }
    else
    {
        printf("Testul 2 a esuat!\n");
    }
    update_leaderboard(username, score);
}

int main()
{
    FILE *fp;
    int port, max_users, num_of_problems, max_time;
    fp = fopen("config.txt", "r");
    if (!fp)
    {
        perror("Eroare la deschiderea fisierului de configurare");
        return 1;
    }
    if (fscanf(fp, "port:%d\nmax_users:%d\nnum_of_problems:%d\nmax_time:%d\n", &port, &max_users, &num_of_problems, &max_time) != 4)
    {
        perror("Eroare la citirea fisierului de configurare");
        fclose(fp);
        return 1;
    }
    fclose(fp);

    struct sockaddr_in server, from;
    int sd;
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[server] Eroare la socket()");
        return errno;
    }

    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);

    if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[server] Eroare la bind()");
        return errno;
    }

    if (listen(sd, max_users) == -1)
    {
        perror("[server] Eroare la listen()");
        return errno;
    }

    printf("Serverul ruleaza pe portul %d\n", port);

    srand(time(NULL));
    int random_number = rand() % num_of_problems + 1;
    // DE STERS
    random_number = 2;
    printf("Random number: %d\n", random_number);
    char filename[100];
    sprintf(filename, "problems/problem%d/enunt.txt", random_number);

    while (1)
    {
        int client, length = sizeof(from);
        client = accept(sd, (struct sockaddr *)&from, &length);
        if (client < 0)
        {
            perror("[server] Eroare la accept()");
            continue;
        }

        // pornim timerul global la prima logare
        if (!timer_started)
        {
            start_time_global = time(NULL);
            timer_started = 1;
        }

        time_t current_time = time(NULL);
        int time_left = max_time - (int)difftime(current_time, start_time_global);
        if (time_left <= 0)
        {
            printf("[server] Timpul a expirat. Refuzam clientul.\n");
            close(client);
            continue;
        }

        // trimite timpul ramas spre client
        if (write(client, &time_left, sizeof(time_left)) < 0)
        {
            perror("[server] Eroare la trimiterea timpului ramas");
            close(client);
            continue;
        }

        int pid = fork();
        if (pid < 0)
        {
            perror("[server] Eroare la fork()");
            close(client);
            continue;
        }

        if (pid == 0)
        { // Proces copil
            close(sd);
            char *content = readFile(filename);
            if (!content)
            {
                perror("Eroare la citirea fisierului");
                close(client);
                exit(1);
            }

            // trimite problema la client
            if (write(client, content, strlen(content)) < 0)
            {
                perror("Eroare la trimiterea problemei catre client");
                free(content);
                close(client);
                exit(1);
            }
            free(content);

            char buffer[1024];
            while (time_left > 0)
            {
                int bytes_received = read(client, buffer, sizeof(buffer) - 1);
                if (bytes_received > 0)
                {
                    buffer[bytes_received] = '\0';
                    if (strstr(buffer, "quit"))
                    {
                        printf("Clientul a ales sa se deconecteze.\n");
                        break;
                    }
                    if (!strstr(buffer, "check "))
                    {
                        char response[200];
                        sprintf(response, "Comanda incorecta(%s), forma corecta :check <username>\n", buffer);
                        if (write(client, response, strlen(response)) < 0)
                        {
                            perror("Eroare la trimiterea rasp catre client");
                            break;
                        }
                    }
                    else
                    {
                        // luam numele la user sa ii compilam problema
                        char username[20];
                        strcpy(username, strchr(buffer, ' ') + 1);
                        printf("Mesaj de la clientul '%s' -> %s\n", username, buffer);
                        current_time = time(NULL);
                        time_left = max_time - (int)difftime(current_time, start_time_global);

                        char response[100] = "Mesaj primit. Poti sa mai trimiti solutii in timpul ramas!";
                        if (time_left > 0)
                        {
                            exec_command(random_number, username);
                        }
                        else
                        {
                            strcpy(response, "Solutia nu va fi compilata pentru ca a expirat timpul ramas");
                        }
                        if (write(client, response, strlen(response)) < 0)
                        {
                            perror("Eroare la trimiterea rasp catre client");
                            break;
                        }
                    }
                }
                else if (bytes_received == 0)
                {
                    printf("clientul s-a deconectat\n");
                    break;
                }
                else
                {
                    perror("eroare la citirea datelor de la client");
                    break;
                }

                // actualizeaza timpul ramas
                // current_time = time(NULL);
                // time_left = max_time - (int)difftime(current_time, start_time_global);
            }

            // Trimite leaderboard-ul la final
            printf("Se trimite leaderboard-ul catre client...\n");
            char *leaderboard = readFile("leaderboard.txt");
            if (leaderboard)
            {
                printf("%s", leaderboard);
                if (write(client, leaderboard, strlen(leaderboard)) < 0)
                {
                    perror("Eroare la trimiterea leaderboard-ului");
                }
                else
                {
                    printf("Leaderboard trimis cu succes catre client\n");
                }
                free(leaderboard);
            }
            else
            {
                perror("Nu s-a putut citi leaderboardul.");
            }

            close(client);
            exit(0);
        }
        else
        { // proces parinte
            close(client);
            while (waitpid(-1, NULL, WNOHANG) > 0)
                ; 
        }
    }

    close(sd);
    return 0;
}
