# GitMate Desktop Agent

A Python-based HTTP server that receives Git commands from the GitMate hardware device and executes them safely on your local repository.

## Features

- **HTTP Server**: Receives commands via HTTP POST requests
- **Safe Git Execution**: Executes Git commands with proper error handling
- **Protected Branches**: Prevents accidental pushes to main/master/production branches
- **Token Authentication**: Secure pairing with hardware device
- **Cross-Platform**: Works on Windows, macOS, and Linux
- **Repository Status**: Provides real-time repo status to the device

## Requirements

- Python 3.8 or higher
- Git installed and accessible from command line
- Flask web framework

## Installation

### 1. Install Python Dependencies

```bash
cd desktop_agent
pip install -r requirements.txt
```

Or install individually:

```bash
pip install Flask==3.0.0 GitPython==3.1.40
```

### 2. Configure Repository Path

On first run, the agent will create a `config.json` file. You can either:

**Option A**: Specify repository path via command line:
```bash
python gitmate_agent.py --repo /path/to/your/git/repository
```

**Option B**: Edit `config.json` directly:
```json
{
  "port": 8888,
  "host": "0.0.0.0",
  "repo_path": "C:\\path\\to\\your\\repo",
  "protected_branches": ["main", "master", "production", "develop"],
  "pairing_token": "auto-generated",
  "debug": false
}
```

## Usage

### Start the Agent

```bash
python gitmate_agent.py --repo /path/to/your/repo
```

Options:
- `--repo PATH`: Path to Git repository (required on first run)
- `--port PORT`: Port to listen on (default: 8888)
- `--debug`: Enable debug mode

### Pairing with GitMate Device

1. **Start the agent** - it will display a pairing token
2. **Find your computer's IP address**:
   - Windows: `ipconfig`
   - macOS/Linux: `ifconfig` or `ip addr`
3. **Update GitMate device config**:
   - Edit `config.h` in the GitMate firmware
   - Set `AGENT_IP` to your computer's IP address
   - Set `WIFI_SSID` and `WIFI_PASSWORD`
4. **Upload firmware to device**
5. **Device will automatically pair** on first connection

The pairing token is stored in `config.json` and persisted across restarts.

## Configuration

### Protected Branches

By default, the following branches are protected (require confirmation on device):
- `main`
- `master`
- `production`
- `develop`

You can modify this list in `config.json`:

```json
{
  "protected_branches": ["main", "master", "custom-branch"]
}
```

### Port Configuration

If port 8888 is already in use, change it in `config.json` or use the `--port` flag:

```bash
python gitmate_agent.py --port 9000
```

**Note**: Also update `AGENT_PORT` in the device's `config.h` file.

## Supported Git Commands

The agent supports the following Git operations:

- **git_init**: Initialize repository
- **git_status**: Get repository status
- **git_add**: Stage all changes
- **git_commit**: Commit staged changes
- **git_pull**: Pull from remote
- **git_push**: Push to remote (with protection)
- **git_checkout**: Switch branches
- **git_create_branch**: Create new branch
- **git_merge**: Merge branches
- **git_stash**: Stash changes
- **git_stash_pop**: Apply stashed changes
- **git_reset**: Reset to previous commit
- **git_diff**: Get diff summary
- **git_conflict**: Check for conflicts
- **git_rollback**: Undo last commit (soft reset)

## API Endpoints

### GET /pair
Pairing endpoint - returns authentication token.

**Response:**
```json
{
  "status": "ok",
  "token": "generated-token",
  "message": "Pairing successful"
}
```

### POST /command
Execute Git command.

**Request:**
```json
{
  "cmd": "git_status",
  "args": {},
  "token": "your-token"
}
```

**Response:**
```json
{
  "status": "ok",
  "msg": "On branch main",
  "payload": {
    "branch": "main",
    "uncommitted": 2,
    "conflicts": 0
  }
}
```

## Troubleshooting

### Agent won't start
- **Check Python version**: `python --version` (need 3.8+)
- **Install dependencies**: `pip install -r requirements.txt`
- **Check repository path**: Ensure path exists and is a Git repo

### Device can't connect
- **Check IP address**: Ensure `AGENT_IP` in device config matches computer IP
- **Check firewall**: Allow incoming connections on port 8888
- **Check network**: Device and computer must be on same network
- **Check port**: Ensure port 8888 is not blocked

### Commands fail
- **Check Git installation**: `git --version`
- **Check repository**: Ensure repo path is correct in `config.json`
- **Check permissions**: Ensure you have write access to repository

### Authentication errors
- **Re-pair device**: Delete `config.json` on both device and agent, restart both
- **Check token**: Ensure token in device preferences matches agent config

## Running as Background Service

### Windows
Create a batch file `start_gitmate_agent.bat`:
```batch
@echo off
cd /d C:\path\to\desktop_agent
python gitmate_agent.py --repo C:\path\to\your\repo
```

Use Task Scheduler to run on startup.

### macOS/Linux
Create a systemd service file `/etc/systemd/system/gitmate-agent.service`:
```ini
[Unit]
Description=GitMate Desktop Agent
After=network.target

[Service]
Type=simple
User=your-username
WorkingDirectory=/path/to/desktop_agent
ExecStart=/usr/bin/python3 /path/to/desktop_agent/gitmate_agent.py --repo /path/to/repo
Restart=on-failure

[Install]
WantedBy=multi-user.target
```

Enable and start:
```bash
sudo systemctl enable gitmate-agent
sudo systemctl start gitmate-agent
```

## Security Considerations

- **Token storage**: Pairing token is stored in plain text in `config.json`
- **Network security**: Agent accepts connections from any device with valid token
- **Local execution**: Agent runs Git commands with your user permissions
- **Protected branches**: Safety check, but not foolproof - review before confirming

## Development

Enable debug mode to see detailed logs:

```bash
python gitmate_agent.py --repo /path/to/repo --debug
```

This will show:
- Incoming command details
- Git command execution
- Request/response payloads
- Error stack traces

## License

This project is part of the GitMate Hardware Git Controller system.

## Support

For issues, questions, or contributions, see the main GitMate project README.
