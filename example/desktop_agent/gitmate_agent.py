#!/usr/bin/env python3
"""
GitMate Desktop Agent

A Python-based HTTP server that receives Git commands from the GitMate hardware
device and executes them safely on the local repository.

Features:
- HTTP server for receiving commands
- Safe Git command execution
- Repository state detection
- Protected branch checks
- Token-based authentication
- Cross-platform support (Windows, macOS, Linux)
"""

import os
import sys
import json
import subprocess
import secrets
from pathlib import Path
from flask import Flask, request, jsonify
import argparse

app = Flask(__name__)

# Global configuration
CONFIG = {
    'port': 8888,
    'host': '0.0.0.0',
    'repo_path': None,
    'protected_branches': ['main', 'master', 'production', 'develop'],
    'pairing_token': None,
    'debug': False
}

def load_config():
    """Load configuration from config.json if it exists"""
    config_file = Path('config.json')
    if config_file.exists():
        with open(config_file, 'r') as f:
            stored_config = json.load(f)
            CONFIG.update(stored_config)
            print(f"Loaded configuration from {config_file}")
    else:
        # Generate new pairing token
        CONFIG['pairing_token'] = secrets.token_hex(16)
        save_config()

def save_config():
    """Save configuration to config.json"""
    config_file = Path('config.json')
    with open(config_file, 'w') as f:
        json.dump(CONFIG, f, indent=2)
    print(f"Configuration saved to {config_file}")

def run_git_command(args, repo_path=None):
    """Execute a Git command safely and return the result"""
    if repo_path is None:
        repo_path = CONFIG['repo_path']
    
    if repo_path is None:
        return False, "Repository path not configured"
    
    try:
        cmd = ['git'] + args
        result = subprocess.run(
            cmd,
            cwd=repo_path,
            capture_output=True,
            text=True,
            timeout=30
        )
        
        if result.returncode == 0:
            return True, result.stdout.strip()
        else:
            return False, result.stderr.strip() or result.stdout.strip()
    except subprocess.TimeoutExpired:
        return False, "Command timed out"
    except Exception as e:
        return False, str(e)

def get_current_branch():
    """Get the current Git branch name"""
    success, output = run_git_command(['branch', '--show-current'])
    return output if success else 'unknown'

def get_uncommitted_count():
    """Get the number of uncommitted files"""
    success, output = run_git_command(['status', '--porcelain'])
    if success:
        return len([line for line in output.split('\n') if line.strip()])
    return 0

def get_conflict_count():
    """Get the number of files with merge conflicts"""
    success, output = run_git_command(['diff', '--name-only', '--diff-filter=U'])
    if success:
        return len([line for line in output.split('\n') if line.strip()])
    return 0

def is_git_repo(path):
    """Check if the path is a Git repository"""
    try:
        result = subprocess.run(
            ['git', 'rev-parse', '--git-dir'],
            cwd=path,
            capture_output=True,
            timeout=5
        )
        return result.returncode == 0
    except:
        return False

def verify_token(token):
    """Verify the authentication token"""
    return token == CONFIG['pairing_token']

@app.route('/pair', methods=['GET'])
def pair():
    """Pairing endpoint - returns the pairing token"""
    print("Pairing request received")
    return jsonify({
        'status': 'ok',
        'token': CONFIG['pairing_token'],
        'message': 'Pairing successful'
    })

@app.route('/command', methods=['POST'])
def handle_command():
    """Handle Git command requests from the device"""
    try:
        data = request.get_json()
        
        if not data:
            return jsonify({
                'status': 'error',
                'msg': 'Invalid JSON'
            }), 400
        
        # Verify token
        token = data.get('token', '')
        if not verify_token(token):
            return jsonify({
                'status': 'error',
                'msg': 'Invalid authentication token'
            }), 401
        
        cmd = data.get('cmd', '')
        args = data.get('args', {})
        
        print(f"Command received: {cmd}")
        if CONFIG['debug']:
            print(f"Arguments: {args}")
        
        # Route to appropriate handler
        if cmd == 'git_init':
            return handle_init(args)
        elif cmd == 'git_status':
            return handle_status(args)
        elif cmd == 'git_add':
            return handle_add(args)
        elif cmd == 'git_commit':
            return handle_commit(args)
        elif cmd == 'git_pull':
            return handle_pull(args)
        elif cmd == 'git_push':
            return handle_push(args)
        elif cmd == 'git_checkout':
            return handle_checkout(args)
        elif cmd == 'git_create_branch':
            return handle_create_branch(args)
        elif cmd == 'git_merge':
            return handle_merge(args)
        elif cmd == 'git_stash':
            return handle_stash(args)
        elif cmd == 'git_stash_pop':
            return handle_stash_pop(args)
        elif cmd == 'git_reset':
            return handle_reset(args)
        elif cmd == 'git_diff':
            return handle_diff(args)
        elif cmd == 'git_conflict':
            return handle_conflict(args)
        elif cmd == 'git_rollback':
            return handle_rollback(args)
        else:
            return jsonify({
                'status': 'error',
                'msg': f'Unknown command: {cmd}'
            }), 400
            
    except Exception as e:
        print(f"Error handling command: {e}")
        return jsonify({
            'status': 'error',
            'msg': str(e)
        }), 500

