---
name: create_document
description: Create a well-formatted document and save it to disk. Use when the user asks to write a report, summary, plan, README, or any structured document.
---
When the user asks you to create a document:

1. Confirm the filename and location if not specified (default: current directory, .md extension).
2. Structure the content appropriately for the type:
   - Reports: title, executive summary, sections, conclusion
   - Plans: goals, steps, timeline, success criteria
   - READMEs: project name, description, install, usage, license
3. Use the file_write tool to save the file.
4. Confirm the file was written and state the full path.

Prefer Markdown for all documents unless the user specifies otherwise.
