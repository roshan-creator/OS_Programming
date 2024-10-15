#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <ncurses.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <pthread.h>
#include <math.h>

char input[100];

// Vector operations functions
struct ThreadData {
    int* vec1;
    int* vec2;
    int* result;
    int size;
};

// Function to edit a file using the vi editor
int is_word_char(int c) {
    // Define what characters are considered word characters.
    // You can extend this based on your requirements.
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

void vi_edit(char *filename, int *lines, int *words, int *chars) {
    initscr();
    keypad(stdscr, TRUE);
    noecho();

    FILE *file = fopen(filename, "a+");

    if (file == NULL) {
        printw("Failed to open file for editing.\n");
        refresh();
        getch();
        endwin();
        return;
    }

    int ch;
    int cursor_x = 0;
    int cursor_y = 0;
    int prev_char = 0; // To keep track of the previous character for word counting

    while (1) {
        clear();
        mvprintw(0, 0, "Vi Editor - %s", filename);

        move(cursor_y + 1, cursor_x);

        ch = getch();

        if (ch == 27) {
            break;
        } else if (ch == KEY_LEFT && cursor_x > 0) {
            cursor_x--;
        } else if (ch == KEY_RIGHT && cursor_x < COLS - 1) {
            cursor_x++;
        } else if (ch == KEY_UP && cursor_y > 0) {
            cursor_y--;
        } else if (ch == KEY_DOWN && cursor_y < LINES - 2) {
            cursor_y++;
        } else if (ch == 127 && cursor_x > 0) {
            mvwaddch(stdscr, cursor_y + 1, cursor_x, ' ');
            cursor_x--;
            fseek(file, cursor_y * COLS + cursor_x, SEEK_SET);
            fputc(' ', file);
        } else if ((ch >= 32 && ch <= 126) || ch == 10) {
            mvwaddch(stdscr, cursor_y + 1, cursor_x, ch);
            fseek(file, cursor_y * COLS + cursor_x, SEEK_SET);
            fputc(ch, file);
            cursor_x++;
            (*chars)++;

            if (ch == 10) {
                (*lines)++;
            }

            if (is_word_char(ch) && !is_word_char(prev_char)) {
                (*words)++;
            }

            prev_char = ch;
        } else if (ch == 19) {
            if (filename[0] != '\0') {
                fclose(file);
                file = fopen(filename, "w");
                if (file == NULL) {
                    printw("Failed to save the file.\n");
                } else {
                    for (int i = 1; i <= *lines; i++) {
                        fseek(file, (i - 1) * COLS, SEEK_SET);
                        for (int j = 0; j < COLS; j++) {
                            int c = mvwinch(stdscr, i, j) & A_CHARTEXT;
                            if (c != ' ' || (i == *lines && j == (*chars) % COLS)) {
                                fputc(c, file);
                            } else {
                                break;
                            }
                        }
                    }
                    printw("File saved successfully.\n");
                }
            }
        } else if (ch == 24) {
            break;
        }
    }

    fclose(file);
    endwin();
}

// Function to execute a vi command
void execute_vi_command(char *command, int *lines, int *words, int *chars) {
    char *token = strtok(command, " \t\n");

    if (token != NULL) {
        char *filename = token;
        vi_edit(filename, lines, words, chars);
    }
}

void execute_command(char *command) {
    char *args[100];
    int arg_count = 0;

    char *token = strtok(command, " \t\n");
    while (token != NULL) {
        args[arg_count] = token;
        arg_count++;
        token = strtok(NULL, " \t\n");
    }
    args[arg_count] = NULL;

    if (arg_count == 0) {
        return;
    }

    if (strcmp(args[0], "pwd") == 0) {
        execlp("pwd", "pwd", NULL);
        perror("execlp");
        exit(1);
    } else if (strcmp(args[0], "cd") == 0) {
        if (arg_count == 2) {
            if (chdir(args[1]) != 0) {
                perror("chdir");
            } else {
                if (getcwd(input, sizeof(input)) != NULL) {
                    printf("%s\n", input);
                } else {
                    perror("getcwd");
                }
            }
        } else {
            printf("Usage: cd <directory_name>\n");
        }
    } else if (strcmp(args[0], "mkdir") == 0) {
        if (arg_count == 2) {
            if (mkdir(args[1], 0777) != 0) {
                perror("mkdir");
            }
        } else {
            printf("Usage: mkdir <directory_name>\n");
        }
    } else if (strcmp(args[0], "ls") == 0) {
        execvp("ls", args);
        perror("ls");
    } else if (strcmp(args[0], "exit") == 0) {
        exit(1);
    } else if (strcmp(args[0], "help") == 0) {
        printf("Available commands:\n");
        printf("1. pwd - shows the present working directory\n");
        printf("2. cd <directory_name> - changes the present working directory\n");
        printf("3. mkdir <directory_name> - creates a new directory\n");
        printf("4. ls <flag> - shows the contents of a directory based on optional flags\n");
        printf("5. exit - exits the shell\n");
        printf("6. help - prints this list of commands\n");
    } else {
        execvp(args[0], args);
        perror("execvp");
        exit(1);
    }
    
}

void* add_vectors(void* data) {
    struct ThreadData* thread_data = (struct ThreadData*)data;
    int* vec1 = thread_data->vec1;
    int* vec2 = thread_data->vec2;
    int* result = thread_data->result;
    int size = thread_data->size;

    for (int i = 0; i < size; i++) {
        result[i] = vec1[i] + vec2[i];
        //printf("%d",result[i]);
    }

    pthread_exit(NULL);
}

void* sub_vectors(void* data) {
    struct ThreadData* thread_data = (struct ThreadData*)data;
    int* vec1 = thread_data->vec1;
    int* vec2 = thread_data->vec2;
    int* result = thread_data->result;
    int size = thread_data->size;

    for (int i = 0; i < size; i++) {
        result[i] = vec1[i] - vec2[i];
    }

    pthread_exit(NULL);
}

void* dot_product(void* data) {
    struct ThreadData* thread_data = (struct ThreadData*)data;
    int* vec1 = thread_data->vec1;
    int* vec2 = thread_data->vec2;
    int* result = thread_data->result;
    int size = thread_data->size;

    int dot = 0;
    for (int i = 0; i < size; i++) {
        dot += vec1[i] * vec2[i];
    }

    *result = dot;
    pthread_exit(NULL);
}

// Function to execute vector operation command
void execute_vector_operation(char* op, char* filename1, char* filename2, int no_threads) {
    FILE* file1 = fopen(filename1, "r");
    FILE* file2 = fopen(filename2, "r");

    if (file1 == NULL || file2 == NULL) {
        printf("Failed to open input files.\n");
        return;
    }

    int size1 = 0, size2 = 0;

    // Count the number of elements in each file
    int temp;
    while (fscanf(file1, "%d", &temp) != EOF) {
        size1++;
    }
    rewind(file1);

    while (fscanf(file2, "%d", &temp) != EOF) {
        size2++;
    }
    rewind(file2);

    if (size1 != size2) {
        printf("Error: Vectors must be of the same size for %s operation.\n", op);
        fclose(file1);
        fclose(file2);
        return;
    }

    int* vec1 = (int*)malloc(size1 * sizeof(int));
    int* vec2 = (int*)malloc(size1 * sizeof(int));
    int* result = (int*)malloc(size1 * sizeof(int));

    if (vec1 == NULL || vec2 == NULL || result == NULL) {
        printf("Memory allocation failed.\n");
        fclose(file1);
        fclose(file2);
        return;
    }

    for (int i = 0; i < size1; i++) {
        fscanf(file1, "%d", &vec1[i]);
        fscanf(file2, "%d", &vec2[i]);
    }

    fclose(file1);
    fclose(file2);

    struct ThreadData thread_data[no_threads];
    thread_data.vec1 = vec1;
    thread_data.vec2 = vec2;
    thread_data.result = result;
    thread_data.size = size1;

    if (strcmp(op, "addvec") == 0) {
        for (int i = 0; i < no_threads; i++) {
            pthread_create(&threads[i], NULL, add_vectors, &thread_data);
        }
    } else if (strcmp(op, "subvec") == 0) {
        for (int i = 0; i < no_threads; i++) {
            pthread_create(&threads[i], NULL, sub_vectors, &thread_data);
        }
    } else if (strcmp(op, "dotprod") == 0) {
        int result = 0;
        thread_data.result = &result;
        for (int i = 0; i < no_threads; i++) {
            pthread_create(&threads[i], NULL, dot_product, &thread_data);
        }
    }

    for (int i = 0; i < no_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Result: ");
    for (int i = 0; i < size1; i++) {
        printf("%d ", result[i]);
    }
    printf("\n");

    free(vec1);
    free(vec2);
    free(result);
}

int main() {
    while (1) {
        char *input = readline("shell> ");
        if (input == NULL) {
            printf("Ctrl+D was pressed, exit the shell");
            break;
        }
        

        if (strlen(input) > 0) {
            add_history(input);

            int background = 0;
            int input_length = strlen(input);

            if (input_length > 1 && input[input_length - 2] == '&') {
                background = 1;
                input[input_length - 2] = '\0';
            }
            int num_lines = 0;
            int num_words = 0;
            int num_chars = 0;

            if (strcmp(input, "exit") == 0) {
                // Exit the shell
                break;
            }
            else if (strncmp(input, "vi", 2) == 0) {
                // Execute vi command
                execute_vi_command(input + 3, &num_lines, &num_words, &num_chars); // Pass counters as arguments
                printf("Number of lines: %d\nNumber of words: %d\nNumber of characters: %d\n", num_lines+1, num_words, num_chars);
            }

            else if (strncmp(input, "addvec", 6) == 0 || strncmp(input, "subvec", 6) == 0 || strncmp(input, "dotprod", 7) == 0) {
                // Execute vector operation
                char* op = strtok(input, " ");
                char* filename1 = strtok(NULL, " ");
                char* filename2 = strtok(NULL, " ");
                char* threads_option = strtok(NULL, " ");
                int no_threads = (threads_option != NULL && threads_option[0] == '-' && threads_option[1] >= '0' && threads_option[1] <= '9')
                    ? atoi(threads_option + 1)
                    : 3;

                execute_vector_operation(op, filename1, filename2, no_threads);
            }
            
            else if (strchr(input, '|') != NULL) {
                // Handle piping
                int pipe_fd[2];
                if (pipe(pipe_fd) == -1) {
                    perror("pipe");
                    exit(1);
                }

                char *token;
                char *command = strtok(input, "|");
                int count = 0;
                //printf("Command is being executed %s",command);
                while (command != NULL) {
                    count++;
                    int child_in = STDIN_FILENO;
                    int child_out = STDOUT_FILENO;

                    if (count == 1) {
                        // First command, no need to read from the pipe
                        child_out = pipe_fd[1];
                    } else if (count == 2) {
                        // Middle command, read from the pipe
                        child_in = pipe_fd[0];
                        child_out = pipe_fd[1];
                    } else {
                        // Last command, no need to write to the pipe
                        child_in = pipe_fd[0];
                    }

                    pid_t pid = fork();

                    if (pid == 0) {
                        // Child process
                        if (count != 1) {
                            // Redirect input
                            dup2(child_in, STDIN_FILENO);
                        }
                        if (count != 2) {
                            // Redirect output
                            dup2(child_out, STDOUT_FILENO);
                        }
                        //printf("Command Is Being executed %s",command);
                        execute_command(command);
                        exit(0);
                    } else if (pid > 0) {
                        // Parent process
                        if (count != 1) {
                            close(pipe_fd[0]);
                        }
                        if (count != 2) {
                            close(pipe_fd[1]);
                        }

                        if (!background) {
                            wait(NULL);
                        }
                    } else {
                        perror("Fork failed");
                    }

                    command = strtok(NULL, "|");
                    //printf("Command 2 is being executed %s",command);
                }
            }
            
            else {
                if (input[input_length - 1] == '\\') {
                    input[input_length - 1] = '\0'; 

                    while (1) {
                        char *next_line = readline("> ");
                        if (next_line == NULL) {
                            break; // Ctrl+D for exit multiline input
                        }
                        if (strlen(next_line) == 0) {
                            break; 
                        }
                        
                        char *temp = (char *)realloc(input, strlen(input) + strlen(next_line) + 2);
                        if (temp == NULL) {
                            perror("realloc");
                            exit(1);
                        }
                        input = temp;
                        strcat(input, " ");
                        int x= strlen(next_line);
                        char z= next_line[x-1];
                        char *t;
                        char *ptr = next_line;
                        while (*ptr != '\0') {
                            if (*ptr == '\\' && *(ptr + 1) == '\0') {
                                *ptr = '\0'; 
                                break; 
                            }
                            ptr++;
                        }
                        strcat(input, next_line);
                        //printf("%s",next_line);
                        
                        if (z == '\\'){
                            
                        }
                        else break;
                    }
                }
                
                else if (strchr(input, '|') != NULL) {
                    // Handle piping
                    int pipe_fd[2];
                    if (pipe(pipe_fd) == -1) {
                        perror("pipe");
                        exit(1);
                    }

                    char *token;
                    char *command = strtok(input, "|");
                    int count = 0;
                    //printf("Command is being executed %s",command);
                    while (command != NULL) {
                        count++;
                        int child_in = STDIN_FILENO;
                        int child_out = STDOUT_FILENO;

                        if (count == 1) {
                            // First command, no need to read from the pipe
                            child_out = pipe_fd[1];
                        } else if (count == 2) {
                            // Middle command, read from the pipe
                            child_in = pipe_fd[0];
                            child_out = pipe_fd[1];
                        } else {
                            // Last command, no need to write to the pipe
                            child_in = pipe_fd[0];
                        }

                        pid_t pid = fork();

                        if (pid == 0) {
                            // Child process
                            if (count != 1) {
                                // Redirect input
                                dup2(child_in, STDIN_FILENO);
                            }
                            if (count != 2) {
                                // Redirect output
                                dup2(child_out, STDOUT_FILENO);
                            }
                            //printf("Command Is Being executed %s",command);
                            execute_command(command);
                            exit(0);
                        } else if (pid > 0) {
                            // Parent process
                            if (count != 1) {
                                close(pipe_fd[0]);
                            }
                            if (count != 2) {
                                close(pipe_fd[1]);
                            }

                            if (!background) {
                                wait(NULL);
                            }
                        } else {
                            perror("Fork failed");
                        }

                        command = strtok(NULL, "|");
                        //printf("Command 2 is being executed %s",command);
                    }
                }

                pid_t pid = fork();
                if (pid == 0) {
                    execute_command(input);
                    exit(0);
                } else if (pid > 0) {
                    if (!background) {
                        wait(NULL);
                    }
                } else {
                    perror("Fork failed");
                }
            }
            
            free(input); 
        }
    }

    return 0;
}