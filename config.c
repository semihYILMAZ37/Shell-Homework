#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

char* get_config_value(KeyValuePair* head, const char* key) {
    KeyValuePair* current = head;
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            return strdup(current->value);
        }
        current = current->next;
    }
    return NULL;
}

KeyValuePair* set_config_value(KeyValuePair* head, const char* key, const char* value) {
    KeyValuePair* current = head;
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            strcpy(current->value, value);
            sync_config_to_file(CONFIG_FILE, head);
            return head;
        }
        current = current->next;
    }

    KeyValuePair* new_pair = (KeyValuePair*)malloc(sizeof(KeyValuePair));
    if (new_pair == NULL) {
        perror("Memory allocation failed");
        return head;
    }
    strcpy(new_pair->key, key);
    strcpy(new_pair->value, value);
    new_pair->next = head;
    head = new_pair;
    append_config_to_file(CONFIG_FILE, new_pair);
    return head;
}

KeyValuePair* load_config_from_file(const char* filename) {
    FILE* file_pointer;
    file_pointer = fopen(filename, "r");
    if (file_pointer == NULL) {
        //perror("Error opening file");
        return NULL;
    }

    KeyValuePair* head = NULL;
    char line[256];
    while (fgets(line, sizeof(line), file_pointer) != NULL) {
        char* key = strtok(line, "=");
        char* value = strtok(NULL, "\n");
        KeyValuePair* new_pair = (KeyValuePair*)malloc(sizeof(KeyValuePair));
        if (new_pair == NULL) {
            perror("Memory allocation failed");
            return head;
        }
        strcpy(new_pair->key, key);
        strcpy(new_pair->value, value);
        new_pair->next = head;
        head = new_pair;
    }
    
    fclose(file_pointer);
    return head;
}

void sync_config_to_file(const char* filename, KeyValuePair* head) {
    FILE* file_pointer;
    file_pointer = fopen(filename, "w");
    if (file_pointer == NULL) {
        perror("Error opening file");
        return;
    }

    KeyValuePair* current = head;
    while (current != NULL) {
        fprintf(file_pointer, "%s=%s\n", current->key, current->value);
        current = current->next;
    }

    fclose(file_pointer);
}

void append_config_to_file(const char* filename, KeyValuePair* new) {
    FILE* file_pointer;
    file_pointer = fopen(filename, "a");
    if (file_pointer == NULL) {
        perror("Error opening file");
        return;
    }

    fprintf(file_pointer, "%s=%s\n", new->key, new->value);
    fclose(file_pointer);
}
