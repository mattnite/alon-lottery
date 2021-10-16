#pragma once

#include <solana_sdk.h>

struct lottery {
    uint64_t lamports_per_ticket;
    uint64_t deadline;
    uint64_t max_tickets;
    uint64_t count;
    struct {
        uint8_t is_set;
        uint64_t val;
    } winner;
};


int lottery_deserialize(const uint8_t *buf, uint64_t len, struct lottery* out);
int lottery_deserialize_account(SolAccountInfo *account, struct lottery* out);
int lottery_deserialize_instruction(SolParameters *params, struct lottery* out);
int lottery_serialize(struct lottery *in, uint8_t *buf, uint64_t len);
int lottery_serialize_account(struct lottery* in, SolAccountInfo *account);
void lottery_deinit(struct lottery *lottery);

struct cmd {
    uint8_t which;
    struct {
        uint8_t is_set;
        uint64_t val;
    } seed;
};


int cmd_deserialize(const uint8_t *buf, uint64_t len, struct cmd* out);
int cmd_deserialize_account(SolAccountInfo *account, struct cmd* out);
int cmd_deserialize_instruction(SolParameters *params, struct cmd* out);
int cmd_serialize(struct cmd *in, uint8_t *buf, uint64_t len);
int cmd_serialize_account(struct cmd* in, SolAccountInfo *account);
void cmd_deinit(struct cmd *cmd);
