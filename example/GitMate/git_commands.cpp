#include "git_commands.h"

// Define the Git command list
static const GitCommand gitCommands[CMD_COUNT] = {
    // name,              cmd,                 requiresConfirm, isDangerous,
    // shortName
    {"Init Repository", "git_init", false, false, "Init"},
    {"Status", "git_status", false, false, "Status"},
    {"Stage All", "git_add", false, false, "Add"},
    {"Commit", "git_commit", false, false, "Commit"},
    {"Pull", "git_pull", false, false, "Pull"},
    {"Push", "git_push", true, false, "Push"},
    {"Checkout Branch", "git_checkout", false, false, "Checkout"},
    {"Create Branch", "git_create_branch", false, false, "New Branch"},
    {"Merge", "git_merge", true, false, "Merge"},
    {"Stash", "git_stash", false, false, "Stash"},
    {"Stash Pop", "git_stash_pop", false, false, "Stash Pop"},
    {"Reset", "git_reset", true, true, "Reset"},
    {"Diff", "git_diff", false, false, "Diff"},
    {"Resolve Conflict", "git_conflict", false, false, "Conflict"}};

const GitCommand *getGitCommands() { return gitCommands; }

int getGitCommandCount() { return CMD_COUNT; }

const GitCommand *getGitCommand(int index) {
  if (index < 0 || index >= CMD_COUNT) {
    return nullptr;
  }
  return &gitCommands[index];
}

const char *getGitCommandName(int index) {
  const GitCommand *cmd = getGitCommand(index);
  return cmd ? cmd->name : "Unknown";
}

bool commandRequiresConfirm(int index) {
  const GitCommand *cmd = getGitCommand(index);
  return cmd ? cmd->requiresConfirm : false;
}

bool commandIsDangerous(int index) {
  const GitCommand *cmd = getGitCommand(index);
  return cmd ? cmd->isDangerous : false;
}
