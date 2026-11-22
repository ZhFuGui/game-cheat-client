import {AppIcon} from "~/components/icons";
import {useTranslation} from "react-i18next";
import {Tooltip} from "~/components/ui/tooltip";
export function NavigationHelp({page,content}:{page:string;content:{title:string;paragraphs:readonly string[]}}){
  const {t}=useTranslation();
  return <Tooltip label={t("{{page}} 说明",{page:t(page)})} className="nav-help-trigger"
    content={<><strong>{t(content.title)}</strong>{content.paragraphs.map(text=><p key={text}>{t(text)}</p>)}</>}>
    <AppIcon name="help" size={14}/>
  </Tooltip>;
}
