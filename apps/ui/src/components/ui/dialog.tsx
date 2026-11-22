import { Dialog as Primitive } from "@base-ui/react/dialog";
import type { ReactNode } from "react";
import { useEffect, useId } from "react";
import { useSurfaceStore } from "~/stores/surfaceStore";
export function Dialog({
  open,
  onOpenChange,
  title,
  children,
}: {
  open: boolean;
  onOpenChange: (value: boolean) => void;
  title: string;
  children: ReactNode;
}) {
  const setOccluded = useSurfaceStore((s) => s.setOccluded);
  const owner = useId();
  useEffect(() => {
    if (!open) return;
    setOccluded(true, owner);
    return () => setOccluded(false, owner);
  }, [open, setOccluded, owner]);
  return (
    <Primitive.Root open={open} onOpenChange={onOpenChange}>
      <Primitive.Portal>
        <Primitive.Backdrop className="dialog-backdrop" />
        <Primitive.Popup className="dialog-popup">
          <Primitive.Title className="dialog-title">{title}</Primitive.Title>
          <Primitive.Description render={<div />}>
            {children}
          </Primitive.Description>
        </Primitive.Popup>
      </Primitive.Portal>
    </Primitive.Root>
  );
}
