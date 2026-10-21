export default function NotFound() {
  return (
    <html lang="de">
      <body
        style={{
          margin: 0,
          minHeight: "100vh",
          display: "grid",
          placeItems: "center",
          background: "#f3efe6",
          color: "#14211c",
          fontFamily: "Georgia, serif",
        }}
      >
        <main style={{ padding: "2rem", maxWidth: "28rem" }}>
          <p style={{ letterSpacing: "-0.04em" }}>base digital</p>
          <h1 style={{ fontWeight: 500, fontSize: "2.4rem", lineHeight: 1.05 }}>
            Seite nicht gefunden
          </h1>
          <p>
            <a href="/" style={{ color: "#143028" }}>
              Zur Startseite
            </a>
          </p>
        </main>
      </body>
    </html>
  );
}
