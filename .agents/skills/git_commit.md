---
name: GitCommit
description: Stage and commit changes to git with a clear, imperative commit message
requires.bins: [git]
---
1. Run `git status` to see what changed.
2. Run `git diff` to review the changes.
3. Run `git add -A` to stage everything.
4. Run `git commit -m '<message>'` with a concise, imperative message (50 chars or less).
