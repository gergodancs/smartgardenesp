import type { Metadata, Viewport } from "next";
import { Fraunces, Outfit } from "next/font/google";
import { hasLocale, NextIntlClientProvider } from "next-intl";
import { getMessages, setRequestLocale } from "next-intl/server";
import { notFound } from "next/navigation";
import { routing } from "@/i18n/routing";
import "../globals.css";

const fraunces = Fraunces({
  subsets: ["latin"],
  display: "swap",
  variable: "--font-serif",
  axes: ["SOFT", "WONK", "opsz"],
});

const outfit = Outfit({
  subsets: ["latin"],
  display: "swap",
  variable: "--font-sans",
});

export const viewport: Viewport = {
  themeColor: "#f3efe6",
  colorScheme: "light",
  width: "device-width",
  initialScale: 1,
};

export function generateStaticParams() {
  return routing.locales.map((locale) => ({ locale }));
}

export async function generateMetadata({
  params,
}: {
  params: Promise<{ locale: string }>;
}): Promise<Metadata> {
  const { locale } = await params;
  const path = locale === "en" ? "/en" : "/";
  const title =
    locale === "en"
      ? "base digital – Marketing for small businesses in Vienna"
      : "base digital – Marketing für kleine Betriebe in Wien";
  const description =
    locale === "en"
      ? "I’m Panka Gyarmati. I look after Vienna businesses, clubs, and organisers who don’t have a marketer of their own. I plan it, I do it, and I show you what it did."
      : "Ich bin Panka Gyarmati und betreue Wiener Betriebe, Vereine und Veranstalter ohne eigene Marketingperson. Ich plane, setze um und zeige dir, was es bringt.";

  return {
    metadataBase: new URL("https://base-digital.at"),
    title,
    description,
    alternates: {
      canonical: path,
      languages: {
        de: "/",
        en: "/en",
        "x-default": "/",
      },
    },
    openGraph: {
      title,
      description,
      url: path,
      siteName: "base digital",
      locale: locale === "de" ? "de_AT" : "en_GB",
      type: "website",
    },
  };
}

export default async function LocaleLayout({
  children,
  params,
}: {
  children: React.ReactNode;
  params: Promise<{ locale: string }>;
}) {
  const { locale } = await params;
  if (!hasLocale(routing.locales, locale)) {
    notFound();
  }

  setRequestLocale(locale);
  const messages = await getMessages();

  return (
    <html lang={locale} className={`${fraunces.variable} ${outfit.variable}`} suppressHydrationWarning>
      <body>
        <NextIntlClientProvider messages={messages}>{children}</NextIntlClientProvider>
      </body>
    </html>
  );
}
