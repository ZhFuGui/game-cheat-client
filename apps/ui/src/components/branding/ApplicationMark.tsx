import icon from "~brand/application-icon.png";

/** Uses the same artwork as the desktop executable and window icon. */
export function ApplicationMark({className}:{className?:string}) {
  return <img src={icon} className={className} alt="" aria-hidden="true" draggable={false}/>;
}
