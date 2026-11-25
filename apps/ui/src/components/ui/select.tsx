import { Select as Primitive } from "@base-ui/react/select";
import { AppIcon } from "~/components/icons";
import { useEffect, useId, type ReactNode } from "react";
import { useSurfaceStore } from "~/stores/surfaceStore";
export interface Option {
  value: string | number;
  label: string;
  leading?: ReactNode;
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
  alignItemWithTrigger = variant === "field",
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
  alignItemWithTrigger?: boolean;
}) {
  const owner = useId();
  const leading = leadingIcon ?? options.find(option => option.value === value)?.leading;
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
        {leading && <span className="ui-select-leading" aria-hidden="true">{leading}</span>}
        <Primitive.Value className="ui-select-value" placeholder={placeholder}>{displayLabel}</Primitive.Value>
        <Primitive.Icon className="ui-select-chevron">
          <AppIcon name="chevronDown" size={14} />
        </Primitive.Icon>
      </Primitive.Trigger>
      <Primitive.Portal>
        <Primitive.Positioner sideOffset={5} className="select-positioner" align={alignItemWithTrigger ? undefined : "start"}
          alignItemWithTrigger={alignItemWithTrigger}>
          <Primitive.Popup className={`select-popup${variant === "menu" ? " select-popup-menu" : ""}`}>
            <Primitive.List>
              {options.map((o) => (
                <Primitive.Item
                  key={o.value}
                  value={String(o.value)}
                  className="select-option"
                >
                  <Primitive.ItemText>{o.leading ? <span className="select-option-label">
                    <span aria-hidden="true">{o.leading}</span>{o.label}
                  </span> : o.label}</Primitive.ItemText>
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
