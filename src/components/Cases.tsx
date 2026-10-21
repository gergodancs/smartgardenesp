"use client";

import { useTranslations } from "next-intl";

type CaseItem = {
  name: string;
  figure: string;
  caption: string;
  text: string;
};

export function Cases() {
  const t = useTranslations("cases");
  const items = t.raw("items") as CaseItem[];

  return (
    <section id="cases" className="section" tabIndex={-1}>
      <div className="wrap">
        <h2 className="section-title">{t("title")}</h2>
        <p className="lede">{t("intro")}</p>
        <div className="cases-grid">
          {items.map((item) => (
            <article className="case" key={item.name}>
              <h3 className="case-name">{item.name}</h3>
              <p className="case-figure">{item.figure}</p>
              <p className="case-caption">{item.caption}</p>
              <p className="case-body">{item.text}</p>
            </article>
          ))}
        </div>
        <p className="also">{t("also")}</p>
      </div>
    </section>
  );
}
