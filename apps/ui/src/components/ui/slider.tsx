import { Slider as Primitive } from "@base-ui/react/slider";

export type SliderRange = { min: number; max: number; step?: number };

type SliderProps = SliderRange &
  Pick<Primitive.Root.Props<number>, "value" | "disabled" | "onValueChange" | "onValueCommitted"> & {
    label: string;
    unit?: string;
    onCancel: () => void;
  };

export function Slider({ label, unit, onCancel, step = 1, ...props }: SliderProps) {
  return (
    <Primitive.Root<number>
      {...props}
      className="ui-slider"
      step={step}
      largeStep={step * 10}
      thumbAlignment="edge"
      onPointerCancel={onCancel}
      onTouchCancel={onCancel}
    >
      <Primitive.Control className="ui-slider-control">
        <Primitive.Track className="ui-slider-track">
          <Primitive.Indicator className="ui-slider-fill" />
        </Primitive.Track>
        <Primitive.Thumb
          className="ui-slider-thumb"
          getAriaLabel={() => label}
          getAriaValueText={unit ? (formatted) => `${formatted} ${unit}` : undefined}
        />
      </Primitive.Control>
    </Primitive.Root>
  );
}
