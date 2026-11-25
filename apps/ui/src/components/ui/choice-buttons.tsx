import { RadioGroup } from "@base-ui/react/radio-group";
import { Radio } from "@base-ui/react/radio";

export function ChoiceButtons<Value extends string>({
  label, value, options, onChange, disabled,
}: {
  label: string;
  value: Value;
  options: readonly { value: Value; label: string }[];
  onChange: (value: Value) => void;
  disabled?: boolean;
}) {
  return <RadioGroup className="ui-choice-buttons" aria-label={label} value={value}
    onValueChange={onChange} disabled={disabled}>
    {options.map(option => <Radio.Root key={option.value} value={option.value}
      className="ui-choice-button" render={<button type="button" />} nativeButton>
      {option.label}
    </Radio.Root>)}
  </RadioGroup>;
}
