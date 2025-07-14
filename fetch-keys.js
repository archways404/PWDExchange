import axios from 'axios';
import fs from 'fs/promises';

const url = 'https://raw.githubusercontent.com/my-org/public-pgp-keys/main/bob@company.com.asc';

async function downloadKey() {
	const res = await axios.get(url);
	await fs.writeFile('bob.asc', res.data);
	console.log('✅ Key downloaded as bob.asc');
}
downloadKey();
