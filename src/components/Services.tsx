"use client";

import { useTranslations } from "next-intl";

type Service = { index: string; title: string; text: string };

export function Services() {
  const t = useTranslations("services");
  const items = t.raw("items") as Service[];

  return (
    <section id="leistungen" className="section" tabIndex={-1}>
      <div className="wrap">
        <h2 className="section-title">{t("title")}</h2>
        <div className="services-grid">
          {items.map((item) => (
            <article className="service" key={item.index}>
              <p className="service-index">{item.index}</p>
              <h3>{item.title}</h3>
              <p>{item.text}</p>
            </article>
          ))}
        </div>
      </div>
    </section>
  );
}
