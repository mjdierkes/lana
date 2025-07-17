# Context

This folder includes a series of helpful github repositories and documentation for AI Agents to grep, index, and read.
It's intentionally committed and pushed to the repo to make it available for Contained Agents.

## Context Manager

The `context-cli` script helps manage a collection of reference repositories for AI development contexts.

### Quick Start

```bash
# Add a new repository
./context-cli add react https://github.com/facebook/react "React JavaScript library"

# List all repositories  
./context-cli list

# Update all repositories
./context-cli sync

# Check status
./context-cli status
```

### Common Commands

```bash
# Add repositories
./context-cli add nextjs https://github.com/vercel/next.js "Next.js React framework" 
./context-cli add django https://github.com/django/django "Django web framework"

# Remove a repository
./context-cli remove react

# Update specific repository
./context-cli update nextjs

# Manual commit and push
./context-cli commit "Updated documentation repos"

# Clean everything
./context-cli clean
```

### Auto Commit & Push

The script automatically commits and pushes changes when repositories are added, removed, or updated. This ensures the context is always available to contained agents.