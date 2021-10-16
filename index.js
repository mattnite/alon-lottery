const web3 = require("@solana/web3.js");
const borsh = require('borsh');
const { argv } = require('process');
const fs = require('fs');

const CMD_CREATE = 0;
const CMD_BUY = 1;
const CMD_END = 2;
const CMD_SEND = 3;

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
  [Cmd, {
    kind: 'struct',
    fields: [
      ['which', 'u8'],
      ['seed', { kind: 'option', type: 'u64' }],
    ],
  }],
  [Lottery, {
    kind: 'struct',
    fields: [
      ['lamports_per_ticket', 'u64'],
      ['max_tickets', 'u64'],
      ['count', 'u64'],
      ['winner', { kind: 'option', type: 'u64' }],
    ],
  }],
]);

const sleep = async (milliseconds) => await new Promise((resolve,) => setTimeout(resolve, milliseconds));

async function getPayer() {
  let payer = web3.Keypair.generate();
  let connection = new web3.Connection(web3.clusterApiUrl('devnet'), 'confirmed');
  let airdropSignature = await connection.requestAirdrop(
    payer.publicKey,
    web3.LAMPORTS_PER_SOL,
  );

  await connection.confirmTransaction(airdropSignature);
  await sleep(10000);

  console.log("created payer");
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
  let connection = new web3.Connection(web3.clusterApiUrl('devnet'), 'confirmed');
  await sleep(1000);
  let minimumLamportsForRentExemption = await connection.getMinimumBalanceForRentExemption(LOTTERY_SIZE);

  console.log(`admin: ${admin.publicKey.toBase58()}`);

  const SEED = 'lottery';
  let lottery = await web3.PublicKey.createWithSeed(
    admin.publicKey,
    SEED,
    programId,
  );

  console.log(`lottery: ${lottery}`);
  console.log(`program id: ${programId}`);
  console.log(`minimum lamports: ${minimumLamportsForRentExemption}`);
  let createLotteryTransaction = new web3.Transaction().add(
    web3.SystemProgram.createAccountWithSeed({
      fromPubkey: admin.publicKey,
      basePubkey: admin.publicKey,
      lamports: minimumLamportsForRentExemption,
      newAccountPubkey: lottery,
      programId,
      seed: SEED,
      space: LOTTERY_SIZE,
    }),
    new web3.TransactionInstruction({
      keys: [{ pubkey: lottery, isSigner: false, isWritable: true }],
      programId,
      data: borsh.serialize(schema, new Cmd({ which: CMD_CREATE })),
    }),
  );
  
  await sleep(1000);
  await web3.sendAndConfirmTransaction(connection, createLotteryTransaction, [admin]);

  console.log('created lottery');
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
        { pubkey: state.lottery, isSigner: false, isWritable: true },
        { pubkey: customer.publicKey, isSigner: true, isWritable: true },
        { pubkey: web3.SystemProgram.programId, isSigner: false, isWritable: false },
      ],
      programId: state.programId,
      data: borsh.serialize(schema, new Cmd({ which: CMD_BUY })),
    })
  );

  await sleep(1000);
  await web3.sendAndConfirmTransaction(connection, buyTicketTransaction, [customer]);
  console.log(`bought a ticket: ${customer.publicKey.toBase58()}`);
  return state;
}

async function endLottery(state) {
  const seed = Math.floor(Math.random() * 4294967295);
  let connection = new web3.Connection(web3.clusterApiUrl('devnet'), 'confirmed');
  const [addr, bump] = await web3.PublicKey.findProgramAddress([Buffer.from("lottery")], state.programId);
  let endLotteryTransaction = new web3.Transaction().add(
    new web3.TransactionInstruction({
      keys: [
        { pubkey: state.lottery, isSigner: false, isWritable: true },
        { pubkey: state.admin.publicKey, isSigner: true, isWritable: true },
        { pubkey: web3.SystemProgram.programId, isSigner: false, isWritable: false },
      ],
      programId: state.programId,
      data: borsh.serialize(schema, new Cmd({ which: CMD_END, seed, bump })),
    })
  );

  await sleep(1000);
  await web3.sendAndConfirmTransaction(connection, endLotteryTransaction, [state.admin]);
  // TODO: print out winner
  console.log('end of lottery');
  return state;
}

async function sendWinnings(state) {
  let connection = new web3.Connection(web3.clusterApiUrl('devnet'), 'confirmed');

  const lotteryInfo = await connection.getAccountInfo(state.lottery);
  const header = borsh.deserializeUnchecked(schema, Lottery, lotteryInfo.data);
  console.log(`header: ${JSON.stringify(header)}`);
  const begin = 64 + (header.winner * 32);
  const winner = new web3.PublicKey(lotteryInfo.data.slice(begin, begin + 32)); 

  console.log(`found winner: ${winner.toBase58()}`);
  let buyTicketTransaction = new web3.Transaction().add(
    new web3.TransactionInstruction({
      keys: [
        { pubkey: state.lottery, isSigner: false, isWritable: true },
        { pubkey: winner, isSigner: false, isWritable: true },
        { pubkey: web3.SystemProgram.programId, isSigner: false, isWritable: false },
      ],
      programId: state.programId,
      data: borsh.serialize(schema, new Cmd({ which: CMD_SEND })),
    })
  );

  await sleep(1000);
  await web3.sendAndConfirmTransaction(connection, buyTicketTransaction, [state.admin]);
  return state;
}


async function lotteryTest() {
  let state = await createLottery();

  for (let i = 0; i < 5; i++) {
    state = await buyTicket(state);
  }

  state = await endLottery(state);
  state = await sendWinnings(state);

  console.log(state);
}

lotteryTest();