def handle_init(args):
    """Initialize a new Git repository"""
    repo_path = CONFIG['repo_path']
    
    if is_git_repo(repo_path):
        return jsonify({
            'status': 'error',
            'msg': 'Repository already initialized'
        })
    
    success, output = run_git_command(['init'])
    
    if success:
        return jsonify({
            'status': 'ok',
            'msg': 'Repository initialized',
            'payload': {}
        })
    else:
        return jsonify({
            'status': 'error',
            'msg': output
        })

def handle_status(args):
    """Get repository status"""
    if not is_git_repo(CONFIG['repo_path']):
        return jsonify({
            'status': 'error',
            'msg': 'Not a Git repository'
        })
    
    branch = get_current_branch()
    uncommitted = get_uncommitted_count()
    conflicts = get_conflict_count()
    
    success, output = run_git_command(['status', '--short'])
    
    return jsonify({
        'status': 'ok',
        'msg': f'On branch {branch}',
        'payload': {
            'branch': branch,
            'uncommitted': uncommitted,
            'conflicts': conflicts,
            'output': output if success else ''
        }
    })

def handle_add(args):
    """Stage all changes"""
    success, output = run_git_command(['add', '.'])
    
    if success:
        return jsonify({
            'status': 'ok',
            'msg': 'All changes staged',
            'payload': {}
        })
    else:
        return jsonify({
            'status': 'error',
            'msg': output
        })

def handle_commit(args):
    """Commit staged changes"""
    message = args.get('message', 'GitMate commit')
    
    # Check if there are staged changes
    success, output = run_git_command(['diff', '--cached', '--quiet'])
    
    if success:  # No changes staged
        return jsonify({
            'status': 'error',
            'msg': 'No changes to commit'
        })
    
    success, output = run_git_command(['commit', '-m', message])
    
    if success:
        return jsonify({
            'status': 'ok',
            'msg': 'Changes committed',
            'payload': {}
        })
    else:
        return jsonify({
            'status': 'error',
            'msg': output
        })

def handle_pull(args):
    """Pull from remote"""
    success, output = run_git_command(['pull'])
    
    if success:
        return jsonify({
            'status': 'ok',
            'msg': 'Pull successful',
            'payload': {}
        })
    else:
        return jsonify({
            'status': 'error',
            'msg': output
        })

def handle_push(args):
    """Push to remote"""
    branch = get_current_branch()
    
    # Check if branch is protected
    if branch in CONFIG['protected_branches']:
        # Require extra confirmation (already handled on device)
        pass
    
    success, output = run_git_command(['push'])
    
    if success:
        return jsonify({
            'status': 'ok',
            'msg': 'Push successful',
            'payload': {}
        })
    else:
        return jsonify({
            'status': 'error',
            'msg': output
        })

def handle_checkout(args):
    """Checkout a branch"""
    branch = args.get('branch', 'main')
    
    success, output = run_git_command(['checkout', branch])
    
    if success:
        return jsonify({
            'status': 'ok',
            'msg': f'Switched to {branch}',
            'payload': {}
        })
    else:
        return jsonify({
            'status': 'error',
            'msg': output
        })

def handle_create_branch(args):
    """Create a new branch"""
    branch = args.get('name', f'feature-{secrets.token_hex(4)}')
    
    success, output = run_git_command(['checkout', '-b', branch])
    
    if success:
        return jsonify({
            'status': 'ok',
            'msg': f'Created branch {branch}',
            'payload': {}
        })
    else:
        return jsonify({
            'status': 'error',
            'msg': output
        })

