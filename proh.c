#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_FILES 100
int i,j;
typedef struct File {
    char filename[100];
} File;
typedef struct Commit {
    int id;
    char message[256];
    time_t timestamp;
    File files[MAX_FILES];
    int file_count;
    struct Commit* next;
    struct Commit* prev; 
} Commit;
typedef struct Repository {
    Commit* head;       
    Commit* tail;         
    File staging_area[MAX_FILES]; 
    int staged_count; 
} Repository;


typedef struct UndoStack {
    Commit* commits[MAX_FILES];
    int top;
} UndoStack;

void init_repository(Repository* repo) {
    repo->head = NULL;
    repo->tail = NULL;
    repo->staged_count = 0;
    printf("Initialized an empty repository.\n");
}

void init_undo_stack(UndoStack* stack) {
    stack->top = -1;
}

void push_undo_stack(UndoStack* stack, Commit* commit) {
    if (stack->top < MAX_FILES - 1) {
        stack->commits[++(stack->top)] = commit;
    } else {
        printf("Undo stack overflow!\n");
    }
}

Commit* pop_undo_stack(UndoStack* stack) {
    if (stack->top >= 0) {
        return stack->commits[(stack->top)--];
    } else {
        printf("Undo stack is empty!\n");
        return NULL;
    }
}

void create_new_file(const char* filename) {
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        printf("Error: Could not create file '%s'.\n", filename);
        return;
    }

    printf("Enter content for the file (Press Enter when done):\n");
    char content[1024];
    fgets(content, sizeof(content), stdin);  
    fprintf(file, "%s", content); 

    fclose(file);
    printf("File '%s' created successfully.\n", filename);
}

void add_file(Repository* repo, const char* filename) {
    if (repo->staged_count < MAX_FILES) {
        strcpy(repo->staging_area[repo->staged_count].filename, filename);
        repo->staged_count++;
        printf("File '%s' added to staging area.\n", filename);
    } else {
        printf("Staging area is full. Cannot add more files.\n");
    }
}

void commit(Repository* repo, const char* message, UndoStack* undo_stack) {
    if (repo->staged_count == 0) {
        printf("No files to commit.\n");
        return;
    }

    Commit* new_commit = (Commit*)malloc(sizeof(Commit));
    new_commit->id = (repo->head == NULL) ? 1 : repo->tail->id + 1;
    strcpy(new_commit->message, message);
    new_commit->timestamp = time(NULL);
    new_commit->file_count = repo->staged_count;

    // Copy files from the staging area to the commit
    for (i = 0; i < repo->staged_count; i++) {
        new_commit->files[i] = repo->staging_area[i];

        // Create a copy of the file for the commit
        char commit_filename[200];
        sprintf(commit_filename, "commit_%d_%s", new_commit->id, new_commit->files[i].filename);
        FILE* src_file = fopen(new_commit->files[i].filename, "r");
        FILE* dest_file = fopen(commit_filename, "w");

        if (src_file == NULL || dest_file == NULL) {
            printf("Error: Cannot copy file '%s'.\n", new_commit->files[i].filename);
            if (src_file) fclose(src_file);
            if (dest_file) fclose(dest_file);
            return;
        }

        char ch;
        while ((ch = fgetc(src_file)) != EOF) {
            fputc(ch, dest_file);
        }

        fclose(src_file);
        fclose(dest_file);

        printf("File '%s' committed as '%s'.\n", new_commit->files[i].filename, commit_filename);
    }

    // Reset staging area
    repo->staged_count = 0;

    // Add the new commit to the linked list
    new_commit->next = NULL;
    new_commit->prev = repo->tail;
    if (repo->tail) {
        repo->tail->next = new_commit;
    }
    repo->tail = new_commit;

    if (repo->head == NULL) {
        repo->head = new_commit;
    }

    // Push the commit to the undo stack
    push_undo_stack(undo_stack, new_commit);

    printf("Committed with message: '%s'\n", message);
}

