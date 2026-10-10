import { useEffect, useRef, useState, type FormEvent } from "react";
import { useWorkspace, type Impact as Result } from "../workspace";
import { PageHeading, Badge } from "../components/Shared";
import { MetricCard } from "../components/MetricCard";
export default function Impact() {
  const { analyze, data } = useWorkspace();
  const [paths, setPaths] = useState("");
  const [result, setResult] = useState<Result | null>(null);
  const [error, setError] = useState("");
  const [busy, setBusy] = useState(false);
  const [chosen, setChosen] = useState("");
  const controller = useRef<AbortController | null>(null);
  useEffect(() => () => controller.current?.abort(), []);
  useEffect(() => {
    setResult(null);
    setError("");
  }, [data?.analyzedAt]);
  async function submit(e: FormEvent) {
    e.preventDefault();
    controller.current?.abort();
    const request = new AbortController();
    controller.current = request;
    const timer = setTimeout(() => request.abort(), 30000);
    setBusy(true);
    setError("");
    setResult(null);
    try {
      const value = await analyze(paths, request.signal);
      if (!request.signal.aborted) {
        setResult(value);
        setChosen(
          value.tests.find((t) => t.selected)?.id ?? value.tests[0]?.id ?? ""
        );
      }
    } catch (e) {
      if (controller.current === request)
        setError(
          request.signal.aborted
            ? "Analysis timed out. Retry or check server logs."
            : e instanceof Error
            ? e.message
            : "Analysis failed."
        );
    } finally {
      clearTimeout(timer);
      if (controller.current === request) setBusy(false);
    }
  }
  const test = result?.tests.find((t) => t.id === chosen);
  return (
    <>
      <PageHeading
        eyebrow="CHANGE IMPACT"
        title="Impact analysis"
        description="Enter changed files to find affected tests and see why they were selected."
      />
      <section className="panel">
        <form className="impact-form" onSubmit={submit}>
          <label>
            Changed files{" "}
            <span className="muted small">
              One repository-relative path per line · up to 128 files
            </span>
            <textarea
              required
              rows={5}
              maxLength={65536}
              placeholder="src/payment/service.hpp"
              value={paths}
              onChange={(e) => {
                setPaths(e.target.value);
                setResult(null);
                setError("");
              }}
              disabled={busy}
            />
          </label>
          <div className="impact-actions">
            <p>Tests are selected for review; they are not run.</p>
            <button className="button primary" disabled={busy || !paths.trim()}>
              {busy ? "Analyzing…" : "Analyze impact"}
            </button>
          </div>
        </form>
      </section>
      {error && (
        <div className="error-banner" role="alert">
          {error}
        </div>
      )}
      {result && (
        <>
          <div className="section-heading">
            <h3>Selection result</h3>
            <Badge tone={result.fallback ? "amber" : "green"}>
              {result.fallback
                ? "Full-suite fallback"
                : `${result.confidence} confidence`}
            </Badge>
          </div>
          <div className="grid metrics">
            <MetricCard
              label="Affected files"
              value={String(result.affectedFiles)}
            />
            <MetricCard
              label="Selected tests"
              value={String(result.selectedTests)}
            />
            <MetricCard
              label="Outside detected impact"
              value={String(result.totalTests - result.selectedTests)}
            />
            <MetricCard label="Total tests" value={String(result.totalTests)} />
          </div>
          {result.reasons.length > 0 && (
            <div className="note">
              <ul>
                {result.reasons.map((r, i) => (
                  <li key={i}>{r}</li>
                ))}
              </ul>
            </div>
          )}
          <div className="grid explanation-grid">
            <section className="panel table-panel">
              <div className="table-scroll">
                <table>
                  <thead>
                    <tr>
                      <th>Test</th>
                      <th>Selection</th>
                    </tr>
                  </thead>
                  <tbody>
                    {result.tests.map((t) => (
                      <tr
                        key={t.id}
                        className={chosen === t.id ? "selected-row" : ""}
                      >
                        <td>
                          <button
                            className="table-link"
                            aria-pressed={chosen === t.id}
                            onClick={() => setChosen(t.id)}
                          >
                            {t.name || t.file}
                          </button>
                          <span className="table-detail mono">{t.file}</span>
                        </td>
                        <td>
                          <Badge tone={t.selected ? "green" : "neutral"}>
                            {t.selected ? "Selected" : "Outside impact"}
                          </Badge>
                        </td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              </div>
              {result.tests.length === 0 && (
                <div className="empty-state">
                  No tests were discovered in this repository.
                </div>
              )}
            </section>
            <section className="panel" aria-live="polite">
              <p className="eyebrow">SELECTION DETAILS</p>
              {test ? (
                <>
                  <h3>{test.name || test.file}</h3>
                  {test.path.length > 0 ? (
                    <div className="path-list">
                      {test.path.map((p, i) => (
                        <div key={i}>
                          <span>{i + 1}</span>
                          <code>{p}</code>
                        </div>
                      ))}
                    </div>
                  ) : (
                    <p>
                      {result.fallback
                        ? "Selected by a configured full-suite fallback."
                        : test.selected
                        ? "Selected by an always-run rule or dependency impact."
                        : "No dependency path or always-run rule selected this test."}
                    </p>
                  )}
                </>
              ) : (
                <p>Select a test to inspect its dependency path.</p>
              )}
            </section>
          </div>
        </>
      )}
    </>
  );
}
