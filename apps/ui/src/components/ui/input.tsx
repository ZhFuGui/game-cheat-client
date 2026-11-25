import { forwardRef, type InputHTMLAttributes } from "react";
import { cn } from "~/lib/utils";
import { AppIcon } from "~/components/icons";
export const Input = forwardRef<
  HTMLInputElement,
  InputHTMLAttributes<HTMLInputElement>
>(function Input({ className, ...props }, ref) {
  return (
    <input
      ref={ref}
      data-slot="input"
      className={cn("ui-input", className)}
      {...props}
    />
  );
});

export function SearchInput({ className, ...props }: InputHTMLAttributes<HTMLInputElement>) {
  return (
    <div className={cn("search-input", className)}>
      <AppIcon name="search" size={15} />
      <Input {...props} type="search" />
    </div>
  );
}
