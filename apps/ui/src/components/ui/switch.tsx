import { Switch as Primitive } from "@base-ui/react/switch";
import type { ReactNode } from "react";
import { cn } from "~/lib/utils";
export function Switch({ className, thumbContent, ...props }: Primitive.Root.Props & { thumbContent?: ReactNode }) {
  return (
    <Primitive.Root {...props} className={cn("ui-switch", className)}>
      <Primitive.Thumb className="ui-switch-thumb">{thumbContent}</Primitive.Thumb>
    </Primitive.Root>
  );
}
