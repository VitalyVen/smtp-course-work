#! /bin/bash
docker exec -i 42_mailhog mhsendmail --smtp-addr="localhost:587" <<EOF
From: Me <me@example.com>
To: You <you@example.com>
Subject: Test Message

Hello there!
EOF
