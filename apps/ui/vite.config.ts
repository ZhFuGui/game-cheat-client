import { defineConfig } from "vite";
import react from "@vitejs/plugin-react-swc";
import tailwindcss from "@tailwindcss/vite";
import { fileURLToPath, URL } from "node:url";
export default defineConfig({
  base: "./",
  plugins: [react(), tailwindcss()],
  resolve: { alias: { "~": fileURLToPath(new URL("./src", import.meta.url)),
    "~brand": fileURLToPath(new URL("../../data/branding", import.meta.url)) } },
  build: {
    outDir: "dist",
    emptyOutDir: true,
    rollupOptions: {
      output: {
        manualChunks: {
          "react-runtime": ["react", "react-dom", "react-router"],
          locale: ["i18next", "react-i18next"],
          validation: ["@mousebox/shared"],
        },
      },
    },
  },
  server: { host: "127.0.0.1", port: 5173, strictPort: true },
});
