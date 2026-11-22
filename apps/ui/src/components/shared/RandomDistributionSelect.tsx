import {randomDistributions, type RandomDistribution} from "@mousebox/shared";
import {useTranslation} from "react-i18next";
import {Select} from "~/components/ui/select";

const labels: Record<RandomDistribution, string> = {
  normal: "正态分布",
  uniform: "均匀分布",
  triangular: "三角分布",
  lower: "偏向下限",
  upper: "偏向上限",
};

export function RandomDistributionSelect({value, onChange, label, disabled}: {
  value: RandomDistribution;
  onChange: (distribution: RandomDistribution) => void;
  label: string;
  disabled?: boolean;
}) {
  const {t} = useTranslation();
  return <Select
    value={value}
    label={label}
    disabled={disabled}
    options={randomDistributions.map(id => ({value: id, label: t(labels[id])}))}
    onChange={requested => {
      const distribution = randomDistributions.find(id => id === requested);
      if (distribution) onChange(distribution);
    }}
  />;
}
