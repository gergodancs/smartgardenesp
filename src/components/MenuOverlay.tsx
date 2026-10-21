"use client";

import { useTranslations } from "next-intl";
import { AnimatePresence, motion, useReducedMotion } from "framer-motion";
import Image from "next/image";
import { useEffect, useRef } from "react";
import { LangSwitch } from "./LangSwitch";
import { EASE } from "./motion";

const LINKS = [
  { id: "leistungen", index: "01", key: "services" },
  { id: "cases", index: "02", key: "cases" },
  { id: "ueber-mich", index: "03", key: "about" },
] as const;

type MenuOverlayProps = {
  open: boolean;
  onNavigate: (id: string) => void;
  onExit: () => void;
};

export function MenuOverlay({ open, onNavigate, onExit }: MenuOverlayProps) {
  const t = useTranslations("nav");
  const reduce = useReducedMotion() === true;
  const panelRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    if (!open) return;
    const root = panelRef.current;
    const toggle = document.getElementById("menu-toggle");
    const selector = 'a[href], button:not([disabled])';

    function nodes() {
      const list = root ? Array.from(root.querySelectorAll<HTMLElement>(selector)) : [];
      if (toggle) list.unshift(toggle);
      return list;
    }

    function onKey(event: KeyboardEvent) {
      if (event.key !== "Tab") return;
      const list = nodes();
      if (list.length === 0) return;
      const first = list[0];
      const last = list[list.length - 1];
      if (event.shiftKey && document.activeElement === first) {
        event.preventDefault();
        last.focus();
      } else if (!event.shiftKey && document.activeElement === last) {
        event.preventDefault();
        first.focus();
      }
    }

    document.addEventListener("keydown", onKey);
    return () => document.removeEventListener("keydown", onKey);
  }, [open]);

  return (
    <AnimatePresence onExitComplete={onExit}>
      {open ? (
        <motion.div
          id="site-menu"
          ref={panelRef}
          className="menu-panel"
          role="dialog"
          aria-modal="true"
          aria-labelledby="menu-title"
          initial={reduce ? { opacity: 0 } : { clipPath: "inset(0 0 0 100%)" }}
          animate={reduce ? { opacity: 1 } : { clipPath: "inset(0 0 0 0%)" }}
          exit={reduce ? { opacity: 0 } : { clipPath: "inset(0 0 0 100%)" }}
          transition={{ duration: reduce ? 0.01 : 0.6, ease: EASE }}
        >
          <h2 id="menu-title" className="sr-only">
            {t("menu")}
          </h2>
          <div className="menu-main">
            <div className="menu-lang">
              <LangSwitch />
            </div>
            <nav className="menu-nav" aria-label={t("menu")}>
              {LINKS.map((link, index) => (
                <motion.a
                  key={link.id}
                  className="menu-link"
                  href={`#${link.id}`}
                  initial={reduce ? false : { opacity: 0, y: 28 }}
                  animate={{ opacity: 1, y: 0 }}
                  exit={
                    reduce
                      ? { opacity: 0, transition: { duration: 0 } }
                      : {
                          opacity: 0,
                          y: 12,
                          transition: { duration: 0.22, delay: (LINKS.length - 1 - index) * 0.05, ease: EASE },
                        }
                  }
                  transition={{
                    duration: reduce ? 0 : 0.5,
                    delay: reduce ? 0 : 0.18 + index * 0.06,
                    ease: EASE,
                  }}
                  onClick={(event) => {
                    event.preventDefault();
                    onNavigate(link.id);
                  }}
                >
                  <span className="menu-index">{link.index}</span>
                  <span className="menu-label">{t(link.key)}</span>
                </motion.a>
              ))}
            </nav>
          </div>
          <aside className="menu-aside">
            <p className="menu-kicker">{t("place")}</p>
            <div className="menu-figure">
              <Image
                src="/panka-portrait.png"
                alt=""
                fill
                sizes="(max-width: 760px) 120px, 36vw"
                className="menu-photo"
              />
            </div>
          </aside>
        </motion.div>
      ) : null}
    </AnimatePresence>
  );
}
