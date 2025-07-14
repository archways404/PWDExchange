import * as openpgp from 'openpgp';
import readline from 'readline/promises';
import { stdin as input, stdout as output } from 'node:process';
import fs from 'fs/promises';

async function encryptWithPGP() {
	const publicKeyArmored = await fs.readFile('./keys/ps-publickey.asc', 'utf-8');
	const publicKey = await openpgp.readKey({ armoredKey: publicKeyArmored });

	const rl = readline.createInterface({ input, output });
	const messageText = await rl.question('Enter the message to encrypt: ');
	const filename = await rl.question(
		'Enter filename to save encrypted message (e.g. message.pgp): '
	);
	rl.close();

	const encrypted = await openpgp.encrypt({
		message: await openpgp.createMessage({ text: messageText }),
		encryptionKeys: publicKey,
	});

	await fs.writeFile(filename, encrypted, 'utf-8');
	console.log(`✅ Encrypted message saved to "${filename}"`);
}

encryptWithPGP();
