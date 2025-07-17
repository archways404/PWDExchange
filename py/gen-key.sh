#!/bin/bash

set -e

FIRSTNAME="Philip"
LASTNAME="Stenberg"
EMAIL="philip.stenberg@arjo.com"
KEY_ID="$FIRSTNAME $LASTNAME <$EMAIL>"
KEY_FILENAME="$FIRSTNAME-publickey.asc"

echo "📦 Generating non-expiring PGP key for: $KEY_ID"
gpg --batch --quiet --yes --quick-generate-key "$KEY_ID" rsa2048 cert,sign,encrypt

echo "📤 Exporting public key to $KEY_FILENAME"
gpg --armor --export "$EMAIL" > "$KEY_FILENAME"

echo "✅ Done. You can now upload $KEY_FILENAME wherever needed."

#chmod +x gen-key.sh
