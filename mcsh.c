#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* TODO:
 * Quote and backslash escaping
 * Piping and redirection
 * More standard builtins
 * Globbing
 * Add config file support (WIP) - Rebase and add aliases
 * Improve history support
 * Rebase source so it is more readable and not obvious it was written in ~1h during the night
 * */

#define CONFIG_FILE "mcsh.conf" // Change to desired config file

char MCSH_PROMPT[256] = "mcsh > ";

// Function to trim leading and trailing whitespace and quotes
char* trim_garbage(char* str) {
    int inside_quotes = 0; // Flag to keep track of quoted sections
    char* start = str;
    char* end = str + strlen(str) - 1;

    // Trim leading whitespace
    while (isspace((unsigned char)*start)) {
        start++;
    }

    // Trim trailing whitespace
    while (end > start && isspace((unsigned char)*end)) {
        end--;
    }

    // Trim leading and trailing quotes
    if (*start == '"' && *end == '"') {
        inside_quotes = 1; // Inside a quoted section
        start++;
        end--;
    } else if (*start == '\'' && *end == '\'') {
        inside_quotes = 1; // Inside a quoted section
        start++;
        end--;
    }

    // Preserve spaces within quotes
    char* current = start;
    while (current <= end) {
        if (*current == '"' || *current == '\'') {
            if (inside_quotes) {
                inside_quotes = 0; // Exiting a quoted section
            } else {
                inside_quotes = 1; // Entering a quoted section
            }
        } else if (inside_quotes && *current == ' ') {
            *current = '\a'; // Replace spaces within quotes with a non-printable character
        }
        current++;
    }

    // Shift non-printable characters back to spaces
    current = start;
    while (current <= end) {
        if (*current == '\a') {
            *current = ' ';
        }
        current++;
    }

    end[1] = '\0';
    return start;
}

// Load and parse the config file
void load_config() {
	FILE* file = fopen(CONFIG_FILE, "r");
	if(file == NULL) {
		perror("Error opening config file");
		return;
	}

	// Read the file line by line
	char line[256];
	while(fgets(line, sizeof(line), file)) {
		// Remove leading and trailing whitespaces
		char* trimmed_line = trim_garbage(line);
		

		// Check if the line is a valid config option
		if (trimmed_line[0] != '#' && trimmed_line[0] != '\0') {
			// Split the line into key and value
			char* key = strtok(trimmed_line, "=");
			char* value = strtok(NULL, "=");
			
			//Process the config option based on the key
			if(key && value) {

				char* trimmed_key = trim_garbage(key);

				// Handle shell prompt setting
				if(strcmp(trimmed_key, "prompt") == 0) {
					char* trimmed_value = trim_garbage(value);
					strncpy(MCSH_PROMPT, trimmed_value, sizeof(MCSH_PROMPT));
				}

			}
		}
	}
	fclose(file);
}

/* Forward Function Declarations for builtin shell commands:*/
int mcsh_cd(char** args);
int mcsh_help(char** args);
int mcsh_exit(char** args);

int mcsh_history(char** args);
#define MAX_HISTORY_SIZE 100

/* List of builtin commands, followed by their corresponding functions */

char* builtin_str[] = {
	"cd",
	"help",
	"exit",
	"history"
};

// Array of function pointers that take an array of strings and returns an int
// Maybe use an array of structs instead?
int (*builtin_func[]) (char**) = {
	&mcsh_cd,
	&mcsh_help,
	&mcsh_exit,
	&mcsh_history
};

int mcsh_num_builtins() {
	return sizeof(builtin_str) / sizeof(char*);
}

/* Builtin function implementations */

int mcsh_cd(char** args) {
	if(args[1] == NULL) {
		fprintf(stderr, "mcsh: expected argument to \"cd\"\n");
	} else {
		if(chdir(args[1]) != 0) {
			perror("mcsh");
		}
	}
	return 1;
}

int mcsh_help(char** args) {
	int i;
	printf("Michael Radu's MCSH\n");
	printf("Inspired by lsh\n");
	printf("Type program names and arguments, and hit enter.\n");
	printf("The following are built in:\n");

	for(i = 0; i < mcsh_num_builtins(); i++) {
		printf("  %s\n", builtin_str[i]);
	}

	printf("Use the man command for information on other programs.\n");
	return 1;
}

int mcsh_exit(char** args) {
	return 0;
}

char* history[MAX_HISTORY_SIZE];
int history_count = 0;

int mcsh_history(char** args) {
	for(int i = 0; i < history_count; i++) {
		printf("%d %s\n", i+1, history[i]);
	}
	return 1;
}

//#define LEGACY
#ifdef LEGACY

