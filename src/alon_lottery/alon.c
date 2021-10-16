#include "alon.h"

#define ENOBUFS (5)

int lottery_deserialize(const uint8_t *buf, uint64_t len, struct lottery* out) {
    sol_memcpy(&out->lamports_per_ticket, buf + 0, sizeof(uint64_t));
    sol_memcpy(&out->deadline, buf + 8, sizeof(uint64_t));
    sol_memcpy(&out->max_tickets, buf + 16, sizeof(uint64_t));
    sol_memcpy(&out->count, buf + 24, sizeof(uint64_t));
    uint64_t offset = 32;
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
    uint64_t offset = 32;
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

            sol_memcpy(&out->seed.val, buf + offset, sizeof(uint64_t));
            out->seed.is_set = 1;
            offset += sizeof(uint64_t);
        } else {
            out->seed.is_set = 0;
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

    if (in->seed.is_set == 0) {
        buf[offset++] = 0;
    } else {
        buf[offset++] = 1;
        if (len < offset + sizeof(uint64_t))
            return -ENOBUFS;

        sol_memcpy(buf + offset, &in->seed.val, sizeof(uint64_t));
        offset += sizeof(uint64_t);
    }
    return 0;
}

int cmd_serialize_account(struct cmd* in, SolAccountInfo *account) {
    return cmd_serialize(in, account->data, account->data_len);
}

void cmd_deinit(struct cmd *cmd) {
}
