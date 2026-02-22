#ifndef GIT_COMMANDS_H
#define GIT_COMMANDS_H

#include <Arduino.h>

// Git command structure
struct GitCommand {
  const char *name;      // Display name
  const char *cmd;       // JSON command identifier
  bool requiresConfirm;  // Needs confirmation before execution
  bool isDangerous;      // Destructive operation (double confirm)
  const char *shortName; // Short name for compact display
};

// Git command indices
enum GitCommandIndex {
  CMD_INIT = 0,
  CMD_STATUS,
  CMD_ADD,
  CMD_COMMIT,
  CMD_PULL,
  CMD_PUSH,
  CMD_CHECKOUT,
  CMD_CREATE_BRANCH,
  CMD_MERGE,
  CMD_STASH,
  CMD_STASH_POP,
  CMD_RESET,
  CMD_DIFF,
  CMD_RESOLVE_CONFLICT,
  CMD_COUNT // Total number of commands
};

// Get the command list
const GitCommand *getGitCommands();

// Get command count
int getGitCommandCount();

// Get command by index
const GitCommand *getGitCommand(int index);

// Get command name by index
const char *getGitCommandName(int index);

// Check if command requires confirmation
bool commandRequiresConfirm(int index);

// Check if command is dangerous
bool commandIsDangerous(int index);

#endif // GIT_COMMANDS_H
