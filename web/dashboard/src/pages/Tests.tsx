import { useState } from "react";
import { useWorkspace } from "../workspace";
import { PageHeading } from "../components/Shared";
export default function Tests() {
  const { data } = useWorkspace();
  const [query, setQuery] = useState("");
  const [framework, setFramework] = useState("all");
  if (!data) return null;
  const rows = data.tests.filter(
    (t) =>
      `${t.name} ${t.file}`.toLowerCase().includes(query.toLowerCase()) &&
      (framework === "all" || t.framework === framework)
  );
  return (
    <>
      <PageHeading
        eyebrow="TEST INVENTORY"
        title="Discovered tests"
        description="Tests detected from repository source files and configured discovery patterns."
      />
      <div className="filter-bar">
        <label>
          Search tests
          <input
            type="search"
            placeholder="Test name or file…"
            value={query}
            onChange={(e) => setQuery(e.target.value)}
          />
        </label>
        <label>
          Framework
          <select
            value={framework}
            onChange={(e) => setFramework(e.target.value)}
          >
            <option value="all">All frameworks</option>
            {[...new Set(data.tests.map((t) => t.framework))]
              .sort()
              .map((f) => (
                <option key={f} value={f}>
                  {f}
                </option>
              ))}
          </select>
        </label>
        <span className="muted small" role="status">
          {rows.length} of {data.tests.length} tests
        </span>
      </div>
      <section className="panel table-panel">
        <div className="table-scroll">
          <table>
            <thead>
              <tr>
                <th>Test</th>
                <th>File</th>
                <th>Framework</th>
              </tr>
            </thead>
            <tbody>
              {rows.map((t) => (
                <tr key={t.id}>
                  <td>{t.name || t.file}</td>
                  <td className="mono">{t.file}</td>
                  <td>{t.framework}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
        {rows.length === 0 && (
          <div className="empty-state">
            <h3>
              {data.tests.length ? "No matching tests" : "No tests discovered"}
            </h3>
            <p>
              {data.tests.length
                ? "Adjust your search or framework filter."
                : "Check test patterns in the repository configuration."}
            </p>
            {data.tests.length > 0 && (
              <button
                className="button secondary"
                onClick={() => {
                  setQuery("");
                  setFramework("all");
                }}
              >
                Clear filters
              </button>
            )}
          </div>
        )}
      </section>
    </>
  );
}
