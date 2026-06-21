---
name: "git-automation"
description: "Automates git source control operations: status check, commit, push, and pull. Invoke when user wants to manage version control, save code, or sync changes."
---

# Git Source Control Automation

This skill provides a standardized workflow for managing source code using Git.

## Usage

Use the `RunCommand` tool to execute Git commands.

### Workflow Steps

1.  **Check Status**: Always check the status first to see what has changed.
    ```bash
    git status
    ```

2.  **Add Changes**: Stage the changes you want to commit.
    *   Add all changes:
        ```bash
        git add .
        ```
    *   Add specific file:
        ```bash
        git add path/to/file
        ```

3.  **Commit**: Commit the staged changes with a descriptive message.
    *   Follow **Conventional Commits** format if possible (e.g., `feat: add login`, `fix: resolve crash`, `docs: update readme`).
    ```bash
    git commit -m "type: description of change"
    ```

4.  **Sync (Pull & Push)**: Synchronize with the remote repository.
    *   **Pull** (fetch and merge remote changes):
        ```bash
        git pull
        ```
    *   **Push** (upload local commits):
        ```bash
        git push
        ```

### Common Scenarios

**Quick Save (Add All + Commit)**
```bash
git add . && git commit -m "wip: save current progress"
```

**Full Sync (Pull + Push)**
```bash
git pull && git push
```

**Complete Workflow (Add + Commit + Push)**
```bash
git add . && git commit -m "feat: implement new feature" && git push
```

## Best Practices

*   **Atomic Commits**: Keep commits small and focused on a single task.
*   **Descriptive Messages**: Write clear commit messages explaining *what* and *why*.
*   **Pull Before Push**: Always pull the latest changes before pushing to avoid conflicts.
*   **Check Branch**: Ensure you are on the correct branch (use `git branch` to check).

## Troubleshooting

*   **Merge Conflicts**: If `git pull` results in conflicts, you must resolve them manually in the files, then `git add` the resolved files and `git commit`.
*   **Permission Denied**: Ensure you have the correct SSH keys or credentials configured.
