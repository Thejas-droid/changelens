const paths: Record<string, string> = {
  overview: "M3 3h7v7H3z M14 3h7v7h-7z M3 14h7v7H3z M14 14h7v7h-7z",
  graph: "M5 5h4v4H5z M15 15h4v4h-4z M5 15h4v4H5z M7 9v6 M9 7h8v8",
  play: "m9 5 11 7-11 7z",
  runs: "M5 4h14v16H5z M8 8h8 M8 12h8 M8 16h5",
  tests: "M9 3h6 M10 3v7L4 20h16l-6-10V3 M8 14h8",
  pulse: "M2 12h5l3-8 4 16 3-8h5",
  workers: "M3 4h18v6H3z M3 14h18v6H3z M6 7h1 M6 17h1 M16 7h2 M16 17h2",
  book: "M12 5v16 M12 5C9 3 5 3 2 4v15c4-1 7-1 10 2 M12 5c3-2 7-2 10-1v15c-4-1-7-1-10 2",
  settings: "M4 6h16 M4 12h16 M4 18h16 M8 3v6 M16 9v6 M10 15v6",
  info: "M12 11v6 M12 7h.01 M22 12a10 10 0 1 1-20 0 10 10 0 0 1 20 0",
  arrow: "M4 12h16 m-6-6 6 6-6 6",
  check: "m5 12 4 4L19 6",
  copy: "M9 9h12v12H9z M5 15H3V3h12v2",
};
export function Icon({ name }: { name: string }) {
  return (
    <svg
      width="20"
      height="20"
      viewBox="0 0 24 24"
      fill="none"
      stroke="currentColor"
      strokeWidth="1.6"
      strokeLinecap="round"
      strokeLinejoin="round"
      aria-hidden="true"
    >
      <path d={paths[name] ?? paths.info} />
    </svg>
  );
}
