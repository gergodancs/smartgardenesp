export function Wordmark() {
  return (
    <span className="wordmark">
      <span className="wordmark-base">base</span>
      <span className="wordmark-digital">digital</span>
      <svg className="mark" viewBox="0 0 32 32" aria-hidden="true">
        <circle cx="16" cy="16" r="13.25" fill="none" stroke="currentColor" strokeWidth="1.5" />
        <circle cx="16" cy="16" r="4" fill="currentColor" />
      </svg>
    </span>
  );
}
