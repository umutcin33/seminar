#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void process_data(const char *user_input) {

    char buffer[50];
    strcpy(buffer, user_input);

    printf("Verarbeitete Daten: %s\n", buffer);

    char *log_entry = (char *)malloc(100 * sizeof(char));
    if (log_entry != NULL) {
        snprintf(log_entry, 100, "Log: %s", buffer);

    }
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        process_data(argv[1]);
    }
    return 0;
}
