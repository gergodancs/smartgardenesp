"use client";

import { useTranslations } from "next-intl";
import { motion, useReducedMotion } from "framer-motion";
import { useEffect, useRef, useState } from "react";
import { About } from "./About";
import { Cases } from "./Cases";
import { Closing } from "./Closing";
import { Footer } from "./Footer";
import { Header } from "./Header";
import { Hero } from "./Hero";
import { MenuOverlay } from "./MenuOverlay";
import { Packages } from "./Packages";
import { ProofBand } from "./ProofBand";
import { Services } from "./Services";
import { EASE } from "./motion";

export function HomePage() {
  const t = useTranslations("nav");
  const reduce = useReducedMotion() === true;
  const [open, setOpen] = useState(false);
  const [headerOnMenu, setHeaderOnMenu] = useState(false);
  const pendingRef = useRef<string | null>(null);

  useEffect(() => {
    if (!headerOnMenu) return;
    const previous = document.body.style.overflow;
    document.body.style.overflow = "hidden";
    return () => {
      document.body.style.overflow = previous;
    };
  }, [headerOnMenu]);

  useEffect(() => {
    if (!open) return;
    const onKey = (event: KeyboardEvent) => {
      if (event.key === "Escape") setOpen(false);
    };
    window.addEventListener("keydown", onKey);
    return () => window.removeEventListener("keydown", onKey);
  }, [open]);

  useEffect(() => {
    if (headerOnMenu || !pendingRef.current) return;
    const id = pendingRef.current;
    pendingRef.current = null;
    const target = document.getElementById(id);
    if (!target) return;
    target.focus({ preventScroll: true });
    target.scrollIntoView({ behavior: reduce ? "auto" : "smooth", block: "start" });
    const next = `${window.location.pathname}${window.location.search}#${id}`;
    window.history.replaceState(null, "", next);
    window.dispatchEvent(new HashChangeEvent("hashchange"));
  }, [headerOnMenu, reduce]);

  function closeMenu() {
    setOpen(false);
  }

  function toggleMenu() {
    if (open) {
      closeMenu();
      return;
    }
    setHeaderOnMenu(true);
    setOpen(true);
  }

  function navigate(id: string) {
    pendingRef.current = id;
    setOpen(false);
  }

  return (
    <>
      <a className="skip" href="#content" inert={headerOnMenu ? true : undefined}>
        {t("skip")}
      </a>
      <Header open={headerOnMenu} onToggle={toggleMenu} onClose={closeMenu} onNavigate={navigate} />
      <motion.div
        className={open ? "page-scale is-open" : "page-scale"}
        initial={false}
        animate={{ scale: open && !reduce ? 0.98 : 1 }}
        transition={{ duration: reduce ? 0 : 0.6, ease: EASE }}
        inert={headerOnMenu ? true : undefined}
      >
        <main id="content">
          <Hero />
          <ProofBand />
          <Services />
          <Packages />
          <Cases />
          <About />
          <Closing />
        </main>
        <Footer />
      </motion.div>
      <motion.div
        className="page-dim"
        aria-hidden="true"
        initial={false}
        animate={{ opacity: open ? 1 : 0 }}
        transition={{ duration: reduce ? 0 : 0.6, ease: EASE }}
      />
      <MenuOverlay
        open={open}
        onNavigate={navigate}
        onExit={() => {
          setHeaderOnMenu(false);
        }}
      />
    </>
  );
}
