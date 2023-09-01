#include <sys/types.h>
#include <sys/socket.h>
#include <syslog.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

volatile int signal_received = 0;

void signal_handler(int signum)
{
    if (signum == SIGINT || signum == SIGTERM)
    {
        syslog(LOG_USER, "Caught signal, exiting");
        signal_received = 1;
    }
}

int append_data_to_file(char *data, size_t num_bytes)
{
    // Append received data to file
    FILE *outputfile = fopen("/var/tmp/aesdsocketdata", "a+");
    if (outputfile == NULL)
    {
        syslog(LOG_ERR, "Opening /var/tmp/aesdsocketdata for appending failed.");
        return -1;
    }
    size_t bytes_written = fwrite(data, 1, num_bytes, outputfile);
    syslog(LOG_USER, "Wrote %ld bytes to file.", bytes_written + 1);
    fprintf(outputfile, "\n");
    fclose(outputfile);
    return 0;
}

int read_file_and_send_back(int client_socket_fd)
{
    // Send back file content line wise
    FILE *inputfile = fopen("/var/tmp/aesdsocketdata", "r");
    if (inputfile == NULL)
    {
        syslog(LOG_ERR, "Opening /var/tmp/aesdsocketdata for reading failed.");
        return -1;
    }

    char *readline = NULL;
    size_t n = 0;
    ssize_t line_length = 0;

    while (-1 != (line_length = getline(&readline, &n, inputfile)))
    {
        syslog(LOG_USER, "Read line from file and send back: %s", readline);
        ssize_t bytes_sent = send(client_socket_fd, readline, line_length, 0);
        if (bytes_sent == -1)
        {
            syslog(LOG_USER, "Send failed.");
        }
        else
        {
            syslog(LOG_USER, "Sent %ld bytes.", bytes_sent);
        }
    }
    free(readline);
    fclose(inputfile);
    return 0;
}

void remove_file()
{
    remove("/var/tmp/aesdsocketdata");
}

int main(int argc, char *argv[])
{
    syslog(LOG_USER, "Starting aesdsocket");

    remove_file();

    int daemon_mode = 0;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-d") == 0)
        {
            syslog(LOG_USER, "Starting in daemon mode.");
            daemon_mode = 1;
            break;
        }
    }

    struct sigaction sigact = {0};
    sigact.sa_handler = signal_handler;

    if (sigaction(SIGINT, &sigact, NULL) != 0)
    {
        return -1;
    }
    if (sigaction(SIGTERM, &sigact, NULL) != 0)
    {
        return -1;
    }

    int server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == server_socket_fd)
    {
        syslog(LOG_ERR, "Opening socket failed.");
        return -1;
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(9000);
    server_address.sin_addr.s_addr = inet_addr("0.0.0.0");

    if (bind(server_socket_fd, (struct sockaddr *)&server_address, sizeof(struct sockaddr_in)) < 0)
    {
        syslog(LOG_ERR, "Binding to port 9000 failed.");
        close(server_socket_fd);
        return -1;
    }

    pid_t pid;
    if (daemon_mode)
    {
        pid = fork();
        if (pid < 0)
        {
            return -1;
        }
        else if (pid > 0)
        {
            close(server_socket_fd);
            return 0;
        }
    }

    if (listen(server_socket_fd, 10) < 0)
    {
        syslog(LOG_ERR, "listen failed.");
        close(server_socket_fd);
        return -1;
    }

    while (!signal_received)
    {
        struct sockaddr_in client_address;
        socklen_t client_addr_len = sizeof(client_address);

        memset(&client_address, 0, sizeof(client_address));
        int client_socket_fd = accept(server_socket_fd, (struct sockaddr *)&client_address, &client_addr_len);
        if (client_socket_fd == -1)
        {
            close(server_socket_fd);
            return -1;
        }

        struct in_addr ip_address = client_address.sin_addr;
        char *input_adress;
        input_adress = inet_ntoa(ip_address);

        syslog(LOG_USER, "Accepted connection from %s", input_adress);

        char rcv_buffer[512];

        size_t current_data_size = 0;
        char *data_to_write = NULL;

        ssize_t num_bytes;

        while (1 && !signal_received) // Receive loop
        {
            // Receive data from the client
            memset(rcv_buffer, 0, sizeof(rcv_buffer) - 1);
            rcv_buffer[sizeof(rcv_buffer) - 1] = '\0';
            syslog(LOG_USER, "Receive data.");
            num_bytes = recv(client_socket_fd, rcv_buffer, sizeof(rcv_buffer) - 1, 0);
            if (num_bytes == -1)
            {
                syslog(LOG_ERR, "recv error.");
                close(client_socket_fd);
                close(server_socket_fd);
                return -1;
            }
            else if (num_bytes == 0)
            {
                syslog(LOG_USER, "Closed connection from %s", input_adress);
                close(client_socket_fd);
                break; // Stop receiving data and wait for new client
            }
            else
            {
                syslog(LOG_USER, "Received %ld bytes from client.", num_bytes);

                char *pointer_newline = strchr(rcv_buffer, '\n');
                if (pointer_newline == NULL)
                {
                    syslog(LOG_USER, "Did not find a new line character. Append to write buffer.");

                    size_t new_arraysize = current_data_size + num_bytes;
                    char *extended_data_to_write = (char *)realloc(data_to_write, new_arraysize);
                    if (extended_data_to_write == NULL)
                    {
                        syslog(LOG_ERR, "Memory reallocation failed.");
                        free(data_to_write);
                        return -1;
                    }

                    data_to_write = extended_data_to_write;
                    strncpy(data_to_write + current_data_size, (const char *)&rcv_buffer, num_bytes);
                    current_data_size = new_arraysize;
                }
                else
                {
                    size_t length_to_write = pointer_newline - rcv_buffer;
                    syslog(LOG_USER, "Found a new line character at position: %ld -> Write and send back data.", length_to_write);
                    size_t new_arraysize = current_data_size + length_to_write;
                    char *extended_data_to_write = (char *)realloc(data_to_write, new_arraysize);
                    if (extended_data_to_write == NULL)
                    {
                        syslog(LOG_ERR, "Memory reallocation failed.");
                        free(data_to_write);
                        return -1;
                    }

                    data_to_write = extended_data_to_write;
                    strncpy(data_to_write + current_data_size, (const char *)&rcv_buffer, length_to_write);
                    current_data_size = new_arraysize;
                    if (-1 == append_data_to_file(data_to_write, current_data_size))
                    {
                        syslog(LOG_ERR, "Writing to file failed.");
                        return -1;
                    }
                    free(data_to_write);
                    current_data_size = 0;
                    if (-1 == read_file_and_send_back(client_socket_fd))
                    {
                        syslog(LOG_ERR, "Sending back file content failed.");
                        return -1;
                    }
                }
            }
        }

        close(client_socket_fd);
    }

    close(server_socket_fd);
    remove_file();

    return 0;
}