---
name: CodeReview
description: Review code for correctness, style, memory safety, and security issues
---
Check for:
- Null dereferences and use-after-free
- Memory leaks and RAII violations
- Off-by-one errors and integer overflow
- Injection risks (command, SQL, format string)
- Thread safety issues

Suggest improvements concisely. Reference file and line numbers.
