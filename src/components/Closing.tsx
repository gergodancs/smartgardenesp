"use client";

import { useTranslations } from "next-intl";

export function Closing() {
  const t = useTranslations("closing");

  return (
    <section id="kontakt" className="closing" tabIndex={-1}>
      <div className="wrap closing-inner">
        <p className="closing-text">{t("text")}</p>
        <a className="btn btn-on-forest" href="#kontakt">
          {t("cta")}
        </a>
      </div>
    </section>
  );
}
