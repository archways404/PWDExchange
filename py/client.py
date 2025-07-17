import requests
import json
import pgpy
import pyperclip
import webbrowser
import urllib.parse

print("\n[PWDExchange]\n")

# Fetch key mapping JSON
json_url = "https://raw.githubusercontent.com/archways404/PWDExchange/refs/heads/master/keys/KeyMap.json"
try:
    response = requests.get(json_url)
    response.raise_for_status()
    keymap = response.json()
except Exception as e:
    print(f"[ERROR] Failed to fetch or parse JSON: {e}")
    exit(1)

# List recipients
emails = list(keymap.keys())
print("Available recipients:")
for i, email in enumerate(emails):
    print(f"  [{i}] {email}")

try:
    choice = int(input("\nEnter the number of the recipient\n> "))
    email = emails[choice]
    filename = keymap[email]
except Exception as e:
    print("[ERROR] Invalid selection.")
    exit(1)

# Fetch public key
key_url = f"https://raw.githubusercontent.com/archways404/PWDExchange/refs/heads/master/keys/{filename}"
try:
    res = requests.get(key_url)
    res.raise_for_status()
    key_blob = res.content.decode("utf-8", errors="ignore").strip()
    pubkey, _ = pgpy.PGPKey.from_blob(key_blob)
except Exception as e:
    print(f"[ERROR] Failed to fetch or parse public key: {e}")
    exit(1)

# Prompt for message
message = input("\nEnter your password\n> ")
msg = pgpy.PGPMessage.new(message)
enc_msg = pubkey.encrypt(msg)

# Output and clipboard
enc_str = str(enc_msg)
print("\n[INFO] Encrypted message:\n")
print(enc_str)
pyperclip.copy(enc_str)
print("[SUCCESS] Copied encrypted message to clipboard.\n")

# Open Teams URL
enc_url = urllib.parse.quote(enc_str)
teams_url = f"https://teams.microsoft.com/l/chat/0/0?users={email}&message={enc_url}"
print(f"[INFO] Teams link: {teams_url}")
try:
    webbrowser.open(teams_url)
except:
    print("[WARNING] Could not open Teams automatically.")
