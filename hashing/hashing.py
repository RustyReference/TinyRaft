from cryptography.hazmat.primitives.ciphers.aead import AESGCM
import os
import base64

# Generate a secure 256-bit key
def generate_key() -> bytes:
    return AESGCM.generate_key(bit_length=256)

# Encrypt plaintext (returns base64-encoded data for storage/transmission)
def encrypt_aes_gcm(plaintext: str, key: bytes) -> str:
    aesgcm = AESGCM(key)
    nonce = os.urandom(12)  # 96-bit recommended
    ciphertext = aesgcm.encrypt(nonce, plaintext.encode(), None)
    # Combine nonce + ciphertext for storage/transmission
    encrypted = nonce + ciphertext
    return base64.b64encode(encrypted).decode()

# Decrypt base64-encoded encrypted text
def decrypt_aes_gcm(encrypted_base64: str, key: bytes) -> str:
    encrypted = base64.b64decode(encrypted_base64)
    nonce = encrypted[:12]
    ciphertext = encrypted[12:]
    aesgcm = AESGCM(key)
    plaintext = aesgcm.decrypt(nonce, ciphertext, None)
    return plaintext.decode()

print(encrypt_aes_gcm('bce', key=bytes([128, 192, 255])))