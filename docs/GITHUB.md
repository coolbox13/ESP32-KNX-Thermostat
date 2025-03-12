# GitHub Cheat Sheet

This document provides a quick reference for common Git and GitHub commands and workflows.

---

## **Basic Commands**

| Command | Description |
|---------|-------------|
| `git init` | Initialize a new Git repository |
| `git clone <repository_url>` | Clone a repository to your local machine |
| `git status` | Check the status of your working directory |
| `git add <file_name>` | Add a specific file to the staging area |
| `git add .` | Add all changes to the staging area |
| `git commit -m "Your message"` | Commit changes with a message |
| `git push origin <branch_name>` | Push changes to a remote branch |
| `git pull origin <branch_name>` | Pull the latest changes from a remote branch |

---

## **Branching**

| Command | Description |
|---------|-------------|
| `git branch <branch_name>` | Create a new branch |
| `git checkout <branch_name>` | Switch to a branch |
| `git checkout -b <branch_name>` | Create and switch to a new branch |
| `git branch` | List all branches |
| `git branch -d <branch_name>` | Delete a branch |

---

## **Merging**

| Command | Description |
|---------|-------------|
| `git merge <branch_name>` | Merge a branch into the current branch |
| `git rebase <branch_name>` | Rebase the current branch onto another branch |

---

## **Undoing Changes**

| Command | Description |
|---------|-------------|
| `git reset <file_name>` | Unstage a file |
| `git checkout -- <file_name>` | Revert changes in a file |
| `git commit --amend` | Amend the last commit |
| `git reset --hard <commit_hash>` | Reset to a previous commit |

---

## **Remote Repositories**

| Command | Description |
|---------|-------------|
| `git remote add origin <repository_url>` | Add a remote repository |
| `git remote -v` | View remote repositories |
| `git remote remove origin` | Remove a remote repository |

---

## **GitHub-Specific Workflows**

| Action | Description |
|--------|-------------|
| **Fork a Repository** | Click the "Fork" button on GitHub |
| **Create a Pull Request (PR)** | Push changes to a branch and click "New Pull Request" on GitHub |
| **Review and Merge a PR** | Review changes and click "Merge Pull Request" on GitHub |
| **Sync a Forked Repository** | Add the original repository as `upstream` and merge changes |

---

## **Advanced Commands**

| Command | Description |
|---------|-------------|
| `git log` | View commit history |
| `git stash` | Stash changes temporarily |
| `git stash pop` | Apply stashed changes |
| `git tag <tag_name>` | Tag a commit |
| `git cherry-pick <commit_hash>` | Apply a specific commit to the current branch |

---

## **Common Workflows**

1. **Feature Branch Workflow**
   - Create a new branch for each feature.
   - Push the branch and create a PR when ready.

2. **Git Flow**
   - Use `main` for production-ready code.
   - Use `develop` for ongoing development.
   - Create feature branches from `develop`.

3. **Forking Workflow**
   - Fork the repository.
   - Work on your fork and submit PRs to the original repository.

---

## **Tips**
- Always pull the latest changes before starting work.
- Write clear and descriptive commit messages.
- Use branches to isolate changes and avoid conflicts.

---

For more detailed documentation, visit the [Git Documentation](https://git-scm.com/doc) or the [GitHub Guides](https://guides.github.com/). 