#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <curl/curl.h>
#include <errno.h>
#include <termios.h>
#include <ctype.h>

#define API_VERSION "v12.0"
#define LOG_FILE "facebook_report.log"
#define MAX_RETRIES 5
#define INITIAL_RETRY_DELAY 2

typedef struct {
    char account_id[256];
    char access_token[256];
    char reason[256];
    char category[256];
    char proxy[256];
    int retries;
    int delay;
} report_params_t;

FILE *log_file = NULL;

// Function to print banner
void print_banner() {
    printf("\033[91m  ██████  ██░ ██  ▄▄▄      ▓█████▄  ▒█████   █     █░     ██████ ▄▄▄█████▓ ██▀███   ██▓ ██ ▄█▀▓█████ \n");
    printf("\033[91m▒██    ▒ ▓██░ ██▒▒████▄    ▒██▀ ██▌▒██▒  ██▒▓█░ █ ░█░   ▒██    ▒ ▓  ██▒ ▓▒▓██ ▒ ██▒▓██▒ ██▄█▒ ▓█   ▀ \n");
    printf("\033[91m░ ▓██▄   ▒██▀▀██░▒██  ▀█▄  ░██   █▌▒██░  ██▒▒█░ █ ░█    ░ ▓██▄   ▒ ▓██░ ▒░▓██ ░▄█ ▒▒██▒▓███▄░ ▒███   \n");
    printf("\033[91m  ▒   ██▒░▓█ ░██ ░██▄▄▄▄██ ░▓█▄   ▌▒██   ██░░█░ █ ░█      ▒   ██▒░ ▓██▓ ░ ▒██▀▀█▄  ░██░▓██ █▄ ▒▓█  ▄ \n");
    printf("\033[91m▒██████▒▒░▓█▒░██▓ ▓█   ▓██▒░▒████▓ ░ ████▓▒░░░██▒██▓    ▒██████▒▒  ▒██▒ ░ ░██▓ ▒██▒░██░▒██▒ █▄░▒████▒\n");
    printf("\033[91m▒ ▒▓▒ ▒ ░ ▒ ░░▒░▒ ▒▒   ▓▒█░ ▒▒▓  ▒ ░ ▒░▒░▒░ ░ ▓░▒ ▒     ▒ ▒▓▒ ▒ ░  ▒ ░░   ░ ▒▓ ░▒▓░░▓  ▒ ▒▒ ▓▒░░ ▒░ ░\n");
    printf("\033[91m░ ░▒  ░ ░ ▒ ░▒░ ░  ▒   ▒▒ ░ ░ ▒  ▒   ░ ▒ ▒░   ▒ ░ ░     ░ ░▒  ░ ░    ░      ░▒ ░ ▒░ ▒ ░░ ░▒ ▒░ ░ ░  ░\n");
    printf("\033[91m░  ░  ░   ░  ░░ ░  ░   ▒    ░ ░  ░ ░ ░ ░ ▒    ░   ░     ░  ░  ░    ░        ░░   ░  ▒ ░░ ░░ ░    ░   \n");
    printf("\033[91m      ░   ░  ░  ░      ░  ░   ░        ░ ░      ░             ░              ░      ░  ░  ░      ░  ░\n");
    printf("                            ░\n");
    printf("\033[0m");
}

void hide_input(char *input, int size) {
    struct termios oldt, newt;
    int i = 0;
    char c;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO);

    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    while (i < size - 1 && (c = getchar()) != '\n' && c != EOF) {
        input[i++] = c;
    }
    input[i] = '\0';

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

void log_message(const char *message) {
    if (log_file != NULL) {
        time_t now = time(NULL);
        fprintf(log_file, "[%s] %s\n", strtok(ctime(&now), "\n"), message);
        fflush(log_file);
    }
}

void handle_rate_limit(long response_code, int *retry_delay) {
    if (response_code == 429) { // HTTP 429 Too Many Requests
        fprintf(stderr, "Rate limit exceeded. Waiting for %d seconds...\n", *retry_delay);
        log_message("Rate limit exceeded");
        sleep(*retry_delay);
        *retry_delay *= 2; // Exponential backoff
    }
}

