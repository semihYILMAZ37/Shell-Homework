#ifndef CONFIG_H
#define CONFIG_H


#define CONFIG_FILE ".alias.conf"
#define MAX_LINE_LENGTH 1024

typedef struct KeyValuePair {
    char key[MAX_LINE_LENGTH];
    char value[MAX_LINE_LENGTH];
    struct KeyValuePair* next;
} KeyValuePair;

char* get_config_value(KeyValuePair* head, const char* key);
KeyValuePair* set_config_value(KeyValuePair* head, const char* key, const char* value);
KeyValuePair* load_config_from_file(const char* filename);
void sync_config_to_file(const char* filename, KeyValuePair* head);
void append_config_to_file(const char* filename, KeyValuePair* new);

#endif /* CONFIG_H */
