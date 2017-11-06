#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

#define STATE_SIZE 8
#define KEY_SIZE 10

uint8_t forward_sbox[16] = {12,  5,  6, 11 , 9,  0, 10, 13,  3, 14, 15,  8,  4,  7,  1,  2};
uint8_t inverse_sbox[16] = { 5, 14, 15,  8, 12,  1,  2, 13, 11,  4,  6,  3,  0,  7,  9, 10};

void
print_state(uint8_t* state)
{
    for (size_t i = 0; i < STATE_SIZE; i++)
        printf("%x", state[i]);
    printf("\n");
}

void
add_round_key(uint8_t* state, uint8_t *round_key)
{
    for (size_t i = 0; i < STATE_SIZE; i++)
        state[i] = state[i] ^ round_key[i];
}

void sbox(uint8_t* state, uint8_t* sbox)
{
    for (size_t i = 0; i < STATE_SIZE; i++) {
        state[i] = (sbox[(int)((state[i] & 0xf0) >> 4)] << 4) | (sbox[(int)(state[i] & 0x0f)]);
    }
}

void forward_pbox(uint8_t* state)
{
    uint8_t tmp[STATE_SIZE];
    memcpy(tmp, state, sizeof(uint8_t) * STATE_SIZE);

    for (size_t i = 0; i < STATE_SIZE; i++) {
        uint8_t c, m, b1, b2, B1;
        c = 0x00;
        m = 0x01;
        for (size_t j = 0; j < 8; j++) {
            m = m << j;
            b2 = i * 8 + j;
            b1 = (b2 % 16) * 4 + b2 / 16;
            B1 = b1 / 8;
            c = c | (( (tmp[B1] & (0x01 << (b1 % 8))) >> (b1 % 8)) << j);
        }
        state[i] = c;
    }
}

void inverse_pbox(uint8_t* state)
{
    uint8_t tmp[STATE_SIZE];
    memcpy(tmp, state, sizeof(uint8_t) * STATE_SIZE);

    uint8_t c, m, B1;

    m = 0x00;
    for (size_t i = 0; i < 8; i++) {
        c = 0x00;
        for (size_t j = 0; j < 8; j++, m += 16) {
            if (m >= 64) m = m % 64 + 1;
            B1 = m / 8;
            c = c | (((tmp[B1] & (0x01 << m % 8)) >> (m % 8)) << j);
        }
        state[i] = c;
    }
}

void forward_key_update(uint8_t* key, uint8_t round)
{
    uint8_t c;
    uint8_t tmp[KEY_SIZE];
    memcpy(tmp, key, sizeof(uint8_t) * KEY_SIZE);

    for (size_t i = 0; i < KEY_SIZE; i++)
        key[(i + 7) % 10] = tmp[i];

    for (size_t i = 0; i < 9; i++) {
        tmp[i+1] = (key[i] & 0xf8) >> 3;
    }
    tmp[0] = (key[9] & 0xf8) >> 3;

    for (size_t i = 0; i < KEY_SIZE; i++) {
        key[i] = (key[i] << 5) | tmp[i];
    }

    c = key[9];
    key[9] = (key[9] & 0x0f) | ((forward_sbox[(c & 0xf0)>>4]<<4));
    key[2] = (key[2] & 0xf0) | ((key[2] & 0x0f) ^ ((round & 0x1e) >> 1));
    key[1] = (key[1] & 0x7f) | ((key[1] & 0x80) ^ ((round & 0x01) << 7));
}

void inverse_key_update(uint8_t* key, uint8_t round)
{
    uint8_t c;
    uint8_t tmp[KEY_SIZE];

    key[2] = (key[2] & 0xf0) | ((key[2] & 0x0f) ^ ((round & 0x1e) >> 1));
    key[1] = (key[1] & 0x7f) | ((key[1] & 0x80) ^ ((round & 0x01) << 7));

    c = key[9];
    key[9] = (key[9] & 0x0f) | ((inverse_sbox[(c & 0xf0) >> 4] << 4));

    memcpy(tmp, key, sizeof(uint8_t) * KEY_SIZE);

    for (size_t i = 0; i < KEY_SIZE; i++)
        key[i] = tmp[(i + 7) % 10];

    for (size_t i = 0; i < 9; i++) {
        tmp[i] = (key[i + 1] & 0x1f) << 3;
    }
    tmp[9] = (key[0] & 0x1f) << 3;

    for (size_t i = 0; i < KEY_SIZE; i++) {
        key[i] = (key[i] >> 5) | tmp[i];
    }
}

void encrypt(uint8_t* state, uint8_t* key)
{
    uint8_t round_key[STATE_SIZE];

    for (size_t k = 0; k < 32; k++) {
        for (size_t j = 0; j < 8; j++) {
            round_key[j] = key[2+j];
        }

        add_round_key(state, round_key);
        sbox(state, forward_sbox);
        forward_pbox(state);
        forward_key_update(key, k);

        for (size_t j = 0; j < 8; j++) {
            round_key[j] = key[2 + j];
        }
    }

    add_round_key(state, round_key);
}

void decrypt(uint8_t* state, uint8_t* key)
{
    uint8_t round_key[STATE_SIZE];

    for (size_t j = 0; j < 8; j++) {
        round_key[j] = key[2 + j];
    }

    add_round_key(state, round_key);
    for (ptrdiff_t k = 31; k >= 0; k--) {

        inverse_key_update(key, k);
        inverse_pbox(state);
        sbox(state, inverse_sbox);

        for (size_t j = 0; j < 8; j++) {
            round_key[j] = key[2 + j];
        }

        add_round_key(state, round_key);
    }
}

int main(void) {
    uint8_t state[STATE_SIZE];
    uint8_t Mkey[KEY_SIZE];

    /* Copy plaintext into state */
    memcpy(state, "barbarro", STATE_SIZE);
    memcpy(Mkey, "_S21FEDCBA", KEY_SIZE);

    print_state(state);

    encrypt(state, Mkey);

    print_state(state);

    decrypt(state, Mkey);

    print_state(state);
    return 0;
}
