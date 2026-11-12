// SPDX-FileCopyrightText: 2026 Nick
// SPDX-License-Identifier: GPL-3.0-or-later

package main

import "C"

import (
	"context"
	"encoding/json"
	"fmt"
	"net/netip"
	"time"

	"tailscale.com/tailcfg"
)

// Disco pings report whether the path is direct or relayed. The local API
// blocks until tailscaled answers, so the caller runs this off the UI thread.
const pingTimeout = 15 * time.Second

//export tailscale_ping
func tailscale_ping(ip *string) *C.char {
	if ip == nil || *ip == "" {
		log_warning("ping called without an address")
		return pingErrorJSON("missing address")
	}

	addr, err := netip.ParseAddr(*ip)
	if err != nil {
		log_warning(fmt.Sprintf("invalid ping address %q: %v", *ip, err))
		return pingErrorJSON(fmt.Sprintf("invalid address: %v", err))
	}

	ctx, cancel := context.WithTimeout(context.Background(), pingTimeout)
	defer cancel()

	res, err := client.Ping(ctx, addr, tailcfg.PingDisco)
	if err != nil {
		log_warning(fmt.Sprintf("ping %s failed: %v", addr, err))
		return pingErrorJSON(err.Error())
	}
	if res == nil {
		log_warning(fmt.Sprintf("ping %s returned no result", addr))
		return pingErrorJSON("empty ping response")
	}

	payload, err := json.Marshal(res)
	if err != nil {
		log_critical(fmt.Sprintf("failed to serialize ping result: %v", err))
		return pingErrorJSON(err.Error())
	}
	return C.CString(string(payload))
}

func pingErrorJSON(message string) *C.char {
	payload, err := json.Marshal(struct {
		Err string
	}{Err: message})
	if err != nil {
		log_critical(fmt.Sprintf("failed to serialize ping error: %v", err))
		return C.CString(`{"Err":"ping failed"}`)
	}
	return C.CString(string(payload))
}
