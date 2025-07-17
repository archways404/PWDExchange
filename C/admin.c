#include <errno.h>
#include <gpgme.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define ONEDRIVE_PATH "/Users/archways404/Downloads/PWDExchange_private.asc.gpg"
#define DOWNLOAD_PATH "/Users/archways404/Downloads/PWDExchange_public.asc"
#define TEMP_KEY_FILE "/tmp/PWDExchange_private.asc"
#define TEMP_GNUPGHOME "/tmp/pgp_admin_keyring"
#define ENCRYPTED_KEY_FILE                                                     \
  "/Users/archways404/Downloads/PWDExchange_private.asc.gpg"

void print_intro() { printf("\n[ PWDExchange Decryptor ]\n\n"); }

int file_exists(const char *path) { return access(path, F_OK) != -1; }

void prompt_passphrase(char *buffer, size_t len) {
  printf("Enter passphrase: ");
  fflush(stdout);

  struct termios oldt, newt;
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);

  if (fgets(buffer, len, stdin)) {
    buffer[strcspn(buffer, "\n")] = 0;
  }

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  printf("\n");
}

void paste_and_decrypt_loop(gpgme_ctx_t ctx) {
  char message[8192] = {0};

  while (1) {
    printf("\nPaste full PGP message (end with '-----END PGP MESSAGE-----') or "
           "type 'exit'\n> ");
    char *ptr = message;
    size_t remaining = sizeof(message);
    int complete = 0;

    while (fgets(ptr, remaining, stdin)) {
      if (strncmp(ptr, "exit", 4) == 0)
        return;

      if (strstr(ptr, "-----END PGP MESSAGE-----")) {
        complete = 1;
        break;
      }

      size_t len = strlen(ptr);
      ptr += len;
      remaining -= len;

      if (remaining <= 0) {
        fprintf(stderr, "[ERROR] Message too long.\n");
        return;
      }
    }

    if (!complete) {
      fprintf(stderr, "[ERROR] Incomplete message. Try again.\n");
      continue;
    }

    gpgme_data_t cipher, plain;
    gpgme_data_new_from_mem(&cipher, message, strlen(message), 0);
    gpgme_data_new(&plain);

    gpgme_error_t err = gpgme_op_decrypt(ctx, cipher, plain);
    if (err) {
      fprintf(stderr, "[ERROR] Decryption failed: %s\n", gpgme_strerror(err));
    } else {
      char out[8192];
      size_t n = gpgme_data_seek(plain, 0, SEEK_END);
      gpgme_data_seek(plain, 0, SEEK_SET);
      gpgme_data_read(plain, out, n);
      out[n] = '\0';
      printf("[RESULT] %s\n", out);

      FILE *clip = popen("pbcopy", "w");
      if (clip) {
        fwrite(out, 1, strlen(out), clip);
        pclose(clip);
        printf("[INFO] Decrypted message copied to clipboard.\n");
      } else {
        fprintf(stderr, "[WARN] Failed to copy to clipboard.\n");
      }
    }

    gpgme_data_release(cipher);
    gpgme_data_release(plain);
  }
}

int main() {
  print_intro();

  setenv("GNUPGHOME", TEMP_GNUPGHOME, 1);
  system("rm -rf " TEMP_GNUPGHOME);
  system("mkdir -p " TEMP_GNUPGHOME);
  system("chmod 700 " TEMP_GNUPGHOME);

  gpgme_check_version(NULL);
  gpgme_ctx_t ctx;
  gpgme_error_t err = gpgme_new(&ctx);
  if (err) {
    fprintf(stderr, "[ERROR] Failed to create GPG context: %s\n",
            gpgme_strerror(err));
    return 1;
  }

  gpgme_set_armor(ctx, 1);
  gpgme_set_pinentry_mode(ctx, GPGME_PINENTRY_MODE_LOOPBACK);

  if (file_exists(ONEDRIVE_PATH)) {
    printf("[INFO] Found encrypted private key at: %s\n", ONEDRIVE_PATH);
    char passphrase[256];
    prompt_passphrase(passphrase, sizeof(passphrase));

    char cmd[1024];
    snprintf(
        cmd, sizeof(cmd),
        "gpg --batch --yes --quiet --pinentry-mode loopback --passphrase '%s' "
        "-o %s -d %s",
        passphrase, TEMP_KEY_FILE, ONEDRIVE_PATH);
    if (system(cmd) != 0) {
      fprintf(stderr, "[ERROR] Failed to decrypt private key.\n");
      return 1;
    }
    printf("[SUCCESS] Decrypted private key.\n");

    snprintf(cmd, sizeof(cmd),
             "gpg --batch --quiet --pinentry-mode loopback --homedir '%s' "
             "--import '%s'",
             TEMP_GNUPGHOME, TEMP_KEY_FILE);
    system(cmd);

    // 🔥 Auto-delete decrypted key
    remove(TEMP_KEY_FILE);

    paste_and_decrypt_loop(ctx);
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

      prompt_passphrase(passphrase, sizeof(passphrase));

      char cmd[1024];
      snprintf(cmd, sizeof(cmd),
               "gpg --batch --yes --quiet --pinentry-mode loopback "
               "--delete-secret-keys '%s <%s>'",
               name, email);
      system(cmd);
      snprintf(cmd, sizeof(cmd),
               "gpg --batch --yes --quiet --pinentry-mode loopback "
               "--delete-keys '%s <%s>'",
               name, email);
      system(cmd);

      snprintf(cmd, sizeof(cmd),
               "gpg --batch --yes --quiet --pinentry-mode loopback "
               "--passphrase '%s' "
               "--quick-generate-key '%s <%s>' default default 1y",
               passphrase, name, email);
      if (system(cmd) != 0) {
        fprintf(stderr, "[ERROR] Failed to generate key.\n");
        return 1;
      }

      snprintf(cmd, sizeof(cmd),
               "gpg --batch --quiet --pinentry-mode loopback --output %s "
               "--armor --export %s",
               DOWNLOAD_PATH, email);
      system(cmd);
      printf("[INFO] Public key saved to: %s\n", DOWNLOAD_PATH);

      snprintf(cmd, sizeof(cmd),
               "gpg --batch --quiet --pinentry-mode loopback --output %s "
               "--armor --export-secret-keys %s",
               TEMP_KEY_FILE, email);
      system(cmd);

      snprintf(cmd, sizeof(cmd),
               "gpg --batch --yes --quiet --pinentry-mode loopback "
               "--passphrase '%s' "
               "-c --cipher-algo AES256 --output %s %s",
               passphrase, ENCRYPTED_KEY_FILE, TEMP_KEY_FILE);
      system(cmd);

      printf("[SUCCESS] Encrypted private key saved to: %s\n",
             ENCRYPTED_KEY_FILE);

      // Clean up temp file after encryption
      remove(TEMP_KEY_FILE);
    } else {
      printf("Exiting...\n");
    }
  }

  gpgme_release(ctx);
  system("rm -rf " TEMP_GNUPGHOME);
  return 0;
}