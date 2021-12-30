#!/usr/bin/env bash

CWD="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"

if [ "$1" == "debug" ]; then
	PREFIX="gdb --args"
else
	PREFIX=
fi

NAME="huobi-futures"

CONFIG_FILE="$CWD/config/$NAME.toml"

URI="api.hbdm.com"

REST_URI="https://$URI"
WS_MARKET_URI="wss://$URI/ws"
WS_ORDER_URI="wss://$URI/notification"

$PREFIX ./roq-huobi-futures \
	--name "huobi-futures" \
	--config_file "$CONFIG_FILE" \
	--client_listen_address $CWD/$NAME.sock \
	--metrics_listen_address 2345 \
	--rest_uri "$REST_URI" \
	--ws_market_uri "$WS_MARKET_URI" \
	--ws_order_uri "$WS_ORDER_URI" \
  --api "" \
	$@
