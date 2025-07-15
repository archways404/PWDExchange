#include <curl/curl.h>
#include <gpgme.h>
#include <jansson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef _WIN32
#include <windows.h>
#define popen _popen
#define pclose _pclose
#endif

#define TEMP_GNUPGHOME "/tmp/pgp_temp_keyring"

int DEBUG = 0; // Enable debug output with --debug flag

#define LOG(fmt, ...)                                                          \
  do {                                                                         \
    if (DEBUG)                                                                 \
      printf("[DEBUG] " fmt, ##__VA_ARGS__);                                   \
  } while (0)

struct string {
  char *ptr;
  size_t len;
};

// Initialize dynamic string buffer
void init_string(struct string *s) {
  s->len = 0;
  s->ptr = malloc(1);
  if (s->ptr)
    s->ptr[0] = '\0';
}

// Callback for libcurl to accumulate response data
size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s) {
  size_t new_len = s->len + size * nmemb;
  s->ptr = realloc(s->ptr, new_len + 1);
  if (!s->ptr) {
    fprintf(stderr, "[ERROR] Out of memory while fetching data.\n");
    exit(EXIT_FAILURE);
  }
  memcpy(s->ptr + s->len, ptr, size * nmemb);
  s->ptr[new_len] = '\0';
  s->len = new_len;
  return size * nmemb;
}

// Perform HTTP GET and return the response as a string
char *fetch_url(const char *url) {
  CURL *curl = curl_easy_init();
  struct string s;
  init_string(&s);

  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      fprintf(stderr, "[ERROR] Failed to fetch %s: %s\n", url,
              curl_easy_strerror(res));
      return NULL;
    }
    curl_easy_cleanup(curl);
  }
  return s.ptr;
}

// Copy text to clipboard (macOS / Windows only)
void copy_to_clipboard(const char *text) {
#ifdef __APPLE__
  FILE *f = popen("pbcopy", "w");
#elif _WIN32
  FILE *f = popen("clip", "w");
#else
  FILE *f = NULL;
#endif
  if (f) {
    fputs(text, f);
    pclose(f);
  }
}