#define MCSH_RL_BUFSIZE 1024
char* mcsh_read_line(void) {
	int bufsize = MCSH_RL_BUFSIZE;
	int position = 0;
	char* buffer = malloc(sizeof(char) * bufsize);
	int c;

	if(!buffer) {
		fprintf(stderr, "mcsh: (-) allocation error\n");
		exit(EXIT_FAILURE);
	}

	while(1) {
		// Read a character
		c = getchar();

		// If we hit EOF, replace it with a null character and return
		if(c == EOF || c == '\n') {
			buffer[position] = '\0';
			return buffer;
		} else {
			buffer[position] = c;
		}
		position++;

		// If we have exceeded the buffer, reallocate
		if(position >= bufsize) {
			bufsize += MCSH_RL_BUFSIZE;
			buffer = realloc(buffer, bufsize);
			if(!buffer) {
				fprintf(stderr, "mcsh: (-) allocation error\n");
				exit(EXIT_FAILURE);
			}
		}
	}
}

#endif

char* mcsh_read_line(void) {
	char* line = NULL;
	ssize_t bufsize = 0; // getline will allocate a buffer for us
	
	if(getline(&line, &bufsize, stdin) == -1) {
		if(feof(stdin)) {
			exit(EXIT_SUCCESS);
		} else {
			perror("readline");
			exit(EXIT_FAILURE);
		}
	}

	return line;
}

#define MCSH_TOK_BUFSIZE 64
#define MCSH_TOK_DELIM " \t\r\n\a"

// TO DO
//// Allow quoting and backslash escaping in command line arguments
char** mcsh_split_line(char* line) {
	int bufsize = MCSH_TOK_BUFSIZE, position = 0;
	char** tokens = malloc(bufsize * sizeof(char*));
	char* token;

	if(!tokens) {
		fprintf(stderr, "mcsh: (-) allocation error\n");
		exit(EXIT_FAILURE);
	}

	token = strtok(line, MCSH_TOK_DELIM);
	while(token != NULL) {
		tokens[position] = token;
		position++;

		if (position >= bufsize) {
			bufsize += MCSH_TOK_BUFSIZE;
			tokens = realloc(tokens, bufsize * sizeof(char*));
			if(!tokens) {
				fprintf(stderr, "mcsh: (-) allocation error\n");
				exit(EXIT_FAILURE);
			}
		}

		token = strtok(NULL, MCSH_TOK_DELIM);
	}

	tokens[position] = NULL;
	return tokens;
}

int mcsh_launch(char** args) {
	pid_t pid, wpid;
	int status;


	// TLDR for non-init Unix processes:
	/// Parent calls fork() to duplicate itself, it gets the PID (process id)
	/// of the child, the child calls exec() to replace itself with the process you
	/// wanted to execute. The parent can call wait() to keep tabs on its children.
	
	pid = fork();
	if(pid == 0) {
		// Child process
		if(execvp(args[0], args) == -1) {
			perror("mcsh");
		}
		exit(EXIT_FAILURE);
	} else if(pid < 0) {
		// Error forking
		perror("mcsh");
	} else {
		// Parent process
		do {
			wpid = waitpid(pid, &status, WUNTRACED);
		} while(!WIFEXITED(status) && !WIFSIGNALED(status));
	}
	return 1;
}

int mcsh_execute(char** args) {
	int i;

	if(args[0] == NULL) {
		// An empty command was entered.
		return 1;
	}

	for(i = 0; i < mcsh_num_builtins(); i++) {
		if(strcmp(args[0], builtin_str[i]) == 0) {
			return (*builtin_func[i])(args);
		}
	}
	
	return mcsh_launch(args);
}

void mcsh_loop() {
	char* line;
	char** args;
	int status;

	do {
		printf("%s", MCSH_PROMPT);
		line = mcsh_read_line();
		args = mcsh_split_line(line);

		// Save command to history
		if (args[0] != NULL && strcmp(args[0], "history") != 0) {
			if(history_count < MAX_HISTORY_SIZE) {
				history[history_count] = strdup(line);
				history_count++;
			} else {
				free(history[0]);
				for(int i=0; i < MAX_HISTORY_SIZE - 1; i++) {
					history[i] = history[i+1];
				}
				history[MAX_HISTORY_SIZE - 1] = strdup(line);
			}
		}

		status = mcsh_execute(args);

		free(line);
		free(args);
	} while(status);
}



int main(int argc, char** argv) {
	// WIP
	// Load config files, if any
	load_config();

	// Run command loop.
	mcsh_loop();

	// TO DO
	// Perform any shutdown/cleanup.
	
	return EXIT_SUCCESS;
}
