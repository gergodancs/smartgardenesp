"use client";

import { useTranslations } from "next-intl";
import { Wordmark } from "./Wordmark";

export function Footer() {
  const t = useTranslations("footer");

  return (
    <footer className="footer">
      <div className="wrap footer-inner">
        <Wordmark />
        <p>{t("copy")}</p>
      </div>
    </footer>
  );
}
