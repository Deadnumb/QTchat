#!/bin/sh
set -eu

cat > /app/config.json <<EOF
{
  "server": { "host": "0.0.0.0", "port": ${VERIFY_SERVER_PORT:-50051} },
  "email":  { "user": "${VERIFY_EMAIL_USER:-}", "pass": "${VERIFY_EMAIL_PASS:-}" },
  "mysql":  { "host": "${MYSQL_HOST:-mysql}", "port": ${MYSQL_PORT:-3306} },
  "redis":  { "host": "${REDIS_HOST:-redis}", "port": ${REDIS_PORT:-6379}, "passwd": "${REDIS_PASSWD:-123456}" }
}
EOF

exec /app/verify-server
