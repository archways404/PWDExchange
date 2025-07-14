import * as openpgp from 'openpgp';
import fs from 'fs/promises';
import readline from 'readline/promises';
import { stdin as input, stdout as output } from 'node:process';

async function decryptMessage() {
	const rl = readline.createInterface({ input, output });
	const filename = await rl.question('Enter path to encrypted file (e.g. message.pgp): ');
	const armoredPrivateKey = await fs.readFile('./keys/privatekey.asc', 'utf-8');
	const passphrase = await rl.question('Enter private key passphrase: ');
	rl.close();

	const encryptedMessage = await fs.readFile(filename, 'utf-8');

	const privateKey = await openpgp.decryptKey({
		privateKey: await openpgp.readPrivateKey({ armoredKey: armoredPrivateKey }),
		passphrase,
	});

	const message = await openpgp.readMessage({
		armoredMessage: encryptedMessage,
	});

	const { data: decrypted } = await openpgp.decrypt({
		message,
		decryptionKeys: privateKey,
	});

	console.log('\n🔓 Decrypted Message:\n');
	console.log(decrypted);
}

decryptMessage();
