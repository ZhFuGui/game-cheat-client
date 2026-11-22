import {
  contracts,
  eventSchemas,
  type CommandInput,
  type CommandName,
  type CommandResult,
  type DesktopApi,
  type EventPayload,
  type Topic,
  acceptsWorkspaceEvent,
  isWorkspaceCommand,
  sameWorkspaceContext,
  type WorkspaceContext,
} from "@mousebox/shared";
import { useRuntimeStore } from "~/runtime/state";
declare global {
  interface Window {
    mousebox?: DesktopApi;
  }
}
export function desktopApi(): DesktopApi {
  if (!window.mousebox) throw new Error("Desktop bridge is unavailable");
  return window.mousebox;
}
export async function invoke<K extends CommandName>(
  name: K,
  args: CommandInput<K>,
  context = useRuntimeStore.getState().workspace?.active,
): Promise<CommandResult<K>> {
  const payload = contracts[name].input.parse(args);
  const catalog = useRuntimeStore.getState().workspace;
  if (catalog && isWorkspaceCommand(name, payload) && !sameWorkspaceContext(context, catalog.active)) throw new Error("游戏已变化，请在当前游戏重新操作");
  const result = context && isWorkspaceCommand(name, payload)
    ? await desktopApi().invoke(name, payload as CommandInput<K>, context)
    : await desktopApi().invoke(name, payload as CommandInput<K>);
  return contracts[name].output.parse(result) as CommandResult<K>;
}
export function subscribe<K extends Topic>(
  topic: K,
  listener: (event: EventPayload<K>, context?: WorkspaceContext) => void,
) {
  return desktopApi().subscribe(topic, (value, context) => {
    if (!acceptsWorkspaceEvent(useRuntimeStore.getState().workspace, topic, context)) return;
    const result = eventSchemas[topic].safeParse(value);
    if (result.success) listener(result.data as EventPayload<K>, context);
  });
}
