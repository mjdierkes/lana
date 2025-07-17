#!/usr/bin/env python3
"""
Context Repository Manager - Like npm for context repositories

Usage:
    python context-manager.py <command> [args]

Commands:
    add <name> <url> [description]  - Add a new repository
    remove <name>                   - Remove a repository
    update [name]                   - Update specific repo or all repos
    sync                           - Sync all repos (fetch new, update existing)
    list                           - List all repositories
    clean                          - Remove all repos and clean context directory
    status                         - Show status of all repositories
    commit [message]               - Manually commit and push changes
"""

import json
import os
import sys
import subprocess
import shutil
import concurrent.futures
import argparse
from datetime import datetime
from pathlib import Path
from urllib.parse import urlparse
import tempfile

class Colors:
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    BLUE = '\033[94m'
    BOLD = '\033[1m'
    END = '\033[0m'

class ContextManager:
    def __init__(self, config_file="context-repos.json"):
        self.config_file = config_file
        self.config = self.load_config()
        self.context_dir = Path(self.config.get('config', {}).get('context_dir', '.'))
        
    def load_config(self):
        """Load configuration from JSON file"""
        if not os.path.exists(self.config_file):
            return {"repositories": {}, "config": {"context_dir": "."}}
        
        try:
            with open(self.config_file, 'r') as f:
                return json.load(f)
        except Exception as e:
            self.error(f"Failed to load config: {e}")
            return {"repositories": {}, "config": {"context_dir": "context"}}
    
    def save_config(self):
        """Save configuration to JSON file"""
        try:
            with open(self.config_file, 'w') as f:
                json.dump(self.config, f, indent=2)
        except Exception as e:
            self.error(f"Failed to save config: {e}")
    
    def info(self, message):
        print(f"{Colors.BLUE}ℹ{Colors.END} {message}")
    
    def success(self, message):
        print(f"{Colors.GREEN}✓{Colors.END} {message}")
    
    def warning(self, message):
        print(f"{Colors.YELLOW}⚠{Colors.END} {message}")
    
    def error(self, message):
        print(f"{Colors.RED}✗{Colors.END} {message}")
        
    def get_default_branch(self, repo_url):
        """Auto-detect the default branch of a repository"""
        try:
            result = subprocess.run([
                'git', 'ls-remote', '--symref', repo_url, 'HEAD'
            ], capture_output=True, text=True, timeout=30)
            
            if result.returncode == 0:
                lines = result.stdout.strip().split('\n')
                for line in lines:
                    if line.startswith('ref: refs/heads/'):
                        return line.split('/')[-1]
            
            # Fallback: try common branch names
            for branch in ['main', 'master', 'develop', 'dev']:
                result = subprocess.run([
                    'git', 'ls-remote', '--heads', repo_url, branch
                ], capture_output=True, text=True, timeout=10)
                if result.returncode == 0 and result.stdout.strip():
                    return branch
                    
        except Exception as e:
            self.warning(f"Could not auto-detect branch for {repo_url}: {e}")
        
        return 'main'  # Default fallback
    
    def git_commit_and_push(self, message):
        """Commit and push changes to git repository"""
        auto_commit = self.config.get('config', {}).get('auto_commit', False)
        auto_push = self.config.get('config', {}).get('auto_push', False)
        
        if not auto_commit:
            return True
            
        try:
            # Check if we're in a git repository
            result = subprocess.run(['git', 'rev-parse', '--git-dir'], 
                                  capture_output=True, text=True, cwd=self.context_dir.parent)
            if result.returncode != 0:
                self.warning("Not in a git repository, skipping commit")
                return True
            
            # Stage all changes in context directory
            subprocess.run(['git', 'add', str(self.context_dir)], 
                          cwd=self.context_dir.parent, check=True)
            
            # Check if there are changes to commit
            result = subprocess.run(['git', 'diff', '--cached', '--quiet'], 
                                  capture_output=True, cwd=self.context_dir.parent)
            if result.returncode == 0:
                self.info("No changes to commit")
                return True
            
            # Commit changes
            subprocess.run(['git', 'commit', '-m', message], 
                          cwd=self.context_dir.parent, check=True)
            self.success(f"Committed changes: {message}")
            
            # Push if auto_push is enabled
            if auto_push:
                subprocess.run(['git', 'push'], 
                              cwd=self.context_dir.parent, check=True)
                self.success("Pushed changes to remote")
            
            return True
            
        except subprocess.CalledProcessError as e:
            self.error(f"Git operation failed: {e}")
            return False
        except Exception as e:
            self.error(f"Git operation error: {e}")
            return False
    
    def clone_repository(self, name, url, target_dir):
        """Clone a repository and remove .git directory"""
        try:
            # Get default branch
            branch = self.get_default_branch(url)
            self.info(f"Detected default branch: {branch}")
            
            # Clone with specific branch
            result = subprocess.run([
                'git', 'clone', '--branch', branch, '--single-branch', url, str(target_dir)
            ], capture_output=True, text=True, timeout=300)
            
            if result.returncode != 0:
                # Try without specific branch as fallback
                self.warning(f"Failed to clone branch {branch}, trying default...")
                if target_dir.exists():
                    shutil.rmtree(target_dir)
                    
                result = subprocess.run([
                    'git', 'clone', url, str(target_dir)
                ], capture_output=True, text=True, timeout=300)
                
                if result.returncode != 0:
                    raise Exception(f"Git clone failed: {result.stderr}")
            
            # Remove .git directory
            git_dir = target_dir / '.git'
            if git_dir.exists():
                shutil.rmtree(git_dir)
                
            return True
            
        except Exception as e:
            self.error(f"Failed to clone {name}: {e}")
            if target_dir.exists():
                shutil.rmtree(target_dir)
            return False
    
    def add_repository(self, name, url, description=""):
        """Add a new repository to the configuration"""
        if name in self.config['repositories']:
            self.error(f"Repository '{name}' already exists")
            return False
        
        # Validate URL
        try:
            parsed = urlparse(url)
            if not parsed.scheme or not parsed.netloc:
                raise ValueError("Invalid URL")
        except ValueError:
            self.error(f"Invalid repository URL: {url}")
            return False
        
        self.config['repositories'][name] = {
            "url": url,
            "description": description,
            "branch": "auto",
            "last_updated": None
        }
        
        self.save_config()
        self.success(f"Added repository '{name}'")
        
        # Ask if user wants to fetch immediately
        response = input(f"Fetch '{name}' now? (y/N): ").strip().lower()
        if response in ['y', 'yes']:
            return self.fetch_repository(name)
        
        return True
    
    def remove_repository(self, name):
        """Remove a repository"""
        if name not in self.config['repositories']:
            self.error(f"Repository '{name}' not found")
            return False
        
        # Remove from filesystem
        repo_dir = self.context_dir / name
        if repo_dir.exists():
            shutil.rmtree(repo_dir)
            self.success(f"Removed directory: {repo_dir}")
        
        # Remove from config
        del self.config['repositories'][name]
        self.save_config()
        self.success(f"Removed repository '{name}' from configuration")
        
        # Auto commit and push if enabled
        self.git_commit_and_push(f"Remove repository: {name}")
        
        return True
    
    def fetch_repository(self, name):
        """Fetch a single repository"""
        if name not in self.config['repositories']:
            self.error(f"Repository '{name}' not found in configuration")
            return False
        
        repo_config = self.config['repositories'][name]
        target_dir = self.context_dir / name
        
        self.info(f"Fetching '{name}' from {repo_config['url']}")
        
        # Create context directory if it doesn't exist
        self.context_dir.mkdir(exist_ok=True)
        
        # Remove existing directory if it exists
        if target_dir.exists():
            self.warning(f"Removing existing directory: {target_dir}")
            shutil.rmtree(target_dir)
        
        # Clone repository
        if self.clone_repository(name, repo_config['url'], target_dir):
            # Update last_updated timestamp
            self.config['repositories'][name]['last_updated'] = datetime.now().isoformat()
            self.save_config()
            self.success(f"Successfully fetched '{name}'")
            
            # Auto commit and push if enabled
            self.git_commit_and_push(f"Add/update repository: {name}")
            
            return True
        
        return False
    
    def update_repositories(self, names=None):
        """Update specific repositories or all repositories"""
        if names is None:
            names = list(self.config['repositories'].keys())
        elif isinstance(names, str):
            names = [names]
        
        if not names:
            self.warning("No repositories to update")
            return True
        
        failed = []
        
        # Check if we should use parallel processing
        use_parallel = self.config.get('config', {}).get('parallel_fetch', True)
        max_workers = self.config.get('config', {}).get('max_parallel', 3)
        
        if use_parallel and len(names) > 1:
            self.info(f"Updating {len(names)} repositories in parallel (max {max_workers} workers)")
            with concurrent.futures.ThreadPoolExecutor(max_workers=max_workers) as executor:
                future_to_name = {executor.submit(self.fetch_repository, name): name for name in names}
                for future in concurrent.futures.as_completed(future_to_name):
                    name = future_to_name[future]
                    try:
                        if not future.result():
                            failed.append(name)
                    except Exception as e:
                        self.error(f"Exception updating {name}: {e}")
                        failed.append(name)
        else:
            for name in names:
                if not self.fetch_repository(name):
                    failed.append(name)
        
        if failed:
            self.error(f"Failed to update: {', '.join(failed)}")
            return False
        
        self.success(f"Successfully updated {len(names)} repositories")
        return True
    
    def list_repositories(self):
        """List all repositories with their status"""
        repos = self.config['repositories']
        
        if not repos:
            self.warning("No repositories configured")
            return
        
        print(f"\n{Colors.BOLD}Configured Repositories:{Colors.END}")
        print("-" * 80)
        
        for name, config in repos.items():
            status = "✓" if (self.context_dir / name).exists() else "✗"
            last_updated = config.get('last_updated')
            if last_updated:
                try:
                    dt = datetime.fromisoformat(last_updated)
                    last_updated = dt.strftime("%Y-%m-%d %H:%M")
                except:
                    last_updated = "Unknown"
            else:
                last_updated = "Never"
            
            print(f"{status} {Colors.BOLD}{name}{Colors.END}")
            print(f"   URL: {config['url']}")
            print(f"   Description: {config.get('description', 'No description')}")
            print(f"   Last Updated: {last_updated}")
            print()
    
    def sync_repositories(self):
        """Sync all repositories (update existing, fetch missing)"""
        self.info("Syncing all repositories...")
        return self.update_repositories()
    
    def clean_context(self):
        """Remove all repositories and clean context directory"""
        if not self.context_dir.exists():
            self.info("Context directory doesn't exist")
            return True
        
        # Confirm with user
        response = input(f"This will remove ALL repositories from {self.context_dir}. Continue? (y/N): ").strip().lower()
        if response not in ['y', 'yes']:
            self.info("Cancelled")
            return False
        
        try:
            shutil.rmtree(self.context_dir)
            self.success(f"Cleaned context directory: {self.context_dir}")
            
            # Reset last_updated timestamps
            for name in self.config['repositories']:
                self.config['repositories'][name]['last_updated'] = None
            self.save_config()
            
            return True
        except Exception as e:
            self.error(f"Failed to clean context directory: {e}")
            return False
    
    def show_status(self):
        """Show status of repositories and context directory"""
        print(f"\n{Colors.BOLD}Context Manager Status{Colors.END}")
        print("-" * 40)
        print(f"Config file: {self.config_file}")
        print(f"Context directory: {self.context_dir}")
        print(f"Total repositories: {len(self.config['repositories'])}")
        
        existing = sum(1 for name in self.config['repositories'] if (self.context_dir / name).exists())
        print(f"Fetched repositories: {existing}")
        
        if self.context_dir.exists():
            total_size = sum(f.stat().st_size for f in self.context_dir.rglob('*') if f.is_file())
            print(f"Total size: {total_size / (1024*1024):.1f} MB")
        
        self.list_repositories()
    
    def manual_commit(self, message=None):
        """Manually commit and push changes"""
        if message is None:
            message = f"Manual update - {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}"
        
        return self.git_commit_and_push(message)

