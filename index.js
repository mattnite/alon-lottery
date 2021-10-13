const web3 = require("@solana/web3.js");
const borsh = require('borsh');
const { argv } = require('process');
const fs = require('fs');

const LOTTERY_SIZE = 41;
const TICKET_SIZE = 8 + 32;

const CMD_CREATE = 0;
const CMD_BUY = 1;
const CMD_END = 2;
const CMD_SET_WINNER = 4;

class Cmd {
  constructor(val) {
    this.which = val.which;
    this.winner = val.which;
  }
}

class Lottery {
  constructor(val) {
    for (const [key, value] of Object.entries(val)) {
      this[key] = value;
    }
  }
}

class Ticket {
  constructor(val) {
    this.num = val.num;
    this.owner = val.owner;
  }
}

const schema = new Map([
  [Cmd, { kind: 'struct', fields: [['which', 'u8'], ['winner', { kind: 'option', type: 'u64' }]]}],
  [Ticket, { kind: 'struct', fields: [['num', 'u64'], ['owner', [32]]]}],
  [Lottery, {
    kind: 'struct',
    fields: [
      ['lamports_per_ticket', 'u64'],
      ['deadline', 'u64'],
      ['max_tickets', 'u64'],
      ['count', 'u64'],
      ['done', 'u8'],
      ['winner', { kind: 'option', type: 'u64' }],
    ],
  }],
]);

function getProgramId() {
  return new web3.PublicKey(fs.readFileSync('program-id').toString());
}

function getLottery() {
  return new web3.PublicKey(fs.readFileSync('lottery').toString());
}

async function getPayer() {
  let payer = web3.Keypair.generate();
  let connection = new web3.Connection(web3.clusterApiUrl('devnet'), 'confirmed');
  let airdropSignature = await connection.requestAirdrop(
    payer.publicKey,
    web3.LAMPORTS_PER_SOL,
  );

  await connection.confirmTransaction(airdropSignature);
  return payer;
}

async function createLottery() {
  let admin = await getPayer();
  let programId = getProgramId();
  let lottery = web3.Keypair.generate();
  let connection = new web3.Connection(web3.clusterApiUrl('devnet'), 'confirmed');
  let minimumLamportsForRentExemption = await connection.getMinimumBalanceForRentExemption(LOTTERY_SIZE);

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
      data: borsh.serialize(schema, new Cmd({ which: CMD_CREATE, winner: null })),
    }),
  );
  
  await web3.sendAndConfirmTransaction(connection, createLotteryTransaction, [admin, lottery]);

  const lotteryAccountInfo = await connection.getAccountInfo(lottery.publicKey);
  const lotteryData = borsh.deserialize(schema, Lottery, lotteryAccountInfo.data);
  console.log(lotteryData);

  return {
    admin: admin,
    lottery: lottery,
  };
}

async function buyTicket() {
  let customer = await getPayer();
  let programId = getProgramId();
  let lottery = getLottery();
  //let list = await getList();
  let ticket = web3.Keypair.generate();

  let buyTicketTransaction = new web3.Transaction().add(
    web3.SystemProgram.createAccount({
      fromPubkey: customer.publicKey,
      lamports: 1000,
      newAccountPubkey: ticket.publicKey,
      programId,
      space: TICKET_SIZE,
    }),
    new web3.Transaction({
      keys: [
        { pubkey: lottery, isSigner: true, isWritable: false },
        { pubkey: customer.publicKey, isSigner: true, isWritable: false },
        { pubkey: ticket.publicKey, isSigner: false, isWritable: true },
      ],
      programId,
      data: borsh.serialize(schema, new Cmd({ which: CMD_BUY, winner: null })),
    })
  );

  await web3.sendAndConfirmTransaction(connection, buyTicketTransaction, [customer, lottery]);
  console.log(`customer ${customer.publicKey.toBase58()} bought ${ticket.publicKey.toBase58()}`);
}

async function endLottery() {
  let admin = await getAdmin();
  let programId = await getProgramId();

  let endLotteryTransaction = new web3.Transaction().add(
    new web3.Transaction({
      keys: [
        { pubkey: lottery.publicKey.toBase58(), isSigner: true, isWritable: true },
        { pubkey: admin.publicKey.toBase58(), isSigner: true, isWritable: false },
      ],
      programId,
      data: borsh.serialize(schema, new Cmd({ which: CMD_SET_END, winner: null })),
    })
  );

  await connection.confirmTransaction(endLotteryTransaction);
}

async function setWinner() {
  let programId = await getProgramId();
  let lottery = await getLottery();

  let connection = new web3.Connection(web3.clusterApiUrl('devnet'), 'confirmed');
  let lotteryInfo = await connection.getAccountInfo(lottery.publicKey);

  // TODO: deserialize lottery account info
  //       calc winner from account info
  let setWinnerTransaction = new web3.Transaction().add(
    new web3.Transaction({
      keys: [
        { pubkey: lottery.publicKey.toBase58(), isSigner: true, isWritable: false },
        { pubkey: admin.publicKey.toBase58(), isSigner: true, isWritable: false },
      ],
      programId,
      // add  serialized winner to data
      data: borsh.serialize(schema, new Cmd({ which: CMD_SET_WINNER, winner: 2 })),
    })
  );

  return winner;
}

async function collectWinnings() {
  // winner starts
  let programId = await getProgramId();
  let lottery = await getLottery();
  let ticket = await getTicket();

  let collectWinningsTransaction = new web3.Transaction().add(
    new web3.Transaction({
      keys: [
        { pubkey: lottery.publicKey.toBase58(), isSigner: true, isWritable: false },
        { pubkey: ticket.publicKey.toBase58(), isSigner: true, isWritable: false },
      ],
      programId,
    })
  );

  await connection.confirmTransaction(collectWinningsTransaction);
}

async function run() {
  switch (argv[2]) {
    case 'deploy':
      const programId = await deploy();
      fs.writeFileSync('program-id', programId);
      console.log(`deployed: ${programId}`);
      break;
    case 'create':
      if (fs.existsSync('lottery')) {
        console.log('lottery file already exists');
        return;
      }

      const result = await createLottery();
      console.log(`created lottery: ${result.lottery.publicKey.toBase58()}`)
      fs.writeFileSync('lottery', result.lottery.publicKey.toBase58());
      break;
    case 'buy':
      await buyTicket();
      break;
    case 'end':
      await endLottery();
      break;
    case 'win':
      await setWinner();
      break;
    case 'collect':
      await collectWinnings();
      break;
    default:
      console.log(`${argv[2]} not recognized subcommand`);
      break;
  }
}

run();
