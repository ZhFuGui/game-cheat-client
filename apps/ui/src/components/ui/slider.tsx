import { Slider as Primitive } from "@base-ui/react/slider";

export type SliderRange = { min: number; max: number; step?: number };

type SliderValue = number | readonly [number, number];

type SliderProps<Value extends SliderValue> = SliderRange &
  Pick<Primitive.Root.Props<Value>, "disabled" | "onValueChange" | "onValueCommitted"> & {
    value: Value;
    labels: Value extends number ? readonly [string] : readonly [string, string];
    unit?: string;
    onCancel: () => void;
  };

export function Slider<Value extends SliderValue>({ labels, unit, onCancel, step = 1, ...props }: SliderProps<Value>) {
  return (
    <Primitive.Root<Value>
      {...props}
      className="ui-slider"
      step={step}
      largeStep={step * 10}
      thumbAlignment="edge"
      thumbCollisionBehavior="none"
      onPointerCancel={onCancel}
      onTouchCancel={onCancel}
    >
      <Primitive.Control className="ui-slider-control">
        <Primitive.Track className="ui-slider-track">
          <Primitive.Indicator className="ui-slider-fill" />
        </Primitive.Track>
        {labels.map((name, index) => <Primitive.Thumb
          key={index}
          index={index}
          className="ui-slider-thumb"
          getAriaLabel={() => name}
          getAriaValueText={unit ? (formatted) => `${formatted} ${unit}` : undefined}
        />)}
      </Primitive.Control>
    </Primitive.Root>
  );
}
