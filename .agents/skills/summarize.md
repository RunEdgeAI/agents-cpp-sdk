---
name: summarize
description: Summarize a long piece of text into a concise summary. Use when the user asks to condense, distill, or summarize an article, transcript, document, or block of text.
---
When the user asks you to summarize text:

1. Read the provided text carefully and identify the key points, main arguments, and any conclusions.
2. Preserve the original meaning — do not introduce facts, opinions, or framing that are not in the source.
3. Default to roughly 100 words. If the user specifies a target length, follow it (clamp to a sensible range, e.g. 10–1000 words).
4. Use clear, readable prose. Prefer a single paragraph for short summaries; use short bullets only if the source is itself a list of distinct items.
5. Omit boilerplate ("This article discusses…"). Lead with the substance.

If the text is too short to meaningfully summarize, say so instead of padding.
