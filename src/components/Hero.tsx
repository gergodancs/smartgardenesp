"use client";

import { useTranslations } from "next-intl";
import { motion, useReducedMotion } from "framer-motion";
import Image from "next/image";
import { EASE } from "./motion";

export function Hero() {
  const t = useTranslations("hero");
  const meta = useTranslations("meta");
  const reduce = useReducedMotion() === true;
  const lines = t.raw("lines") as string[];

  return (
    <section className="hero">
      <div className="wrap hero-grid">
        <div className="hero-text">
          <p className="eyebrow">{t("eyebrow")}</p>
          <h1 className="hero-title">
            {lines.map((line) => (
              <span className="line" key={line}>
                <motion.span
                  className="reveal-line"
                  initial={reduce ? false : { y: "110%" }}
                  animate={{ y: "0%" }}
                  transition={{
                    duration: reduce ? 0 : 0.85,
                    delay: reduce ? 0 : 0.08 + lines.indexOf(line) * 0.08,
                    ease: EASE,
                  }}
                >
                  {line}
                </motion.span>
              </span>
            ))}
          </h1>
          <div className="hero-rest">
            <p className="hero-body">{t("body")}</p>
            <div className="hero-actions">
              <a className="btn" href="#kontakt">
                {t("primary")}
              </a>
              <a className="text-link" href="#pakete">
                {t("secondary")}
              </a>
            </div>
            <p className="audiences">{t("audiences")}</p>
          </div>
        </div>
        <div className="hero-media">
          <Image
            src="/panka-portrait.png"
            alt={meta("portraitAlt")}
            width={724}
            height={1024}
            priority
            quality={90}
            sizes="(max-width: 800px) 100vw, 42vw"
            className="hero-photo"
          />
        </div>
      </div>
    </section>
  );
}
