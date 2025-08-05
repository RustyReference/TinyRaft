// aes.c
#include <stdio.h>
#include <string.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

// AES-256 requires 32-byte key and 16-byte IV
#define KEY_SIZE 32
#define IV_SIZE 16

void handle_errors() {
    ERR_print_errors_fp(stderr);
    abort();
}

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
