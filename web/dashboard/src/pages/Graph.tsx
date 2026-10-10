import { useMemo, useState } from "react";
import {
  ReactFlow,
  Background,
  Controls,
  MarkerType,
  Position,
  type Node,
  type Edge,
} from "@xyflow/react";
import "@xyflow/react/dist/style.css";
import { useWorkspace } from "../workspace";
import { PageHeading, Badge } from "../components/Shared";
export default function Graph() {
  const { data } = useWorkspace();
  const [query, setQuery] = useState("");
  const [chosen, setChosen] = useState("");
  const files = data?.nodes ?? [];
  const selected = files.find((n) => n.id === chosen);
  const matches = files.filter((n) =>
    n.path.toLowerCase().includes(query.toLowerCase())
  );
  const { nodes, edges, limited } = useMemo(() => {
    if (!data) return { nodes: [], edges: [], limited: false };
    let visible = data.nodes.filter((n) =>
      n.path.toLowerCase().includes(query.toLowerCase())
    );
    if (chosen) {
      const related = new Set([chosen]);
      data.edges.forEach((e) => {
        if (e.from === chosen) related.add(e.to);
        if (e.to === chosen) related.add(e.from);
      });
      visible = data.nodes.filter((n) => related.has(n.id));
    }
    const limited = visible.length > 80;
    visible = visible.slice(0, 80);
    const ids = new Set(visible.map((n) => n.id));
    const nodes: Node[] = visible.map((n, i) => ({
      id: n.id,
      position: { x: (i % 4) * 260, y: Math.floor(i / 4) * 90 },
      sourcePosition: Position.Left,
      targetPosition: Position.Right,
      data: { label: n.path },
      className: `repository-node ${n.kind === "TEST" ? "test-node" : ""}`,
      style: { width: 220 },
    }));
    const edges: Edge[] = data.edges
      .filter((e) => ids.has(e.from) && ids.has(e.to))
      .map((e, i) => ({
        id: `edge-${i}`,
        source: e.from,
        target: e.to,
        markerEnd: { type: MarkerType.ArrowClosed },
      }));
    return { nodes, edges, limited };
  }, [data, query, chosen]);
  if (!data) return null;
  const dependencies = selected
    ? data.edges
        .filter((e) => e.from === selected.id)
        .map((e) => files.find((n) => n.id === e.to))
        .filter(Boolean)
    : [];
  const dependents = selected
    ? data.edges
        .filter((e) => e.to === selected.id)
        .map((e) => files.find((n) => n.id === e.from))
        .filter(Boolean)
    : [];
  return (
    <>
      <PageHeading
        eyebrow="DEPENDENCY GRAPH"
        title="Repository connections"
        description="Resolved relationships from the latest repository scan. Arrows point from a file to the dependency it uses."
      />
      <div className="filter-bar">
        <label>
          Find a file
          <input
            type="search"
            value={query}
            placeholder="Filter by repository-relative path…"
            onChange={(e) => {
              setQuery(e.target.value);
              setChosen("");
            }}
          />
        </label>
        <label>
          Inspect file
          <select value={chosen} onChange={(e) => setChosen(e.target.value)}>
            <option value="">Filtered graph</option>
            {matches.slice(0, 500).map((n) => (
              <option key={n.id} value={n.id}>
                {n.path}
              </option>
            ))}
            {selected && !matches.some((n) => n.id === chosen) && (
              <option value={chosen}>{selected.path}</option>
            )}
          </select>
        </label>
      </div>
      <div className="graph-toolbar">
        <Badge>
          {nodes.length} displayed files · {edges.length} connections
        </Badge>
        {limited && (
          <span className="muted small">
            View limited to 80 files. Filter or select a file to inspect its
            neighborhood.
          </span>
        )}
      </div>
      <section className="panel graph">
        {nodes.length ? (
          <ReactFlow
            key={`${query}-${chosen}-${data.analyzedAt}`}
            nodes={nodes}
            edges={edges}
            nodesDraggable={false}
            nodesConnectable={false}
            fitView
            minZoom={0.1}
            colorMode="light"
            onNodeClick={(_, n) => setChosen(n.id)}
          >
            <Background gap={24} color="#d8dde4" />
            <Controls showInteractive={false} />
          </ReactFlow>
        ) : (
          <div className="empty-state">No matching files.</div>
        )}
      </section>
      <section className="panel config-panel" aria-live="polite">
        <p className="eyebrow">FILE INSPECTOR</p>
        {selected ? (
          <>
            <h3 className="mono">{selected.path}</h3>
            <Badge tone={selected.kind === "TEST" ? "green" : "neutral"}>
              {selected.kind}
            </Badge>
            <div className="grid overview-bottom inspector-lists">
              <div>
                <h3>Dependencies · {dependencies.length}</h3>
                {dependencies.length ? (
                  <ul className="issue-list">
                    {dependencies.map(
                      (n) =>
                        n && (
                          <li key={n.id}>
                            <button
                              className="table-link"
                              onClick={() => {
                                setQuery("");
                                setChosen(n.id);
                              }}
                            >
                              {n.path}
                            </button>
                          </li>
                        )
                    )}
                  </ul>
                ) : (
                  <p>No resolved dependencies.</p>
                )}
              </div>
              <div>
                <h3>Direct dependents · {dependents.length}</h3>
                {dependents.length ? (
                  <ul className="issue-list">
                    {dependents.map(
                      (n) =>
                        n && (
                          <li key={n.id}>
                            <button
                              className="table-link"
                              onClick={() => {
                                setQuery("");
                                setChosen(n.id);
                              }}
                            >
                              {n.path}
                            </button>
                          </li>
                        )
                    )}
                  </ul>
                ) : (
                  <p>No direct dependents.</p>
                )}
              </div>
            </div>
          </>
        ) : (
          <p>
            Select a file to inspect its dependencies and dependents in text
            form.
          </p>
        )}
      </section>
    </>
  );
}
