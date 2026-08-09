# Security for lob

Lob is a math library with no I/O, no networking, and no dynamic memory allocation, so the attack surface is blessedly small. But perhaps not zero. I'd rather know about problems you discover. If you've found something that looks like a vulnerability, thank you for looking. Here's how to tell me.

## What to do

Report security issues with [GitHub's private vulnerability reporting](https://github.com/joelbenway/lob/security/advisories/new) rather than a public issue. If private reporting isn't available for the repository, [raise an issue](https://github.com/joelbenway/lob/issues) to request a private channel.

Please do not disclose the vulnerability publicly until I've had a chance to triage it. Whatever channel you use, be generous with detail: version and commit, platform and compiler, and a minimal reproduction. A good report is worth a lot more than a good fix.

## What happens next

Lob is a solo-authored project, but rest assured, our entire security department will work tirelessly to acknowledge a report within a few days. I'll post updates as the investigation proceeds. If I'm too quiet for your tastes, feel free to poke me.

Once triaged, one of these will happen:

* **Accepted:** I'll work to fix it, release the fix, and thank you in the release notes (or privately, if you'd rather stay anonymous).
* **Declined:** I'll explain why I don't think it's a vulnerability and we'll go from there.