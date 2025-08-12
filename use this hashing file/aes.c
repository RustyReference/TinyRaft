#include <stdio.h>
#include <string.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

// AES-256 requires 32-byte key and 16-byte IV
#define KEY_SIZE 32
#define IV_SIZE 16

/**<Handles OpenSSL errors by printing the error stack to stderr
   * and aborting the program. This is used whenever a crypto function fails.
   */
void handle_errors() {
    ERR_print_errors_fp(stderr);
    abort();
}

/**<Encrypts plaintext using AES-256-CBC mode.
   * Initializes an OpenSSL EVP encryption context, processes the plaintext, 
   * and produces the ciphertext output.
   * @param plaintext The input data to encrypt.
   * @param plaintext_len Length of the plaintext in bytes.
   * @param key 32-byte AES key.
   * @param iv 16-byte initialization vector.
   * @param ciphertext Buffer where the resulting ciphertext will be stored.
   * @return The length of the generated ciphertext in bytes.
   */
int encrypt(unsigned char *plaintext, int plaintext_len,
            unsigned char *key, unsigned char *iv,
            unsigned char *ciphertext) {
    EVP_CIPHER_CTX *ctx;
    int len, ciphertext_len;

    if (!(ctx = EVP_CIPHER_CTX_new()))
        handle_errors();

    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
        handle_errors();

    if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        handle_errors();
    ciphertext_len = len;

    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
        handle_errors();
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    return ciphertext_len;
}

/**<Decrypts ciphertext using AES-256-CBC mode.
   * Initializes an OpenSSL EVP decryption context, processes the ciphertext, 
   * and reconstructs the original plaintext.
   * @param ciphertext The encrypted data to decrypt.
   * @param ciphertext_len Length of the ciphertext in bytes.
   * @param key 32-byte AES key.
   * @param iv 16-byte initialization vector.
   * @param plaintext Buffer where the resulting plaintext will be stored.
   * @return The length of the recovered plaintext in bytes.
   */
int decrypt(unsigned char *ciphertext, int ciphertext_len,
            unsigned char *key, unsigned char *iv,
            unsigned char *plaintext) {
    EVP_CIPHER_CTX *ctx;
    int len, plaintext_len;

    if (!(ctx = EVP_CIPHER_CTX_new()))
        handle_errors();

    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
        handle_errors();

    if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        handle_errors();
    plaintext_len = len;

    if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
        handle_errors();
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    return plaintext_len;
}

/**<Demonstrates AES-256-CBC encryption and decryption using a static key and IV.
   * Encrypts a test message, prints the ciphertext in hex, 
   * then decrypts it back and prints the original plaintext.
   */
int main() {
    // 256-bit key and 128-bit IV
    unsigned char key[KEY_SIZE] = "01234567890123456789012345678901";  // 32 bytes
    unsigned char iv[IV_SIZE]   = "0123456789012345";                  // 16 bytes

    unsigned char *plaintext = (unsigned char *)"This is a secret message!";
    unsigned char ciphertext[128];
    unsigned char decrypted[128];

    int ciphertext_len = encrypt(plaintext, strlen((char *)plaintext), key, iv, ciphertext);

    printf("Ciphertext (hex): ");
    for (int i = 0; i < ciphertext_len; i++)
        printf("%02x", ciphertext[i]);
    printf("\n");

    int decrypted_len = decrypt(ciphertext, ciphertext_len, key, iv, decrypted);
    decrypted[decrypted_len] = '\0'; // null terminate

    printf("Decrypted text: %s\n", decrypted);
    return 0;
}
// Compile with: gcc aes.c -o aes -lssl -lcrypto