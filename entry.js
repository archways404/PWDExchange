import axios from 'axios';
import readline from 'readline/promises';
import { stdin as input, stdout as output } from 'node:process';
import * as openpgp from 'openpgp';
import clipboard from 'clipboardy';

/*
import { execFile } from 'child_process';
import open from 'open';
*/

const RAW_BASE = 'https://raw.githubusercontent.com/archways404/PWDExchange/master';

/*
function sendViaOutlook(email, subject, body) {
	return new Promise((resolve, reject) => {
		execFile(
			'powershell',
			['-ExecutionPolicy', 'Bypass', '-File', 'send_mail.ps1', email, subject, body],
			(err, stdout, stderr) => {
				if (err) {
					console.error('❌ Failed to send via Outlook:', stderr || err.message);
					reject(err);
				} else {
					console.log('✅ Email sent via Outlook!');
					resolve();
				}
			}
		);
	});
}

function checkForIncomingPWD() {
	return new Promise((resolve, reject) => {
		execFile(
			'powershell',
			['-ExecutionPolicy', 'Bypass', '-File', 'get_pwd_messages.ps1'],
			(err, stdout, stderr) => {
				if (err) {
					console.error('❌ Failed to check inbox:', stderr || err.message);
					resolve(null);
				} else {
					const messages = stdout.trim();
					if (messages) {
						console.log('\n📩 Incoming PWD Message(s):\n');
						console.log(
							messages
								.split('---MSG---')
								.map((m, i) => `#${i + 1}\n${m.trim()}`)
								.join('\n\n')
						);
						resolve(messages);
					} else {
						console.log('\n📭 No new PWDExchange messages.');
						resolve(null);
					}
				}
			}
		);
	});
}
  */

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
	const keyUrl = `${RAW_BASE}/keys/${keyFile}`;

	// Step 3: Download selected .asc key
	const { data: armoredKey } = await axios.get(keyUrl);
	const publicKey = await openpgp.readKey({ armoredKey });

	// Step 4: Encrypt the message
	const encrypted = await openpgp.encrypt({
		message: await openpgp.createMessage({ text: message }),
		encryptionKeys: publicKey,
	});

	console.log(`\n🔐 Encrypted Message:\n\n${encrypted}\n`);

	/*

	const subject = 'PWDExchange';
	const body = encrypted;

	console.log(`\n🔐 Encrypted Message:\n\n${body}\n`);

	// Step 5: Try sending via Outlook → fallback to mailto
	try {
		await sendViaOutlook(selectedEmail, subject, body);
	} catch (err) {
		console.log('📨 Falling back to default mail client...');
		open(
			`mailto:${selectedEmail}?subject=${encodeURIComponent(subject)}&body=${encodeURIComponent(
				body
			)}`
		);
	}

	// Step 6: Check for any incoming PWD messages
	await checkForIncomingPWD();

  */

	const tagged = `[#PWDExchange]\n\n${encrypted}`;
	await clipboard.write(tagged);
	console.log('\n✅ Encrypted PGP message copied to clipboard.');
	console.log(`💬 Paste it into a Teams chat with ${selectedEmail}.\n`);

	// Try to open Microsoft Teams
	try {
		await open(`msteams:/l/chat/0/0?users=${encodeURIComponent(selectedEmail)}`);
		console.log('🚀 Opening Microsoft Teams...');
	} catch (err) {
		console.warn('⚠️ Could not launch Teams (is it installed and associated with msteams: URI?)');
	}
}

main();
