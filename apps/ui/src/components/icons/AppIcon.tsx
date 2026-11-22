import type { SVGProps } from "react";
import { iconDefinitions, type AppIconName } from "./registry";

export type AppIconProps = Omit<SVGProps<SVGSVGElement>, "children" | "width" | "height" | "strokeWidth"> & {
  name: AppIconName;
  size?: number;
};

/** Square, currentColor SVGs. A labelled control normally owns the accessible name. */
export function AppIcon({ name, size = 18, className, ...props }: AppIconProps) {
  const { glyph: Glyph, provider } = iconDefinitions[name];
  const named = props["aria-label"] !== undefined || props["aria-labelledby"] !== undefined;
  return <Glyph {...props} size={size} className={className ? `app-icon ${className}` : "app-icon"}
    data-icon={name} data-icon-provider={provider} focusable="false"
    aria-hidden={props["aria-hidden"] ?? (named ? undefined : true)} role={props.role ?? (named ? "img" : undefined)} />;
}
