const web3 = require("@solana/web3.js");
const borsh = require('borsh');
const { argv } = require('process');
const fs = require('fs');

const CMD_CREATE = 0;
const CMD_BUY = 1;
const CMD_END = 2;

const LOTTERY_SIZE = 64 + (100 * 32)

class Cmd {
  constructor(val) {
    for (const [key, value] of Object.entries(val)) {
      this[key] = value;
    }
  }
}

class Lottery {
  constructor(val) {
    for (const [key, value] of Object.entries(val)) {
      this[key] = value;
    }
  }
}

const schema = new Map([
  [Cmd, { kind: 'struct', fields: [['which', 'u8'], ['seed', { kind: 'option', type: 'u64' }]]}],
  [Lottery, {
    kind: 'struct',
    fields: [
      ['lamports_per_ticket', 'u64'],
      ['deadline', 'u64'],
      ['max_tickets', 'u64'],
      ['done', 'u8'],
      ['winner', { kind: 'option', type: 'u64' }],
    ],
  }],
]);

const sleep = async (milliseconds) => await new Promise((resolve,) => setTimeout(resolve, milliseconds));

async function getPayer() {
  let payer = web3.Keypair.generate();
  let connection = new web3.Connection(web3.clusterApiUrl('devnet'), 'confirmed');
  await sleep(1000);
  let airdropSignature = await connection.requestAirdrop(
    payer.publicKey,
    web3.LAMPORTS_PER_SOL,
  );

  await sleep(1000);
  await connection.confirmTransaction(airdropSignature);

  return payer;
}

function getProgramId() {
  const data = fs.readFileSync('program-id').toString();
  const lines = data.split(/\r?\n/);
  return new web3.PublicKey(lines[0]);
}

async function createLottery() {
  let admin = await getPayer();
  let programId = getProgramId();
  let lottery = web3.Keypair.generate();
  let connection = new web3.Connection(web3.clusterApiUrl('devnet'), 'confirmed');
  await sleep(1000);
  let minimumLamportsForRentExemption = await connection.getMinimumBalanceForRentExemption(LOTTERY_SIZE);

  console.log(`program id: ${programId}`);
  console.log(`minimum lamports: ${minimumLamportsForRentExemption}`);
  let createLotteryTransaction = new web3.Transaction().add(
    web3.SystemProgram.createAccount({
      fromPubkey: admin.publicKey,
      lamports: minimumLamportsForRentExemption,
      newAccountPubkey: lottery.publicKey,
      programId,
      space: LOTTERY_SIZE,
    }),
    new web3.TransactionInstruction({
      keys: [{ pubkey: lottery.publicKey, isSigner: false, isWritable: true }],
      programId,
      data: borsh.serialize(schema, new Cmd({ which: CMD_CREATE })),
    }),
  );
  
  await sleep(1000);
  await web3.sendAndConfirmTransaction(connection, createLotteryTransaction, [admin, lottery]);


  return {
    admin,
    lottery,
    programId,
    customers: [],
  };
}

async function buyTicket(state) {
  let customer = await getPayer();
  state.customers.push(customer);
  let connection = new web3.Connection(web3.clusterApiUrl('devnet'), 'confirmed');

  let buyTicketTransaction = new web3.Transaction().add(
    new web3.TransactionInstruction({
      keys: [
        { pubkey: state.lottery.publicKey, isSigner: true, isWritable: true },
        { pubkey: customer.publicKey, isSigner: true, isWritable: true },
        { pubkey: web3.SystemProgram.programId, isSigner: false, isWritable: false },
      ],
      programId: state.programId,
      data: borsh.serialize(schema, new Cmd({ which: CMD_BUY })),
    })
  );

  await sleep(1000);
  await web3.sendAndConfirmTransaction(connection, buyTicketTransaction, [state.lottery, customer]);
  console.log(`bought a ticket: ${customer.publicKey.toBase58()}`);
  return state;
}

async function endLottery(state) {
  const seed = Math.floor(Math.random() * 4294967295);
  let connection = new web3.Connection(web3.clusterApiUrl('devnet'), 'confirmed');
  let endLotteryTransaction = new web3.Transaction().add(
    new web3.TransactionInstruction({
      keys: [
        { pubkey: state.lottery, isSigner: true, isWritable: true },
        { pubkey: state.admin.publicKey.toBase58(), isSigner: true, isWritable: true },
      ],
      programId: state.programId,
      data: borsh.serialize(schema, new Cmd({ which: CMD_END, seed })),
    })
  );

  await sleep(1000);
  await web3.sendAndConfirmTransaction(connection, endLotteryTransaction, [state.lottery, state.admin]);
  // TODO: print out winner
  return state;
}


async function lotteryTest() {
  let state = await createLottery();

  for (let i = 0; i < 5; i++) {
    state = await buyTicket(state);
  }

  state = await endLottery(state);

  console.log(state);
}

lotteryTest();
