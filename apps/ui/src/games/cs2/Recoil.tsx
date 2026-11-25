import {Link, Navigate, useSearchParams} from "react-router";
import {useTranslation} from "react-i18next";
import {EmptyState} from "~/components/shared/Content";
import {PageScaffold} from "~/framework/pages/PageScaffold";
import {recoilPages} from "~/navigation/pageGroups";

function RecoilGuidance() {
  const {t} = useTranslation();
  return (
    <EmptyState className="recoil-guidance" icon="recoil" message={t("后坐补偿在 Aimbot 页设置。")}
      action={<Link to="/aim?section=aimbot">{t("打开 Aimbot")}</Link>} />
  );
}

const views = {guidance: RecoilGuidance};

export function Recoil(){
  const [params]=useSearchParams();
  if(params.get("section")==="rcs")return <Navigate to="/aim?section=aimbot" replace/>;
  return <PageScaffold group={recoilPages} views={views} className="recoil-page"/>;
}