int main(int argc, char *argv[]) {
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--debug") == 0) {
      DEBUG = 1;
    }
  }

  printf("\n[PWDExchange] \n\n");

  // Fetch key mapping JSON file
  const char *json_url = "https://raw.githubusercontent.com/archways404/"
                         "PWDExchange/refs/heads/master/keys/KeyMap.json";
  char *json_data = fetch_url(json_url);
  if (!json_data) {
    fprintf(stderr, "[ERROR] Failed to fetch JSON data.\n");
    return 1;
  }
  LOG("Fetched JSON metadata.\n");

  // Parse JSON data
  json_error_t error;
  json_t *root = json_loads(json_data, 0, &error);
  if (!root) {
    fprintf(stderr, "[ERROR] JSON parsing error: %s\n", error.text);
    return 1;
  }

  // Prompt user to select recipient
  const char *email = NULL;
  const char *filename = NULL;
  int index = 0;
  void *iter = json_object_iter(root);
  printf("Available recipients:\n");
  while (iter) {
    email = json_object_iter_key(iter);
    printf("  [%d] %s\n", index++, email);
    iter = json_object_iter_next(root, iter);
  }

  printf("\nEnter the number of the recipient\n> ");
  int choice;
  scanf("%d", &choice);

  // Match selected index
  index = 0;
  iter = json_object_iter(root);
  while (iter) {
    if (index == choice) {
      email = json_object_iter_key(iter);
      filename = json_string_value(json_object_iter_value(iter));
      break;
    }
    iter = json_object_iter_next(root, iter);
    index++;
  }

  if (!filename) {
    fprintf(stderr, "[ERROR] Invalid selection.\n");
    return 1;
  }

  // Fetch the public key file
  char key_url[512];
  snprintf(key_url, sizeof(key_url),
           "https://raw.githubusercontent.com/archways404/PWDExchange/refs/"
           "heads/master/keys/%s",
           filename);
  char *key_data = fetch_url(key_url);
  if (!key_data || strlen(key_data) < 100) {
    fprintf(stderr, "[ERROR] Failed to fetch valid key data.\n");
    return 1;
  }

  LOG("Public key downloaded (%zu bytes).\n", strlen(key_data));

  // Setup temporary keyring
  setenv("GNUPGHOME", TEMP_GNUPGHOME, 1);
  system("rm -rf " TEMP_GNUPGHOME);
  system("mkdir -p " TEMP_GNUPGHOME);

  // Init GPG context
  gpgme_check_version(NULL);
  gpgme_ctx_t ctx;
  gpgme_error_t err = gpgme_new(&ctx);
  if (err) {
    fprintf(stderr, "[ERROR] Failed to create GPG context: %s\n",
            gpgme_strerror(err));
    return 1;
  }
  gpgme_set_armor(ctx, 1);

  // Import public key
  gpgme_data_t key_data_obj;
  gpgme_data_new_from_mem(&key_data_obj, key_data, strlen(key_data), 0);
  err = gpgme_op_import(ctx, key_data_obj);
  gpgme_data_release(key_data_obj);

  gpgme_import_result_t import_result = gpgme_op_import_result(ctx);
  if (!import_result || import_result->imported == 0 ||
      !import_result->imports || !import_result->imports->fpr) {
    fprintf(stderr, "[ERROR] Key import failed or no usable key found.\n");
    return 1;
  }

  const char *fingerprint = import_result->imports->fpr;
  LOG("Imported key with fingerprint: %s\n", fingerprint);

  getchar(); // clear newline from scanf
  char message[4096];
  printf("\nEnter your password\n> ");
  fgets(message, sizeof(message), stdin);

  // Encrypt the message
  gpgme_data_t plain, cipher;
  gpgme_data_new_from_mem(&plain, message, strlen(message), 0);
  gpgme_data_new(&cipher);

  gpgme_key_t recp[2] = {0};
  err = gpgme_get_key(ctx, fingerprint, &recp[0], 0);
  if (err) {
    fprintf(stderr, "[ERROR] Failed to get key: %s\n", gpgme_strerror(err));
    return 1;
  }

  err = gpgme_op_encrypt(ctx, recp, GPGME_ENCRYPT_ALWAYS_TRUST, plain, cipher);
  if (err) {
    fprintf(stderr, "[ERROR] Encryption failed: %s\n", gpgme_strerror(err));
    return 1;
  }

  // Read encrypted message
  char *result = NULL;
  size_t result_len = gpgme_data_seek(cipher, 0, SEEK_END);
  gpgme_data_seek(cipher, 0, SEEK_SET);
  result = malloc(result_len + 1);
  gpgme_data_read(cipher, result, result_len);
  result[result_len] = '\0';

  // Print and copy to clipboard
  printf("\n[INFO] Encrypted message:\n%s\n", result);
  copy_to_clipboard(result);
  printf("[SUCCESS] Message copied to clipboard.\n");

  // Optional: send a message via Teams
  message[strcspn(message, "\n")] = '\0'; // remove newline

  // URL-encode the message

  char teams_url[1024];
  snprintf(teams_url, sizeof(teams_url),
           "https://teams.microsoft.com/l/chat/0/0?users=%s&message=%s", email,
           result);

  printf("[INFO] Attempting to open Microsoft Teams chat with: %s\n", email);

  char open_cmd[1100];
#ifdef __APPLE__
  snprintf(open_cmd, sizeof(open_cmd), "open \"%s\"", teams_url);
#elif _WIN32
  snprintf(open_cmd, sizeof(open_cmd), "start \"\" \"%s\"", teams_url);
#else
  snprintf(open_cmd, sizeof(open_cmd), "xdg-open \"%s\"", teams_url);
#endif

  int open_result = system(open_cmd);
  if (open_result != 0) {
    fprintf(stderr, "[WARNING] Failed to open Teams URL automatically.\n");
  }

  printf("[INFO] Teams chat link: %s\n", teams_url);

  while (1) {
    char retry_input[10];
    printf("Retry opening Teams? (y/n): ");
    fgets(retry_input, sizeof(retry_input), stdin);

    if (retry_input[0] == 'y' || retry_input[0] == 'Y') {
      printf("[INFO] Retrying...\n");
      system(open_cmd);
    } else {
      break;
    }
  }

  // Cleanup
  err = gpgme_op_delete(ctx, recp[0], 0);
  if (err) {
    fprintf(stderr, "[WARNING] Failed to delete imported key: %s\n",
            gpgme_strerror(err));
  }

  free(result);
  free(json_data);
  gpgme_key_release(recp[0]);
  gpgme_data_release(plain);
  gpgme_data_release(cipher);
  gpgme_release(ctx);
  json_decref(root);

  LOG("Cleaning up temporary GPG home folder...\n");
  system("rm -rf " TEMP_GNUPGHOME);

  printf("[SUCCESS] Done.\n");
  return 0;
}