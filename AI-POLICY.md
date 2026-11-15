# KelpOS AI Policy

KelpOS is a FOSS project. The use of AI tools (Copilot, chat LLMs,
code generators) is allowed, but bound to clear rules:

## Rules

1. **Transparency**: AI-generated contributions must be clearly marked
   in the PR (e.g. label `ai-assisted` + note in the commit body).
2. **Human responsibility**: The submitting human carries full
   responsibility for the correctness, style, and safety of the code —
   "the AI wrote it" is no excuse.
3. **Comprehension requirement**: Submitted AI code must be explainable
   by the author (in their own words). Reviewers may follow up.
4. **No AI bloat**: No unprompted generated files (doc fluff,
   hallucinations, non-compiling code). Less is more.
5. **License hygiene**: AI tools must not bring in foreign-licensed
   snippets. If training-data copy-paste is suspected: reject.
6. **Security review**: AI code that touches privileged kernel code
   (memory, interrupts, syscalls) is reviewed with extra care.

## For maintainers

- PRs without any response to comprehension questions can be rejected.
- Repeated rule violations: block the contributor.

Signed, the maintainers.
