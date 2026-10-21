"use client";

import { useLocale } from "next-intl";
import { useEffect, useState } from "react";

export function LangSwitch() {
  const locale = useLocale();
  const [hash, setHash] = useState("");

  useEffect(() => {
    const sync = () => setHash(window.location.hash);
    sync();
    window.addEventListener("hashchange", sync);
    return () => window.removeEventListener("hashchange", sync);
  }, []);

  const items = [
    { code: "de", label: "DE", href: `/${hash}` },
    { code: "en", label: "EN", href: `/en${hash}` },
  ] as const;

  return (
    <nav className="lang" aria-label={locale === "de" ? "Sprache" : "Language"}>
      {items.map((item, index) => (
        <span key={item.code} className="lang-pair">
          {index > 0 ? (
            <span className="lang-slash" aria-hidden="true">
              /
            </span>
          ) : null}
          {locale === item.code ? (
            <span className="is-current" aria-current="page" lang={item.code}>
              {item.label}
            </span>
          ) : (
            <a href={item.href} hrefLang={item.code} lang={item.code}>
              {item.label}
            </a>
          )}
        </span>
      ))}
    </nav>
  );
}
