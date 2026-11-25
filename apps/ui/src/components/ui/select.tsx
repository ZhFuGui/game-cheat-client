import { Select as Primitive } from "@base-ui/react/select";
import { AppIcon } from "~/components/icons";
import { useEffect, useId, type ReactNode } from "react";
import { useSurfaceStore } from "~/stores/surfaceStore";
export interface Option {
  value: string | number;
  label: string;
}
export function Select({
  id,
  value,
  options,
  onChange,
  disabled,
  label,
  placeholder,
  variant = "field",
  leadingIcon,
  title,
  displayLabel,
}: {
  id?: string;
  value: string | number;
  options: Option[];
  onChange: (value: string | number) => void;
  disabled?: boolean;
  label?: string;
  placeholder?: string;
  variant?: "field" | "menu";
  leadingIcon?: ReactNode;
  title?: string;
  displayLabel?: string;
}) {
  const owner = useId();
  useEffect(
    () => () => useSurfaceStore.getState().setOccluded(false, owner),
    [owner],
  );
  return (
    <Primitive.Root
      value={String(value)}
      onValueChange={(v) => {
        const option = options.find((o) => String(o.value) === v);
        if (option) onChange(option.value);
      }}
      onOpenChange={(open) =>
        useSurfaceStore.getState().setOccluded(open, owner)
      }
      disabled={disabled}
      items={options.map((o) => ({ ...o, value: String(o.value) }))}
    >
      <Primitive.Trigger id={id} className={`ui-select${variant === "menu" ? " ui-select-menu" : ""}`} aria-label={label} title={title}>
        {leadingIcon && <span className="ui-select-leading" aria-hidden="true">{leadingIcon}</span>}
        <Primitive.Value className="ui-select-value" placeholder={placeholder}>{displayLabel}</Primitive.Value>
        <Primitive.Icon className="ui-select-chevron">
          <AppIcon name="chevronDown" size={14} />
        </Primitive.Icon>
      </Primitive.Trigger>
      <Primitive.Portal>
        <Primitive.Positioner sideOffset={5} className="select-positioner" align={variant === "menu" ? "start" : undefined}
          alignItemWithTrigger={variant === "field"}>
          <Primitive.Popup className={`select-popup${variant === "menu" ? " select-popup-menu" : ""}`}>
            <Primitive.List>
              {options.map((o) => (
                <Primitive.Item
                  key={o.value}
                  value={String(o.value)}
                  className="select-option"
                >
                  <Primitive.ItemText>{o.label}</Primitive.ItemText>
                  <Primitive.ItemIndicator>
                    <AppIcon name="check" size={14} />
                  </Primitive.ItemIndicator>
                </Primitive.Item>
              ))}
            </Primitive.List>
          </Primitive.Popup>
        </Primitive.Positioner>
      </Primitive.Portal>
    </Primitive.Root>
  );
}
