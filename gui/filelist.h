typedef struct {
    char *name;
    int isdir;
} file_ent;

file_ent * file_list_create(const char *dir, int *entries, int (*filter)(file_ent *entry));
void file_list_sort(file_ent *list, int n_entries);
void file_list_free(file_ent *list, int n_entries);

