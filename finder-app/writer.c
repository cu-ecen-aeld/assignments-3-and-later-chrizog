#include <syslog.h>
#include <stdio.h>

int main(int argc, char **argv) {
    
    openlog("writer", 0, LOG_USER);
    
    if (argc != 3) {
        syslog(LOG_ERR, "The number of arguments has to be 2");
        return 1;
    }

    const char* writefile = argv[1];
    const char* writestr = argv[2];

    char log_str[100];
    snprintf(&log_str, 100, "Writing %s to %s", writestr, writefile);

    syslog(LOG_DEBUG, log_str);

    FILE *f = fopen(writefile, "a");
    if (f == NULL) {
        syslog(LOG_ERR, "Cannot create file");
        return 1;       
    }

    if (!fprintf(f, writestr)) {
        syslog(LOG_ERR, "Writing to file failed");
    }
    if (!fprintf(f, "\n")) {
        syslog(LOG_ERR, "Writing to file failed");
    }
    fclose(f);



    return 0;
}