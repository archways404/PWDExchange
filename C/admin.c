#include <errno.h>
#include <gpgme.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ONEDRIVE_PATH "/home/sandbox/OneDrive/PWDExchange.asc.gpg"
#define DOWNLOAD_PATH "/home/sandbox/Downloads/PWDExchange_public.asc"
#define TEMP_KEY_FILE "/tmp/PWDExchange_private.asc"

void print_intro() { printf("\n[ PWDExchange Decryptor ]\n\n"); }

int file_exists(const char *path) { return access(path, F_OK) != -1; }

void prompt_passphrase(char *buffer, size_t len) {
  printf("Enter passphrase: ");
  if (fgets(buffer, len, stdin)) {
    buffer[strcspn(buffer, "\n")] = 0;
  }
}

void paste_and_decrypt_loop() {
  char buffer[8192];
  while (1) {
    printf("\nPaste PGP message (or 'exit' to quit):\n> ");
    if (!fgets(buffer, sizeof(buffer), stdin))
      break;
    if (strncmp(buffer, "exit", 4) == 0)
      break;

    // TODO: decrypt using GPGME
    printf("[INFO] Decrypting...\n");
    // Decryption placeholder
    printf("[RESULT] Decryption feature not implemented yet.\n");
  }
}

int main() {
  print_intro();

  if (file_exists(ONEDRIVE_PATH)) {
    printf("[INFO] Found encrypted private key at: %s\n", ONEDRIVE_PATH);
    char passphrase[256];
    prompt_passphrase(passphrase, sizeof(passphrase));

    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
             "gpg --batch --yes --quiet --passphrase " % s " -o " / tmp /
                 PWDExchange_private.asc " -d " % s "",
             passphrase, ONEDRIVE_PATH);
    int result = system(cmd);
    if (result != 0) {
      fprintf(stderr, "[ERROR] Failed to decrypt private key.\n");
      return 1;
    }
    printf("[SUCCESS] Decrypted private key.\n");

    // TODO: load into GPGME session and decrypt messages
    paste_and_decrypt_loop();

  } else {
    printf("[WARN] No key found at %s\n", ONEDRIVE_PATH);
    printf("Would you like to generate a new key? (y/n): ");
    char yn[10];
    fgets(yn, sizeof(yn), stdin);
    if (yn[0] == 'y' || yn[0] == 'Y') {
      char name[128], email[128], passphrase[128];
      printf("Enter name: ");
      fgets(name, sizeof(name), stdin);
      name[strcspn(name, "\n")] = 0;

      printf("Enter email: ");
      fgets(email, sizeof(email), stdin);
      email[strcspn(email, "\n")] = 0;

      printf("Choose passphrase for private key: ");
      fgets(passphrase, sizeof(passphrase), stdin);
      passphrase[strcspn(passphrase, "\n")] = 0;

      char gen_cmd[1024];
            snprintf(gen_cmd, sizeof(gen_cmd),
                     "gpg --batch --yes --quiet --passphrase "%s" "
                     "--quick-generate-key "%s <%s>" default default 1y",
                     passphrase, name, email);
        if (system(gen_cmd) != 0) {
          fprintf(stderr, "[ERROR] Failed to generate key.\n");
          return 1;
        }

        snprintf(gen_cmd, sizeof(gen_cmd),
                 "gpg --output " / home / sandbox / Downloads /
                     PWDExchange_public.asc " --armor --export " % s "",
                 email);
        system(gen_cmd);
        printf("[INFO] Public key saved to: "
               "/home/sandbox/Downloads/PWDExchange_public.asc\n");

        snprintf(gen_cmd, sizeof(gen_cmd),
                 "gpg --output " / tmp /
                     PWDExchange_private.asc " --armor --export-secret-keys " %
                     s "",
                 email);
        system(gen_cmd);

        snprintf(gen_cmd, sizeof(gen_cmd),
                 "gpg -c --cipher-algo AES256 --output " / home / sandbox /
                     OneDrive / PWDExchange.asc.gpg " " / tmp /
                     PWDExchange_private.asc "");
        system(gen_cmd);

        printf("[SUCCESS] Encrypted private key saved to OneDrive.\n");
        } else {
        printf("Exiting...\n");
        }
    }

    return 0;
  }
