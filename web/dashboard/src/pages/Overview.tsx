import type { Page } from "../App";
import { useWorkspace } from "../workspace";
import { PageHeading, Badge } from "../components/Shared";
import { MetricCard } from "../components/MetricCard";
import { Icon } from "../components/Icon";
export default function Overview({
  navigate,
}: {
  navigate: (page: Page) => void;
}) {
  const { data } = useWorkspace();
  if (!data) return null;
  return (
    <>
      <PageHeading
        eyebrow="REPOSITORY OVERVIEW"
        title={data.repository}
        description="Source dependencies and test inventory from your configured repository."
        action={
          <button
            className="button primary"
            onClick={() => navigate("Impact analysis")}
          >
            Analyze a change <Icon name="arrow" />
          </button>
        }
      />
      <div className="grid metrics">
        <MetricCard
          label="Files scanned"
          value={String(data.filesScanned)}
          detail={`${data.sourceFiles} source files`}
        />
        <MetricCard
          label="Dependency edges"
          value={String(data.edges.length)}
          detail="Resolved file relationships"
        />
        <MetricCard
          label="Discovered tests"
          value={String(data.tests.length)}
          detail={`Across ${data.testFiles} test files`}
        />
        <MetricCard
          label="Unresolved imports"
          value={String(data.unresolved.length)}
          detail={`Parser confidence: ${data.confidence.toLowerCase()}`}
        />
      </div>
      <div className="grid overview-bottom">
        <section className="panel">
          <div className="panel-heading">
            <h3>Repository status</h3>
            <Badge tone={data.confidence === "HIGH" ? "green" : "amber"}>
              {data.confidence} confidence
            </Badge>
          </div>
          <dl className="glossary">
            <div>
              <dt>Analysis duration</dt>
              <dd>{data.durationMs} ms</dd>
            </div>
            <div>
              <dt>Last scanned</dt>
              <dd>{new Date(data.analyzedAt).toLocaleString()}</dd>
            </div>
            <div>
              <dt>Test frameworks</dt>
              <dd>
                {[...new Set(data.tests.map((t) => t.framework))].join(", ") ||
                  "No tests discovered"}
              </dd>
            </div>
          </dl>
          <button
            className="text-button"
            onClick={() => navigate("Dependency explorer")}
          >
            Inspect dependencies <Icon name="arrow" />
          </button>
        </section>
        <section className="panel">
          <div className="panel-heading">
            <h3>Unresolved dependencies</h3>
            <Badge tone={data.unresolved.length ? "amber" : "green"}>
              {data.unresolved.length} issues
            </Badge>
          </div>
          {data.unresolved.length ? (
            <>
              <p>
                These imports could not be mapped to repository files. Review
                them before relying on selective test execution.
              </p>
              <ul className="issue-list">
                {data.unresolved.slice(0, 12).map((issue, i) => (
                  <li key={i}>
                    <code>{issue}</code>
                  </li>
                ))}
              </ul>
              {data.unresolved.length > 12 && (
                <p>
                  {data.unresolved.length - 12} additional issues. Refresh after
                  correcting import configuration.
                </p>
              )}
            </>
          ) : (
            <div className="empty-state">
              <Icon name="check" />
              <h3>No unresolved local imports</h3>
              <p>
                Static parser confidence is a signal, not a guarantee of runtime
                coverage.
              </p>
            </div>
          )}
        </section>
      </div>
    </>
  );
}
