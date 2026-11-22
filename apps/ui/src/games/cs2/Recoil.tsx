import {Navigate, useSearchParams} from "react-router";
import {PageScaffold} from "~/framework/pages/PageScaffold";
import {recoilPages} from "~/navigation/pageGroups";

export function Recoil(){
  const [params]=useSearchParams();
  if(params.get("section")==="rcs")return <Navigate to="/aim?section=aimbot" replace/>;
  return <PageScaffold group={recoilPages} views={{}} className="recoil-page"/>;
}
