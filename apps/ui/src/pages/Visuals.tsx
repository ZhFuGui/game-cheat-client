import { lazy } from "react";
import { AppIcon } from "~/components/icons";
import { PageScaffold } from "~/framework/pages/PageScaffold";
import { visualsPages } from "~/navigation/pageGroups";

const views = { esp: lazy(() => import("./Esp")), radar: lazy(() => import("./Radar")) };
const icons = {
  settings: <AppIcon name="settings" size={14} />,
  esp: <AppIcon name="esp" size={14} />,
  radar: <AppIcon name="radar" size={14} />,
};

export default function Visuals() {
  return <PageScaffold group={visualsPages} views={views} icons={icons} className="visuals-page" />;
}
