// SPDX-FileCopyrightText: 2026 Nick
// SPDX-License-Identifier: GPL-3.0-or-later

package main

import "C"

import (
	"context"
	"encoding/json"
	"fmt"
)

//export tailscale_waiting_file_names
func tailscale_waiting_file_names() *C.char {
	files, err := client.WaitingFiles(context.Background())
	if err != nil {
		log_warning(fmt.Sprintf("failed to list waiting taildrop files: %v", err))
		return nil
	}

	names := make([]string, 0, len(files))
	for _, file := range files {
		if file.Name == "" {
			continue
		}
		names = append(names, file.Name)
	}

	payload, err := json.Marshal(names)
	if err != nil {
		log_critical(fmt.Sprintf("failed to serialize waiting taildrop file names: %v", err))
		return nil
	}
	return C.CString(string(payload))
}
