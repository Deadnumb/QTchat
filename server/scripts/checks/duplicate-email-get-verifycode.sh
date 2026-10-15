#!/usr/bin/env bash
set -euo pipefail

project=/home/deadnumb/C++ChatProject
config="$project/out/build/gate-server-debug/bin/config.ini"
gate_url=http://127.0.0.1:8080
mysql_password=123456

mysql_query() {
    (cd "$project" && docker compose exec -T mysql         mysql --protocol=socket -uroot -p"$mysql_password" -NBe "$1")
}

redis_args=(docker compose exec -T redis redis-cli -a 123456 --raw)
redis_call() {
    (cd "$project" && "${redis_args[@]}" "$@")
}

if [[ "$(redis_call PING 2>/dev/null || true)" != "PONG" ]]; then
    echo 'FAIL: Redis is unavailable'
    exit 1
fi

existing_email=$(mysql_query 'SELECT email FROM chatdb.user ORDER BY uid LIMIT 1')
if [[ -z "$existing_email" ]]; then
    echo 'FAIL: chatdb.user has no fixture user'
    exit 1
fi

key="verify_code_$existing_email"
had_key=$(redis_call EXISTS "$key")
old_value=''
old_ttl=-2
if [[ "$had_key" == "1" ]]; then
    old_value=$(redis_call GET "$key")
    old_ttl=$(redis_call TTL "$key")
fi

restore_redis() {
    set +e
    if [[ "$had_key" == "1" ]]; then
        if [[ "$old_ttl" -gt 0 ]]; then
            redis_call SET "$key" "$old_value" EX "$old_ttl" >/dev/null 2>&1
        elif [[ "$old_ttl" == "-1" ]]; then
            redis_call SET "$key" "$old_value" >/dev/null 2>&1
        else
            redis_call DEL "$key" >/dev/null 2>&1
        fi
    else
        redis_call DEL "$key" >/dev/null 2>&1
    fi
    set -e
}
trap restore_redis EXIT

response=$(curl --silent --show-error --fail     -X POST "$gate_url/get_verifycode"     -H 'Content-Type: application/json'     --data-binary "{"email":"$existing_email"}")
error_code=$(printf '%s' "$response" | python3 -c     'import json, sys; print(json.load(sys.stdin)["error"])')

if [[ "$error_code" != "1007" ]]; then
    echo "FAIL: duplicate email returned error=$error_code, expected error=1007"
    exit 1
fi

after_key=$(redis_call EXISTS "$key")
if [[ "$had_key" == "1" ]]; then
    after_value=$(redis_call GET "$key")
    after_ttl=$(redis_call TTL "$key")
    if [[ "$after_value" != "$old_value" || "$after_ttl" -le 0 || "$after_ttl" -gt "$old_ttl" ]]; then
        echo 'FAIL: existing verification key changed'
        exit 1
    fi
elif [[ "$after_key" != "0" ]]; then
    echo 'FAIL: duplicate email created a verification key'
    exit 1
fi

echo 'PASS: duplicate email was rejected before verification-code issuance'
