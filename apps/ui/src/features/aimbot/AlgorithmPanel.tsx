import { useEffect, useMemo, useState } from "react";
import { useTranslation } from "react-i18next";
import { EmptyState, Section, Hint } from "~/components/shared/Content";
import { SearchInput } from "~/components/ui/input";
import { Button } from "~/components/ui/button";
import { Select } from "~/components/ui/select";
import { applyAlgorithmDisplay } from "~/runtime/connection";
import { useRuntimeStore } from "~/runtime/state";
import { invoke } from "~/lib/desktopApi";
import { useCommand } from "~/hooks/useCommand";
import {algorithmSectionsForScope,type AlgorithmScope} from './algorithmScope';
import {SpeedPresets} from '~/features/aim/common/SpeedPresets';
import { sameWorkspaceContext } from "@mousebox/shared";
export function AlgorithmPanel({scope,allowBallisticsReload=true}:{scope:AlgorithmScope;allowBallisticsReload?:boolean}) {
  const { t } = useTranslation(),
    algorithm = useRuntimeStore((s) => s.algorithm),
    sections = useRuntimeStore((s) => s.algorithmSections),
    { run } = useCommand();
  const [query, setQuery] = useState(""),
    [phase, setPhase] = useState<string | number>("all"),
    [expanded, setExpanded] = useState(false);
  useEffect(() => {
    const context = useRuntimeStore.getState().workspace?.active;
    let alive = true;
    void invoke("algorithm.subscribe", { enabled: true }, context).catch(() => {});
    void invoke("algorithm.read", {}, context)
      .then(value => { if (alive && (!context || sameWorkspaceContext(context, useRuntimeStore.getState().workspace?.active))) applyAlgorithmDisplay(value); })
      .catch(() => {});
    return () => {
      alive = false;
      void invoke("algorithm.subscribe", { enabled: false }, context).catch(() => {});
    };
  }, []);
  const shown = useMemo(
    () =>
      algorithmSectionsForScope(sections,scope)
        .map((section) => ({
          ...section,
          facts: section.ids.flatMap((id) => {
            const fact = algorithm.facts?.[id];
            if (!fact) return [];
            if (
              phase === "live" &&
              !["wired", "conditional"].includes(fact.phase)
            )
              return [];
            if (phase === "plan" && !["agreed", "missing"].includes(fact.phase))
              return [];
            if (phase === "excluded" && fact.phase !== "excluded") return [];
            if (phase === "observed" && fact.phase !== "observed") return [];
            if (
              query.trim() &&
              ![
                fact.title,
                fact.current,
                fact.source,
                fact.principle,
                fact.status,
              ]
                .join(" ")
                .toLocaleLowerCase()
                .includes(query.trim().toLocaleLowerCase())
            )
              return [];
            return [fact];
          }),
        }))
        .filter((section) => section.facts.length),
    [sections, algorithm.facts, phase, query,scope],
  );
  return (
    <div className="algorithm-panel">
      <Section title={scope==='mouse'?'鼠标移动链路':'当前计算链路'}>
        <p>{scope==='mouse'?algorithm.mouseSummary || t('角度误差换算为鼠标计数，主动接近使用短 S 曲线，锁定后使用快速纠偏。'):algorithm.summary || "—"}</p>
        {scope==='aimbot'&&<p>{algorithm.assumptionSummary || "—"}</p>}
        <Hint>{algorithm.note || "—"}</Hint>
        <code>{algorithm.revision || "—"}</code>
        <div className="actions">
          {scope==='aimbot'&&allowBallisticsReload&&<Button
            variant="outline"
            onClick={() => void run("ballistics.reload", {})}
          >
            {t("重新载入弹道资料")}
          </Button>}
          <Button
            variant="ghost"
            onClick={() => void run("algorithm.copy", {})}
          >
            {t("复制全部说明")}
          </Button>
        </div>
      </Section>
      {scope==='mouse'&&<SpeedPresets/>}
      <div className="algorithm-toolbar">
        <SearchInput
          aria-label={t("搜索计算项、来源或原理")}
          placeholder={t("搜索计算项、来源或原理")}
          value={query}
          onChange={(e) => setQuery(e.target.value)}
        />
        <Select
          label={t("计算阶段")}
          value={phase}
          onChange={setPhase}
          options={[
            { value: "all", label: t("全部") },
            { value: "live", label: t("已接入 / 条件使用") },
            { value: "plan", label: t("约定 / 待接入") },
            { value: "observed", label: t("仅原生遥测") },
            { value: "excluded", label: t("未参与") },
          ]}
        />
        <Button variant="outline" onClick={() => setExpanded((v) => !v)}>
          {t(expanded ? "收起原理" : "展开全部原理")}
        </Button>
      </div>
      {shown.map((section) => (
        <section key={section.title} className="algorithm-group">
          <h2>{section.title}</h2>
          {section.facts.map((fact) => (
            <details
              key={`${fact.id}:${expanded}`}
              data-fact={fact.id}
              open={expanded}
              className="algorithm-fact"
            >
              <summary>
                <div>
                  <strong>{fact.title}</strong>
                  <span>{fact.current}</span>
                </div>
                <small>{fact.status}</small>
              </summary>
              <div className="algorithm-explanation">
                <p>
                  <span>{t("来源")}</span>
                  {fact.source}
                </p>
                <p>
                  <span>{t("原理")}</span>
                  {fact.principle}
                </p>
              </div>
            </details>
          ))}
        </section>
      ))}
      {!shown.length && (
        <EmptyState icon="search" message={t("没有匹配的计算项。")} />
      )}
    </div>
  );
}
