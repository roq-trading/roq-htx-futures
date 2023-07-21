#!/usr/bin/env bash

if [ "$1" == "debug" ]; then
  PREFIX="gdb --args"
else
  PREFIX=
fi

NAME="huobi-futures"

CONFIG="${CONFIG:-$NAME}"

CONFIG_FILE="$ROQ_CONFIG_PATH/roq-huobi-futures/$CONFIG.toml"

URI="api.hbdm.com"

REST_URI="https://$URI"
WS_MARKET_URI="wss://$URI/swap-ws"
WS_ORDER_URI="wss://$URI/swap-notification"

$PREFIX ./roq-huobi-futures \
  --name "huobi-futures" \
  --config_file "$CONFIG_FILE" \
  --cache_dir "$HOME/var/lib/roq/cache" \
  --event_log_dir "$HOME/var/lib/roq/data" \
  --event_log_symlink true \
  --client_listen_address "$HOME/run/$NAME.sock" \
  --service_listen_address "$HOME/run/metrics/${NAME}.sock" \
  --rest_uri "$REST_URI" \
  --ws_market_uri "$WS_MARKET_URI" \
  --ws_order_uri "$WS_ORDER_URI" \
  --api "swap" \
  $@
