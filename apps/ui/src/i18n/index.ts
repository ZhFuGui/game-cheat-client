import i18n from "i18next";
import { initReactI18next } from "react-i18next";
import legacyEnglish from "./locales/en.json";
import webEnglish from "./locales/web-en.json";
import mouseDebugEnglish from './locales/mouse-debug-en.json';
import contextEnglish from "./locales/context-en.json";
import gameEnglish from "./locales/games-en.json";
import cs2English from "./locales/cs2-en.json";
import accuracyEnglish from "./locales/accuracy-en.json";
import aimVersionsEnglish from "./locales/aim-versions-en.json";
import aimControlsEnglish from "./locales/aim-controls-en.json";
import aimGeneralEnglish from "./locales/aim-general-en.json";
import extensionEnglish from "./locales/extensions-en.json";
import hitboxEnglish from "./locales/hitboxes-en.json";
import overlayEnglish from "./locales/overlay-en.json";
import readerChainEnglish from "./locales/reader-chain-en.json";
import espTimingEnglish from "./locales/esp-timing-en.json";
import sourceDataEnglish from "./locales/source-data-en.json";
import triggerEnglish from "./locales/trigger-en.json";
import rageEnglish from "./locales/rage-en.json";
import rageScoringEnglish from "./locales/rage-scoring-en.json";
import radarEnglish from "./locales/radar-en.json";
import valorantEnglish from "./locales/valorant-en.json";
export const english: Record<string, string> = {
  ...legacyEnglish,
  ...webEnglish,
  ...mouseDebugEnglish,
  ...contextEnglish,
  ...gameEnglish,
  ...cs2English,
  ...accuracyEnglish,
  ...aimVersionsEnglish,
  ...aimControlsEnglish,
  ...aimGeneralEnglish,
  ...extensionEnglish,
  ...hitboxEnglish,
  ...overlayEnglish,
  ...readerChainEnglish,
  ...espTimingEnglish,
  ...sourceDataEnglish,
  ...triggerEnglish,
  ...rageEnglish,
  ...rageScoringEnglish,
  ...radarEnglish,
  ...valorantEnglish,
};
const chinese = Object.fromEntries(
  Object.keys(english).map((key) => [key, key]),
);
void i18n.use(initReactI18next).init({
  resources: { en: { translation: english }, zh: { translation: chinese } },
  lng: "zh",
  fallbackLng: "zh",
  keySeparator: false,
  nsSeparator: false,
  interpolation: { escapeValue: false },
  react: { useSuspense: false },
});
export default i18n;
