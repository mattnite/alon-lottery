#include <solana_sdk.h>

#define ENOBUFS (5)
// alon.h pretty much
struct lottery {
    uint64_t lamports_per_ticket;
    uint64_t deadline;
    uint64_t max_tickets;
    uint64_t count;
    uint8_t done;
    struct {
        uint8_t is_set;
        uint64_t val;
    } winner;
};

struct ticket {
    uint64_t num;
    uint8_t owner[32];
};

struct cmd {
    uint8_t which;
    struct {
        uint8_t is_set;
        uint64_t val;
    } winner;
};

int lottery_deserialize(const uint8_t *buf, uint64_t len, struct lottery* out) {
    sol_memcpy(&out->lamports_per_ticket, buf + 0, sizeof(uint64_t));
    sol_memcpy(&out->deadline, buf + 8, sizeof(uint64_t));
    sol_memcpy(&out->max_tickets, buf + 16, sizeof(uint64_t));
    sol_memcpy(&out->count, buf + 24, sizeof(uint64_t));
    sol_memcpy(&out->done, buf + 32, sizeof(uint8_t));
    uint64_t offset = 33;
    {
        uint8_t is_set;
        if (len < offset + sizeof(uint8_t))
            return -ENOBUFS;

        sol_memcpy(&is_set, buf + offset, sizeof(uint8_t));
        offset += sizeof(uint8_t);
        if (is_set != 0) {
            if (len < offset + sizeof(uint64_t))
                return -ENOBUFS;

            sol_memcpy(&out->winner.val, buf + offset, sizeof(uint64_t));
            out->winner.is_set = 1;
            offset += sizeof(uint64_t);
        } else {
            out->winner.is_set = 0;
        }
    }
    return 0;
}

int lottery_deserialize_account(SolAccountInfo *account, struct lottery* out) {
    return lottery_deserialize(account->data, account->data_len, out);
}

int lottery_deserialize_instruction(SolParameters *params, struct lottery* out) {
    return lottery_deserialize(params->data, params->data_len, out);

}

int lottery_serialize(struct lottery *in, uint8_t *buf, uint64_t len) {
    sol_memcpy(buf + 0, &in->lamports_per_ticket, sizeof(uint64_t));
    sol_memcpy(buf + 8, &in->deadline, sizeof(uint64_t));
    sol_memcpy(buf + 16, &in->max_tickets, sizeof(uint64_t));
    sol_memcpy(buf + 24, &in->count, sizeof(uint64_t));
    sol_memcpy(buf + 32, &in->done, sizeof(uint8_t));
    uint64_t offset = 33;
    if (len < offset + sizeof(uint8_t))
        return -ENOBUFS;

    if (in->winner.is_set == 0) {
        buf[offset++] = 0;
    } else {
        buf[offset++] = 1;
        if (len < offset + sizeof(uint64_t))
            return -ENOBUFS;

        sol_memcpy(buf + offset, &in->winner.val, sizeof(uint64_t));
        offset += sizeof(uint64_t);
    }
    return 0;
}

int lottery_serialize_account(struct lottery* in, SolAccountInfo *account) {
    return lottery_serialize(in, account->data, account->data_len);
}

void lottery_deinit(struct lottery *lottery) {
}

int ticket_deserialize(const uint8_t *buf, uint64_t len, struct ticket* out) {
    if (len < 40) {
        return -ENOBUFS;
    }

    sol_memcpy(&out->num, buf + 0, sizeof(uint64_t));
    sol_memcpy(out->owner, buf + 8, sizeof(out->owner));
    return 0;
}

int ticket_deserialize_account(SolAccountInfo *account, struct ticket* out) {
    return ticket_deserialize(account->data, account->data_len, out);
}

int ticket_deserialize_instruction(SolParameters *params, struct ticket* out) {
    return ticket_deserialize(params->data, params->data_len, out);

}

int ticket_serialize(struct ticket *in, uint8_t *buf, uint64_t len) {
    if (len < 40) {
        return -ENOBUFS;
    }

    sol_memcpy(buf + 0, &in->num, sizeof(uint64_t));
    sol_memcpy(buf + 8, in->owner, sizeof(in->owner));
    return 0;
}

int ticket_serialize_account(struct ticket* in, SolAccountInfo *account) {
    return ticket_serialize(in, account->data, account->data_len);
}

void ticket_deinit(struct ticket *ticket) {
}

int cmd_deserialize(const uint8_t *buf, uint64_t len, struct cmd* out) {
    sol_memcpy(&out->which, buf + 0, sizeof(uint8_t));
    uint64_t offset = 1;
    {
        uint8_t is_set;
        if (len < offset + sizeof(uint8_t))
            return -ENOBUFS;

        sol_memcpy(&is_set, buf + offset, sizeof(uint8_t));
        offset += sizeof(uint8_t);
        if (is_set != 0) {
            if (len < offset + sizeof(uint64_t))
                return -ENOBUFS;

            sol_memcpy(&out->winner.val, buf + offset, sizeof(uint64_t));
            out->winner.is_set = 1;
            offset += sizeof(uint64_t);
        } else {
            out->winner.is_set = 0;
        }
    }
    return 0;
}

int cmd_deserialize_account(SolAccountInfo *account, struct cmd* out) {
    return cmd_deserialize(account->data, account->data_len, out);
}

