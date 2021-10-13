OUT_DIR := out
SOLANA_TOOLS = $(shell dirname $(shell which cargo-build-bpf))
include ~/code/solana/sdk/bpf/c/bpf.mk