def main():
    parser = argparse.ArgumentParser(description="Context Repository Manager")
    parser.add_argument('command', help='Command to execute')
    parser.add_argument('args', nargs='*', help='Command arguments')
    
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    
    args = parser.parse_args()
    manager = ContextManager()
    
    try:
        if args.command == 'add':
            if len(args.args) < 2:
                print("Usage: add <name> <url> [description]")
                sys.exit(1)
            name, url = args.args[0], args.args[1]
            description = args.args[2] if len(args.args) > 2 else ""
            success = manager.add_repository(name, url, description)
            
        elif args.command == 'remove':
            if len(args.args) < 1:
                print("Usage: remove <name>")
                sys.exit(1)
            success = manager.remove_repository(args.args[0])
            
        elif args.command == 'update':
            names = args.args if args.args else None
            success = manager.update_repositories(names)
            
        elif args.command == 'sync':
            success = manager.sync_repositories()
            
        elif args.command == 'list':
            manager.list_repositories()
            success = True
            
        elif args.command == 'clean':
            success = manager.clean_context()
            
        elif args.command == 'status':
            manager.show_status()
            success = True
            
        elif args.command == 'commit':
            message = ' '.join(args.args) if args.args else None
            success = manager.manual_commit(message)
            
        else:
            print(f"Unknown command: {args.command}")
            print(__doc__)
            sys.exit(1)
        
        sys.exit(0 if success else 1)
        
    except KeyboardInterrupt:
        print(f"\n{Colors.YELLOW}Interrupted by user{Colors.END}")
        sys.exit(130)
    except Exception as e:
        print(f"{Colors.RED}Unexpected error: {e}{Colors.END}")
        sys.exit(1)

if __name__ == "__main__":
    main() 