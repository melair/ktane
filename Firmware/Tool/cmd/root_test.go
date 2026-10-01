package cmd

import "testing"

func TestBackplaneMonitorCommandRegistered(t *testing.T) {
	command, _, err := newRootCommand().Find([]string{"backplane", "bus", "monitor"})
	if err != nil {
		t.Fatal(err)
	}
	if command.CommandPath() != "tool backplane bus monitor" {
		t.Fatalf("command path %q, want %q", command.CommandPath(), "tool backplane bus monitor")
	}
	if command.Flag("port") == nil || command.Flag("baud") == nil || command.Flag("duration") == nil {
		t.Fatal("monitor command is missing port, baud, or duration flag")
	}
}
