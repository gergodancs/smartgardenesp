"use client";

import { useLocale, useTranslations } from "next-intl";
import { useReducedMotion } from "framer-motion";
import { LangSwitch } from "./LangSwitch";
import { Wordmark } from "./Wordmark";

type HeaderProps = {
  open: boolean;
  onToggle: () => void;
  onClose: () => void;
  onNavigate: (id: string) => void;
};

export function Header({ open, onToggle, onClose, onNavigate }: HeaderProps) {
  const t = useTranslations("nav");
  const locale = useLocale();
  const reduce = useReducedMotion();
  const home = locale === "en" ? "/en" : "/";

  return (
    <header className={open ? "header is-open" : "header"}>
      <div className="wrap header-inner">
        <a
          className="wordmark-link"
          href={home}
          aria-label="base digital"
          onClick={(event) => {
            event.preventDefault();
            if (open) onClose();
            window.scrollTo({ top: 0, behavior: reduce ? "auto" : "smooth" });
            if (window.location.hash) {
              const path = `${window.location.pathname}${window.location.search}`;
              window.history.pushState(null, "", path);
              window.dispatchEvent(new HashChangeEvent("hashchange"));
            }
          }}
        >
          <Wordmark />
        </a>
        <div className="header-actions">
          <LangSwitch />
          <a
            className="btn btn-small header-cta"
            href="#kontakt"
            onClick={(event) => {
              if (!open) return;
              event.preventDefault();
              onNavigate("kontakt");
            }}
          >
            {t("cta")}
          </a>
          <button
            id="menu-toggle"
            className="menu-toggle"
            type="button"
            aria-expanded={open}
            aria-controls="site-menu"
            onClick={onToggle}
          >
            <span>{open ? t("close") : t("menu")}</span>
            <svg width="16" height="16" viewBox="0 0 16 16" aria-hidden="true">
              {open ? (
                <path d="M3.2 3.2l9.6 9.6M12.8 3.2l-9.6 9.6" stroke="currentColor" strokeWidth="1.4" />
              ) : (
                <path d="M2 4.5h12M2 11.5h12" stroke="currentColor" strokeWidth="1.4" />
              )}
            </svg>
          </button>
        </div>
      </div>
    </header>
  );
}
