export function MetricCard({
  label,
  value,
  detail,
}: {
  label: string;
  value: string;
  detail?: string;
}) {
  return (
    <div className="card">
      <div className="muted">{label}</div>
      <div className="metric">{value}</div>
      {detail && <div className="muted small">{detail}</div>}
    </div>
  );
}