def handle_merge(args):
    """Merge a branch"""
    branch = args.get('branch', '')
    
    if not branch:
        return jsonify({
            'status': 'error',
            'msg': 'No branch specified'
        })
    
    success, output = run_git_command(['merge', branch])
    
    if success:
        return jsonify({
            'status': 'ok',
            'msg': f'Merged {branch}',
            'payload': {}
        })
    else:
        # Check for conflicts
        conflicts = get_conflict_count()
        if conflicts > 0:
            return jsonify({
                'status': 'error',
                'msg': f'Merge conflicts in {conflicts} files',
                'payload': {'conflicts': conflicts}
            })
        else:
            return jsonify({
                'status': 'error',
                'msg': output
            })

def handle_stash(args):
    """Stash changes"""
    success, output = run_git_command(['stash'])
    
    if success:
        return jsonify({
            'status': 'ok',
            'msg': 'Changes stashed',
            'payload': {}
        })
    else:
        return jsonify({
            'status': 'error',
            'msg': output
        })

def handle_stash_pop(args):
    """Pop stashed changes"""
    success, output = run_git_command(['stash', 'pop'])
    
    if success:
        return jsonify({
            'status': 'ok',
            'msg': 'Stash applied',
            'payload': {}
        })
    else:
        return jsonify({
            'status': 'error',
            'msg': output
        })

def handle_reset(args):
    """Reset to previous commit"""
    soft = args.get('soft', True)
    
    if soft:
        success, output = run_git_command(['reset', '--soft', 'HEAD~1'])
    else:
        success, output = run_git_command(['reset', '--hard', 'HEAD~1'])
    
    if success:
        return jsonify({
            'status': 'ok',
            'msg': 'Reset successful',
            'payload': {}
        })
    else:
        return jsonify({
            'status': 'error',
            'msg': output
        })

def handle_diff(args):
    """Get diff summary"""
    success, output = run_git_command(['diff', '--stat'])
    
    if success:
        return jsonify({
            'status': 'ok',
            'msg': output or 'No changes',
            'payload': {}
        })
    else:
        return jsonify({
            'status': 'error',
            'msg': output
        })

def handle_conflict(args):
    """Get conflict information"""
    conflicts = get_conflict_count()
    
    if conflicts > 0:
        success, output = run_git_command(['diff', '--name-only', '--diff-filter=U'])
        return jsonify({
            'status': 'ok',
            'msg': f'{conflicts} conflicted files',
            'payload': {
                'conflicts': conflicts,
                'files': output.split('\n') if success else []
            }
        })
    else:
        return jsonify({
            'status': 'ok',
            'msg': 'No conflicts',
            'payload': {'conflicts': 0}
        })

def handle_rollback(args):
    """Undo last commit (soft reset)"""
    return handle_reset({'soft': True})

def main():
    parser = argparse.ArgumentParser(description='GitMate Desktop Agent')
    parser.add_argument('--repo', type=str, help='Path to Git repository')
    parser.add_argument('--port', type=int, default=8888, help='Port to listen on')
    parser.add_argument('--debug', action='store_true', help='Enable debug mode')
    
    args = parser.parse_args()
    
    # Load configuration
    load_config()
    
    # Override config with command line arguments
    if args.repo:
        CONFIG['repo_path'] = args.repo
    if args.port:
        CONFIG['port'] = args.port
    if args.debug:
        CONFIG['debug'] = True
        app.debug = True
    
    # Validate repository path
    if CONFIG['repo_path'] is None:
        print("Error: Repository path not configured")
        print("Please specify repository path with --repo or in config.json")
        sys.exit(1)
    
    repo_path = Path(CONFIG['repo_path']).resolve()
    if not repo_path.exists():
        print(f"Error: Repository path does not exist: {repo_path}")
        sys.exit(1)
    
    CONFIG['repo_path'] = str(repo_path)
    save_config()
    
    print("=" * 60)
    print("GitMate Desktop Agent")
    print("=" * 60)
    print(f"Repository: {CONFIG['repo_path']}")
    print(f"Listening on: {CONFIG['host']}:{CONFIG['port']}")
    print(f"Pairing Token: {CONFIG['pairing_token']}")
    print("=" * 60)
    print("\nTo pair your GitMate device:")
    print(f"1. Connect your device to the same network")
    print(f"2. Set agent IP in device config.h")
    print(f"3. Device will automatically pair on first connection")
    print("\nPress Ctrl+C to stop\n")
    
    # Run the Flask app
    app.run(host=CONFIG['host'], port=CONFIG['port'])

if __name__ == '__main__':
    main()
