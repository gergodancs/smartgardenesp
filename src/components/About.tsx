"use client";

import { useTranslations } from "next-intl";

export function About() {
  const t = useTranslations("about");

  return (
    <section id="ueber-mich" className="section about" tabIndex={-1}>
      <div className="wrap about-grid">
        <h2 className="section-title">{t("title")}</h2>
        <p className="about-body">{t("body")}</p>
      </div>
    </section>
  );
}
