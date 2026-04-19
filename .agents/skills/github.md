---
name: github
description: Read GitHub repositories, files, READMEs, and directory listings. Use for any github.com URL.
requires.bins: [curl]
---
When the user provides a github.com URL or asks about a GitHub repository:

Never fetch github.com HTML directly — it is a JavaScript SPA and returns no useful content.

Instead, use fetch_webpage with these URL transformations:

- Repo overview: `https://api.github.com/repos/{owner}/{repo}`
- README (base64-encoded): `https://api.github.com/repos/{owner}/{repo}/readme`
- Raw file content: `https://raw.githubusercontent.com/{owner}/{repo}/main/{path}`
- Directory listing: `https://api.github.com/repos/{owner}/{repo}/contents/{path}`
- Branches: `https://api.github.com/repos/{owner}/{repo}/branches`

No auth header is needed for public repos. For the README, the API returns JSON with a
base64-encoded `content` field — use the shell tool to decode it:
  echo '<base64_content>' | base64 --decode