// Function to undo the last commit
void undo_commit(Repository* repo, UndoStack* undo_stack) {
    if (repo->tail == NULL) {
        printf("No commits to undo.\n");
        return;
    }

    Commit* last_commit = pop_undo_stack(undo_stack);
    if (last_commit == NULL) {
        return;
    }

    printf("Undoing commit %d: %s\n", last_commit->id, last_commit->message);

    // Restore the previous version of each file in the commit
    for (i = 0; i < last_commit->file_count; i++) {
        char commit_filename[200];
        sprintf(commit_filename, "commit_%d_%s", last_commit->id, last_commit->files[i].filename);

        FILE* src_file = fopen(commit_filename, "r");
        FILE* dest_file = fopen(last_commit->files[i].filename, "w");

        if (src_file == NULL || dest_file == NULL) {
            printf("Error: Cannot restore file '%s'.\n", last_commit->files[i].filename);
            if (src_file) fclose(src_file);
            if (dest_file) fclose(dest_file);
            return;
        }

        char ch;
        while ((ch = fgetc(src_file)) != EOF) {
            fputc(ch, dest_file);
        }

        fclose(src_file);
        fclose(dest_file);

        printf("Restored file '%s' from commit.\n", last_commit->files[i].filename);
    }

    // Remove the last commit from the linked list
    if (last_commit->prev) {
        last_commit->prev->next = NULL;
        repo->tail = last_commit->prev;
    } else {
        repo->head = NULL;
        repo->tail = NULL;
    }

    free(last_commit);
}

// Function to edit an existing file
void edit_file(const char* filename) {
    FILE* file = fopen(filename, "a");
    if (file == NULL) {
        printf("Error: Could not open file '%s'.\n", filename);
        return;
    }

    printf("Enter additional content for the file (Press Enter when done):\n");
    char content[1024];
    fgets(content, sizeof(content), stdin);  // Get user input for the additional content
    fprintf(file, "%s", content);  // Append the content to the file

    fclose(file);
    printf("File '%s' edited successfully.\n", filename);
}

// Function to view the commit log
void log_commits(Repository* repo) {
    if (repo->head == NULL) {
        printf("No commits yet.\n");
        return;
    }

    Commit* current = repo->head;
    while (current) {
        printf("Commit %d: %s\n", current->id, current->message);
        printf("Date: %s", ctime(&current->timestamp));
        printf("Files:\n");
        for (j = 0; j < current->file_count; j++) {
            printf("  - %s\n", current->files[j].filename);
        }
        printf("\n");
        current = current->next;
    }
}

int main() {
    Repository repo;
    UndoStack undo_stack;
    init_repository(&repo);
    init_undo_stack(&undo_stack);

    int choice;
    char filename[100];
    char message[256];

    while (1) {
        printf("\n1. Create a new file\n");
        printf("2. Add file to staging\n");
        printf("3. Commit files\n");
        printf("4. Edit existing file\n");
        printf("5. View commit log\n");
        printf("6. Undo last commit\n");
        printf("7. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);
        getchar();  // Consume newline character

        switch (choice) {
            case 1:
                printf("Enter file name: ");
                scanf("%s", filename);
                getchar();  // Consume newline character
                create_new_file(filename);
                break;
            case 2:
                printf("Enter file name to add to staging: ");
                scanf("%s", filename);
                getchar();  // Consume newline character
                add_file(&repo, filename);
                break;
            case 3:
                printf("Enter commit message: ");
                fgets(message, sizeof(message), stdin);
                message[strcspn(message, "\n")] = '\0';  // Remove newline
                commit(&repo, message, &undo_stack);
                break;
            case 4:
                printf("Enter file name to edit: ");
                scanf("%s", filename);
                getchar();  // Consume newline character
                edit_file(filename);
                printf("Enter commit message for edited file: ");
                fgets(message, sizeof(message), stdin);
                message[strcspn(message, "\n")] = '\0';  // Remove newline
                add_file(&repo, filename);  // Add edited file to staging
                commit(&repo, message, &undo_stack);
                break;
            case 5:
                log_commits(&repo);
                break;
            case 6:
                undo_commit(&repo, &undo_stack);
                break;
            case 7:
                printf("Exiting...\n");
                exit(0);
            default:
                printf("Invalid choice. Please try again.\n");
        }
    }

    return 0;
}

