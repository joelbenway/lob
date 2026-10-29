# Solver header design: top links and “Ballistics Explorer”

Date: 2026-10-28
Status: approved simplified scope
Scope: browser solver demo header text and links only; no solver behavior changes.

## Intent

Make Documentation and Benchmarks discoverable from the top of the demo,
rather than only from the footer. Keep the current hero styling and change
only the title, top links, and footer links, without changing the demo’s
input/output workflow.

## Goals

- Add top-of-page links for:
  - Documentation: `https://joelbenway.github.io/lob/docs/`
  - Benchmarks: `https://joelbenway.github.io/lob/bench`
- Use header title: “Ballistics Explorer”.
- Keep the repository link in the footer rather than the title.
- Keep the header responsive and accessible on desktop and phone widths.
- Preserve existing hero meaning: configure solver inputs, then view results.

## Non-goals and anti-goals

- No changes to solver inputs, outputs, validation, styling system, or plot behavior.
- No new links, resources, analytics, or page structure beyond the header/footer link move.
- Failure would be a header that competes with the form, wraps badly at 390 px,
  or leaves Documentation/Benchmarks duplicated or unreachable.

## Constraints

- Existing palette, typography, dark mode, and sticky output behavior stay intact.
- Header changes must remain dependency-free static HTML/CSS.
- Browser checks remain 1440 px, 500 px, and 390 px widths in light and dark modes.

## Approaches considered

1. Keep the current theme and only change text/links, chosen:
   - Keep the existing hero proportions, palette, typography, and layout.
   - Add a header nav with Documentation and Benchmarks.
   - Title becomes “Ballistics Explorer”.
   - Tradeoff: no visual redesign, but exactly matches the approved scope.

2. Editorial drop header, rejected:
   - Larger cinematic title and explainer.
   - Tradeoff: conflicts with “keep the current theme.”

3. Solver console header, rejected:
   - Pills plus live model summary in the header.
   - Tradeoff: duplicates status/model output and conflicts with “keep the current theme.”

## Builder decisions

- Update both the document `<title>` and hero `<h1>` to “Ballistics Explorer”.
- Move Documentation and Benchmarks links to a header nav.
- Restore the footer credit links and append Benchmarks:
  `Built on lob · docs · Benchmarks`, with docs pointing at
  `https://joelbenway.github.io/lob/docs/`.
- External links use normal anchors; no JavaScript is required.
- If a link label needs an abbreviation at narrow widths, preserve full accessible names.

## Left to builder

- Exact header/nav CSS, responsive breakpoints, focus styling, and final spacing.
- Verify top links, title link, responsive layout, dark mode, lint/format, and demo build.
