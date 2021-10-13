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
  if (lottery->count > 0)
    return ERROR_INVALID_ARGUMENT;

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
  SolAccountInfo *ticket_acct = &params->ka[2];

  sol_assert(!lottery->done);
  sol_assert(!lottery->winner.is_set);
  sol_assert(lottery->count < lottery->max_tickets);
  sol_assert(lottery_acct->is_signer);
  sol_assert(lottery_acct->is_writable);
  sol_assert(customer_acct->is_signer);
  sol_assert(customer_acct->is_writable);
  sol_assert(*customer_acct->lamports > lottery->lamports_per_ticket);

  *customer_acct->lamports -= lottery->lamports_per_ticket;
  *lottery_acct->lamports += lottery->lamports_per_ticket;

  // TODO: create ticket

  struct ticket ticket;
  if (0 < ticket_deserialize_account(ticket_acct, &ticket))
    return ERROR_INVALID_ARGUMENT;

  ticket.num = ++lottery->count;
  sol_memcpy(ticket.owner, customer_acct->owner->x, sizeof(SolPubkey));

  if (0 < ticket_serialize_account(&ticket, ticket_acct))
    return ERROR_INVALID_ARGUMENT;
  
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

  // TODO: generate derived account and transfer to owner

  return ERROR_INVALID_ARGUMENT;
}

uint64_t entrypoint(const uint8_t *input) {
  SolAccountInfo accounts[3];
  SolParameters params = { .ka = accounts };

  if (!sol_deserialize(input, &params, SOL_ARRAY_SIZE(accounts)))
    return ERROR_INVALID_ARGUMENT;

  struct cmd cmd;
  if (0 < cmd_deserialize_instruction(&params, &cmd))
    return ERROR_INVALID_ARGUMENT;

  struct lottery lottery;
  if (0 < lottery_deserialize_account(&params.ka[0], &lottery))
    return ERROR_INVALID_ARGUMENT;

  int err = 0;
  switch (cmd.which) {
    case CMD_CREATE:
      err = create_lottery(&lottery, &params);
      break;
    case CMD_BUY:
      err = buy_ticket(&lottery, &params);
      break;
    case CMD_END:
      err = end_lottery(&lottery, &params);
      break;
    case CMD_SET_WINNER:
      err = set_winner(&lottery, &cmd, &params);
      break;
    default:
      return ERROR_INVALID_ARGUMENT;
  }

  if (err != 0)
      return ERROR_INVALID_ARGUMENT;

  if (0 < lottery_serialize_account(&lottery, &params.ka[0]))
      return ERROR_INVALID_ARGUMENT;

  return SUCCESS;
}
