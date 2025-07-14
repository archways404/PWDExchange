import axios from 'axios';
import readline from 'readline/promises';
import { stdin as input, stdout as output } from 'node:process';
import * as openpgp from 'openpgp';

const RAW_BASE = 'https://raw.githubusercontent.com/archways404/PWDExchange/master';

async function main() {
	// Step 1: Download keys.json
	const jsonUrl = `${RAW_BASE}/keys/KeyMap.json`;
	const { data: keyMap } = await axios.get(jsonUrl);

	const emails = Object.keys(keyMap);
	console.log('📧 Available Recipients:\n');
	emails.forEach((email, i) => {
		console.log(`[${i}] ${email}`);
	});

	// Step 2: Choose recipient + enter message
	const rl = readline.createInterface({ input, output });
	const index = await rl.question('\nChoose a recipient by number: ');
	const message = await rl.question('Enter the message to encrypt: ');
	rl.close();

	const selectedEmail = emails[Number(index)];
	const keyFile = keyMap[selectedEmail];
	const keyUrl = `${RAW_BASE}/${keyFile}`;

	// Step 3: Download selected .asc key
	const { data: armoredKey } = await axios.get(keyUrl);
	const publicKey = await openpgp.readKey({ armoredKey });

	// Step 4: Encrypt the message
	const encrypted = await openpgp.encrypt({
		message: await openpgp.createMessage({ text: message }),
		encryptionKeys: publicKey,
	});

	console.log(`\n🔐 Encrypted Message:\n\n${encrypted}`);
}

main();
