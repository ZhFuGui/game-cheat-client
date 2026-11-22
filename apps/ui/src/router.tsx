import { createMemoryRouter, Navigate } from "react-router";
import { AppLayout } from "./shell/AppLayout";
import { WorkspaceRoute } from "./workspaces/WorkspaceRoute";
import { startupRoute } from "./lib/startupRoute";
import { LegacyAimRedirect } from "./navigation/LegacyAimRedirect";
import { visualsPages } from "./navigation/pageGroups";
export const router = createMemoryRouter([{
  path: "/", Component: AppLayout, children: [
    { index: true, lazy: async () => {
      const Page = (await import("./pages/Reader")).default;
      return { Component: () => <WorkspaceRoute><Page /></WorkspaceRoute> };
    }},
    { path: "dashboard", Component:()=> <Navigate to="/?section=dashboard" replace/> },
    { path: "visuals", lazy: async () => {
      const Page = (await import("./pages/Visuals")).default;
      return { Component: () => <WorkspaceRoute capability="esp"><Page /></WorkspaceRoute> };
    }},
    { path: "esp", Component: () => <Navigate to={visualsPages.route} replace /> },
    { path: "aim", lazy: async () => {
      const Page = (await import("./pages/Aim")).default;
      return { Component: () => <WorkspaceRoute><Page /></WorkspaceRoute> };
    }},
    { path: "aimbot", Component:LegacyAimRedirect },
    { path: "triggerbot", Component:LegacyAimRedirect },
    { path: "rage", Component:LegacyAimRedirect },
    { path: "game-settings", lazy: async () => {
      const Page = (await import("./pages/GameSettings")).default;
      return { Component: () => <WorkspaceRoute><Page /></WorkspaceRoute> };
    }},
    { path: "recoil", lazy: async () => {
      const Page = (await import("./pages/Recoil")).default;
      return { Component: () => <WorkspaceRoute capability="recoil"><Page /></WorkspaceRoute> };
    }},
    { path: "workspaces", Component: () => <Navigate to="/" replace /> },
    { path: "learning", Component: () => <Navigate to="/mouse-debug?section=learning" replace /> },
    { path: "mouse-debug", lazy: async () => ({ Component: (await import("./pages/MouseDebug")).default }) },
    { path: "logs", lazy: async () => ({ Component: (await import("./pages/Logs")).default }) },
    { path: "settings", lazy: async () => ({ Component: (await import("./pages/Settings")).default }) },
  ],
}], { initialEntries: [startupRoute(window.location.search)] });

