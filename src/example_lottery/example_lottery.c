// A super simple lottery program to show off Alon, don't actually use it with money

#include "example_lottery.h"

#define CMD_CREATE      (0)
#define CMD_BUY         (1)
#define CMD_END         (2)
#define CMD_SEND        (3)

uint64_t create_lottery(struct lottery *lottery, SolParameters *params) {
  sol_assert(lottery->count == 0);

  *lottery = (struct lottery) {
    .lamports_per_ticket = 100000,
    .max_tickets = 100,
    .deadline = 0,
    .count = 0,
    .winner = {
      .is_set = 0,
      .val = 0,
    },
  };

  return SUCCESS;
}

SolPubkey *get_ticket(SolAccountInfo *lottery_acct, uint64_t index) {
    uint64_t offset = 64 + (index * sizeof(SolPubkey));
    sol_assert(offset + sizeof(SolPubkey) < lottery_acct->data_len);
    return (SolPubkey *)(lottery_acct->data + offset);
}

uint64_t buy_ticket(struct lottery *lottery, SolParameters *params) {
  SolAccountInfo *lottery_acct = &params->ka[0];
  SolAccountInfo *customer_acct = &params->ka[1];
  SolAccountInfo *system_program_acct = &params->ka[2];

  sol_assert(!lottery->winner.is_set);
  sol_assert(lottery->count < lottery->max_tickets);
  sol_assert(!lottery_acct->is_signer);
  sol_assert(lottery_acct->is_writable);
  sol_assert(customer_acct->is_signer);
  sol_assert(customer_acct->is_writable);
  sol_assert(*customer_acct->lamports > lottery->lamports_per_ticket);
  sol_assert(SolPubkey_same(lottery_acct->owner, params->program_id));

  // transfer funds
  SolAccountMeta arguments[] = {
    {customer_acct->key, true, true},
    {lottery_acct->key, true, false}};

  uint8_t data[12];
  *(uint32_t *)data = 2;
  *(uint64_t *)(data + 4) = lottery->lamports_per_ticket;

  const SolInstruction instruction = {
    system_program_acct->key,
    arguments,
    SOL_ARRAY_SIZE(arguments),
    data,
    SOL_ARRAY_SIZE(data),
  };

  sol_assert(SUCCESS == sol_invoke(&instruction, params->ka, params->ka_num));
  sol_memcpy(get_ticket(lottery_acct, lottery->count), customer_acct->key, sizeof(SolPubkey));
  lottery->count += 1;

  return SUCCESS;
}

uint64_t end_lottery(struct lottery *lottery, struct cmd *cmd, SolParameters *params) {
  SolAccountInfo *lottery_acct = &params->ka[0];
  SolAccountInfo *admin_acct = &params->ka[1];
  SolAccountInfo *system_program_acct = &params->ka[2];

  sol_assert(!lottery->winner.is_set);
  sol_assert(cmd->seed.is_set);
  sol_assert(lottery_acct->is_writable);

  lottery->winner.val = cmd->seed.val % lottery->count;
  lottery->winner.is_set = true;

  SolPubkey* winner = get_ticket(lottery_acct, lottery->winner.val);
  sol_log("winner:");
  sol_log_pubkey(winner);

  return SUCCESS;
}

uint64_t send_winnings(struct lottery* lottery, SolParameters* params) {
  SolAccountInfo *lottery_acct = &params->ka[0];
  SolAccountInfo *customer_acct = &params->ka[1];

  sol_assert(lottery->winner.is_set);
  sol_assert(SolPubkey_same(customer_acct->key, get_ticket(lottery_acct, lottery->winner.val)));

  *customer_acct->lamports += *lottery_acct->lamports;
  *lottery_acct->lamports = 0;

  return SUCCESS;
}

uint64_t entrypoint(const uint8_t *input) {
  SolAccountInfo accounts[3];
  SolParameters params = { .ka = accounts };

  struct cmd cmd;
  struct lottery lottery;

  sol_assert(sol_deserialize(input, &params, SOL_ARRAY_SIZE(accounts)));
  sol_assert(0 == cmd_deserialize_instruction(&params, &cmd));
  sol_assert(0 == lottery_deserialize_account(&params.ka[0], &lottery));

  switch (cmd.which) {
    case CMD_CREATE:
      sol_assert (0 == create_lottery(&lottery, &params));
      break;
    case CMD_BUY:
      sol_assert(0 == buy_ticket(&lottery, &params));
      break;
    case CMD_END:
      sol_assert(0 == end_lottery(&lottery, &cmd, &params));
      break;
    case CMD_SEND:
      sol_assert(0 == send_winnings(&lottery, &params));
      break;
    default:
      return ERROR_INVALID_ARGUMENT;
  }

  sol_assert(0 == lottery_serialize_account(&lottery, &params.ka[0]));
  return SUCCESS;
}

void test_get_ticket__works() {
    SolAccountInfo lottery = {
        .data = sol_calloc(5, 32),
        .data_len = 5 * 32,
    };

    sol_assert((lottery.data + 64) == (uint8_t*)get_ticket(&lottery, 0));
    sol_assert(NULL != get_ticket(&lottery, 1));
    sol_assert(NULL != get_ticket(&lottery, 2));
}

void test_send_winnings__works() {
    uint64_t lottery_lamports = 64;
    uint64_t winner_lamports = 0;
    const uint64_t data_len = 64 + (3 * sizeof(SolPubkey));
    SolAccountInfo accounts[] = {
        {
            .data = sol_calloc(1, data_len),
            .data_len = data_len,
            .lamports = &lottery_lamports,
        },
        {
            .key = sol_calloc(1, sizeof(SolPubkey)),
            .lamports = &winner_lamports,
        },
    };

    SolParameters params = {
        .ka = accounts,
        .ka_num = 2,
    };

    struct lottery lottery = {
        .winner = {
            .is_set = true,
            .val = 2,
        },
    };

    sol_assert(SUCCESS == send_winnings(&lottery, &params));
    sol_assert(lottery_lamports == 0);
    sol_assert(winner_lamports == 64);
}
