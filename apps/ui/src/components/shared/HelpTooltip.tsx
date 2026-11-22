import type {ReactNode} from "react";
import {useTranslation} from "react-i18next";
import {AppIcon} from "~/components/icons";
import {Tooltip} from "~/components/ui/tooltip";
export function HelpTooltip({label,children}:{label:string;children:ReactNode}){
  const {t}=useTranslation();
  return <Tooltip label={t("{{page}} 说明",{page:t(label)})} content={children} className="help-trigger">
    <AppIcon name="help" size={14}/>
  </Tooltip>;
}
