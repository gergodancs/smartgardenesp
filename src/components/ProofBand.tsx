"use client";

import { useTranslations } from "next-intl";

type ProofItem = { figure: string; text: string };

export function ProofBand() {
  const t = useTranslations("proof");
  const items = t.raw("items") as ProofItem[];

  return (
    <section className="proof">
      <div className="wrap proof-grid">
        {items.map((item) => (
          <article className="proof-item" key={item.figure}>
            <p className="proof-figure">{item.figure}</p>
            <p>{item.text}</p>
          </article>
        ))}
      </div>
    </section>
  );
}
