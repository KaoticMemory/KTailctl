// SPDX-FileCopyrightText: 2026 Nick
// SPDX-License-Identifier: GPL-3.0-or-later

package main

import "C"

import (
	"context"
	"encoding/json"
	"fmt"
	"io"
	"os"
	"path/filepath"
	"strings"

	"tailscale.com/client/tailscale/apitype"
	"tailscale.com/tailcfg"
)

func filesError(message string) *C.char {
	log_warning(message)
	return C.CString(message)
}

func filesErrorf(format string, args ...any) *C.char {
	return filesError(fmt.Sprintf(format, args...))
}

// singlePathSegment reports whether name is a single file name with no
// directory components. Taildrop waiting-file names are base names; anything
// else is rejected before it can be joined onto a destination directory.
func singlePathSegment(name string) bool {
	if name == "" || name == "." || name == ".." {
		return false
	}
	if strings.ContainsAny(name, `/\`) {
		return false
	}
	return filepath.Base(name) == name
}

//export tailscale_file_targets
func tailscale_file_targets() *C.char {
	targets, err := client.FileTargets(context.Background())
	if err != nil {
		log_warning(fmt.Sprintf("failed to list taildrop targets: %v", err))
		return nil
	}

	ids := make([]string, 0, len(targets))
	for _, target := range targets {
		if target.Node == nil || target.Node.StableID == "" {
			continue
		}
		ids = append(ids, string(target.Node.StableID))
	}

	payload, err := json.Marshal(ids)
	if err != nil {
		log_critical(fmt.Sprintf("failed to serialize taildrop targets: %v", err))
		return nil
	}
	return C.CString(string(payload))
}

//export tailscale_waiting_files
func tailscale_waiting_files() *C.char {
	files, err := client.WaitingFiles(context.Background())
	if err != nil {
		log_warning(fmt.Sprintf("failed to list waiting taildrop files: %v", err))
		return nil
	}
	if files == nil {
		files = []apitype.WaitingFile{}
	}

	payload, err := json.Marshal(files)
	if err != nil {
		log_critical(fmt.Sprintf("failed to serialize waiting taildrop files: %v", err))
		return nil
	}
	return C.CString(string(payload))
}

//export tailscale_push_file
func tailscale_push_file(targetID *string, path *string) *C.char {
	if targetID == nil || *targetID == "" {
		return filesError("missing taildrop target")
	}
	if path == nil || *path == "" {
		return filesError("missing file path")
	}
	if !filepath.IsAbs(*path) {
		return filesErrorf("refusing to send relative path %q", *path)
	}

	info, err := os.Stat(*path)
	if err != nil {
		return filesErrorf("cannot read %s: %v", filepath.Base(*path), err)
	}
	if !info.Mode().IsRegular() {
		return filesErrorf("%s is not a regular file", filepath.Base(*path))
	}

	file, err := os.Open(*path)
	if err != nil {
		return filesErrorf("cannot open %s: %v", filepath.Base(*path), err)
	}
	defer func() {
		_ = file.Close()
	}()

	err = client.PushFile(context.Background(), tailcfg.StableNodeID(*targetID), info.Size(), filepath.Base(*path), file)
	if err != nil {
		return filesErrorf("failed to send %s: %v", filepath.Base(*path), err)
	}
	log_info(fmt.Sprintf("sent %s to %s", filepath.Base(*path), *targetID))
	return nil
}

//export tailscale_save_waiting_file
func tailscale_save_waiting_file(name *string, destPath *string) *C.char {
	if name == nil || !singlePathSegment(*name) {
		return filesError("invalid waiting file name")
	}
	if destPath == nil || *destPath == "" || !filepath.IsAbs(*destPath) {
		return filesError("invalid destination path")
	}
	if !singlePathSegment(filepath.Base(*destPath)) {
		return filesError("invalid destination file name")
	}

	body, size, err := client.GetWaitingFile(context.Background(), *name)
	if err != nil {
		return filesErrorf("failed to fetch %s: %v", *name, err)
	}
	defer func() {
		_ = body.Close()
	}()

	if err = os.MkdirAll(filepath.Dir(*destPath), 0o755); err != nil {
		return filesErrorf("failed to create %s: %v", filepath.Dir(*destPath), err)
	}

	// O_EXCL so a destination chosen by the UI cannot overwrite an existing file
	// if one appears between the check and the write.
	output, err := os.OpenFile(*destPath, os.O_CREATE|os.O_EXCL|os.O_WRONLY, 0o644)
	if err != nil {
		return filesErrorf("failed to create %s: %v", filepath.Base(*destPath), err)
	}

	written, copyErr := io.Copy(output, body)
	closeErr := output.Close()
	if copyErr != nil || closeErr != nil || (size >= 0 && written != size) {
		_ = os.Remove(*destPath)
		if copyErr != nil {
			return filesErrorf("failed to write %s: %v", filepath.Base(*destPath), copyErr)
		}
		if closeErr != nil {
			return filesErrorf("failed to write %s: %v", filepath.Base(*destPath), closeErr)
		}
		return filesErrorf("incomplete write of %s", filepath.Base(*destPath))
	}

	if err = client.DeleteWaitingFile(context.Background(), *name); err != nil {
		return filesErrorf("saved %s but failed to remove it from the waiting list: %v", *name, err)
	}
	log_info(fmt.Sprintf("saved taildrop file %s", filepath.Base(*destPath)))
	return nil
}

//export tailscale_delete_waiting_file
func tailscale_delete_waiting_file(name *string) *C.char {
	if name == nil || !singlePathSegment(*name) {
		return filesError("invalid waiting file name")
	}
	if err := client.DeleteWaitingFile(context.Background(), *name); err != nil {
		return filesErrorf("failed to delete %s: %v", *name, err)
	}
	log_info(fmt.Sprintf("deleted waiting taildrop file %s", *name))
	return nil
}
