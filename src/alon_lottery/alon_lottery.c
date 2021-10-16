#include <solana_sdk.h>
#include "alon.h"

#define CMD_CREATE      (0)
#define CMD_BUY         (1)
#define CMD_END         (2)

uint64_t create_lottery(struct lottery *lottery, SolParameters *params) {
  sol_assert(lottery->count == 0);

  *lottery = (struct lottery) {
    // TODO: user defined options
    .lamports_per_ticket = 1000,
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

  sol_assert(!lottery->winner.is_set);
  sol_assert(lottery->count < lottery->max_tickets);
  sol_assert(!lottery_acct->is_signer);
  sol_assert(lottery_acct->is_writable);
  sol_assert(customer_acct->is_signer);
  sol_assert(customer_acct->is_writable);
  sol_assert(*customer_acct->lamports > lottery->lamports_per_ticket);

  *customer_acct->lamports -= lottery->lamports_per_ticket;
  *lottery_acct->lamports += lottery->lamports_per_ticket;
  sol_memcpy(get_ticket(lottery_acct, lottery->count), customer_acct->key, sizeof(SolPubkey));

  lottery->count += 1;

  return SUCCESS;
}

uint64_t end_lottery(struct lottery *lottery, struct cmd *cmd, SolParameters *params) {
  SolAccountInfo *lottery_acct = &params->ka[0];
  SolAccountInfo *system_program_acct = &params->ka[1];

  sol_assert(!lottery->winner.is_set);
  sol_assert(cmd->seed.is_set);
  sol_assert(lottery_acct->is_writable);

  // here at alon industries we only use the most advanced of algorithms
  lottery->winner.val = cmd->seed.val % lottery->count;
  lottery->winner.is_set = true;

  SolPubkey* winner = get_ticket(lottery_acct, lottery->winner.val);
  sol_log("winner:");
  sol_log_pubkey(winner);

  // transfer funds
  SolAccountMeta arguments[] = {
    {lottery_acct->key, true, true},
    {winner, true, false}};

  uint8_t data[12];
  *(uint32_t *)data = 2;
  *(uint64_t *)(data + 4) = *lottery_acct->lamports;

  const SolInstruction instruction = {
    system_program_acct->key,
    arguments,
    SOL_ARRAY_SIZE(arguments),
    data,
    SOL_ARRAY_SIZE(data),
  };

  return sol_invoke(&instruction, params->ka, params->ka_num);
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
    default:
      return ERROR_INVALID_ARGUMENT;
  }

  sol_assert(0 == lottery_serialize_account(&lottery, &params.ka[0]));
  return SUCCESS;
}
