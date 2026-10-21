import { getTranslations, setRequestLocale } from "next-intl/server";
import { HomePage } from "@/components/HomePage";

export default async function Page({
  params,
}: {
  params: Promise<{ locale: string }>;
}) {
  const { locale } = await params;
  setRequestLocale(locale);
  const t = await getTranslations({ locale, namespace: "meta" });

  const jsonLd = {
    "@context": "https://schema.org",
    "@type": "ProfessionalService",
    name: "base digital",
    url: "https://base-digital.at",
    founder: {
      "@type": "Person",
      name: "Panka Gyarmati",
    },
    areaServed: "Wien",
    description: t("description"),
    inLanguage: locale,
  };

  return (
    <>
      <script
        type="application/ld+json"
        dangerouslySetInnerHTML={{ __html: JSON.stringify(jsonLd) }}
      />
      <HomePage />
    </>
  );
}
