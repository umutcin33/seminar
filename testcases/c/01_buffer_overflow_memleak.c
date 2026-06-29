#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// SCHWACHSTELLE 1: Buffer Overflow (Pufferueberlauf)
// SCHWACHSTELLE 2: Memory Leak (Speicherleck)

void process_data(const char *user_input) {
    // Erwarteter Fund 1:
    // buffer ist 50 Byte gross, aber die Laenge von user_input wird nicht geprueft.
    char buffer[50];
    strcpy(buffer, user_input);

    printf("Verarbeitete Daten: %s\n", buffer);

    // Erwarteter Fund 2:
    // Fuer log_entry wird Speicher reserviert, am Ende aber nicht mit free() freigegeben.
    char *log_entry = (char *)malloc(100 * sizeof(char));
    if (log_entry != NULL) {
        snprintf(log_entry, 100, "Log: %s", buffer);
        // FEHLER HIER: free(log_entry); fehlt.
    }
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        process_data(argv[1]);
    }
    return 0;
}
