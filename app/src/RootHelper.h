// SPDX-FileCopyrightText: claudemods
// SPDX-License-Identifier: MIT
//
// The administrator helper of Claude Codex Cloner: "claude-codex-cloner --root-helper", started once
// through pkexec by RootSession. It reads JSON requests (one per line) on stdin, runs the allowed
// programs as root and sends their output and exit codes back on stdout. When the window closes (stdin
// ends) it closes the tasks' stdin - a cancellable cloner task then stops and cleans up - lets the others
// finish, and exits.

#pragma once

int runRootHelper( int argc, char** argv );
