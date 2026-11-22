import {
  combineRuntimeState,
  pickDeviceState,
  sameWorkspaceContext,
  type Bootstrap,
  type EventPayload,
  type Topic,
  type WorkspaceCatalog,
} from "@mousebox/shared";
import { displayText } from "~/i18n/displayText";
import { RuntimeDisplayProjection } from "./displayProjection";
import { useRuntimeStore } from "./state";

const display = new RuntimeDisplayProjection();
const { getState, setState } = useRuntimeStore;
const language = () => getState().preferences.language;

export function applyAlgorithmDisplay(value: EventPayload<"algorithm">) {
  display.remember(value);
  setState(display.translate(value, language()));
}

export function applyWorkspace(catalog: WorkspaceCatalog) {
  if (!sameWorkspaceContext(getState().workspace?.active, catalog.active)) display.resetWorkspace();
  getState().applyWorkspace(catalog);
}

export function applyBootstrap(boot: Bootstrap) {
  const { state, learning, algorithm, algorithmSections, ports, graphicsAdapters, preferences } = boot;
  display.remember({ state, learning, algorithm, algorithmSections, ports, graphicsAdapters });
  display.rememberDevices(boot.devices ?? pickDeviceState(state));
  getState().initialize({
    ...boot,
    ...display.inLanguage(preferences.language),
    ...(boot.devices ? { devices: display.devicesInLanguage(preferences.language) } : {}),
  });
}

type Listen = <K extends Topic>(topic: K, handle: (payload: EventPayload<K>) => void) => void;

/** Runtime events can be applied from any typed event source. */
export function listenRuntimeEvents(listen: Listen) {
  listen("devices", devices => {
    display.rememberDevices(devices);
    const deviceState = display.devicesInLanguage(language());
    setState(current => ({
      deviceState,
      devicesProvided: true,
      state: combineRuntimeState(current.state, deviceState),
    }));
  });
  listen("application", applicationValues => setState({ applicationValues, applicationProvided: true }));
  listen("values", values => {
    setState({ values });
    if (!getState().applicationProvided)
      setState({ applicationValues: { dpi: values.dpi, renderAdapter: values.renderAdapter } });
  });
  listen("espValues", espValues => setState({ espValues }));
  listen("speedPresets", speedPresets => setState({ speedPresets }));
  listen("state", state => {
    const current = getState();
    if (!current.devicesProvided) {
      display.rememberDevices(pickDeviceState(state));
      setState({ deviceState: display.devicesInLanguage(language()) });
    }
    display.remember({ state });
    setState({ state: display.inLanguage(language()).state, workspaceRefreshing: !getState().values });
  });
  listen("learning", learning => {
    display.remember({ learning });
    setState({ learning: display.translate(learning, language()) });
  });
  listen("mouseReports", mouseReports => setState({ mouseReports }));
  listen("ports", ports => {
    display.remember({ ports });
    setState({ ports: display.translate(ports, language()) });
  });
  listen("graphicsAdapters", graphicsAdapters => {
    display.remember({ graphicsAdapters });
    setState({ graphicsAdapters: display.translate(graphicsAdapters, language()) });
  });
  listen("preferences", preferences => {
    setState({
      ...display.inLanguage(preferences.language),
      deviceState: display.devicesInLanguage(preferences.language),
      preferences,
    });
  });
  listen("algorithm", applyAlgorithmDisplay);
  listen("logs", value => getState().appendLogs(value));
  listen("error", value => getState().setError(displayText(value.message, language())));
  listen("bridge", value => setState({ connected: value.connected, error: value.message ?? null }));
}
