import * as openpgp from 'openpgp';
import fs from 'fs/promises';

async function generateKey() {
	const { privateKey, publicKey } = await openpgp.generateKey({
		type: 'rsa',
		rsaBits: 2048,
		userIDs: [{ name: 'Test User', email: 'test@example.com' }],
		passphrase: 'testpass',
	});

	await fs.writeFile('publickey.asc', publicKey, 'utf-8');
	await fs.writeFile('privatekey.asc', privateKey, 'utf-8');

	console.log('✅ Keys generated and saved: publickey.asc and privatekey.asc');
}

generateKey();
