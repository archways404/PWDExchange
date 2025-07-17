import subprocess
import getpass
import os

print("\n[PWDExchange Key Generator]\n")

firstname = input("First name: ").strip()
lastname = input("Last name: ").strip()
email = input("Email: ").strip()
name = f"{firstname} {lastname}"
uid = f"{name} <{email}>"
filename = f"{firstname}-publickey.asc"

passphrase = getpass.getpass("Choose a passphrase for your key (hidden): ")

# Prepare GPG batch input
batch_config = f"""
Key-Type: RSA
Key-Length: 2048
Subkey-Type: RSA
Subkey-Length: 2048
Name-Real: {name}
Name-Email: {email}
Expire-Date: 0
Passphrase: {passphrase}
%commit
"""

print("\n📦 Generating PGP key (non-expiring)...")

# Write batch to temp file
with open("gpg_batch.conf", "w") as f:
    f.write(batch_config)

try:
    subprocess.run(["gpg", "--batch", "--generate-key", "gpg_batch.conf"], check=True)
except subprocess.CalledProcessError:
    print("❌ GPG key generation failed.")
    os.remove("gpg_batch.conf")
    exit(1)

os.remove("gpg_batch.conf")

# Export public key
print("📤 Exporting public key...")
with open(filename, "w") as f:
    subprocess.run(["gpg", "--armor", "--export", email], stdout=f, check=True)

print(f"\n✅ Public key saved to {filename}.")
print("You can now share this public key as needed.\n")