import { create } from "zustand";
const occluders = new Set<string>();
export const useSurfaceStore = create<{
  occluded: boolean;
  setOccluded: (value: boolean, owner?: string) => void;
}>((set) => ({
  occluded: false,
  setOccluded: (value, owner = "default") => {
    if (value) occluders.add(owner);
    else occluders.delete(owner);
    set({ occluded: occluders.size > 0 });
  },
}));