void report_facebook_account(report_params_t *params) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Failed to initialize libcurl.\n");
        log_message("Failed to initialize libcurl");
        return;
    }

    struct curl_slist *headers = curl_slist_append(NULL, "Content-Type: application/x-www-form-urlencoded");
    if (!headers) {
        fprintf(stderr, "Failed to set HTTP headers.\n");
        log_message("Failed to set HTTP headers");
        curl_easy_cleanup(curl);
        return;
    }

    char *api_url = NULL;
    asprintf(&api_url, "https://graph.facebook.com/%s/%s/abuse_reports", API_VERSION, params->account_id);
    char *post_fields = NULL;
    asprintf(&post_fields, "access_token=%s&reason=%s&category=%s", params->access_token, params->reason, params->category);

    curl_easy_setopt(curl, CURLOPT_URL, api_url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields);

    if (strlen(params->proxy) > 0) {
        curl_easy_setopt(curl, CURLOPT_PROXY, params->proxy);
    }

    CURLcode res;
    int retry_count = params->retries;
    int retry_delay = params->delay;

    do {
        res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            long response_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            if (response_code == 200) {
                printf("\033[1;32mSuccess!\033[0m Facebook account %s reported successfully.\n", params->account_id);
                log_message("Reported account successfully");
                break;
            } else {
                fprintf(stderr, "Failed to report. HTTP Response Code: %ld\n", response_code);
                log_message("Failed to report account");
                handle_rate_limit(response_code, &retry_delay);
            }
        } else {
            fprintf(stderr, "Request error: %s\n", curl_easy_strerror(res));
            log_message("Request error");
        }

        if (--retry_count > 0) {
            log_message("Retrying...");
            sleep(retry_delay);
            retry_delay *= 2;  // Exponential backoff
        }
    } while (retry_count > 0);

    free(api_url);
    free(post_fields);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

void *report_account_thread(void *arg) {
    report_params_t *params = (report_params_t *)arg;
    report_facebook_account(params);
    free(params);
    return NULL;
}

int load_config(const char *filename, report_params_t *params) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open config file %s: %s\n", filename, strerror(errno));
        return -1;
    }

    fscanf(file, "%255s %255s %255s %255s %255s %d %d",
           params->account_id, params->access_token, params->reason, params->category, params->proxy, &params->retries, &params->delay);

    fclose(file);
    return 0;
}

void save_config(const char *filename, report_params_t *params) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Error: Could not open config file %s: %s\n", filename, strerror(errno));
        return;
    }

    fprintf(file, "%s %s %s %s %s %d %d",
            params->account_id, params->access_token, params->reason, params->category, params->proxy, params->retries, params->delay);

    fclose(file);
}

void interactive_mode(report_params_t *params) {
    printf("\033[1;34mEnter the Facebook account ID you want to report: \033[0m");
    fgets(params->account_id, sizeof(params->account_id), stdin);
    params->account_id[strcspn(params->account_id, "\n")] = 0;

    printf("\033[1;34mEnter your Facebook Graph API access token: \033[0m");
    hide_input(params->access_token, sizeof(params->access_token));

    printf("\033[1;34mEnter the reason for reporting: \033[0m");
    fgets(params->reason, sizeof(params->reason), stdin);
    params->reason[strcspn(params->reason, "\n")] = 0;

    printf("\033[1;34mEnter the category for reporting: \033[0m");
    fgets(params->category, sizeof(params->category), stdin);
    params->category[strcspn(params->category, "\n")] = 0;

    printf("\033[1;34mEnter the proxy (if any, otherwise leave blank): \033[0m");
    fgets(params->proxy, sizeof(params->proxy), stdin);
    params->proxy[strcspn(params->proxy, "\n")] = 0;

    printf("\033[1;34mEnter the number of retries: \033[0m");
    scanf("%d", &params->retries);

    printf("\033[1;34mEnter the delay between retries (in seconds): \033[0m");
    scanf("%d", &params->delay);

    // Clear the newline character left in the input buffer
    while (getchar() != '\n');

    save_config("config.txt", params);
}

int main(int argc, char *argv[]) {
    print_banner();

    report_params_t params;
    memset(&params, 0, sizeof(params));
    params.retries = MAX_RETRIES;
    params.delay = INITIAL_RETRY_DELAY;

    if (argc > 1 && strcmp(argv[1], "-c") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Usage: %s -c <config_file>\n", argv[0]);
            return EXIT_FAILURE;
        }
        if (load_config(argv[2], &params) != 0) {
            return EXIT_FAILURE;
        }
    } else if (argc == 2 && strcmp(argv[1], "-i") == 0) {
        interactive_mode(&params);
    } else {
        fprintf(stderr, "Usage: %s [-c <config_file> | -i]\n", argv[0]);
        return EXIT_FAILURE;
    }

    log_file = fopen(LOG_FILE, "a");
    if (!log_file) {
        fprintf(stderr, "Error: Could not open log file.\n");
        return EXIT_FAILURE;
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);

    pthread_t thread;
    while (1) {
        report_params_t *thread_params = malloc(sizeof(report_params_t));
        if (!thread_params) {
            fprintf(stderr, "Memory allocation error\n");
            break;
        }
        memcpy(thread_params, &params, sizeof(report_params_t));
        pthread_create(&thread, NULL, report_account_thread, (void *)thread_params);
        pthread_join(thread, NULL);
    }

    curl_global_cleanup();
    fclose(log_file);

    return EXIT_SUCCESS;
}

