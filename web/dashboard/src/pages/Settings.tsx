import { useWorkspace } from "../workspace";
import { PageHeading, Badge } from "../components/Shared";
export default function Settings() {
  const { data } = useWorkspace();
  if (!data) return null;
  const config = data.configuration;
  return (
    <>
      <PageHeading
        eyebrow="REPOSITORY CONFIGURATION"
        title="Analysis policy"
        description="Effective configuration loaded from testgraph.yml or testgraph.yaml, with built-in defaults when neither file exists."
        action={<Badge>Read only</Badge>}
      />
      <div className="grid overview-bottom">
        <section className="panel">
          <h3>Always-run patterns</h3>
          <p>
            Matching tests are added to the impact selection independently of
            detected dependencies.
          </p>
          {config.alwaysRun.length ? (
            <ul className="issue-list">
              {config.alwaysRun.map((p) => (
                <li key={p}>
                  <code>{p}</code>
                </li>
              ))}
            </ul>
          ) : (
            <p>No always-run patterns configured.</p>
          )}
        </section>
        <section className="panel">
          <h3>Full-suite fallback patterns</h3>
          <p>Matching changed files select the entire discovered suite.</p>
          <ul className="issue-list">
            {config.fallbackPatterns.map((p) => (
              <li key={p}>
                <code>{p}</code>
              </li>
            ))}
          </ul>
        </section>
      </div>
      <section className="panel config-panel">
        <h3>Execution library defaults</h3>
        <p>
          These settings are available to the core execution library. This
          analysis service does not execute jobs or use cached test results.
        </p>
        <dl className="glossary">
          <div>
            <dt>Worker count</dt>
            <dd>{config.workers}</dd>
          </div>
          <div>
            <dt>Command timeout</dt>
            <dd>{config.timeoutSeconds} seconds</dd>
          </div>
          <div>
            <dt>Passing-result cache</dt>
            <dd>
              {config.cacheEnabled
                ? "Enabled in configuration"
                : "Disabled in configuration"}
            </dd>
          </div>
        </dl>
      </section>
    </>
  );
}
