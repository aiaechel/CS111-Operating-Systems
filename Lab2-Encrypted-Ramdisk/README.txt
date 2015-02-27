Design Problem README


Names: Andrew Lee, Michelle Chang
UID: 204304351, 904262084
Problem: Lab 2 #3: Encrypted Ramdisk

What we did:
We coded an authentication process into the ramdisk: at first there is no password, but the user can set a new password using an option (-p). From that point, when the user tries to read the disk, the program will prompt for a password. If the password is correct, then the output will be decrypted and sensible. If the password is incorrect, the output will be encrypted and will appear to be nonsense text.

To implement the encryption, we used a stream cipher, where we generated a random key and XOR'd the key with the plain text to obtain the ciphertext to store into the ramdisk. Since XOR is symmetric, we were able to use XOR to both encrypt and decrypt the text. To make sure the key is secure (never use stream cipher keys more than once), we generate a new key every read and re-encrypt the ramdisk memory that was already written. This way, attackers cannot guess the key, since the key is regenerated often.