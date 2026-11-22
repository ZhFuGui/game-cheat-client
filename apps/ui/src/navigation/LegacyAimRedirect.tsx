import {Navigate,useLocation} from "react-router";
import {legacyAimHref} from "./aimLocation";

export function LegacyAimRedirect(){
  const {pathname,search}=useLocation();
  return <Navigate to={legacyAimHref(pathname,search)} replace/>;
}