int cmd_deserialize_instruction(SolParameters *params, struct cmd* out) {
    return cmd_deserialize(params->data, params->data_len, out);

}

int cmd_serialize(struct cmd *in, uint8_t *buf, uint64_t len) {
    sol_memcpy(buf + 0, &in->which, sizeof(uint8_t));
    uint64_t offset = 1;
    if (len < offset + sizeof(uint8_t))
        return -ENOBUFS;

    if (in->winner.is_set == 0) {
        buf[offset++] = 0;
    } else {
        buf[offset++] = 1;
        if (len < offset + sizeof(uint64_t))
            return -ENOBUFS;

        sol_memcpy(buf + offset, &in->winner.val, sizeof(uint64_t));
        offset += sizeof(uint64_t);
    }
    return 0;
}

int cmd_serialize_account(struct cmd* in, SolAccountInfo *account) {
    return cmd_serialize(in, account->data, account->data_len);
}

void cmd_deinit(struct cmd *cmd) {
}

//-----------------------------------------------------------------------------

#include <solana_sdk.h>

#define CMD_CREATE      (0)
#define CMD_BUY         (1)
#define CMD_END         (2)
#define CMD_SET_WINNER  (3)

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

uint64_t buy_ticket(struct lottery *lottery, SolParameters *params) {
  SolAccountInfo *lottery_acct = &params->ka[0];
  SolAccountInfo *customer_acct = &params->ka[1];
  SolAccountInfo *system_program_acct = &params->ka[2];

  sol_assert(!lottery->done);
  sol_assert(!lottery->winner.is_set);
  sol_assert(lottery->count < lottery->max_tickets);
  //sol_assert(lottery_acct->is_signer);
  sol_assert(lottery_acct->is_writable);
  sol_assert(customer_acct->is_signer);
  sol_assert(customer_acct->is_writable);
  sol_assert(*customer_acct->lamports > lottery->lamports_per_ticket);

  *customer_acct->lamports -= lottery->lamports_per_ticket;
  *lottery_acct->lamports += lottery->lamports_per_ticket;

  // TODO: create ticket
  uint64_t num = lottery->count + 1;
  uint8_t bump;
  const SolSignerSeed seeds[] = {
    { .addr = params->program_id->x, .len = sizeof(SolPubkey) },
    { .addr = lottery_acct->key->x, .len = sizeof(SolPubkey) },
    { .addr = (uint8_t *)&num, .len = sizeof(num) },
    { .addr = &bump, .len = 1 },
  };

  SolPubkey ticket;
  sol_assert(SUCCESS == sol_try_find_program_address(
          seeds, SOL_ARRAY_SIZE(seeds) - 1, params->program_id, &ticket, &bump));

  sol_log("found program address");
  // invoke account creation system program
  SolAccountMeta arguments[] = {
    { customer_acct->key, true, true },
    { &ticket, true, true },
  };

  uint8_t data[4 + 8 + 8 + 32]; // TODO: not sure what the first 4 bytes are for?
  *(uint64_t *)(data + 4) = 1000; // lamports
  *(uint64_t *)(data + 4 + 8) = 40; // space
  sol_memcpy(data + 4 + 8 + 8, customer_acct->key, sizeof(SolPubkey)); // owner

  const SolInstruction instruction = {
    .program_id = system_program_acct->key,
    .accounts = arguments,
    .account_len = SOL_ARRAY_SIZE(arguments),
    .data = data,
    .data_len = SOL_ARRAY_SIZE(data),
  };

  const SolSignerSeeds signer_seeds[] = {{seeds, 4}};
  sol_assert(SUCCESS == sol_invoke_signed(
    &instruction,
    params->ka,
    params->ka_num,
    signer_seeds,
    SOL_ARRAY_SIZE(signer_seeds)));

  //SolAccountMeta argument = { &ticket_acct, true, true };
  //const SolInstruction init_instruction = {
  //  .program_id = params->program_id,
  //  .accounts = &argument,
  //  .account_len = 1,
  //  .data = init_data,
  //  .data_len = sizeof(init_data);
  //};

  //sol_assert(SUCCESS = sol_invoke_signed(
  //  &init_instruction,
  //  ,
  //  ,
  //  signer_seeds

  // recursive call to init ticket

  return SUCCESS;
}

uint64_t end_lottery(struct lottery *lottery, SolParameters *params) {
  SolAccountInfo *lottery_acct = &params->ka[0];

  sol_assert(!lottery->done);
  sol_assert(lottery_acct->is_writable);

  lottery->done = true;
  return SUCCESS;
}

uint64_t set_winner(struct lottery *lottery, struct cmd *cmd, SolParameters *params) {
  SolAccountInfo *lottery_acct = &params->ka[0];

  sol_assert(lottery_acct->is_writable);
  sol_assert(lottery->done);
  sol_assert(lottery->count > 0);
  sol_assert(!lottery->winner.is_set);
  sol_assert(cmd->winner.is_set);
  sol_assert(cmd->winner.val <= lottery->count);

  lottery->winner.is_set = true;
  lottery->winner.val = cmd->winner.val;

  return ERROR_INVALID_ARGUMENT;
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
      sol_assert(0 == end_lottery(&lottery, &params));
      break;
    default:
      return ERROR_INVALID_ARGUMENT;
  }

  sol_assert(0 == lottery_serialize_account(&lottery, &params.ka[0]));
  return SUCCESS;
}
