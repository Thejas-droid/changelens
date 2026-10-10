import { useEffect, useRef, useState } from "react";
import { WorkspaceProvider, useWorkspace } from "./workspace";
import { Icon } from "./components/Icon";
import Overview from "./pages/Overview";
import Impact from "./pages/Impact";
import Graph from "./pages/Graph";
import Tests from "./pages/Tests";
import Settings from "./pages/Settings";
const navigation = [
  { label: "Overview", icon: "overview" },
  { label: "Impact analysis", icon: "pulse" },
  { label: "Dependency explorer", icon: "graph" },
  { label: "Tests", icon: "tests" },
  { label: "Configuration", icon: "settings" },
] as const;
export type Page = (typeof navigation)[number]["label"];
const slug = (name: string) => name.toLowerCase().replaceAll(" ", "-");
const fromHash = (): Page =>
  navigation.find((item) => slug(item.label) === location.hash.slice(1))
    ?.label ?? "Overview";
function Dashboard() {
  const [page, setPage] = useState<Page>(fromHash);
  const { data, loading, error, unauthorized, refresh, signIn } =
    useWorkspace();
  const [credential, setCredential] = useState("");
  const heading = useRef<HTMLHeadingElement>(null);
  useEffect(() => {
    const changed = () => {
      if (location.hash !== "#main") setPage(fromHash());
    };
    window.addEventListener("hashchange", changed);
    return () => window.removeEventListener("hashchange", changed);
  }, []);
  const navigate = (next: Page) => {
    setPage(next);
    location.hash = slug(next);
    heading.current?.focus();
    window.scrollTo({ top: 0 });
  };
  return (
    <>
      <a className="skip-link" href="#main">
        Skip to content
      </a>
      <div className="app">
        <aside className="sidebar">
          <a className="brand" href="#overview">
            <span className="brand-mark">
              <Icon name="graph" />
            </span>
            <span className="brand-name">
              ChangeLens
            </span>
          </a>
          <p className="tag">Change-impact analysis</p>
          <p className="nav-group">Workspace</p>
          <nav aria-label="Main navigation">
            {navigation.map((item) => (
              <button
                key={item.label}
                className={`nav-item ${page === item.label ? "active" : ""}`}
                aria-current={page === item.label ? "page" : undefined}
                onClick={() => navigate(item.label)}
              >
                <Icon name={item.icon} />
                {item.label}
                {page === item.label && (
                  <span className="nav-dot" aria-hidden="true" />
                )}
              </button>
            ))}
          </nav>
          <div className="sidebar-bottom">
            <span className={`status-dot ${error ? "offline" : ""}`} />
            <div>
              <strong>{data?.repository ?? "No repository available"}</strong>
              <span>
                {error
                  ? "Connection requires attention"
                  : loading
                  ? "Analyzing repository…"
                  : "Connected to analysis service"}
              </span>
            </div>
          </div>
        </aside>
        <div className="workspace">
          <header className="topbar">
            <div className="breadcrumb">
              Workspace <span>/</span>
              <strong>{page}</strong>
            </div>
            <button
              className="button secondary"
              onClick={refresh}
              disabled={loading}
            >
              {loading ? "Analyzing…" : "Refresh analysis"}
            </button>
          </header>
          <main id="main" tabIndex={-1}>
            <h1 className="sr-only" ref={heading} tabIndex={-1}>
              {page}
            </h1>
            {error && (
              <div className="error-banner" role="alert">
                <Icon name="info" />
                <div>
                  <strong>
                    {unauthorized
                      ? "Authentication required"
                      : "Analysis unavailable"}
                  </strong>
                  <p>
                    {error}
                    {data
                      ? " Previously loaded results remain visible below."
                      : ""}
                  </p>
                </div>
              </div>
            )}
            {!data ? (
              <section className="panel connection-panel">
                <p className="eyebrow">REPOSITORY Workspace</p>
                <h2>
                  {loading
                    ? "Loading repository analysis…"
                    : unauthorized
                    ? "Connect to your workspace"
                    : "Connect to the analysis service"}
                </h2>
                <p>
                  {loading
                    ? "Scanning source files, dependencies, and tests."
                    : "The dashboard requires the ChangeLens server with a configured repository. Enter the deployment access token if authentication is enabled."}
                </p>
                {!loading && (
                  <>
                    <form
                      onSubmit={(e) => {
                        e.preventDefault();
                        signIn(credential);
                        setCredential("");
                      }}
                    >
                      <label>
                        API access token
                        <input
                          type="password"
                          required
                          autoComplete="off"
                          value={credential}
                          onChange={(e) => setCredential(e.target.value)}
                        />
                      </label>
                      <button className="button primary">Connect</button>
                    </form>
                    <button className="text-button" onClick={refresh}>
                      Retry connection
                    </button>
                  </>
                )}
              </section>
            ) : (
              <>
                {page === "Overview" && <Overview navigate={navigate} />}{" "}
                {page === "Impact analysis" && <Impact />}
                {page === "Dependency explorer" && <Graph />}
                {page === "Tests" && <Tests />}
                {page === "Configuration" && <Settings />}
              </>
            )}
            <footer>
              ChangeLens
              <span>
                Repository analysis ·{" "}
                {data
                  ? `Last scanned ${new Date(data.analyzedAt).toLocaleString()}`
                  : "Awaiting connection"}
              </span>
            </footer>
          </main>
        </div>
      </div>
    </>
  );
}
export default function App() {
  return (
    <WorkspaceProvider>
      <Dashboard />
    </WorkspaceProvider>
  );
}
