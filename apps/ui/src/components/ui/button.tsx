// Adapted from DIMO's shadcn base-nova button. See THIRD_PARTY_NOTICES.md.
import { Button as Primitive } from "@base-ui/react/button";
import { cva, type VariantProps } from "class-variance-authority";
import { cn } from "~/lib/utils";
export const buttonVariants = cva("ui-button", {
  variants: {
    variant: {
      default: "button-primary",
      outline: "button-outline",
      secondary: "button-secondary",
      ghost: "button-ghost",
      destructive: "button-destructive",
    },
    size: {
      default: "button-default",
      sm: "button-small",
      icon: "button-icon",
    },
  },
  defaultVariants: { variant: "default", size: "default" },
});
export function Button({
  className,
  variant,
  size,
  ...props
}: Primitive.Props & VariantProps<typeof buttonVariants>) {
  return (
    <Primitive
      data-slot="button"
      className={cn(buttonVariants({ variant, size }), className)}
      {...props}
    />
  );
}
