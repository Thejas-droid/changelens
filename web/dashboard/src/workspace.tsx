import {
  createContext,
  useCallback,
  useContext,
  useEffect,
  useState,
  type ReactNode,
} from "react";
export type FileNode = {
  id: string;
  path: string;
  kind: string;
  language: string;
};
export type Test = {
  id: string;
  fileId: string;
  file: string;
  name: string;
  framework: string;
};
export type Snapshot = {
  repository: string;
  analyzedAt: number;
  durationMs: number;
  confidence: string;
  filesScanned: number;
  sourceFiles: number;
  testFiles: number;
  nodes: FileNode[];
  edges: { from: string; to: string }[];
  tests: Test[];
  unresolved: string[];
  configuration: {
    workers: number;
    timeoutSeconds: number;
    cacheEnabled: boolean;
    alwaysRun: string[];
    fallbackPatterns: string[];
  };
};
export type Impact = {
  confidence: string;
  fallback: boolean;
  affectedFiles: number;
  selectedTests: number;
  totalTests: number;
  reasons: string[];
  tests: {
    id: string;
    name: string;
    file: string;
    selected: boolean;
    path: string[];
  }[];
};
type State = {
  data: Snapshot | null;
  loading: boolean;
  error: string;
  unauthorized: boolean;
  refresh: () => void;
  signIn: (token: string) => void;
  analyze: (paths: string, signal: AbortSignal) => Promise<Impact>;
};
const Context = createContext<State | undefined>(undefined);
function record(value: unknown): value is Record<string, unknown> {
  return typeof value === "object" && value !== null;
}
const strings = (value: unknown): value is string[] =>
  Array.isArray(value) && value.every((item) => typeof item === "string");
const count = (value: unknown): value is number =>
  typeof value === "number" && Number.isSafeInteger(value) && value >= 0;
function validSnapshot(value: unknown): value is Snapshot {
  if (!record(value) || !record(value.configuration)) return false;
  const config = value.configuration;
  return (
    typeof value.repository === "string" &&
    count(value.analyzedAt) &&
    count(value.durationMs) &&
    typeof value.confidence === "string" &&
    count(value.filesScanned) &&
    count(value.sourceFiles) &&
    count(value.testFiles) &&
    Array.isArray(value.nodes) &&
    value.nodes.every(
      (n) => record(n) && strings([n.id, n.path, n.kind, n.language])
    ) &&
    Array.isArray(value.edges) &&
    value.edges.every((e) => record(e) && strings([e.from, e.to])) &&
    Array.isArray(value.tests) &&
    value.tests.every(
      (t) => record(t) && strings([t.id, t.fileId, t.file, t.name, t.framework])
    ) &&
    strings(value.unresolved) &&
    count(config.workers) &&
    count(config.timeoutSeconds) &&
    typeof config.cacheEnabled === "boolean" &&
    strings(config.alwaysRun) &&
    strings(config.fallbackPatterns)
  );
}
function validImpact(value: unknown): value is Impact {
  return (
    record(value) &&
    typeof value.confidence === "string" &&
    typeof value.fallback === "boolean" &&
    count(value.affectedFiles) &&
    count(value.selectedTests) &&
    count(value.totalTests) &&
    value.selectedTests <= value.totalTests &&
    strings(value.reasons) &&
    Array.isArray(value.tests) &&
    value.tests.every(
      (t) =>
        record(t) &&
        strings([t.id, t.name, t.file]) &&
        typeof t.selected === "boolean" &&
        strings(t.path)
    )
  );
}

async function request(path: string, token: string, options: RequestInit = {}) {
  const response = await fetch(path, {
    ...options,
    headers: {
      ...options.headers,
      ...(token ? { Authorization: `Bearer ${token}` } : {}),
    },
  });
  if (!response.ok) {
    let message = `Server returned HTTP ${response.status}.`;
    try {
      const body = await response.json();
      if (typeof body.error === "string") message = body.error;
    } catch {
      /* An unavailable proxy may return HTML. */
    }
    const error = new Error(message);
    Object.assign(error, { status: response.status });
    throw error;
  }
  return response.json();
}
export function WorkspaceProvider({ children }: { children: ReactNode }) {
  const [token, setToken] = useState("");
  const [revision, setRevision] = useState(0);
  const [data, setData] = useState<Snapshot | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState("");
  const [unauthorized, setUnauthorized] = useState(false);
  useEffect(() => {
    let disposed = false;
    const controller = new AbortController();
    const timer = setTimeout(() => controller.abort(), 30000);
    setLoading(true);
    setError("");
    setUnauthorized(false);
    // Defer until mount cleanup finishes so development StrictMode does not
    // launch a second expensive scan for the same initial page load.
    Promise.resolve()
      .then(() => {
        controller.signal.throwIfAborted();
        return request("/api/v1/workspace", token, { signal: controller.signal });
      })
      .then((value) => {
        if (disposed) return;
        if (!validSnapshot(value))
          throw new Error("The server returned an invalid workspace response.");
        setData(value);
      })
      .catch((e) => {
        if (!controller.signal.aborted) {
          setError(e.message || "Repository analysis is unavailable.");
          setUnauthorized(e.status === 401);
          if (e.status === 401) setData(null);
        } else if (!disposed)
          setError(
            "The analysis request timed out. Retry or check the server."
          );
      })
      .finally(() => {
        if (!disposed) setLoading(false);
        clearTimeout(timer);
      });
    return () => {
      disposed = true;
      clearTimeout(timer);
      controller.abort();
    };
  }, [token, revision]);
  const analyze = useCallback(
    (paths: string, signal: AbortSignal) =>
      request("/api/v1/impact", token, {
        method: "POST",
        body: paths,
        headers: { "Content-Type": "text/plain" },
        signal,
      }).then((value) => {
        if (!validImpact(value))
          throw new Error("The server returned an invalid impact response.");
        return value;
      }),
    [token]
  );
  return (
    <Context.Provider
      value={{
        data,
        loading,
        error,
        unauthorized,
        refresh: () => setRevision((n) => n + 1),
        signIn: setToken,
        analyze,
      }}
    >
      {children}
    </Context.Provider>
  );
}
export function useWorkspace() {
  const value = useContext(Context);
  if (!value) throw new Error("Workspace provider missing");
  return value;
}
