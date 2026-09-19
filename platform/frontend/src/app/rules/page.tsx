"use client";

// 规则介绍页：地图规则 / CSV 数字含义 / 任务规则 / 使用流程（全双语）

import Link from "next/link";
import { useI18n } from "@/lib/i18n";
import type { TKey } from "@/lib/i18n";

const MAP_RULE_KEYS = ["rules.map.b1", "rules.map.b2", "rules.map.b3"] as const;

const TASK_RULE_KEYS = [
  "rules.task.b1",
  "rules.task.b2",
  "rules.task.b3",
  "rules.task.b4",
  "rules.task.b5",
] as const;

const FLOW_STEP_KEYS = [
  "rules.flow.s1",
  "rules.flow.s2",
  "rules.flow.s3",
  "rules.flow.s4",
] as const;

// CSV 数字含义表（颜色与地图图例一致）
const CSV_ROWS: Array<{ v: number; key: TKey; color: string; stroke?: boolean }> = [
  { v: 0, key: "rules.csv.v0", color: "#ffffff", stroke: true },
  { v: 1, key: "rules.csv.v1", color: "#475569" },
  { v: 2, key: "rules.csv.v2", color: "#f59e0b" },
  { v: 3, key: "rules.csv.v3", color: "#2563eb" },
  { v: 4, key: "rules.csv.v4", color: "#10b981" },
];

function Section({
  title,
  children,
}: {
  title: string;
  children: React.ReactNode;
}) {
  return (
    <section className="rounded-2xl border border-slate-200 bg-white p-6 shadow-sm">
      <h2 className="mb-4 text-base font-semibold text-slate-800">{title}</h2>
      {children}
    </section>
  );
}

export default function RulesPage() {
  const { lang, setLang, t } = useI18n();

  return (
    <div className="flex min-h-screen flex-col">
      {/* 顶部导航：返回主页 + 标题 + 语言切换 */}
      <header className="border-b border-slate-200 bg-white shadow-sm">
        <div className="mx-auto flex max-w-4xl items-center justify-between gap-4 px-6 py-4">
          <div className="flex items-center gap-3">
            <Link
              href="/"
              className="h-9 rounded-lg border border-slate-200 px-3 text-sm font-medium text-slate-600 transition hover:bg-slate-50"
            >
              {t("rules.back")}
            </Link>
            <h1 className="text-xl font-bold text-slate-800">
              {t("rules.title")}
            </h1>
          </div>
          <div className="flex items-center gap-1 rounded-lg border border-slate-200 bg-slate-50 p-1">
            {(["zh", "en"] as const).map((l) => (
              <button
                key={l}
                type="button"
                onClick={() => setLang(l)}
                className={`h-8 rounded-md px-3 text-sm font-medium transition ${
                  lang === l
                    ? "bg-indigo-600 text-white shadow-sm"
                    : "text-slate-500 hover:text-slate-700"
                }`}
              >
                {t(l === "zh" ? "lang.zh" : "lang.en")}
              </button>
            ))}
          </div>
        </div>
      </header>

      <main className="mx-auto flex w-full max-w-4xl flex-1 flex-col gap-5 px-6 py-6">
        {/* 地图规则 */}
        <Section title={t("rules.map.title")}>
          <ul className="list-inside list-disc space-y-2 text-sm text-slate-600">
            {MAP_RULE_KEYS.map((key) => (
              <li key={key}>{t(key)}</li>
            ))}
          </ul>
        </Section>

        {/* CSV 数字含义 */}
        <Section title={t("rules.csv.title")}>
          <table className="w-full text-sm">
            <thead>
              <tr className="border-b border-slate-100 text-left text-xs text-slate-400">
                <th className="w-24 pb-2 font-medium">
                  {t("rules.csv.value")}
                </th>
                <th className="pb-2 font-medium">{t("rules.csv.meaning")}</th>
              </tr>
            </thead>
            <tbody>
              {CSV_ROWS.map((row) => (
                <tr
                  key={row.v}
                  className="border-b border-slate-50 last:border-0"
                >
                  <td className="py-2.5">
                    <span
                      className={`inline-flex h-7 w-7 items-center justify-center rounded font-mono text-sm font-bold ${
                        row.stroke
                          ? "border border-slate-300 text-slate-600"
                          : "text-white"
                      }`}
                      style={{ backgroundColor: row.color }}
                    >
                      {row.v}
                    </span>
                  </td>
                  <td className="py-2.5 text-slate-600">{t(row.key)}</td>
                </tr>
              ))}
            </tbody>
          </table>
          <p className="mt-3 text-xs text-slate-400">{t("rules.csv.note")}</p>
        </Section>

        {/* 任务规则 */}
        <Section title={t("rules.task.title")}>
          <ul className="list-inside list-disc space-y-2 text-sm text-slate-600">
            {TASK_RULE_KEYS.map((key) => (
              <li key={key}>{t(key)}</li>
            ))}
          </ul>
        </Section>

        {/* 使用流程 */}
        <Section title={t("rules.flow.title")}>
          <ol className="list-inside list-decimal space-y-2 text-sm text-slate-600">
            {FLOW_STEP_KEYS.map((key) => (
              <li key={key}>{t(key)}</li>
            ))}
          </ol>
        </Section>
      </main>
    </div>
  );
}
