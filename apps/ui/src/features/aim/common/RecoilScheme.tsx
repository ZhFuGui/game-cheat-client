import {type ReactNode} from "react";
import {useTranslation} from "react-i18next";
import {HelpTooltip} from "~/components/shared/HelpTooltip";
import {FeatureSection} from "~/components/shared/FeatureSection";

export interface RecoilFactor {
  title: string;
  fields?: readonly string[];
  detail: string;
}

export function RecoilScheme({name, help, factors, children}: {
  name: string;
  help: ReactNode;
  factors: readonly RecoilFactor[];
  children?: ReactNode;
}) {
  const {t} = useTranslation();
  return <FeatureSection title="后座方案" help={help}>
    <div className="aim-fields">
      <div className="aim-value-row"><span>{t("计算方式")}</span><span>{t(name)}</span></div>
      {children}
      <details className="aim-version-details aim-full-row">
        <summary>{t("参与计算的字段")}</summary>
        <div className="aim-version-factors">{factors.map(factor =>
          <div className="aim-version-factor" key={factor.title}>
            <div className="field-label"><strong>{t(factor.title)}</strong>
              <HelpTooltip label={factor.title}>{t(factor.detail)}</HelpTooltip>
            </div>
            {factor.fields?.map(field => <code key={field}>{field}</code>)}
          </div>,
        )}</div>
      </details>
    </div>
  </FeatureSection>;
}
